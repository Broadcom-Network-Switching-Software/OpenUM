/*! \file cmicx_led.h
 *
 * CMICx specific LED APIs and base driver.
 *
 * The base driver implement common steps of all CMICx-LED drivers.
 *    - Download Broadcom CMICx-LED FW when LED driver initialization.
 *    - Download Customer CMICx-LED FW as API/CLI request.
 *    - Enable/Disable LED FW by mailbox driver.
 *    - Read/write LED FW's port data(control_data) by mailbox driver.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef CMICX_LED_H
#define CMICX_LED_H

/*!
 * \brief Setting LED refresh rate.
 *
 * \param [in] unit Unit number.
 * \param [in] src_clk_freq Source clock frequency(Hz).
 * \param [in] refresh_freq LED refresh rate(Hz).
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX Fail to set LED refresh rate.
 */
extern int
cmicx_led_refresh_freq_set(int unit, uint32 src_clk_freq,
                           uint32 refresh_freq);

/*!
 * \brief Setting LED output clock rate.
 *
 * \param [in] unit Unit number.
 * \param [in] src_clk_freq Source clock frequency(Hz).
 * \param [in] led_clk_freq LED output clock rate(Hz).
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX Fail to setup clock rate.
 */
extern int
cmicx_led_clk_freq_set(int unit, uint32 src_clk_freq,
                       uint32 led_clk_freq);

/*!
 * \brief Setting "last_port" of LED port chain for chip internal.
 *
 * \param [in] unit Unit number.
 * \param [in] last_port last_port setting for chip internal chain.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX Fail to set CMICx "last_port".
 */
extern int
cmicx_led_last_port_set(int unit, uint32 last_port);

#endif /* CMICX_LED_H */
