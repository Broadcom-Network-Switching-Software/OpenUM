/*! \file cmicx_gpio.h
 *
 *  CMICx GPIO driver header.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef CMICX_GPIO_H
#define CMICX_GPIO_H

#include <system.h>
#include <boardapi/gpio.h>

/**
 * \brief Set CMICx GPIO mode.
 *
 * \param [in] [in] unit Unit number.
 * \param [in] gpio GPIO number.
 * \param [in] mode GPIO mode.
 *
 * \retval SYS_OK No errors.
 */
extern int
cmicx_gpio_mode_set(int unit, uint32 gpio, board_gpio_mode_t mode);

/**
 * \brief Set CMICx GPIO value.
 *
 * \param [in] unit Unit number.
 * \param [in] gpio GPIO pin number.
 * \param [in] val GPIO value.
 *
 * \retval SYS_OK No errors.
 */
extern int
cmicx_gpio_value_set(int unit, uint32 gpio, bool val);

/**
 * \brief Get CMICx GPIO value.
 *
 * \param [in] unit Unit number.
 * \param [in] GPIO number.
 * \param [out] val GPIO value.
 *
 * \retval SYS_OK No errors.
 */
extern int
cmicx_gpio_value_get(int unit, uint32 gpio, bool *val);

/**
 * \brief Get CMICx interrupt handler function.
 *
 * \param [in] unit Unit number.
 * \param [in] GPIO number.
 * \param [in] intr_func Interrupt handler.
 * \param [in] intr_param Parameter for interrupt handler.
 *
 * \retval SYS_OK No errors.
 */
extern int
cmicx_gpio_intr_func_set(int unit, uint32 gpio,
                         board_intr_f intr_func, uint32 intr_param);

/*!
 * \brief Enable/disable GPIO interrupt.
 *
 * \param [in] unit Unit number.
 * \param [in] gpio GPIO pin number.
 * \param [in] enable Interrupt enable. 0: disable otherwise enable.
 *
 * \retval SYS_OK No errors.
 */
extern int
cmicx_gpio_intr_enable_set(int unit, uint32 gpio, bool enable);

/*!
 * \brief Set GPIO interrupt type.
 *
 * \param [in] unit Unit number.
 * \param [in] gpio GPIO pin number.
 * \param [in] type GPIO interrupt type.
 *
 * \retval SYS_OK No errors.
 */
extern int
cmicx_gpio_intr_type_set(int unit, uint32 gpio, board_gpio_intr_type_t type);

/*!
 * \brief Clear GPIO interrupt status.
 *
 * \param [in] unit Unit number.
 * \param [in] gpio GPIO pin number.
 *
 * \retval SYS_OK No errors.
 */
extern int
cmicx_gpio_intr_status_clear(int unit, uint32 gpio);

#endif /* CMICX_GPIO_H */
