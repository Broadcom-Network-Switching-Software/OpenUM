/*! \file flpcm.c
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#define __int8_t_defined
#define __uint32_t_defined
#include <system.h>
#undef _SOC_PHYCTRL_H_
#include <soc/phyctrl.h>
#undef SOC_IF_ERROR_RETURN
#include <soc/error.h>
#include <pcm/pcm_int.h>
#include <soc/bcm5607x.h>

/* from sdk/include/sal/compiler.h */
#define COMPILER_REFERENCE(_a)    ((void)(_a))
#define COMPILER_ATTRIBUTE(_a)    __attribute__ (_a)
#ifndef FUNCTION_NAME
#define FUNCTION_NAME() (__FUNCTION__)
#endif

#include <shared/bsl.h>
#include <shared/bslenum.h>
#include <shared/bsltypes.h>
#include <soc/phy/phyctrl.h>
#include <soc/phy/drv.h>
#include <soc/property.h>
#include <utils/system.h>

extern int
(*_phy_tscf_firmware_set_helper[1])(int, int, uint8 *, int);
extern int
(*_phy_tsce_firmware_set_helper[1])(int, int, uint8 *, int);

extern int
soc_firelight_info_config(int unit, soc_control_t *soc);

static sys_error_t
bcm5607x_pcm_port_software_init(int unit)
{
    soc_phy_firmware_load_f fw_load;

    soc_firelight_info_config(unit, &SOC_CONTROL(unit));
    soc_phyctrl_software_init(unit);

    fw_load = SOC_CONTROL(unit).soc_functions->soc_phy_firmware_load;
    _phy_tscf_firmware_set_helper[unit] = fw_load;
    _phy_tsce_firmware_set_helper[unit] = fw_load;

    return SYS_OK;
}

static sys_error_t
bcm5607x_pcm_phy_reg_sbus_get(int unit, uint32 phy_addr,
                              uint32 phy_reg, uint32 *phy_data)
{
    int rv;
    soc_sbus_mdio_read_f sbus_read;

    if (!phy_data) {
        return SYS_ERR_PARAMETER;
    }

    sbus_read = SOC_CONTROL(unit).soc_functions->soc_sbus_mdio_read;
    if (!sbus_read) {
        return SYS_ERR_UNAVAIL;
    }

    rv = sbus_read(unit, phy_addr, phy_reg, phy_data);
    if (rv != SOC_E_NONE) {
        return SYS_ERR;
    }

    return SYS_OK;
}

static sys_error_t
bcm5607x_pcm_phy_reg_sbus_set(int unit, uint32 phy_addr,
                              uint32 phy_reg, uint32 phy_data)
{
    int rv;
    soc_sbus_mdio_write_f sbus_write;

    sbus_write = SOC_CONTROL(unit).soc_functions->soc_sbus_mdio_write;
    if (!sbus_write) {
        return SYS_ERR_UNAVAIL;
    }

    rv = sbus_write(unit, phy_addr, phy_reg, phy_data);
    if (rv != SOC_E_NONE) {
        return SYS_ERR;
    }

    return SYS_OK;
}

static sys_error_t
bcm5607x_pcm_port_probe_init(int unit, pbmp_t all_lpbmp, pbmp_t *okay_lpbmp)
{
    uint8 lport;
    uint8 mac[8];

    /* Probe ports */
    SOC_IF_ERROR_RETURN
        (pcm_phyctrl_port_probe_init(unit, all_lpbmp, okay_lpbmp));

    /* Initialize port configurations */
    get_system_mac(mac);
    PBMP_ITER(*okay_lpbmp, lport) {
        SOC_IF_ERROR_RETURN
            (pcm_phyctrl_port_pause_addr_set(unit,lport, mac));
        SOC_IF_ERROR_RETURN
            (pcm_port_update(unit, lport, 0));
        fl_sw_info.link[lport] = PORT_LINK_DOWN;
#if defined(CFG_SWITCH_EEE_INCLUDED)
        fl_sw_info.need_process_for_eee_1s[lport] = FALSE;
#endif
        SOC_IF_ERROR_RETURN
            (pcm_port_enable_set(unit, lport, 1));
    }

    return SYS_OK;
}

static sys_error_t
bcm5607x_pcm_phy_synce_clock_get(int unit, int lport, uint32 *mode0,
                                 uint32 *mode1, uint32 *sdm_value)
{
    int rv;

    if (!mode0 || !mode1 || !sdm_value) {
        return SYS_ERR_PARAMETER;
    }

    rv = soc_phyctrl_synce_clock_get(unit, lport, mode0, mode1, sdm_value);

    return (rv == SOC_E_NONE) ? SYS_OK : SYS_ERR;
}

