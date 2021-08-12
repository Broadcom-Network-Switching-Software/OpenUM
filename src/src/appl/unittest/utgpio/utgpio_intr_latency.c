/*
 * $Id: utgpio_intr_latency.c,v 1.4 Broadcom SDK $
 *
 * To profile the gpio interrupt response time.
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#include <system.h>
#include <boardapi/base.h>
#include <boardapi/gpio.h>
#include <boardapi/intr.h>
#include <utils/shr/shr_debug.h>
/* #define DEBUG */
#include "utgpio_internal.h"

#ifdef CFG_INTR_INCLUDED

/******************************************************
 *   Local variable
 */
extern uint32 sys_intr_entry_tick;

#define TICKS_PER_US    ((BOARD_CPU_CLOCK)/1000000)
#define TEST_TIME       (50)
#define TEST_PORT_NUM   (8)

/******************************************************
 *   Public function.
 */
#ifdef UTGPIO_TIMER_DIVER
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

#endif

sys_error_t
utgpio_latency(void)
{
    int uport;
    uint32 t0, t1, delta, us;
    uint32 (*funcptr)(void) = (uint32 (*)(void))_getticks;

    SHR_FUNC_ENTER(0);

    /* Cache invalid. */
#ifndef __LINUX__
    CACHE_INVALID();
#endif

    t0 = funcptr();

    for (uport = 1; uport <= TEST_PORT_NUM; uport ++) {
        SHR_IF_ERR_CONT
            (board_port_class_set(uport, 0, 32));
    }
    t1 = funcptr();
    delta = t1 - t0;
    us = delta / TICKS_PER_US;
    SHR_LOG_DEBUG("%x %x %x %d\n", t1, t0, delta, us);
    delta = t1 - sys_intr_entry_tick;
    us = delta / TICKS_PER_US;
    SHR_LOG_DEBUG("%x %x %x %d\n", t1, sys_intr_entry_tick, delta, us);
    sal_printf("Interrupt latency: %d us port number = %d \n", us, TEST_PORT_NUM);

    SHR_FUNC_EXIT();
}

void
utgpio_latency_handler(uint32 param)
{
    (void)utgpio_latency();
}

sys_error_t
utgpio_intr_latency_test(void)  {

    bool val;
    int i;

    SHR_FUNC_ENTER(0);

    SHR_IF_ERR_EXIT
        (board_gpio_intr_func_set(TEST_GPIO, utgpio_latency_handler, 0));

    SHR_IF_ERR_EXIT
        (board_gpio_mode_set(TEST_GPIO, BOARD_GPIO_MODE_IN));

    SHR_IF_ERR_EXIT
        (board_gpio_intr_type_set
            (TEST_GPIO, BOARD_GPIO_INTR_DUAL_EDGE_TRIGGER));

    board_intr_handling_enable();

    SHR_IF_ERR_EXIT
        (board_gpio_mode_set(TEST_GPIO, BOARD_GPIO_MODE_OUT));

    SHR_IF_ERR_EXIT
        (board_gpio_value_set(TEST_GPIO, 1));

    SHR_IF_ERR_EXIT
        (board_gpio_intr_status_clear(TEST_GPIO));

    SHR_IF_ERR_EXIT
        (board_gpio_intr_enable_set(TEST_GPIO, 1));

    val = 0;
    SHR_IF_ERR_EXIT
        (board_gpio_value_set(TEST_GPIO, val));

    for (i = 0; i < TEST_TIME; i++ ) {
        /* For polled irq. */
        POLLED_IRQ();
        POLL();
        sal_usleep(5000);
        /* Flip GPIO value. */
        val = (val == 0);
        board_gpio_value_set(TEST_GPIO, val);
        board_wdt_ping();
    }

exit:
    SHR_FUNC_EXIT();
}

#endif /* CFG_INTR_INCLUDED */
