/*! \file utled.c
 *
 * LED and hardware linkscan test case.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_LED_INCLUDED

#include <utils/shr/shr_debug.h>
#include <boardapi/base.h>
#include <boardapi/led.h>
#include <boardapi/link.h>
#include <binfs.h>

#include <utled.h>

#define LED_HW_RX              (1 << 0)
#define LED_HW_TX              (1 << 1)
#define LED_HW_COLLISION       (1 << 2)
#define LED_HW_SPEED           (3 << 3)
#define LED_HW_DUPLEX          (1 << 5)
#define LED_HW_FLOW_CTRL       (1 << 6)
#define LED_HW_LINKUP          (1 << 7)
#define LED_HW_ENABLE          (1 << 8)

#define USE_HW_LINKSCAN   1
#define TEST_DELAY        2000
/*******************************************
 *   Local definition.
 */
static uint16 test_uport;
static int test_link = 0;

static const uint8 cmicx_test_custom_led_bin[] = {
    0xF0, 0xB5, 0x5F, 0x46, 0x56, 0x46, 0x4D, 0x46, 0x44, 0x46, 0xF0, 0xB4, 0x83, 0xB0, 0xC0, 0x26,
    0x76, 0x00, 0x00, 0x22, 0x80, 0x23, 0x5D, 0x00, 0xAC, 0x46, 0x7F, 0x23, 0x19, 0x40, 0x03, 0x27,
    0xB9, 0x46, 0x02, 0x23, 0x9A, 0x46, 0x01, 0x25, 0xA8, 0x46, 0x67, 0x46, 0x01, 0x97, 0x8B, 0x46,
    0x91, 0x00, 0x03, 0x68, 0x5C, 0x5A, 0x43, 0x68, 0x5B, 0x5A, 0x01, 0x9D, 0x2C, 0x42, 0x0F, 0xD0,
    0x4F, 0x46, 0xBB, 0x43, 0x55, 0x46, 0x2B, 0x43, 0x9B, 0xB2, 0x47, 0x46, 0x27, 0x42, 0x01, 0xD0,
    0x08, 0x25, 0x2B, 0x43, 0x02, 0x25, 0x25, 0x42, 0x07, 0xD0, 0x04, 0x25, 0x2B, 0x43, 0x04, 0xE0,
    0x00, 0x27, 0xBB, 0x45, 0x01, 0xD1, 0xFF, 0x25, 0xAB, 0x43, 0x85, 0x68, 0x27, 0x4F, 0xEF, 0x5D,
    0xBC, 0x46, 0x01, 0x27, 0xBC, 0x45, 0x05, 0xD1, 0xAC, 0x55, 0x85, 0x68, 0xAD, 0x19, 0x24, 0x0A,
    0x6C, 0x70, 0x03, 0xE0, 0xAD, 0x18, 0x81, 0x35, 0xFF, 0x35, 0x2B, 0x70, 0x44, 0x68, 0x63, 0x52,
    0x01, 0x32, 0x02, 0x36, 0xBE, 0x23, 0x5B, 0x00, 0x9A, 0x42, 0xC9, 0xD1, 0x01, 0x22, 0x01, 0x26,
    0x00, 0x25, 0x3F, 0x21, 0x88, 0x46, 0x04, 0x24, 0x40, 0x23, 0x9C, 0x46, 0x7F, 0x27, 0xBA, 0x46,
    0x01, 0xE0, 0x01, 0x32, 0xD2, 0xB2, 0x53, 0x1E, 0xDB, 0xB2, 0x00, 0x2B, 0x02, 0xD0, 0x01, 0x2B,
    0x17, 0xD1, 0x0A, 0xE0, 0x19, 0x01, 0x41, 0x18, 0xCE, 0x60, 0x0D, 0x61, 0x01, 0x33, 0x1B, 0x01,
    0xC3, 0x18, 0x41, 0x46, 0x59, 0x60, 0x9C, 0x60, 0xEB, 0xE7, 0x19, 0x01, 0x41, 0x18, 0xCE, 0x60,
    0x67, 0x46, 0x0F, 0x61, 0x01, 0x33, 0x1B, 0x01, 0xC3, 0x18, 0x51, 0x46, 0x59, 0x60, 0x9C, 0x60,
    0xDF, 0xE7, 0x1B, 0x01, 0xC3, 0x18, 0xDD, 0x60, 0x04, 0x2A, 0xDA, 0xD9, 0x03, 0xB0, 0x3C, 0xBC,
    0x90, 0x46, 0x99, 0x46, 0xA2, 0x46, 0xAB, 0x46, 0xF0, 0xBD, 0xC0, 0x46, 0x7F, 0x01, 0x00, 0x00,
};