static sys_error_t
bcm5607x_pcm_phy_synce_clock_set(int unit, int lport, uint32 mode0,
                                 uint32 mode1, uint32 sdm_value)
{
    int rv;

    rv = soc_phyctrl_synce_clock_set(unit, lport, mode0, mode1, sdm_value);

    return (rv == SOC_E_NONE) ? SYS_OK : SYS_ERR;
}

static sys_error_t
bcm5607x_pcm_port_timesync_tx_info_get(int unit, int lport,
                                       pcm_port_timesync_tx_info_t *info)
{
    int rv, mac_ts_disable;
    phy_ctrl_t *int_pc;
    phy_ctrl_t *ext_pc;
    phy_driver_t *pd = NULL;
    soc_port_timesync_tx_info_t tx_info;
    CLPORT_MAC_CONTROLr_t cl_mac_ctrl;

    if (!info) {
        return SYS_ERR_PARAMETER;
    }

    sal_memset(&tx_info, 0, sizeof(tx_info));

    mac_ts_disable = 0;
    if (IS_CL_PORT(lport)) {
        SOC_IF_ERROR_RETURN
            (READ_CLPORT_MAC_CONTROLr(unit, lport, cl_mac_ctrl));
        mac_ts_disable = CLPORT_MAC_CONTROLr_CLMAC_TS_DISABLEf_GET(cl_mac_ctrl);
    }

    if (mac_ts_disable) {
        int_pc = INT_PHY_SW_STATE(unit, lport);
        ext_pc = EXT_PHY_SW_STATE(unit, lport);
        if (!ext_pc && !int_pc) {
            return SYS_ERR;
        }

        if (ext_pc) {
            pd = ext_pc->pd;
        } else {
            pd = int_pc->pd;
        }

        rv = PHY_TIMESYNC_TX_INFO__GET(pd, unit, lport, &tx_info);
    } else {
        rv = MAC_TIMESYNC_TX_INFO_GET(PORT(unit, lport).p_mac, unit, lport,
                                      &tx_info);
    }

    if (rv != SOC_E_NONE) {
        return SYS_ERR;
    }

    info->timestamps_in_fifo = tx_info.timestamps_in_fifo;
    info->timestamps_in_fifo_hi = tx_info.timestamps_in_fifo_hi;
    info->sequence_id = tx_info.sequence_id;
    info->sub_ns = tx_info.sub_ns;

    return SYS_OK;
}

static sys_error_t
bcm5607x_pcm_port_fault_status_get(int unit, int lport,
                                   board_port_fault_st_t *st)
{
    int rv;
    CLMAC_CLEAR_RX_LSS_STATUSr_t clmac_clr_lss;
    XLMAC_CLEAR_RX_LSS_STATUSr_t xlmac_clr_lss;

    /* Get fault state. */
    rv = pcm_phyctrl_port_fault_status_get(unit, lport, st);
    if (rv != SYS_OK) {
        return rv;
    }

    /* Clear fault state after read. */
    if (IS_XL_PORT(lport)) {
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_CLEAR_RX_LSS_STATUSr(unit, lport, xlmac_clr_lss));
        XLMAC_CLEAR_RX_LSS_STATUSr_CLEAR_LOCAL_FAULT_STATUSf_SET(xlmac_clr_lss, 0);
        XLMAC_CLEAR_RX_LSS_STATUSr_CLEAR_REMOTE_FAULT_STATUSf_SET(xlmac_clr_lss, 0);
        SOC_IF_ERROR_RETURN
            (WRITE_XLMAC_CLEAR_RX_LSS_STATUSr(unit, lport, xlmac_clr_lss));
        XLMAC_CLEAR_RX_LSS_STATUSr_CLEAR_LOCAL_FAULT_STATUSf_SET(xlmac_clr_lss, 1);
        XLMAC_CLEAR_RX_LSS_STATUSr_CLEAR_REMOTE_FAULT_STATUSf_SET(xlmac_clr_lss, 1);
        SOC_IF_ERROR_RETURN
            (WRITE_XLMAC_CLEAR_RX_LSS_STATUSr(unit, lport, xlmac_clr_lss));
    } else if (IS_CL_PORT(lport)) {
        SOC_IF_ERROR_RETURN
            (READ_CLMAC_CLEAR_RX_LSS_STATUSr(unit, lport, clmac_clr_lss));
        CLMAC_CLEAR_RX_LSS_STATUSr_CLEAR_LOCAL_FAULT_STATUSf_SET(clmac_clr_lss, 0);
        CLMAC_CLEAR_RX_LSS_STATUSr_CLEAR_REMOTE_FAULT_STATUSf_SET(clmac_clr_lss, 0);
        SOC_IF_ERROR_RETURN
            (WRITE_CLMAC_CLEAR_RX_LSS_STATUSr(unit, lport, clmac_clr_lss));
        CLMAC_CLEAR_RX_LSS_STATUSr_CLEAR_LOCAL_FAULT_STATUSf_SET(clmac_clr_lss, 1);
        CLMAC_CLEAR_RX_LSS_STATUSr_CLEAR_REMOTE_FAULT_STATUSf_SET(clmac_clr_lss, 1);
        SOC_IF_ERROR_RETURN
            (WRITE_CLMAC_CLEAR_RX_LSS_STATUSr(unit, lport, clmac_clr_lss));
    }

    return SYS_OK;
}

