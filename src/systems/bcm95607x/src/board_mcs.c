/*! \file board_mcs.c
 *
 *  APIs for MCS (Multiple Controller System).
 *
 *      MCS contains four ARM Cortex-R5 and a shared SRAM
 *      User can use APIs to
 *          - Download CR5 firmware.
 *          - Start/reset CR5 uc.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include <system.h>

#include <mcs.h>
#include <boardapi/mcs.h>

int
board_mcs_uc_fw_load(uint32 uc, uint32 offset, const uint8 *buf, uint32 len)
{
    return mcs_uc_fw_load(0, uc, offset, buf, len);
}

int
board_mcs_uc_num_get(void)
{
    return mcs_uc_num_get(0);
}

int
board_mcs_uc_start(uint32 uc)
{
    return mcs_uc_start(0, uc);
}

int
board_mcs_uc_reset(uint32 uc)
{
    return mcs_uc_reset(0, uc);
}

int
board_mcs_uc_start_get(uint32 uc, bool *start)
{
    return mcs_uc_start_get(0, uc, start);
}
