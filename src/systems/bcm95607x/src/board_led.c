/*! \file board_led.c
 *
 * BCM56070 board APIs for LED module.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include <system.h>
#include <boardapi/led.h>
#include <led.h>

#include <utils/shr/shr_debug.h>

sys_error_t
board_led_uc_num_get(int *led_uc_num)
{
    return led_uc_num_get(0, led_uc_num);
}

sys_error_t
board_led_fw_start_set(int led_uc, int start)
{
    if (start) {
        return led_fw_start(0, led_uc);
    } else {
        return led_fw_stop(0, led_uc);
    }
}

sys_error_t
board_led_fw_start_get(int led_uc, int *start)
{
    return led_fw_start_get(0, led_uc, start);
}

sys_error_t
board_led_control_data_read(int led_uc, int offset, uint8 *data, int len)
{
    int i;
    uint8 data8;

    SHR_FUNC_ENTER(0);
    SHR_NULL_CHECK(data, SYS_ERR_PARAMETER);

    for (i = 0; i < len; i ++) {
        SHR_IF_ERR_EXIT
            (led_control_data_read(0, led_uc, i + offset, &data8));
        data[i] = data8;
    }

exit:
    SHR_FUNC_EXIT();
}

sys_error_t
board_led_control_data_write(int led_uc, int offset, const uint8 *data, int len)
{
    int i;
    uint8 data8;

    SHR_FUNC_ENTER(0);
    SHR_NULL_CHECK(data, SYS_ERR_PARAMETER);

    for (i = 0; i < len; i ++) {
        data8 = data[i];
        SHR_IF_ERR_EXIT
            (led_control_data_write(0, led_uc, i + offset, data8));
    }

exit:
    SHR_FUNC_EXIT();
}

sys_error_t
board_led_pport_to_led_uc_port(int port, int *led_uc, int *led_uc_port)
{
    SHR_FUNC_ENTER(0);

    SHR_NULL_CHECK(led_uc, SYS_ERR_PARAMETER);
    SHR_NULL_CHECK(led_uc_port, SYS_ERR_PARAMETER);

    SHR_IF_ERR_EXIT
        (led_pport_to_led_uc_port(0, port, led_uc, led_uc_port));

exit:
    SHR_FUNC_EXIT();
}

sys_error_t
board_led_fw_load(int led_uc, const uint8 *data, int len)
{
    SHR_FUNC_ENTER(0);

    SHR_NULL_CHECK(data, SYS_ERR_PARAMETER);

    SHR_IF_ERR_EXIT
        (led_fw_load(0, led_uc, data, len));

exit:
    SHR_FUNC_EXIT();
}

/*!
 * \biref LED behavior mode setting.
 *        Loop detected mode and normal mode.
 * \param [in] lport Logical port.
 * \param [in] mode LED mode.
 *
 * \retval TRUE No errors.
 */
BOOL
board_phy_led_mode_set(uint8 lport, int mode)
{

#ifdef CFG_LED_MICROCODE_INCLUDED
    switch (mode) {
        case BOARD_PHY_LED_LOOP_FOUND:
        break;
        case BOARD_PHY_LED_NORMAL:
        default:
        break;
    }
#endif
    return TRUE;
}

