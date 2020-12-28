/*! \file wdt.h
 *
 * Watchdog timer supported API
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __WDT_H__
#define __WDT_H__

#include "config.h"

#ifdef CFG_WDT_INCLUDED

/*!
 * \brief Enable watchdog timer.
 *
 * \return void.
 */
extern void wdt_enable();

/*!
 * \brief Disable watchdog timer.
 *
 * \return void.
 */
extern void wdt_disable();

/*!
 * \brief Watchdog timer reset triggered or not.
 *
 * \return 1 if watchdog timer reset is triggered, otherwise 0.
 */
extern int wdt_reset_triggered();

#endif /* CFG_WDT_INCLUDED */

#endif /* __WDT_H__ */
