/*
 * $Id: utgpio_intr_regression.c,v 1.4 Broadcom SDK $
 *
 * Use timer to mimic GPIO interrupt to do interrupt regression.
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#include <system.h>
#include <utils/shr/shr_debug.h>
#include <boardapi/gpio.h>
#include <boardapi/intr.h>
#include "utgpio_internal.h"
#define INTR_INTERVAL  2500000

#ifdef CFG_INTR_INCLUDED

/****************************************
 *  Local define.
 */
/* Period of background task. */
#define BGTIMER_INTERVAL               10000

/* Periold of timer interrupt. */
#define INTR_INTERVAL                  2500000

/* Marco to disable/enable interrupt. */
#define IRQ_DISABLE(en)                do { if (en) board_intr_handling_disable(); } while(0)
#define IRQ_ENABLE(en)                 do { if (en) board_intr_handling_enable(); } while(0)

/* Sbus test info. */
#define INTR_ID                        3
#define BG_ID                          2

/* Flash test info. */
#define FLASH_TEST_BUF_WSIZE           0x100
#define FLASH_TEST_BLOCK_OFFSET        0xE00000
#define FLASH_TEST_BLOCK_SIZE          0x20000

typedef struct {
    bool lock;
    int id;
    uint32 data[16];
} sbus_test_info_t;

typedef struct {

    /*! Flag for interrupt disable/enable during FLASH program. */
    bool lock;

    /*! FLASH test block starting offset. */
    uint32 start;

    /*! FLASH current testing offset. */
    uint32 offset;

    /*! FLASH test block size. */
    uint32 size;

    /*! Data buffer for test. */
    uint32 data[FLASH_TEST_BUF_WSIZE];

} flash_test_info_t;

typedef struct context_s {

    /*! Context name. */
    const char *name;

    /* Test counter. */
    int cnt;

    /* Test counter maximum. */
    int cnt_max;


    /*! Interrupt disable as SBUS test. */
    int sbus_lock;

    int flash_test;

    /* SBUS test info. */
    sbus_test_info_t reg;
    sbus_test_info_t mem;
    sbus_test_info_t tcam;

    /* FLASH test info. */
    flash_test_info_t f;

    /*! Cleanup function as regression failed. */
    void (*cleanup) (struct context_s *data);
} context_t;

/****************************************
 *  Local variable.
 */
static context_t bg_context;
static context_t intr_context;

/****************************************
 *  Local functions.
 */
static sys_error_t
timer_intr_init(uint32 interval)
{
    IPROCGENRES_TIM0_TIM_TIMER1BGLOADr_t bgload;
    IPROCGENRES_TIM0_TIM_TIMER1LOADr_t load;
    IPROCGENRES_TIM0_TIM_TIMER1CONTROLr_t ctrl;
    IPROCGENRES_TIM0_TIM_TIMER1INTCLRr_t intrclr;

    SHR_FUNC_ENTER(0);

    /* 10000 / 25 / 1000000 = */
    IPROCGENRES_TIM0_TIM_TIMER1BGLOADr_SET(bgload, interval);
    SHR_IF_ERR_EXIT
        (WRITE_IPROCGENRES_TIM0_TIM_TIMER1BGLOADr(0, bgload));
    IPROCGENRES_TIM0_TIM_TIMER1LOADr_SET(load, interval);
    SHR_IF_ERR_EXIT
        (WRITE_IPROCGENRES_TIM0_TIM_TIMER1LOADr(0, load));

    IPROCGENRES_TIM0_TIM_TIMER1CONTROLr_CLR(ctrl);
    IPROCGENRES_TIM0_TIM_TIMER1CONTROLr_TIMERSIZEf_SET(ctrl,1);
    IPROCGENRES_TIM0_TIM_TIMER1CONTROLr_INTENABLEf_SET(ctrl,1);
    IPROCGENRES_TIM0_TIM_TIMER1CONTROLr_TIMERMODEf_SET(ctrl,0);
    IPROCGENRES_TIM0_TIM_TIMER1CONTROLr_TIMERENf_SET(ctrl, 1);
    IPROCGENRES_TIM0_TIM_TIMER1CONTROLr_ONESHOTf_SET(ctrl, 1);
    SHR_IF_ERR_EXIT
        (WRITE_IPROCGENRES_TIM0_TIM_TIMER1CONTROLr(0, ctrl));

    IPROCGENRES_TIM0_TIM_TIMER1INTCLRr_CLR(intrclr);
    IPROCGENRES_TIM0_TIM_TIMER1INTCLRr_INTCLRf_SET(intrclr, 1);
    SHR_IF_ERR_EXIT
        (WRITE_IPROCGENRES_TIM0_TIM_TIMER1INTCLRr(0, intrclr));

exit:
    SHR_FUNC_EXIT();
}

