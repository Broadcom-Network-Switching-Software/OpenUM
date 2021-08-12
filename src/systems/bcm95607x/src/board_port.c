/*! \file board_port.c
 *
 * BCM56070 board APIs for port module.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include <system.h>
#include <boardapi/port.h>
#include "pcm.h"

sys_error_t
board_port_fault_status_get(uint16 uport, board_port_fault_st_t *fault_st)
{
    sys_error_t rv;
    uint8 unit, lport;

    rv = board_uport_to_lport(uport, &unit, &lport);
    if (rv != SYS_OK) {
        return rv;
    }

    return pcm_port_fault_status_get(unit, lport, fault_st);
}

sys_error_t
board_port_fault_ctrl_set(uint16 uport, board_port_fault_ctrl_t *fault_ctrl)
{
    sys_error_t rv;
    uint8 unit, lport;

    rv = board_uport_to_lport(uport, &unit, &lport);
    if (rv != SYS_OK) {
        return rv;
    }

    return pcm_port_fault_ctrl_set(unit, lport, fault_ctrl);
}

sys_error_t
board_port_fault_ctrl_get(uint16 uport, board_port_fault_ctrl_t *fault_ctrl)
{
    sys_error_t rv;
    uint8 unit, lport;

    rv = board_uport_to_lport(uport, &unit, &lport);
    if (rv != SYS_OK) {
        return rv;
    }

    return pcm_port_fault_ctrl_get(unit, lport, fault_ctrl);
}

sys_error_t
board_port_phy_power_set(uint16 uport, int power)
{
    sys_error_t rv;
    uint8 unit, lport;

    rv = board_uport_to_lport(uport, &unit, &lport);
    if (rv != SYS_OK) {
        return rv;
    }

    return pcm_phy_power_set(unit, lport, power);
}

sys_error_t
board_port_phy_power_get(uint16 uport, int *power)
{
    sys_error_t rv;
    uint8 unit, lport;

    rv = board_uport_to_lport(uport, &unit, &lport);
    if (rv != SYS_OK) {
        return rv;
    }

    return pcm_phy_power_get(unit, lport, power);
}

sys_error_t
board_port_phy_fec_mode_set(uint16 uport, board_phy_fec_mode_t fec_mode)
{
    sys_error_t rv;
    uint8 unit, lport;

    rv = board_uport_to_lport(uport, &unit, &lport);
    if (rv != SYS_OK) {
        return rv;
    }

    return pcm_phy_fec_mode_set(unit, lport, fec_mode);
}

sys_error_t
board_port_phy_fec_mode_get(uint16 uport, board_phy_fec_mode_t *fec_mode)
{
    sys_error_t rv;
    uint8 unit, lport;

    rv = board_uport_to_lport(uport, &unit, &lport);
    if (rv != SYS_OK) {
        return rv;
    }

    return pcm_phy_fec_mode_get(unit, lport, fec_mode);
}

sys_error_t
board_port_phy_fec_status_get(uint16 uport, board_phy_fec_st_t *fec_st)
{
    sys_error_t rv;
    uint8 unit, lport;

    rv = board_uport_to_lport(uport, &unit, &lport);
    if (rv != SYS_OK) {
        return rv;
    }

    return pcm_phy_fec_status_get(unit, lport, fec_st);
}

sys_error_t
board_port_phy_tx_ctrl_set(uint16 uport, board_phy_tx_ctrl_t *tx_ctrl)
{
    sys_error_t rv;
    uint8 unit, lport;

    rv = board_uport_to_lport(uport, &unit, &lport);
    if (rv != SYS_OK) {
        return rv;
    }

    return pcm_phy_tx_ctrl_set(unit, lport, tx_ctrl);
}

sys_error_t
board_port_phy_tx_ctrl_get(uint16 uport, board_phy_tx_ctrl_t *tx_ctrl)
{
    sys_error_t rv;
    uint8 unit, lport;

    rv = board_uport_to_lport(uport, &unit, &lport);
    if (rv != SYS_OK) {
        return rv;
    }

    return pcm_phy_tx_ctrl_get(unit, lport, tx_ctrl);
}

sys_error_t
board_port_phy_rx_status_get(uint16 uport, board_phy_rx_st_t *rx_st)
{
    sys_error_t rv;
    uint8 unit, lport;

    rv = board_uport_to_lport(uport, &unit, &lport);
    if (rv != SYS_OK) {
        return rv;
    }

    return pcm_phy_rx_status_get(unit, lport, rx_st);
}

sys_error_t
board_port_phy_prbs_ctrl_set(uint16 uport, int flags,
                             board_phy_prbs_ctrl_t *prbs)
{
    sys_error_t rv;
    uint8 unit, lport;

    rv = board_uport_to_lport(uport, &unit, &lport);
    if (rv != SYS_OK) {
        return rv;
    }

    return pcm_phy_prbs_ctrl_set(unit, lport, flags, prbs);
}

sys_error_t
board_port_phy_prbs_ctrl_get(uint16 uport, int flags,
                             board_phy_prbs_ctrl_t *prbs)
{
    sys_error_t rv;
    uint8 unit, lport;

    rv = board_uport_to_lport(uport, &unit, &lport);
    if (rv != SYS_OK) {
        return rv;
    }

    return pcm_phy_prbs_ctrl_get(unit, lport, flags, prbs);
}

sys_error_t
board_port_phy_prbs_enable_set(uint16 uport, int flags, int en)
{
    sys_error_t rv;
    uint8 unit, lport;

    rv = board_uport_to_lport(uport, &unit, &lport);
    if (rv != SYS_OK) {
        return rv;
    }

    return pcm_phy_prbs_enable_set(unit, lport, flags, en);
}

sys_error_t
board_port_phy_prbs_enable_get(uint16 uport, int flags, int *en)
{
    sys_error_t rv;
    uint8 unit, lport;

    rv = board_uport_to_lport(uport, &unit, &lport);
    if (rv != SYS_OK) {
        return rv;
    }

    return pcm_phy_prbs_enable_get(unit, lport, flags, en);
}

sys_error_t
board_port_phy_prbs_status_get(uint16 uport, board_phy_prbs_st_t *st)
{
    sys_error_t rv;
    uint8 unit, lport;

    rv = board_uport_to_lport(uport, &unit, &lport);
    if (rv != SYS_OK) {
        return rv;
    }

    return pcm_phy_prbs_status_get(unit, lport, st);
}

sys_error_t
board_port_phy_stats_get(uint16 uport, board_phy_stats_t *stats)
{
    sys_error_t rv;
    uint8 unit, lport;

    rv = board_uport_to_lport(uport, &unit, &lport);
    if (rv != SYS_OK) {
        return rv;
    }

    return pcm_phy_stats_get(unit, lport, stats);
}
