/*! \file cmicx_m0ssq_fw.c
 *
 * This file includes APIs to initialize/cleanup firmware.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include <system.h>

#include <utils/shr/shr_debug.h>

#include <m0ssq.h>
#include <m0ssq_internal.h>

#include "cmicx_m0ssq_fw.h"

/*******************************************************************************
 * Local definitions.
 */

/* Number of uC for CMICx-LED and CMICx-Linkscan. */
#define CMICX_FW_MAX_NUM_OF_UC       (2)

/*******************************************************************************
 * Local variables.
 */

/*! Bit map to show which feature use cmicx_linkscan_led_fw. */
static uint32_t
cmicx_linkscan_led_fw_usage[CONFIG_MAX_UNITS][CMICX_FW_MAX_NUM_OF_UC] = {{0}};

/*! Flag indicates firmware is used by LED. */
#define FW_USED_BY_LED              (0x1)

/*! Flag indicates firmware is used by Linkscan. */
#define FW_USED_BY_LINKSCAN         (0x2)

/*******************************************************************************
 * Internal Functions.
 */
static int
cmicx_m0ssq_fw_led_linkscan_init(int unit, int uc)
{

    SHR_FUNC_ENTER(unit);

    COMPILER_REFERENCE(cmicx_linkscan_led_fw_type);
    COMPILER_REFERENCE(cmicx_linkscan_led_fw_version);

    /* If no feature ever init firmware, then do FW init. */
    if (cmicx_linkscan_led_fw_usage[unit][uc] == 0) {
        SHR_LOG_DEBUG("%s version %s loaded\n", cmicx_linkscan_led_fw_type, cmicx_linkscan_led_fw_version);
        SHR_IF_ERR_EXIT
            (m0ssq_uc_stop(unit, uc));
        SHR_IF_ERR_EXIT
            (m0ssq_uc_fw_load(unit, uc, 0, cmicx_linkscan_led_fw_bin, sizeof(cmicx_linkscan_led_fw_bin)));
        SHR_IF_ERR_EXIT
            (m0ssq_uc_start(unit, uc));
    }

exit:
    SHR_FUNC_EXIT();
}

static int
cmicx_m0ssq_fw_led_linkscan_cleanup(int unit, int uc)
{
    /* If no feature use firmware, then stop FW. */
    if (cmicx_linkscan_led_fw_usage[unit][uc] == 0) {
        return m0ssq_uc_stop(unit, uc);
    }

    return SYS_OK;
}

/*******************************************************************************
 * Public Functions.
 */
int
cmicx_m0ssq_fw_led_init(int unit, int uc)
{
    SHR_FUNC_ENTER(unit);

    /* Download the led & linkscan combined firmware. */
    SHR_IF_ERR_EXIT
        (cmicx_m0ssq_fw_led_linkscan_init(unit, uc));

    /* Mark the reference bitmap for LED feature.*/
    cmicx_linkscan_led_fw_usage[unit][uc] |= (FW_USED_BY_LED);

exit:
    SHR_FUNC_EXIT();
}

int
cmicx_m0ssq_fw_led_cleanup(int unit, int uc) {

    /* Cleanup the use bitmap for LED. */
    cmicx_linkscan_led_fw_usage[unit][uc] &= (~FW_USED_BY_LED);

    return cmicx_m0ssq_fw_led_linkscan_cleanup(unit, uc);
}

int
cmicx_m0ssq_fw_linkscan_init(int unit, int uc)
{
    SHR_FUNC_ENTER(unit);

    /* Download the led & linkscan combined firmware. */
    SHR_IF_ERR_EXIT
        (cmicx_m0ssq_fw_led_linkscan_init(unit, uc));

    /* Mark the use bitmap for LED. */
    cmicx_linkscan_led_fw_usage[unit][uc] |= (FW_USED_BY_LED);

exit:
    SHR_FUNC_EXIT();
}

int
cmicx_m0ssq_fw_linkscan_cleanup(int unit, int uc)
{
    /* Cleanup the use bitmap for FW linkscan. */
    cmicx_linkscan_led_fw_usage[unit][uc] &= (~FW_USED_BY_LINKSCAN);

    return cmicx_m0ssq_fw_led_linkscan_cleanup(unit, uc);
}
