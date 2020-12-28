/*
 * $Id: flintr.c,v 1.16 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#include <bsp_config.h>

#ifdef CFG_INTR_INCLUDED

#ifndef __LINUX__
static int polled_intr = CFG_POLLED_INTR;
#else
static int polled_intr = 1;
#endif

static sys_intr_f intr_func[256] = { NULL };
static uint32 intr_param[256];

bool bcm5607x_intr_handling_enable = 0;
bool bcm5607x_intr_context = 0;

int intrnum_min = 256;
int intrnum_max = 0;

sys_error_t
bcm5607x_intr_entry(int unit)
{
    int intrnum, i, j;
    MHOST_0_MHOST_INTR_STATUSr_t intr_status;

    if (bcm5607x_intr_handling_enable == 0)  {
        return SYS_OK;
    }

    if (!polled_intr) {

        /* Switch SBUS channel to interrupt. */
        bcm5607x_schan_ch = BCM5607X_SCHAN_CH_INTR;

        /* Set interrupt context indicator. */
        bcm5607x_intr_context = 1;
    }

    for (intrnum = intrnum_min; intrnum < intrnum_max; intrnum += 32) {
        READ_MHOST_0_MHOST_INTR_STATUSr(unit, intrnum / 32, intr_status);
        if (MHOST_0_MHOST_INTR_STATUSr_GET(intr_status)) {
            for (i = 0; i < 32; i ++) {
                 j = intrnum + i;
                 if (MHOST_0_MHOST_INTR_STATUSr_GET(intr_status) & (1 << i) &&
                     intr_func[j] != NULL) {
                     intr_func[j](intr_param[j]);
                 }
            }
        }
    }

    if (!polled_intr) {

        /* Switch SBUS channel to default. */
        bcm5607x_schan_ch = BCM5607X_SCHAN_CH_DEFAULT;

        /* Clear interrupt context indicator. */
        bcm5607x_intr_context = 0;

    }

    return SYS_OK;
}

sys_error_t
bcm5607x_intr_func_set(int unit, int intrnum, sys_intr_f fn, uint32 param)
{
    if (intrnum < 0 || intrnum >= 256) {
        return SYS_ERR_PARAMETER;
    }

    intr_func[intrnum] = fn;
    intr_param[intrnum] = param;

    return SYS_OK;
}

sys_error_t
bcm5607x_intr_enable_set(int unit, int intrnum, int enable)
{
    MHOST_0_MHOST_INTR_MASKr_t intr_mask;
    MHOST_VICINTENABLEr_t vic_intr_enable;
    MHOST_VICINTENCLEARr_t vic_intr_en_clear;
    int floor, ceiling;

    if (intrnum < 0 || intrnum >= 256) {
        return SYS_ERR_PARAMETER;
    }

    floor = (intrnum / 32) * 32;
    ceiling = floor + 32;

    if (floor < intrnum_min) {
        intrnum_min = floor;
    }

    if (ceiling > intrnum_max) {
        intrnum_max = ceiling;
    }

    enable = (enable != 0);

    READ_MHOST_0_MHOST_INTR_MASKr(unit, intrnum / 32, intr_mask);
    MHOST_0_MHOST_INTR_MASKr_SET(intr_mask,
        MHOST_0_MHOST_INTR_MASKr_GET(intr_mask) & ~(1 << (intrnum % 32)));
    MHOST_0_MHOST_INTR_MASKr_SET(intr_mask,
        MHOST_0_MHOST_INTR_MASKr_GET(intr_mask) | (enable << (intrnum % 32)));
    WRITE_MHOST_0_MHOST_INTR_MASKr(unit, intrnum / 32, intr_mask);

    if (!polled_intr) {
        if (enable) {
            READ_MHOST_VICINTENABLEr(unit, vic_intr_enable);
            MHOST_VICINTENABLEr_SET(vic_intr_enable, (1 << (intrnum / 8)));
            WRITE_MHOST_VICINTENABLEr(unit, vic_intr_enable);
        } else {
            READ_MHOST_VICINTENCLEARr(unit, vic_intr_en_clear);
            MHOST_VICINTENCLEARr_SET(vic_intr_en_clear, (1 << (intrnum / 8)));
            WRITE_MHOST_VICINTENCLEARr(unit, vic_intr_en_clear);
        }
    }

    return SYS_OK;
}

sys_error_t
bcm5607x_intr_enable_get(int unit, int intrnum, int *enable)
{
    MHOST_0_MHOST_INTR_MASKr_t intr_mask;

    if (intrnum < 0 || intrnum >= 256) {
        return SYS_ERR_PARAMETER;
    }

    READ_MHOST_0_MHOST_INTR_MASKr(unit, intrnum / 32, intr_mask);
    *enable = MHOST_0_MHOST_INTR_MASKr_GET(intr_mask) & (1 << (intrnum % 32));
    *enable = (*enable != 0);

    return SYS_OK;
}

sys_error_t
bcm5607x_intr_handling_save_disable(int unit, uint32 *status) {

    uint32 tmp = 0;

    /* No interrupt disable in interrupt context. */
    if (bcm5607x_intr_context) {
        return SYS_ERR;
    }

    bcm5607x_intr_handling_enable = 0;

    if (!polled_intr) {
#ifndef __LINUX__
        /* Read CPSR.IRQ bit to tmp CPSR then disable interrupt. */
        asm("mrs %0, cpsr \n \
             cpsid i \n \
             and %0, %0, #0x80\n": "=r"(tmp): "r"(tmp):);
#endif
    }

    if (status) {
        *status = (tmp != 0);
    }

    return SYS_OK;
}

sys_error_t
bcm5607x_intr_handling_restore_enable(int unit, uint32 *status) {

    /* No interrupt enable in interrupt context. */
    if (bcm5607x_intr_context) {
        return SYS_ERR;
    }

    if (status && *status == 0) {
        return SYS_OK;
    }

    if (!polled_intr) {
#ifndef __LINUX__
        /* Enable interrupt. */
        asm("cpsie i");
#endif
    }

    bcm5607x_intr_handling_enable = 1;

    return SYS_OK;
}

/* Driver template. */
static sys_intr_drv_t bcm5607x_drv = {
    .intr_func_set = bcm5607x_intr_func_set,
    .intr_handling_save_disable = bcm5607x_intr_handling_save_disable,
    .intr_handling_restore_enable = bcm5607x_intr_handling_restore_enable,
    .intr_enable_set = bcm5607x_intr_enable_set,
    .intr_enable_get = bcm5607x_intr_enable_get,
    .intr_entry = bcm5607x_intr_entry
};

sys_error_t
bcm5607x_intr_init(int unit)
{
    return sys_intr_drv_init(&bcm5607x_drv);
}

#endif /* CFG_INTR_INCLUDED */
