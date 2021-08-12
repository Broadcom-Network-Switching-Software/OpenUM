/*! \file wdt.c
 *
 * Watchdog timer routines
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"

#ifdef CFG_WDT_INCLUDED

#define WDT_CLOCK_RATE      (BOARD_CPU_CLOCK >> 2)

static BOOL wdt_reset_flag = FALSE;
static BOOL wdt_reset_checked = FALSE;
static uint32 wdt_load_value = 0;

static void
wdt_task(void *param)
{
    IPROCGENRES_WDT0_WDT_WDOGINTCLRr_t wdt_intclr;
    IPROCGENRES_WDT0_WDT_WDOGLOADr_t wdt_load;

    /* Clear timer interrupt if any  */
    IPROCGENRES_WDT0_WDT_WDOGINTCLRr_SET(wdt_intclr, 1);
    WRITE_IPROCGENRES_WDT0_WDT_WDOGINTCLRr(0, wdt_intclr);

    /* Set load value */
    IPROCGENRES_WDT0_WDT_WDOGLOADr_CLR(wdt_load);
    IPROCGENRES_WDT0_WDT_WDOGLOADr_WDOGLOADf_SET(wdt_load, \
                                                    (uint32)wdt_load_value);
    WRITE_IPROCGENRES_WDT0_WDT_WDOGLOADr(0, wdt_load);
}

sys_error_t
board_wdt_enable(void)
{
    IPROCGENRES_WDT0_WDT_WDOGLOADr_t wdt_load;
    IPROCGENRES_WDT0_WDT_WDOGCONTROLr_t wdt_ctrl;

    /* Set watchdog timer load value corresponding to CFG_WDT_TIMEOUT / 2 */
    wdt_load_value = (CFG_WDT_TIMEOUT / 1000000.) * WDT_CLOCK_RATE / 2;
    IPROCGENRES_WDT0_WDT_WDOGLOADr_CLR(wdt_load);
    IPROCGENRES_WDT0_WDT_WDOGLOADr_WDOGLOADf_SET(wdt_load, wdt_load_value);
    WRITE_IPROCGENRES_WDT0_WDT_WDOGLOADr(0, wdt_load);

    /* Add watchdog task, periodically running per (CFG_WDT_TIMEOUT >> 1) us */
    timer_add(wdt_task, NULL, CFG_WDT_TIMEOUT >> 1);

    /* Enable watchdog timer */
    IPROCGENRES_WDT0_WDT_WDOGCONTROLr_CLR(wdt_ctrl);
    IPROCGENRES_WDT0_WDT_WDOGCONTROLr_ITENf_SET(wdt_ctrl, 1);
    IPROCGENRES_WDT0_WDT_WDOGCONTROLr_RESENf_SET(wdt_ctrl, 1);
    WRITE_IPROCGENRES_WDT0_WDT_WDOGCONTROLr(0, wdt_ctrl);

    return SYS_OK;
}

sys_error_t
board_wdt_disable(void)
{
    IPROCGENRES_WDT0_WDT_WDOGCONTROLr_t wdt_ctrl;

    /* Disable watchdog timer */
    IPROCGENRES_WDT0_WDT_WDOGCONTROLr_CLR(wdt_ctrl);
    WRITE_IPROCGENRES_WDT0_WDT_WDOGCONTROLr(0, wdt_ctrl);

    /* Remove watchdog task */
    timer_remove(wdt_task);

    return SYS_OK;
}

BOOL
board_wdt_reset_triggered(void)
{
    if (!wdt_reset_checked) {
        DMU_PCU_CRU_RESET_REASONr_t reason;

        READ_DMU_PCU_CRU_RESET_REASONr(0, reason);
        if (DMU_PCU_CRU_RESET_REASONr_WATCHDOG_RESETf_GET(reason)) {
            wdt_reset_flag = TRUE;
            /* clear reset reason */
            WRITE_DMU_PCU_CRU_RESET_REASONr(0, reason);
        }

        wdt_reset_checked = TRUE;
    }

    return wdt_reset_flag;
}

void board_wdt_ping(void) __attribute__((section(".2ram")));
void
board_wdt_ping(void)
{
    IPROCGENRES_WDT0_WDT_WDOGINTCLRr_t wdt_intclr;
    IPROCGENRES_WDT0_WDT_WDOGLOADr_t wdt_load;

    /* Clear timer interrupt if any  */
    IPROCGENRES_WDT0_WDT_WDOGINTCLRr_SET(wdt_intclr, 1);
    WRITE_IPROCGENRES_WDT0_WDT_WDOGINTCLRr(0, wdt_intclr);

    /* Set load value */
    IPROCGENRES_WDT0_WDT_WDOGLOADr_CLR(wdt_load);
    IPROCGENRES_WDT0_WDT_WDOGLOADr_WDOGLOADf_SET(wdt_load, wdt_load_value);
    WRITE_IPROCGENRES_WDT0_WDT_WDOGLOADr(0, wdt_load);
}

#endif /* CFG_WDT_INCLUDED */
