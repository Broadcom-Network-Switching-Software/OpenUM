/*! \file gpio.h
 *
 * GPIO board APIs.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef GPIO_H
#define GPIO_H

#include <types.h>

/*! Types of GPIO directions. */
typedef enum board_gpio_mode_e {

    /*! Input mode. */
    BOARD_GPIO_MODE_IN,

    /*! Output mode. */
    BOARD_GPIO_MODE_OUT

} board_gpio_mode_t;

/*! Types of GPIO interrupt. */
typedef enum board_gpio_intr_type_e {

    /*! Dual edge trigger. */
    BOARD_GPIO_INTR_DUAL_EDGE_TRIGGER,

    /*! Falling edge trigger. */
    BOARD_GPIO_INTR_FALL_EDGE_TRIGGER,

    /*! Rising edge trigger. */
    BOARD_GPIO_INTR_RISE_EDGE_TRIGGER,

    /*! Low level trigger. */
    BOARD_GPIO_INTR_LOW_LEVEL_TRIGGER,

    /*! High level trigger. */
    BOARD_GPIO_INTR_HIGH_LEVEL_TRIGGER,

} board_gpio_intr_type_t;

/*!
 * \brief Function prototype of board interrupt handler.
 *
 * \param [in] param Parameter for interrupt handler.
 */
typedef void (*board_intr_f) (uint32 param);

/*!
 * \brief Set GPIO mode.
 *
 * This function is used to configure the mode of the given GPIO pin.
 *
 * \param [in] gpio GPIO pin number.
 * \param [in] mode GPIO mode.
 *
 * \retval SYS_OK No errors.
 */
extern int
board_gpio_mode_set(uint32 gpio, board_gpio_mode_t mode);

/*!
 * \brief Set GPIO value.
 *
 * This function is used to set the value of the given GPIO pin.
 *
 * If \c gpio is configured as input, then this function has no effect.
 *
 * \param [in] gpio GPIO pin number.
 * \param [in] val GPIO value.
 *
 * \retval SYS_OK No errors.
 */
extern int
board_gpio_value_set(uint32 gpio, bool val);

/*!
 * \brief Get GPIO value.
 *
 * \param [in] gpio GPIO pin number.
 * \param [out] val GPIO value.
 *
 * \retval SYS_OK No errors.
 */
extern int
board_gpio_value_get(uint32 gpio, bool *val);

/*!
 * \brief Install/uninstall GPIO interrupt handler.
 *
 * \param [in] gpio GPIO pin number.
 * \param [in] intr_func Interrupt hanlder function.
 *                       Set intr_func as NULL to uninstall handler.
 * \param [in] intr_param Parameter for interrupt handler.
 *
 * \retval SYS_OK No errors.
 */
extern int
board_gpio_intr_func_set(uint32 gpio,
                         board_intr_f intr_func, uint32 intr_param);

/*!
 * \brief Enable/disable GPIO interrupt.
 *
 * \param [in] gpio GPIO pin number.
 * \param [in] enable Interrupt enable. 0: disable otherwise enable.
 *
 * \retval SYS_OK No errors.
 */
extern int
board_gpio_intr_enable_set(uint32 gpio, bool enable);

/*!
 * \brief Set GPIO interrupt type.
 *
 * \param [in] gpio GPIO pin number.
 * \param [in] type GPIO interrupt type.
 *
 * \retval SYS_OK No errors.
 */
extern int
board_gpio_intr_type_set(uint32 gpio, board_gpio_intr_type_t type);

/*!
 * \brief Clear GPIO interrupt status.
 *
 * \param [in] gpio GPIO pin number.
 *
 * \retval SYS_OK No errors.
 */
extern int
board_gpio_intr_status_clear(uint32 gpio);

#endif /* GPIO_H */