static sys_error_t
bcm5607x_pcm_port_fault_ctrl_get(int unit, int lport,
                                 board_port_fault_ctrl_t *ctrl)
{
    int rv, local_fault_en = 0, remote_fault_en = 0, forced_remote_fault_en = 0;

    if (!ctrl) {
        return SYS_ERR_PARAMETER;
    }

    rv = MAC_CONTROL_GET(PORT(unit, lport).p_mac, unit, lport,
                         SOC_MAC_CONTROL_FAULT_LOCAL_ENABLE, &local_fault_en);
    if (rv != SYS_OK && rv != SYS_ERR_UNAVAIL) {
        return rv;
    }

    rv = MAC_CONTROL_GET(PORT(unit, lport).p_mac, unit, lport,
                         SOC_MAC_CONTROL_FAULT_REMOTE_ENABLE, &remote_fault_en);
    if (rv != SYS_OK && rv != SYS_ERR_UNAVAIL) {
        return rv;
    }

    rv = MAC_CONTROL_GET(PORT(unit, lport).p_mac, unit, lport,
                         SOC_MAC_CONTROL_FAULT_REMOTE_TX_FORCE_ENABLE,
                         &forced_remote_fault_en);
    if (rv != SYS_OK && rv != SYS_ERR_UNAVAIL) {
        return rv;
    }

    ctrl->local_fault = local_fault_en;
    ctrl->remote_fault = remote_fault_en;
    ctrl->forced_remote_fault = forced_remote_fault_en;

    return SYS_OK;
}

static sys_error_t
bcm5607x_pcm_port_fault_ctrl_set(int unit, int lport,
                                 board_port_fault_ctrl_t *ctrl)
{
    int rv;

    if (!ctrl) {
        return SYS_ERR_PARAMETER;
    }

    rv = MAC_CONTROL_SET(PORT(unit, lport).p_mac, unit, lport,
                         SOC_MAC_CONTROL_FAULT_LOCAL_ENABLE, ctrl->local_fault);
    if (rv == SYS_ERR_UNAVAIL) {
        if (ctrl->local_fault == 1) {
            return SYS_ERR_PARAMETER;
        }
    } else if (rv != SYS_OK) {
        return rv;
    }

    rv = MAC_CONTROL_SET(PORT(unit, lport).p_mac, unit, lport,
                         SOC_MAC_CONTROL_FAULT_REMOTE_ENABLE,
                         ctrl->remote_fault);
    if (rv == SYS_ERR_UNAVAIL) {
        if (ctrl->remote_fault == 1) {
            return SYS_ERR_PARAMETER;
        }
    } else if (rv != SYS_OK) {
        return rv;
    }

    rv = MAC_CONTROL_SET(PORT(unit, lport).p_mac, unit, lport,
                         SOC_MAC_CONTROL_FAULT_REMOTE_TX_FORCE_ENABLE,
                         ctrl->forced_remote_fault);
    if (rv == SYS_ERR_UNAVAIL) {
        if (ctrl->forced_remote_fault == 1) {
            return SYS_ERR_PARAMETER;
        }
    } else if (rv != SYS_OK) {
        return rv;
    }

    return SYS_OK;
}