static sys_error_t
timer_intr_clear(void)
{
    IPROCGENRES_TIM0_TIM_TIMER1INTCLRr_t intrclr;

    SHR_FUNC_ENTER(0);

    IPROCGENRES_TIM0_TIM_TIMER1INTCLRr_CLR(intrclr);
    IPROCGENRES_TIM0_TIM_TIMER1INTCLRr_INTCLRf_SET(intrclr, 1);
    SHR_IF_ERR_EXIT
        (WRITE_IPROCGENRES_TIM0_TIM_TIMER1INTCLRr(0, intrclr));

exit:
    SHR_FUNC_EXIT();
}

static sys_error_t
timer_intr_stop(void)
{
    IPROCGENRES_TIM0_TIM_TIMER1CONTROLr_t ctrl;
    SHR_FUNC_ENTER(0);

    IPROCGENRES_TIM0_TIM_TIMER1CONTROLr_CLR(ctrl);
    SHR_IF_ERR_EXIT
        (WRITE_IPROCGENRES_TIM0_TIM_TIMER1CONTROLr(0, ctrl));

    SHR_IF_ERR_EXIT
        (timer_intr_clear());

exit:
    SHR_FUNC_EXIT();
}

static int
sbus_test_init(context_t *d)
{
    VLANm_t vlan;
    FP_GLOBAL_MASK_TCAMm_t fp_global_mask;
    IUNKNOWN_MCAST_BLOCK_MASK_LO_64r_t mcast_block_lo;
    int i;

    SHR_FUNC_ENTER(0);

    sal_memset(d->mem.data, 0, sizeof(d->mem.data));
    for (i = 0; i < (sizeof(vlan) / 4); i++) {
        vlan.v[i] = sal_get_ticks();
    }

    VLANm_PORT_BITMAPf_GET(vlan, d->mem.data);
    IRQ_DISABLE(d->mem.lock);
    SHR_IF_ERR_EXIT
        (READ_VLANm(0, d->mem.id, vlan));
    IRQ_ENABLE(d->mem.lock);

    VLANm_PORT_BITMAPf_SET(vlan, d->mem.data);

    IRQ_DISABLE(d->mem.lock);
    SHR_IF_ERR_EXIT
        (WRITE_VLANm(0, d->mem.id, vlan));
    IRQ_ENABLE(d->mem.lock);

    sal_memset(d->tcam.data, 0, sizeof(d->tcam.data));
    for (i = 0; i < (sizeof(fp_global_mask) / 4); i++) {
        fp_global_mask.v[i] = sal_get_ticks();
    }
    FP_GLOBAL_MASK_TCAMm_IPBMf_GET(fp_global_mask, d->tcam.data);

    IRQ_DISABLE(d->tcam.lock);
    SHR_IF_ERR_EXIT
        (READ_FP_GLOBAL_MASK_TCAMm(0, d->tcam.id, fp_global_mask));
    IRQ_ENABLE(d->tcam.lock);

    FP_GLOBAL_MASK_TCAMm_IPBMf_SET(fp_global_mask, d->tcam.data);
    FP_GLOBAL_MASK_TCAMm_IPBM_MASKf_SET(fp_global_mask, d->tcam.data);
    IRQ_DISABLE(d->tcam.lock);
    SHR_IF_ERR_EXIT
        (WRITE_FP_GLOBAL_MASK_TCAMm(0, d->tcam.id, fp_global_mask));
    IRQ_ENABLE(d->tcam.lock);

    sal_memset(d->reg.data, 0, sizeof(d->reg.data));
    for (i = 0; i < (sizeof(mcast_block_lo) / 4); i++) {
        mcast_block_lo.v[i] = sal_get_ticks();
    }

    IUNKNOWN_MCAST_BLOCK_MASK_LO_64r_BLK_BITMAPf_GET(mcast_block_lo, d->reg.data);

    IRQ_DISABLE(d->reg.lock);
    SHR_IF_ERR_EXIT
        (READ_IUNKNOWN_MCAST_BLOCK_MASK_LO_64r(0, d->reg.id, mcast_block_lo));
    IRQ_ENABLE(d->reg.lock);

    IUNKNOWN_MCAST_BLOCK_MASK_LO_64r_BLK_BITMAPf_SET(mcast_block_lo, d->reg.data);

    IRQ_DISABLE(d->reg.lock);
    SHR_IF_ERR_EXIT
        (WRITE_IUNKNOWN_MCAST_BLOCK_MASK_LO_64r(0, d->reg.id, mcast_block_lo));
    IRQ_ENABLE(d->reg.lock);

exit:
    SHR_FUNC_EXIT();
}

