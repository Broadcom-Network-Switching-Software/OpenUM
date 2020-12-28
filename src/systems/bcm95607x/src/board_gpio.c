/*! \file board_gpio.c
 *
 * BCM56070 GPIO Board APIs.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <system.h>

#include <boardapi/gpio.h>
#include <cmicx_gpio.h>

int
board_gpio_mode_set(uint32 gpio, board_gpio_mode_t mode)
{
    return cmicx_gpio_mode_set(0, gpio, mode);
}

int
board_gpio_value_set(uint32 gpio, bool val)
{
    return cmicx_gpio_value_set(0, gpio, val);
}

int
board_gpio_value_get(uint32 gpio, bool *pval)
{
    return cmicx_gpio_value_get(0, gpio, pval);
}

int
board_gpio_intr_func_set(uint32 gpio,
                         board_intr_f intr_func, uint32 intr_param)
{
    return cmicx_gpio_intr_func_set(0, gpio, intr_func, intr_param);
}

int
board_gpio_intr_enable_set(uint32 gpio, bool enable)
{
    return cmicx_gpio_intr_enable_set(0, gpio, enable);
}

int
board_gpio_intr_type_set(uint32 gpio, board_gpio_intr_type_t type)
{
    return cmicx_gpio_intr_type_set(0, gpio, type);
}

int
board_gpio_intr_status_clear(uint32 gpio)
{
    return cmicx_gpio_intr_status_clear(0, gpio);
}