static sys_error_t
bcm5607x_pcm_phy_fec_mode_set(int unit, int lport, board_phy_fec_mode_t mode)
{
    int rv;
    soc_phy_control_t type;

    switch (mode) {
    case BOARD_PHY_FEC_CL74:
        type = SOC_PHY_CONTROL_FORWARD_ERROR_CORRECTION;
        break;
    case BOARD_PHY_FEC_CL91:
        type = SOC_PHY_CONTROL_FORWARD_ERROR_CORRECTION_CL91;
        break;
    case BOARD_PHY_FEC_CL108:
        type = SOC_PHY_CONTROL_FORWARD_ERROR_CORRECTION_CL108;
        break;
    case BOARD_PHY_FEC_NONE:
        break;
    default:
        return SYS_ERR_PARAMETER;
    }

    /* Disable all kinds of FEC modes first */
    soc_phyctrl_control_set(unit, lport,
                            SOC_PHY_CONTROL_FORWARD_ERROR_CORRECTION, 0);
    soc_phyctrl_control_set(unit, lport,
                            SOC_PHY_CONTROL_FORWARD_ERROR_CORRECTION_CL91, 0);
    soc_phyctrl_control_set(unit, lport,
                            SOC_PHY_CONTROL_FORWARD_ERROR_CORRECTION_CL108, 0);

    if (mode == BOARD_PHY_FEC_NONE) {
        return SYS_OK;
    }

    /* Enable the specified FEC mode */
    rv = soc_phyctrl_control_set(unit, lport, type, 1);
    if (rv == SOC_E_UNAVAIL) {
        return SYS_ERR_UNAVAIL;
    } else if (rv != SOC_E_NONE) {
        return SYS_ERR;
    }

    return SYS_OK;
}

static sys_error_t
bcm5607x_pcm_phy_fec_mode_get(int unit, int lport, board_phy_fec_mode_t *mode)
{
    int rv;
    uint32 en;

    rv = soc_phyctrl_control_get(unit, lport,
                                 SOC_PHY_CONTROL_FORWARD_ERROR_CORRECTION, &en);
    if (rv == SOC_E_NONE && en == 1) {
        *mode = BOARD_PHY_FEC_CL74;
        return SYS_OK;
    }

    rv = soc_phyctrl_control_get(unit, lport,
                                 SOC_PHY_CONTROL_FORWARD_ERROR_CORRECTION_CL91,
                                 &en);
    if (rv == SOC_E_NONE && en == 1) {
        *mode = BOARD_PHY_FEC_CL91;
        return SYS_OK;
    }

    rv = soc_phyctrl_control_get(unit, lport,
                                 SOC_PHY_CONTROL_FORWARD_ERROR_CORRECTION_CL108,
                                 &en);
    if (rv == SOC_E_NONE && en == 1) {
        *mode = BOARD_PHY_FEC_CL108;
        return SYS_OK;
    }

    *mode = BOARD_PHY_FEC_NONE;

    return SYS_OK;
}

static sys_error_t
bcm5607x_pcm_phy_stats_get(int unit, int lport, board_phy_stats_t *stats)
{
    int rv;
    phy_ctrl_t *int_pc;
    phy_ctrl_t *ext_pc;
    phy_driver_t *pd;
    soc_phy_diag_stats_t phy_stats;

    int_pc = INT_PHY_SW_STATE(unit, lport);
    ext_pc = EXT_PHY_SW_STATE(unit, lport);
    if (ext_pc) {
        pd = ext_pc->pd;
    } else if (int_pc) {
        pd = int_pc->pd;
    } else {
        return SYS_ERR;
    }

    if (!stats) {
        return SYS_ERR_PARAMETER;
    }
    sal_memset(stats, 0, sizeof(*stats));

    rv = PHY_DIAG_STATS_GET(pd, unit, lport, &phy_stats);
    if (rv != SOC_E_NONE) {
        return SYS_ERR;
    }

    sal_memcpy(stats->bip_count, phy_stats.pcs_bip_err_count,
               sizeof(stats->bip_count));
    sal_memcpy(stats->ber_count, phy_stats.pcs_ber_count,
               sizeof(stats->ber_count));

    return SYS_OK;
}