static int
sbus_test_verify(context_t *d)
{
    VLANm_t vlan;
    FP_GLOBAL_MASK_TCAMm_t fp_global_mask;
    IUNKNOWN_MCAST_BLOCK_MASK_LO_64r_t mcast_block_lo;
    uint32 check[16];

    SHR_FUNC_ENTER(0);

    sal_memset(check, 0, sizeof(check));
    IRQ_DISABLE(d->mem.lock);
    SHR_IF_ERR_EXIT
        (READ_VLANm(0, d->mem.id, vlan));
    IRQ_ENABLE(d->mem.lock);

    VLANm_PORT_BITMAPf_GET(vlan, check);
    if (sal_memcmp(check, d->mem.data, sizeof(check))) {
        SHR_ERR_EXIT(SYS_ERR);
    }

    sal_memset(check, 0, sizeof(check));
    IRQ_DISABLE(d->tcam.lock);
    SHR_IF_ERR_EXIT
        (READ_FP_GLOBAL_MASK_TCAMm(0, d->tcam.id, fp_global_mask));
    IRQ_ENABLE(d->tcam.lock);
    FP_GLOBAL_MASK_TCAMm_IPBMf_GET(fp_global_mask, check);
    if (sal_memcmp(check, d->tcam.data, sizeof(check))) {
        SHR_ERR_EXIT(SYS_ERR);
    }

    sal_memset(check, 0, sizeof(check));
    IRQ_DISABLE(d->reg.lock);
    SHR_IF_ERR_EXIT
        (READ_IUNKNOWN_MCAST_BLOCK_MASK_LO_64r(0, d->reg.id, mcast_block_lo));
    IRQ_ENABLE(d->reg.lock);
    IUNKNOWN_MCAST_BLOCK_MASK_LO_64r_BLK_BITMAPf_GET(mcast_block_lo, check);
    if (sal_memcmp(check, d->reg.data, sizeof(check))) {
        SHR_ERR_EXIT(SYS_ERR);
    }

exit:
    SHR_FUNC_EXIT();
}

static int
sbus_test(context_t *d)
{
    SHR_FUNC_ENTER(0);

    IRQ_DISABLE(d->sbus_lock);
    SHR_IF_ERR_EXIT
        (sbus_test_init(d));

    SHR_IF_ERR_EXIT
        (sbus_test_verify(d));
    IRQ_ENABLE(d->sbus_lock);

exit:
    SHR_FUNC_EXIT();
}

static sys_error_t
flash_test_init(flash_test_info_t *f)
{
    uint32 addr;
    uint32 block_begin;
    int i;

    SHR_FUNC_ENTER(0);

    addr = f->offset;

    block_begin = flash_block_begin(addr, flash_dev_get());
    if (block_begin == addr) {
        SHR_IF_ERR_EXIT
            (flash_erase(f->offset, f->size));
    }

    for (i = 0; i < FLASH_TEST_BUF_WSIZE; i ++) {
         f->data[i] = sal_get_ticks();
    }

    SHR_IF_ERR_EXIT
        (flash_program(addr, f->data, FLASH_TEST_BUF_WSIZE * 4));

exit:
    SHR_FUNC_EXIT();
}
static void
dump_memory(uint32* addr, int wlen) {
    int i;

    for (i = 0; i < wlen; i ++) {

        if ((i % 4) == 0) {
            sal_printf("\n%p: ", addr);
        }

        sal_printf("%08x ", addr[i]);
    }

}
static sys_error_t
flash_verify(flash_test_info_t *f)
{
    uint32 compare[FLASH_TEST_BUF_WSIZE];
    uint32 addr;

    SHR_FUNC_ENTER(0);

    addr = f->offset;
    SHR_IF_ERR_EXIT
        (flash_read(addr, compare, sizeof(compare)));

    if (sal_memcmp(compare, f->data, sizeof(compare))) {
        SHR_LOG_ERROR("Flash write fail\n");
        sal_printf("flash data:\n");
        dump_memory((uint32 *) addr, sizeof(compare)/4);
        sal_printf("orignal data:\n");
        dump_memory(f->data, sizeof(compare)/4);
        sal_printf("compare data:\n");
        dump_memory(compare, sizeof(compare)/4);
        SHR_ERR_EXIT(SYS_ERR);
    }

    f->offset += (FLASH_TEST_BUF_WSIZE * 4);
    if (f->offset >= (f->start + f->size)) {
        f->offset = f->start;
    }
    SHR_LOG_TRACE("flash addr: %x\n", f->offset);

exit:
    SHR_FUNC_EXIT();
}

