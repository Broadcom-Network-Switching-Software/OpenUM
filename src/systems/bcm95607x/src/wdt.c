/*! \file wdt.c
 *
 * Watchdog timer routines
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"

#ifdef CFG_WDT_INCLUDED

#define WDT_CLOCK_RATE             (166750000)
#define CFG_WDT_BASE               0x3231000
#define WDT_LOAD                   (CFG_WDT_BASE + 0x0)
#define WDT_VALUE                  (CFG_WDT_BASE + 0x4)
#define WDT_CTRL                   (CFG_WDT_BASE + 0x8)
#define REST_ENABLE                0x2
#define INTR_ENABLE                0x1
#define WDT_INT_CLR                (CFG_WDT_BASE + 0xC)
#define CRU_RESET_REASON           0x3240014
#define WD_RESET                   0x1

static uint32 wdt_reset_flag = 0;
static uint32 wdt_reset_checked = 0;
static float wdt_load_value = .0;

static
void wdt_task(void *param)
{
    /* Clear timer interrupt if any  */
    SYS_REG_WRITE32(WDT_INT_CLR, 0x1);

    /* Set load value */
    SYS_REG_WRITE32(WDT_LOAD, (uint32)wdt_load_value);
}

void wdt_enable(void)
{
    /* Set watchdog timer load value corresponding to CFG_WDT_TIMEOUT / 2 */
    wdt_load_value = (CFG_WDT_TIMEOUT / 1000000.) * WDT_CLOCK_RATE / 2;
    SYS_REG_WRITE32(WDT_LOAD, (uint32)wdt_load_value);

    /* Add watchdog task, periodically running per (CFG_WDT_TIMEOUT >> 1) us */
    timer_add(wdt_task, NULL, CFG_WDT_TIMEOUT >> 1);

    /* Enable watchdog timer */
    SYS_REG_WRITE32(WDT_CTRL, INTR_ENABLE + REST_ENABLE);
}

void wdt_disable(void)
{
    /* Disable watchdog timer */
    SYS_REG_WRITE32(WDT_CTRL, 0);

    /* Remove watchdog task */
    timer_remove(wdt_task);
}

int wdt_reset_triggered(void)
{
    if (!wdt_reset_checked) {
        uint32 reason;

        reason = SYS_REG_READ32(CRU_RESET_REASON);
        if (reason & WD_RESET) {
            wdt_reset_flag = 1;
            /* clear reset reason */
            SYS_REG_WRITE32(CRU_RESET_REASON, WD_RESET);
        }

        wdt_reset_checked = 1;
    }

    return wdt_reset_flag;
}

#endif /* CFG_WDT_INCLUDED */