#if USE_HW_LINKSCAN
static void
linkchange_hw_handler(uint8* uplist)
{
    uint8 data8;
    int pport;
    int led_uc;
    int led_uc_port;
    uint8 unit, lport;

    SHR_FUNC_ENTER(0);

    /* Check link up event. */
    if (uplist_port_matched(uplist, test_uport) == SYS_OK) {
        test_link = 1;
        data8 = 1;
        sal_printf("Port %d link up.\n", test_uport);
    } else {
        test_link = 0;
        data8 = 0;
        sal_printf("Port %d link down.\n", test_uport);
    }

    board_uport_to_lport(test_uport, &unit, &lport);

    pport = SOC_PORT_L2P_MAPPING(lport);

    SHR_IF_ERR_EXIT
        (board_led_pport_to_led_uc_port(pport, &led_uc, &led_uc_port));

    SHR_IF_ERR_EXIT
        (board_led_control_data_write(led_uc, led_uc_port,
                                      &data8, sizeof(data8)));
exit:
    return;
}

static int
utled_init(void)
{
    uint8 uplist[MAX_UPLIST_WIDTH];

    SHR_FUNC_ENTER(0);

    SHR_IF_ERR_EXIT
        (board_hw_linkchange_intr_func_set(linkchange_hw_handler));

    for (test_uport = 0; test_uport < board_uport_count(); test_uport ++) {
        uplist[test_uport / 8] |= (1 << (test_uport % 8));
    }

    SHR_IF_ERR_EXIT
        (board_hw_linkchange_config(uplist));

exit:
    SHR_FUNC_EXIT();
}

static int
utled_deinit(void)
{
    SHR_FUNC_ENTER(0);

    SHR_IF_ERR_EXIT
        (board_hw_linkchange_intr_func_set(NULL));

exit:
    SHR_FUNC_EXIT();
}

#else

static void
linkchange_handler(uint16 port, BOOL link, void *arg)
{
    uint8 data8;
    int led_uc;
    int led_uc_port, pport;
    uint8 unit, lport;

    SHR_FUNC_ENTER(0);

    if (link) {
        test_link = 1;
        sal_printf("port %d link up.\n", port);
        data8 = 1;
    } else {
        test_link = 0;
        sal_printf("port %d link down.\n", port);
        data8 = 0;
    }

    board_uport_to_lport(port, &unit, &lport);

    pport = SOC_PORT_L2P_MAPPING(lport);

    SHR_IF_ERR_EXIT
        (board_led_pport_to_led_uc_port(pport, &led_uc, &led_uc_port));

    SHR_IF_ERR_EXIT
        (board_led_control_data_write(led_uc, led_uc_port,
                                      &data8, sizeof(data8)));
exit:
    return;
}

static int
utled_init(void)
{
    if (sys_register_linkchange(linkchange_handler, NULL) == TRUE) {
        return SYS_OK;
    }
    return SYS_ERR;
}

static int
utled_deinit(void)
{
    sys_unregister_linkchange(linkchange_handler);
    return SYS_OK;
}
#endif