const pcm_port_funtion_t pcm_phyctrl = {
    .phy_reg_get = pcm_phyctrl_phy_reg_get,
    .phy_reg_set = pcm_phyctrl_phy_reg_set,
    .phy_driver_name_get = pcm_phyctrl_phy_get_driver_name,
    .phy_addr_get = pcm_phyctrl_phy_addr_get,
    .phy_cable_diag = pcm_phyctrl_phy_cable_diag,
    .phy_cable_diag_support = pcm_phyctrl_phy_cable_diag_support,
    .port_eee_enable_set = pcm_phyctrl_port_eee_enable_set,
    .port_control_get = bcm5607x_port_control_get,
    .port_control_set = bcm5607x_port_control_set,
    .port_interface_set = pcm_phyctrl_port_interface_set,
    .port_interface_get = pcm_phyctrl_port_interface_get,
    .port_speed_set = pcm_phyctrl_port_speed_set,
    .port_speed_get = pcm_phyctrl_port_speed_get,
    .port_link_status_get = pcm_phyctrl_port_link_status_get,
    .port_ability_advert_set = pcm_phyctrl_port_ability_advert_set,
    .port_ability_local_get = pcm_phyctrl_port_ability_local_get,
    .port_ability_remote_get = pcm_phyctrl_port_ability_remote_get,
    .port_autoneg_set = pcm_phyctrl_port_autoneg_set,
    .port_autoneg_get = pcm_phyctrl_port_autoneg_get,
    .port_pause_set = pcm_portctrl_port_pause_set,
    .port_pause_get = pcm_portctrl_port_pause_get,
    .port_duplex_set = pcm_phyctrl_port_duplex_set,
    .port_duplex_get = pcm_phyctrl_port_duplex_get,
    .port_enable_set = pcm_phyctrl_port_enable_set,
    .port_enable_get = pcm_phyctrl_port_enable_get,
    .port_loopback_set = pcm_phyctrl_port_loopback_set,
    .port_loopback_get = pcm_phyctrl_port_loopback_get,
    .port_pause_addr_get = pcm_phyctrl_port_pause_addr_get,
    .port_pause_addr_set = pcm_phyctrl_port_pause_addr_set,
    .port_frame_max_get = bcm5607x_port_frame_max_get,
    .port_frame_max_set = bcm5607x_port_frame_max_set,
    .port_ifg_get = pcm_portctrl_port_ifg_get,
    .port_ifg_set = pcm_portctrl_port_ifg_set,
    .port_class_get = bcm5607x_port_class_get,
    .port_class_set = bcm5607x_port_class_set,
    .port_update = pcm_phyctrl_port_update,
    .port_probe_init = bcm5607x_pcm_port_probe_init,
    .port_reinit = pcm_phyctrl_port_reinit,
    .port_notify = pcm_phyctrl_phy_notify,
    .software_init = bcm5607x_pcm_port_software_init,
    .phy_reg_sbus_get = bcm5607x_pcm_phy_reg_sbus_get,
    .phy_reg_sbus_set = bcm5607x_pcm_phy_reg_sbus_set,
    .phy_synce_clock_get = bcm5607x_pcm_phy_synce_clock_get,
    .phy_synce_clock_set = bcm5607x_pcm_phy_synce_clock_set,
    .phy_timesync_enable_get = pcm_phyctrl_phy_timesync_enable_get,
    .phy_timesync_enable_set = pcm_phyctrl_phy_timesync_enable_set,
    .phy_timesync_ctrl_get = pcm_phyctrl_phy_timesync_ctrl_get,
    .phy_timesync_ctrl_set = pcm_phyctrl_phy_timesync_ctrl_set,
    .port_timesync_tx_info_get = bcm5607x_pcm_port_timesync_tx_info_get,
    .port_fault_status_get = bcm5607x_pcm_port_fault_status_get,
    .port_fault_ctrl_get = bcm5607x_pcm_port_fault_ctrl_get,
    .port_fault_ctrl_set = bcm5607x_pcm_port_fault_ctrl_set,
    .phy_fec_mode_get = bcm5607x_pcm_phy_fec_mode_get,
    .phy_fec_mode_set = bcm5607x_pcm_phy_fec_mode_set,
    .phy_fec_status_get = pcm_phyctrl_phy_fec_status_get,
    .phy_power_get = pcm_phyctrl_phy_power_get,
    .phy_power_set = pcm_phyctrl_phy_power_set,
    .phy_tx_ctrl_get = pcm_phyctrl_phy_tx_ctrl_get,
    .phy_tx_ctrl_set = pcm_phyctrl_phy_tx_ctrl_set,
    .phy_rx_status_get = pcm_phyctrl_phy_rx_status_get,
    .phy_prbs_ctrl_set = pcm_phyctrl_phy_prbs_ctrl_set,
    .phy_prbs_ctrl_get = pcm_phyctrl_phy_prbs_ctrl_get,
    .phy_prbs_enable_set = pcm_phyctrl_phy_prbs_enable_set,
    .phy_prbs_enable_get = pcm_phyctrl_phy_prbs_enable_get,
    .phy_prbs_status_get = pcm_phyctrl_phy_prbs_status_get,
    .phy_stats_get = bcm5607x_pcm_phy_stats_get,
    .port_link_scan = bcm5607x_linkscan_task,
};

int
bcm5607x_pcm_software_init(int unit)
{
    uint8 lport;

    SOC_LPORT_ITER(lport) {
        pcm_port_ctrl[lport].f = &pcm_phyctrl;
    }

    return pcm_phyctrl.software_init(unit);
}
