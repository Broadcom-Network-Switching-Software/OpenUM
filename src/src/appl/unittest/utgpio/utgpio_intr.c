/*
 * $Id: utgpio_intr.c,v 1.4 Broadcom SDK $
 *
 * GPIO unit test.
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

/******************************************************
 *   Local variable
 */

/* GPIO interrupt counter. */
int utgpio_intr_cnt = 0;

/* GPIO for testing. */
int utgpio = TEST_GPIO;

/* Default interrupt type. */
int utgpio_intr_type = BOARD_GPIO_INTR_DUAL_EDGE_TRIGGER;

/* GPIO interrupt mode list. */
static const int
gpio_intr_mode[] = {
    BOARD_GPIO_INTR_DUAL_EDGE_TRIGGER,
    BOARD_GPIO_INTR_FALL_EDGE_TRIGGER,
    BOARD_GPIO_INTR_RISE_EDGE_TRIGGER,
    BOARD_GPIO_INTR_LOW_LEVEL_TRIGGER,
    BOARD_GPIO_INTR_HIGH_LEVEL_TRIGGER,
};

/******************************************************
 *   Public function.
 */
sys_error_t
utgpio_intr(void) {

    int i;
    sys_error_t rv = SYS_OK;
    int fail;

    board_gpio_intr_func_set(utgpio, utgpio_intr_handler, 0);
    board_gpio_mode_set(utgpio, BOARD_GPIO_MODE_IN);
    board_intr_handling_enable();

    for (i = 0; i < COUNTOF(gpio_intr_mode); i ++) {
        fail = 0;
        utgpio_intr_type = gpio_intr_mode[i];
        board_gpio_intr_enable_set(utgpio, 0);
        board_gpio_intr_type_set(utgpio, utgpio_intr_type);
        switch (utgpio_intr_type) {
        case BOARD_GPIO_INTR_DUAL_EDGE_TRIGGER:
            TEST_HEADER("Test GPIO dual edge interrupt");
            board_gpio_mode_set(utgpio, BOARD_GPIO_MODE_OUT);
            board_gpio_value_set(utgpio, 1);
            board_gpio_intr_status_clear(utgpio);
            utgpio_intr_cnt = 0;
            board_gpio_intr_enable_set(utgpio, 1);
            sal_printf("1.Set GPIO from 1 -> 0 (expect only one interrupt).\n");
            board_gpio_value_set(utgpio, 0);
            POLLED_IRQ();
            if (utgpio_intr_cnt != 1) {
                fail = 1;
                break;
            }
            sal_printf("2.Set GPIO from 0 -> 1 (expect only one interrupt).\n");
            board_gpio_value_set(utgpio, 1);
            POLLED_IRQ();
            if (utgpio_intr_cnt != 2) {
                fail = 1;
            }
            break;
        case BOARD_GPIO_INTR_FALL_EDGE_TRIGGER:
            TEST_HEADER("Test GPIO falling edge interrupt");
            board_gpio_mode_set(utgpio, BOARD_GPIO_MODE_OUT);
            board_gpio_value_set(utgpio, 1);
            board_gpio_intr_status_clear(utgpio);
            board_gpio_intr_enable_set(utgpio, 1);
            sal_printf("1.Set GPIO from 1 -> 0 (expect only one interrupt).\n");
            utgpio_intr_cnt = 0;
            board_gpio_value_set(utgpio, 0);
            POLLED_IRQ();

            if (utgpio_intr_cnt != 1) {
                fail = 1;
                break;
            }

            sal_printf("2.Set GPIO from 0 -> 1 (expect no interrupt.)\n");
            board_gpio_value_set(utgpio, 1);
            POLLED_IRQ();
            if (utgpio_intr_cnt != 1) {
                fail = 1;
            }
            break;
        case BOARD_GPIO_INTR_RISE_EDGE_TRIGGER:
            TEST_HEADER("Test GPIO rising edge interrupt");
            board_gpio_mode_set(utgpio, BOARD_GPIO_MODE_OUT);
            board_gpio_value_set(utgpio, 0);
            board_gpio_intr_status_clear(utgpio);
            board_gpio_intr_enable_set(utgpio, 1);
            sal_printf("1.Set GPIO from 0 -> 1 (expect only one interrupt).\n");
            utgpio_intr_cnt = 0;
            board_gpio_value_set(utgpio, 1);
            POLLED_IRQ();
            if (utgpio_intr_cnt != 1) {
                fail = 1;
                break;
            }

            sal_printf("2.Set GPIO from 1 -> 0 (expect no interrupt.)\n");
            utgpio_intr_cnt = 0;
            board_gpio_value_set(utgpio, 0);
            POLLED_IRQ();
            if (utgpio_intr_cnt) {
                fail = 1;
            }
            break;
        case BOARD_GPIO_INTR_LOW_LEVEL_TRIGGER:
            TEST_HEADER("Test GPIO low level trigger");
            board_gpio_mode_set(utgpio, BOARD_GPIO_MODE_OUT);
            board_gpio_value_set(utgpio, 1);
            board_gpio_intr_status_clear(utgpio);
            board_gpio_intr_enable_set(utgpio, 1);
            sal_printf("1.Set GPIO as 0 and interrupt happen.\n");

            utgpio_intr_cnt = 0;
            board_gpio_value_set(utgpio, 0);
            POLLED_IRQ();
            if (utgpio_intr_cnt == 0) {
                fail = 1;
                break;
            }

            sal_printf("2.Set GPIO as 1 and expect no interrupt happen.\n");
            /* Set to 1 */
            board_gpio_value_set(utgpio, 1);
            board_gpio_intr_status_clear(utgpio);

            /* Wait for a while. No interrupt expected. */
            utgpio_intr_cnt = 0;
            for (i = 0; i < 30; i++) {
                POLLED_IRQ();
                sal_usleep(100);
            }
            if (utgpio_intr_cnt) {
                fail = 1;
            }
            break;
        case BOARD_GPIO_INTR_HIGH_LEVEL_TRIGGER:
            TEST_HEADER("Test GPIO high level trigger");

            board_gpio_mode_set(utgpio, BOARD_GPIO_MODE_OUT);
            board_gpio_value_set(utgpio, 0);
            board_gpio_intr_status_clear(utgpio);
            board_gpio_intr_enable_set(utgpio, 1);

            sal_printf("1.Set GPIO as 1 and interrupt happen.\n");
            board_gpio_value_set(utgpio, 1);
            POLLED_IRQ();
            if (utgpio_intr_cnt == 0) {
                fail = 1;
                break;
            }

            sal_printf("2.Set GPIO as 0 and expect no interrupt happen.\b");
            /* Set to 0 */
            board_gpio_value_set(utgpio, 0);
            board_gpio_intr_status_clear(utgpio);

            /* Wait for a while. No interrupt expected. */
            utgpio_intr_cnt = 0;
            for (i = 0; i < 30; i++) {
                POLLED_IRQ();
                sal_usleep(100);
            }
            if (utgpio_intr_cnt) {
                fail = 1;
            }
            break;
        default:
            break;
        }

        board_gpio_intr_enable_set(utgpio, 0);
        board_gpio_value_set(utgpio, 1);
        board_gpio_mode_set(utgpio, BOARD_GPIO_MODE_IN);

        if (fail) {
            sal_printf("Fail\n");
        } else {
            sal_printf("Success\n");
        }

    }
    return rv;
}
#endif /* CFG_INTR_INCLUDED */