sys_error_t
utled_test(void)
{
    int fail = 0;
    uint8 data[2], data8, link;
    int pport;
    int led_uc;
    int led_uc_port;
    uint8 unit, lport;
    uint16 pattern, accu;

    SHR_FUNC_ENTER(0);

    /* Customer LED firmware for testing. */
    SHR_IF_ERR_EXIT
        (board_led_fw_start_set(0, 0));
    SHR_IF_ERR_EXIT
        (board_led_fw_load(0, cmicx_test_custom_led_bin,
                           sizeof(cmicx_test_custom_led_bin)));
    SHR_IF_ERR_EXIT
        (board_led_fw_start_set(0, 1));

    /* Init linkchange handler to update LED status as linkchange. */
    SHR_IF_ERR_EXIT
        (utled_init());

    board_wdt_ping();

    /* Test ports one by one. */
    for (test_uport = 1; test_uport <= board_uport_count(); test_uport++) {

        test_link = 0;

        /* Enable internal PHY loopback. */
        sal_printf("Enable port %d internal PHY loopback\n", test_uport);
        SHR_IF_ERR_EXIT
            (board_port_loopback_set(test_uport, PORT_LOOPBACK_PHY));

        /* Awaiting for link up and execution of link change handler. */
        sal_sleep(TEST_DELAY);

        /* Start to collect LED status. */
        board_uport_to_lport(test_uport, &unit, &lport);

        pport = SOC_PORT_L2P_MAPPING(lport);

        SHR_IF_ERR_EXIT
            (board_led_pport_to_led_uc_port(pport, &led_uc, &led_uc_port));

        /* Disable interrupt to prevent race condition on led_control_data */
        board_intr_handling_disable();

        SHR_IF_ERR_EXIT
            (board_led_control_data_read(led_uc, led_uc_port,
                                         &link, sizeof(link)));

        /* Read accumulation RAM. */
        data8 = 1;
        SHR_IF_ERR_EXIT
            (board_led_control_data_write(0, 383, &data8, 1));

        /* Awaiting for Accumulation RAM data ready. (typically, 30ms) */
        sal_sleep(TEST_DELAY);

        SHR_IF_ERR_EXIT
            (board_led_control_data_read(led_uc, (led_uc_port * 2) + 384,
                                         data, 2));

        accu = data[0] | (data[1] << 8);

        /* Read send pattern RAM. */
        data8 = 0;
        SHR_IF_ERR_EXIT
            (board_led_control_data_write(0, 383, &data8, 1));

        /* Awaiting for send pattern data ready. (typically, 30ms) */
        sal_sleep(TEST_DELAY);

        SHR_IF_ERR_EXIT
            (board_led_control_data_read(led_uc, led_uc_port + 384,
                                         data, 1));

        pattern = data[0];

        board_intr_handling_enable();

        sal_printf("pattern = %x accu = %x link %d test_link %d\n",
                   pattern, accu, link, test_link);

        /* Check link, accumulation RAM and send pattern RAM .*/
        if (link != test_link || !(accu & LED_HW_ENABLE) || !(pattern & 0x3)) {
            fail ++;
        }

        test_link = 1;

        board_wdt_ping();

        /* Disable internal PHY loopback. */
        sal_printf("Disable port %d internal PHY loopback\n", test_uport);
        SHR_IF_ERR_EXIT
            (board_port_loopback_set(test_uport, PORT_LOOPBACK_NONE));

        /* Awaiting for link down and execution of link change handler. */
        sal_sleep(TEST_DELAY);

        /* Start to collect LED status. */
        board_uport_to_lport(test_uport, &unit, &lport);

        pport = SOC_PORT_L2P_MAPPING(lport);

        SHR_IF_ERR_EXIT
            (board_led_pport_to_led_uc_port(pport, &led_uc, &led_uc_port));

        /* Disable interrupt to prevent race condition on led_control_data */
        board_intr_handling_disable();

        sal_printf("uport = %d pport = %d lport = %d led_uc = %d led_uc_port = %d\n",
                   test_uport, pport, lport, led_uc, led_uc_port);

        SHR_IF_ERR_EXIT
            (board_led_control_data_read(led_uc, led_uc_port,
                                          &link, sizeof(link)));

        /* Read accumulation RAM. */
        data8 = 1;
        SHR_IF_ERR_EXIT
            (board_led_control_data_write(0, 383, &data8, 1));

        /* Awaiting for Accumulation RAM data ready. (typically, 30ms) */
        sal_sleep(TEST_DELAY);

        SHR_IF_ERR_EXIT
            (board_led_control_data_read(led_uc, (led_uc_port * 2) + 384,
                                         data, 2));

        accu = data[0] | (data[1] << 8);

        /* Read send pattern RAM. */
        data8 = 0;
        SHR_IF_ERR_EXIT
            (board_led_control_data_write(0, 383, &data8, 1));

        /* Awaiting for send pattern data ready. (typically, 30ms) */
        sal_sleep(TEST_DELAY);

        SHR_IF_ERR_EXIT
            (board_led_control_data_read(led_uc, led_uc_port + 384, data, 1));

        pattern = data[0];

        board_intr_handling_enable();

        /* Check link, accumulation RAM and send pattern RAM .*/
        sal_printf("pattern = %x accu = %x link %d test_link %d\n",
                   pattern, accu, link, test_link);
        if (link != test_link || (pattern & LED_HW_ENABLE) || (pattern & 0x3)) {
            fail ++;
        }
        board_wdt_ping();
    }

exit:
    sal_printf("Total = %d\n", board_uport_count());
    sal_printf("Fail = %d\n", fail);

    utled_deinit();
    board_intr_handling_enable();

    SHR_FUNC_EXIT();
}

#endif /* CFG_SDKCLI_LED_INCLUDED */
