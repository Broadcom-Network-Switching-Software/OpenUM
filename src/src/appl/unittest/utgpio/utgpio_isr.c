/*
 * $Id: utgpio_isr.c,v 1.4 Broadcom SDK $
 *
 * GPIO interrupt handler for unit test.
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <system.h>
#include <boardapi/gpio.h>
#include <boardapi/intr.h>
/* #define DEBUG */
#include "utgpio_internal.h"

#ifdef CFG_INTR_INCLUDED
void
utgpio_intr_handler(uint32 param)
{
    COMPILER_REFERENCE(param);

    utgpio_intr_cnt++;

    DEBUG_PRINT("Enter utgpio intr hanlder ....\n");

    /* Set GPIO level to stop interrupt trigger. */
    if (utgpio_intr_type == BOARD_GPIO_INTR_LOW_LEVEL_TRIGGER) {
        board_gpio_value_set(utgpio, 1);
    } else if (utgpio_intr_type == BOARD_GPIO_INTR_HIGH_LEVEL_TRIGGER) {
        board_gpio_value_set(utgpio, 0);
    }

    DEBUG_PRINT("Leave utgpio intr hanlder ....\n");
}

#endif /* CFG_INTR_INCLUDED */
