/*! \file led.c
 *
 * Top-level LED API.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#include <system.h>
#include <utils/shr/shr_debug.h>
#include <led_internal.h>

/*******************************************************************************
 * Local variables
 */
/*! LED driver functions. */
static const led_drv_t *bd_led_drv[CONFIG_MAX_UNITS] = {NULL};

/*! LED per-device data. */
static led_dev_t bd_led_dev[CONFIG_MAX_UNITS];

/*******************************************************************************
 * Public functions
 */

int
led_drv_init(int unit, const led_drv_t *led_drv)
{
    if (!UNIT_VALID(unit)) {
        return SYS_ERR_PARAMETER;
    }

    bd_led_drv[unit] = led_drv;

    return SYS_OK;
}

int
led_dev_get(int unit, led_dev_t **led_dev)
{
    if (!UNIT_VALID(unit)) {
        return SYS_ERR_PARAMETER;
    }

    *led_dev = &bd_led_dev[unit];

    return SYS_OK;
}

int
led_init(int unit)
{
    const led_drv_t *led_drv;

    if (!UNIT_VALID(unit)) {
        return SYS_ERR_PARAMETER;
    }

    led_drv = bd_led_drv[unit];
    if (led_drv && led_drv->init) {
        return led_drv->init(unit);
    }

    return SYS_OK;
}

int
led_cleanup(int unit)
{
    const led_drv_t *led_drv;

    if (!UNIT_VALID(unit)) {
        return SYS_ERR_PARAMETER;
    }

    led_drv = bd_led_drv[unit];
    if (led_drv && led_drv->cleanup) {
        return led_drv->cleanup(unit);
    }
    return SYS_OK;
}


int
led_fw_load(int unit, int led_uc, const uint8_t *buf, int len)
{
    const led_drv_t *led_drv;

    if (!UNIT_VALID(unit)) {
        return SYS_ERR_PARAMETER;
    }

    led_drv = bd_led_drv[unit];
    if (led_drv && led_drv->fw_load) {
        return led_drv->fw_load(unit, led_uc, buf, len);
    }
    return SYS_ERR_UNAVAIL;
}

int
led_uc_num_get(int unit, int *uc_num)
{
    const led_drv_t *led_drv;

    if (!UNIT_VALID(unit)) {
        return SYS_ERR_PARAMETER;
    }

    led_drv = bd_led_drv[unit];
    if (led_drv && led_drv->uc_num_get) {
        return led_drv->uc_num_get(unit, uc_num);
    }
    return SYS_ERR_UNAVAIL;
}

int
led_fw_start(int unit, int led_uc)
{
    const led_drv_t *led_drv;

    if (!UNIT_VALID(unit)) {
        return SYS_ERR_PARAMETER;
    }

    led_drv = bd_led_drv[unit];
    if (led_drv && led_drv->uc_start_set) {
        return led_drv->uc_start_set(unit, led_uc, 1);
    }
    return SYS_ERR_UNAVAIL;
}

int
led_fw_stop(int unit, int led_uc)
{
    const led_drv_t *led_drv;

    if (!UNIT_VALID(unit)) {
        return SYS_ERR_PARAMETER;
    }

    led_drv = bd_led_drv[unit];
    if (led_drv && led_drv->uc_start_set) {
        return led_drv->uc_start_set(unit, led_uc, 0);
    }
    return SYS_ERR_UNAVAIL;
}

int
led_fw_start_get(int unit, int led_uc, int *start)
{
    const led_drv_t *led_drv;

    if (!UNIT_VALID(unit)) {
        return SYS_ERR_PARAMETER;
    }

    led_drv = bd_led_drv[unit];
    if (led_drv && led_drv->uc_start_get) {
        return led_drv->uc_start_get(unit, led_uc, start);
    }
    return SYS_ERR_UNAVAIL;


}

int
led_control_data_write(int unit, int led_uc, int offset, uint8_t value)
{

    const led_drv_t *led_drv;

    if (!UNIT_VALID(unit)) {
        return SYS_ERR_PARAMETER;
    }

    led_drv = bd_led_drv[unit];
    if (led_drv && led_drv->uc_control_data_write) {
        return led_drv->uc_control_data_write(unit, led_uc, offset, value);
    }
    return SYS_ERR_UNAVAIL;
}

int
led_control_data_read(int unit, int led_uc, int offset, uint8_t *value)
{

    const led_drv_t *led_drv;

    if (!UNIT_VALID(unit)) {
        return SYS_ERR_PARAMETER;
    }

    led_drv = bd_led_drv[unit];
    if (led_drv && led_drv->uc_control_data_read) {
        return led_drv->uc_control_data_read(unit, led_uc, offset, value);
    }
    return SYS_ERR_UNAVAIL;
}

int
led_pport_to_led_uc_port(int unit, int port, int *led_uc, int *led_uc_port)
{
    const led_drv_t *led_drv;

    if (!UNIT_VALID(unit)) {
        return SYS_ERR_PARAMETER;
    }

    led_drv = bd_led_drv[unit];
    if (led_drv && led_drv->pport_to_led_uc_port) {
        return led_drv->pport_to_led_uc_port(unit, port, led_uc, led_uc_port);
    }
    return SYS_ERR_UNAVAIL;
}
