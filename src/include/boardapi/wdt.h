/*! \file wdt.h
 *
 * Watchdog timer supported APIs.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef __WDT_H__
#define __WDT_H__

#ifdef CFG_WDT_INCLUDED

/*!
 * \brief Enable watchdog timer.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_wdt_enable(void);

/*!
 * \brief Disable watchdog timer.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_wdt_disable(void);

/*!
 * \brief Watchdog timer reset triggered or not.
 *
 * \retval TRUE if watchdog timer reset is triggered, otherwise FALSE.
 */
extern BOOL board_wdt_reset_triggered(void);

/*!
 * \brief Ping watchdog timer.
 *
 * Keep watchdog timer alive.
 */
extern void board_wdt_ping(void);

#endif /* CFG_WDT_INCLUDED */

#endif /* __WDT_H__ */