static sys_error_t
flash_test(flash_test_info_t *f)
{
    SHR_FUNC_ENTER(0);

    SHR_IF_ERR_EXIT
        (flash_test_init(f));

    SHR_IF_ERR_EXIT
        (flash_verify(f));

exit:
    SHR_FUNC_EXIT();
}

static sys_error_t
regression_task(context_t *d)
{
    SHR_FUNC_ENTER(0);

    SHR_IF_ERR_EXIT
        (sbus_test(d));

    if (d->flash_test) {
        SHR_IF_ERR_EXIT
            (flash_test(&(d->f)));
    }

    d->cnt ++;
    if (d->cnt > d->cnt_max) {
        SHR_LOG_DEBUG("%d %s runs\n", d->cnt_max, d->name);
        d->cnt = 0;
    }

exit:
    if (SHR_FUNC_ERR()) {
        d->cleanup(d);
    }

    SHR_FUNC_EXIT();
}

static void
regression_task_wrapper(void *d)
{
    regression_task((context_t *) d);
}

static void
bgtask_cleanup(context_t *d)
{
    SHR_LOG_ERROR("%s\n", __FUNCTION__);
    timer_remove(regression_task_wrapper);
}

static void
intr_cb(uint32 d)
{
    timer_intr_clear();
    timer_intr_init(INTR_INTERVAL);

    regression_task((context_t *)d);
}

static void
intr_cleanup(context_t *d)
{
    SHR_LOG_ERROR("%s\n", __FUNCTION__);

    timer_intr_stop();

    /* Stop interrupt timer. */
    sys_intr_enable_set(32, 0);
    sys_intr_func_set(32, NULL, 0);
}

/****************************************
 *  Local functions.
 */
sys_error_t
utgpio_intr_regression_test(void) {

    /* Add timer interrupt callback. */
    sal_memset(&intr_context, 0, sizeof(context_t));
    intr_context.mem.id = INTR_ID;
    intr_context.mem.lock = 0;
    intr_context.tcam.id = INTR_ID;
    intr_context.tcam.lock = 0;
    intr_context.reg.id = INTR_ID;
    intr_context.reg.lock = 0;
    intr_context.cleanup = intr_cleanup;
    intr_context.name = "Interrupt";
    intr_context.flash_test = 0;
    intr_context.cnt_max = 1000;
    sys_intr_func_set(32, intr_cb, (uint32) &intr_context);
    sys_intr_enable_set(32, 1);
    timer_intr_clear();
    timer_intr_init(INTR_INTERVAL);
    board_intr_handling_enable();

    /* Add timer thread in background context. */
    sal_memset(&bg_context, 0, sizeof(context_t));
    bg_context.mem.id = BG_ID;
    bg_context.mem.lock = 0;
    bg_context.tcam.id = BG_ID;
    bg_context.tcam.lock = 0;
    bg_context.reg.id = BG_ID;
    bg_context.reg.lock = 0;
    bg_context.cleanup = bgtask_cleanup;
    bg_context.name = "Background timer";
    bg_context.cnt_max = 100;
    bg_context.flash_test = 1;
    bg_context.f.start = CFG_FLASH_START_ADDRESS + FLASH_TEST_BLOCK_OFFSET;
    bg_context.f.offset = bg_context.f.start;
    bg_context.f.size = FLASH_TEST_BLOCK_SIZE;
    timer_add(regression_task_wrapper, &bg_context, BGTIMER_INTERVAL);

    return SYS_OK;
}

#endif /* CFG_INTR_INCLUDED */
