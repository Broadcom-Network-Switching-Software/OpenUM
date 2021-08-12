/*! \file fltimesync.c
 *
 * bcm5607x device-specific timesync driver.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#include "system.h"

#ifdef CFG_SWITCH_TIMESYNC_INCLUDED

/* Timesync Profile index defines */
#define TS_PORT_INDEX               0
#define TIMESYNC_MAX_PROFILE        1

#define BYTES2BITS(x)        ((x) * 8)

#define BCM_MAC_IS_ZERO(_mac_)  \
    (((_mac_)[0] | (_mac_)[1] | (_mac_)[2] | \
      (_mac_)[3] | (_mac_)[4] | (_mac_)[5]) == 0)

#define COMPILER_64_SET(dst, src_hi, src_lo)                \
    ((dst) = (((uint64) ((uint32)(src_hi))) << 32) | ((uint64) ((uint32)(src_lo))))

/*!
 * \brief Internal function adds timesync control profiles.
 *
 * \param [in] unit The chip unit number
 * \param [in] ts_config The Timesync config
 * \param [in] index The profile index
 *
 * \retval SYS_OK No errors.
 */
static sys_error_t
_bcm_port_timesync_control_profile_entry_add(uint8 unit,
                                bcm_port_timesync_config_t *ts_config, uint32 index)
{
    int cnt;
    ING_1588_INGRESS_CTRLm_t ing_ctrl;

    ING_1588_INGRESS_CTRLm_CLR(ing_ctrl);
    /* Set timesync message control tocpu/drop fields */
    for (cnt = 0; cnt < BYTES2BITS(sizeof(uint32)); cnt++ ) {
        switch (ts_config->pkt_drop & (1 << cnt)) {
            case BCM_PORT_TIMESYNC_PKT_SYNC:
                ING_1588_INGRESS_CTRLm_RX_TS_SYNC_DROPf_SET(ing_ctrl, 1);
                break;
            case BCM_PORT_TIMESYNC_PKT_DELAY_REQ:
                ING_1588_INGRESS_CTRLm_RX_TS_DELAY_REQ_DROPf_SET(ing_ctrl, 1);
                break;
            case BCM_PORT_TIMESYNC_PKT_PDELAY_REQ:
                ING_1588_INGRESS_CTRLm_RX_TS_PDELAY_REQ_DROPf_SET(ing_ctrl, 1);
                break;
            case BCM_PORT_TIMESYNC_PKT_PDELAY_RESP:
                ING_1588_INGRESS_CTRLm_RX_TS_PDELAY_RESP_DROPf_SET(ing_ctrl, 1);
                break;
            case BCM_PORT_TIMESYNC_PKT_FOLLOWUP:
                ING_1588_INGRESS_CTRLm_RX_TS_FOLLOW_UP_DROPf_SET(ing_ctrl, 1);
                break;
            case BCM_PORT_TIMESYNC_PKT_DELAY_RESP:
                ING_1588_INGRESS_CTRLm_RX_TS_DELAY_RESP_DROPf_SET(ing_ctrl, 1);
                break;
            case BCM_PORT_TIMESYNC_PKT_PDELAY_RESP_FOLLOWUP:
                ING_1588_INGRESS_CTRLm_RX_TS_PDELAY_RESP_FOLLOW_UP_DROPf_SET(ing_ctrl, 1);
                break;
            case BCM_PORT_TIMESYNC_PKT_ANNOUNCE:
                ING_1588_INGRESS_CTRLm_RX_TS_MSG_TYPE_11_DROPf_SET(ing_ctrl, 1);
                break;
            case BCM_PORT_TIMESYNC_PKT_SIGNALLING:
                ING_1588_INGRESS_CTRLm_RX_TS_MSG_TYPE_12_DROPf_SET(ing_ctrl, 1);
                break;
            case BCM_PORT_TIMESYNC_PKT_MANAGMENT:
                ING_1588_INGRESS_CTRLm_RX_TS_MSG_TYPE_13_DROPf_SET(ing_ctrl, 1);
                break;
            default:
                break;
        }

        switch (ts_config->pkt_tocpu & (1 << cnt)) {
            case BCM_PORT_TIMESYNC_PKT_SYNC:
                ING_1588_INGRESS_CTRLm_RX_TS_SYNC_TO_CPUf_SET(ing_ctrl, 1);
                break;
            case BCM_PORT_TIMESYNC_PKT_DELAY_REQ:
                ING_1588_INGRESS_CTRLm_RX_TS_DELAY_REQ_TO_CPUf_SET(ing_ctrl, 1);
                break;
            case BCM_PORT_TIMESYNC_PKT_PDELAY_REQ:
                ING_1588_INGRESS_CTRLm_RX_TS_PDELAY_REQ_TO_CPUf_SET(ing_ctrl, 1);
                break;
            case BCM_PORT_TIMESYNC_PKT_PDELAY_RESP:
                ING_1588_INGRESS_CTRLm_RX_TS_PDELAY_RESP_TO_CPUf_SET(ing_ctrl, 1);
                break;
            case BCM_PORT_TIMESYNC_PKT_FOLLOWUP:
                ING_1588_INGRESS_CTRLm_RX_TS_FOLLOW_UP_TO_CPUf_SET(ing_ctrl, 1);
                break;
            case BCM_PORT_TIMESYNC_PKT_DELAY_RESP:
                ING_1588_INGRESS_CTRLm_RX_TS_DELAY_RESP_TO_CPUf_SET(ing_ctrl, 1);
                break;
            case BCM_PORT_TIMESYNC_PKT_PDELAY_RESP_FOLLOWUP:
                ING_1588_INGRESS_CTRLm_RX_TS_PDELAY_RESP_FOLLOW_UP_TO_CPUf_SET(ing_ctrl, 1);
                break;
            case BCM_PORT_TIMESYNC_PKT_ANNOUNCE:
                ING_1588_INGRESS_CTRLm_RX_TS_MSG_TYPE_11_TO_CPUf_SET(ing_ctrl, 1);
                break;
            case BCM_PORT_TIMESYNC_PKT_SIGNALLING:
                ING_1588_INGRESS_CTRLm_RX_TS_MSG_TYPE_12_TO_CPUf_SET(ing_ctrl, 1);
                break;
            case BCM_PORT_TIMESYNC_PKT_MANAGMENT:
                ING_1588_INGRESS_CTRLm_RX_TS_MSG_TYPE_13_TO_CPUf_SET(ing_ctrl, 1);
                break;
            default:
                break;
        }
    }

    return WRITE_ING_1588_INGRESS_CTRLm(unit, index, ing_ctrl);
}

/*!
 * \brief Internal function gets timesync control profiles for the index.
 *
 * \param [in] unit The chip unit number
 * \param [in] ts_config The Timesync config
 * \param [in] index The profile index
 *
 * \retval SYS_OK No errors.
 */
static sys_error_t
_bcm_port_timesync_control_profile_entry_get(uint8 unit,
                                bcm_port_timesync_config_t *ts_config, uint32 index)
{
    int cnt;
    ING_1588_INGRESS_CTRLm_t ing_ctrl;

    SOC_IF_ERROR_RETURN(READ_ING_1588_INGRESS_CTRLm(unit, index, ing_ctrl));

    /* Set timesync message control tocpu/drop fields */
    for (cnt = 0; cnt < BYTES2BITS(sizeof(uint32)); cnt++ ) {
        switch (1 << cnt) {
            case BCM_PORT_TIMESYNC_PKT_SYNC:
                if (ING_1588_INGRESS_CTRLm_RX_TS_SYNC_DROPf_GET(ing_ctrl)) {
                    ts_config->pkt_drop |= BCM_PORT_TIMESYNC_PKT_SYNC;
                }
                break;
            case BCM_PORT_TIMESYNC_PKT_DELAY_REQ:
                if (ING_1588_INGRESS_CTRLm_RX_TS_DELAY_REQ_DROPf_GET(ing_ctrl)) {
                    ts_config->pkt_drop |= BCM_PORT_TIMESYNC_PKT_DELAY_REQ;
                }
                break;
            case BCM_PORT_TIMESYNC_PKT_PDELAY_REQ:
                if (ING_1588_INGRESS_CTRLm_RX_TS_PDELAY_REQ_DROPf_GET(ing_ctrl)) {
                    ts_config->pkt_drop |= BCM_PORT_TIMESYNC_PKT_PDELAY_REQ;
                }
                break;
            case BCM_PORT_TIMESYNC_PKT_PDELAY_RESP:
                if (ING_1588_INGRESS_CTRLm_RX_TS_PDELAY_RESP_DROPf_GET(ing_ctrl)) {
                    ts_config->pkt_drop |= BCM_PORT_TIMESYNC_PKT_PDELAY_RESP;
                }
                break;
            case BCM_PORT_TIMESYNC_PKT_FOLLOWUP:
                if (ING_1588_INGRESS_CTRLm_RX_TS_FOLLOW_UP_DROPf_GET(ing_ctrl)) {
                    ts_config->pkt_drop |= BCM_PORT_TIMESYNC_PKT_FOLLOWUP;
                }
                break;
            case BCM_PORT_TIMESYNC_PKT_DELAY_RESP:
                if (ING_1588_INGRESS_CTRLm_RX_TS_DELAY_RESP_DROPf_GET(ing_ctrl)) {
                    ts_config->pkt_drop |= BCM_PORT_TIMESYNC_PKT_DELAY_RESP;
                }
                break;
            case BCM_PORT_TIMESYNC_PKT_PDELAY_RESP_FOLLOWUP:
                if (ING_1588_INGRESS_CTRLm_RX_TS_PDELAY_RESP_FOLLOW_UP_DROPf_GET(ing_ctrl)) {
                    ts_config->pkt_drop |= BCM_PORT_TIMESYNC_PKT_PDELAY_RESP_FOLLOWUP;
                }
                break;
            case BCM_PORT_TIMESYNC_PKT_ANNOUNCE:
                if (ING_1588_INGRESS_CTRLm_RX_TS_MSG_TYPE_11_DROPf_GET(ing_ctrl)) {
                    ts_config->pkt_drop |= BCM_PORT_TIMESYNC_PKT_ANNOUNCE;
                }
                break;
            case BCM_PORT_TIMESYNC_PKT_SIGNALLING:
                if (ING_1588_INGRESS_CTRLm_RX_TS_MSG_TYPE_12_DROPf_GET(ing_ctrl)) {
                    ts_config->pkt_drop |= BCM_PORT_TIMESYNC_PKT_SIGNALLING;
                }
                break;
            case BCM_PORT_TIMESYNC_PKT_MANAGMENT:
                if (ING_1588_INGRESS_CTRLm_RX_TS_MSG_TYPE_13_DROPf_GET(ing_ctrl)) {
                    ts_config->pkt_drop |= BCM_PORT_TIMESYNC_PKT_MANAGMENT;
                }
                break;
            default:
                break;
        }

        switch (1 << cnt) {
            case BCM_PORT_TIMESYNC_PKT_SYNC:
                if (ING_1588_INGRESS_CTRLm_RX_TS_SYNC_TO_CPUf_GET(ing_ctrl)) {
                    ts_config->pkt_tocpu |= BCM_PORT_TIMESYNC_PKT_SYNC;
                }
                break;
            case BCM_PORT_TIMESYNC_PKT_DELAY_REQ:
                if (ING_1588_INGRESS_CTRLm_RX_TS_DELAY_REQ_TO_CPUf_GET(ing_ctrl)) {
                    ts_config->pkt_tocpu |= BCM_PORT_TIMESYNC_PKT_DELAY_REQ;
                }
                break;
            case BCM_PORT_TIMESYNC_PKT_PDELAY_REQ:
                if (ING_1588_INGRESS_CTRLm_RX_TS_PDELAY_REQ_TO_CPUf_GET(ing_ctrl)) {
                    ts_config->pkt_tocpu |= BCM_PORT_TIMESYNC_PKT_PDELAY_REQ;
                }
                break;
            case BCM_PORT_TIMESYNC_PKT_PDELAY_RESP:
                if (ING_1588_INGRESS_CTRLm_RX_TS_PDELAY_RESP_TO_CPUf_GET(ing_ctrl)) {
                    ts_config->pkt_tocpu |= BCM_PORT_TIMESYNC_PKT_PDELAY_RESP;
                }
                break;
            case BCM_PORT_TIMESYNC_PKT_FOLLOWUP:
                if (ING_1588_INGRESS_CTRLm_RX_TS_FOLLOW_UP_TO_CPUf_GET(ing_ctrl)) {
                    ts_config->pkt_tocpu |= BCM_PORT_TIMESYNC_PKT_FOLLOWUP;
                }
                break;
            case BCM_PORT_TIMESYNC_PKT_DELAY_RESP:
                if (ING_1588_INGRESS_CTRLm_RX_TS_DELAY_RESP_TO_CPUf_GET(ing_ctrl)) {
                    ts_config->pkt_tocpu |= BCM_PORT_TIMESYNC_PKT_DELAY_RESP;
                }
                break;
            case BCM_PORT_TIMESYNC_PKT_PDELAY_RESP_FOLLOWUP:
                if (ING_1588_INGRESS_CTRLm_RX_TS_PDELAY_RESP_FOLLOW_UP_TO_CPUf_GET(ing_ctrl)) {
                    ts_config->pkt_tocpu |= BCM_PORT_TIMESYNC_PKT_PDELAY_RESP_FOLLOWUP;
                }
                break;
            case BCM_PORT_TIMESYNC_PKT_ANNOUNCE:
                if (ING_1588_INGRESS_CTRLm_RX_TS_MSG_TYPE_11_TO_CPUf_GET(ing_ctrl)) {
                    ts_config->pkt_tocpu |= BCM_PORT_TIMESYNC_PKT_ANNOUNCE;
                }
                break;
            case BCM_PORT_TIMESYNC_PKT_SIGNALLING:
                if (ING_1588_INGRESS_CTRLm_RX_TS_MSG_TYPE_12_TO_CPUf_GET(ing_ctrl)) {
                    ts_config->pkt_tocpu |= BCM_PORT_TIMESYNC_PKT_SIGNALLING;
                }
                break;
            case BCM_PORT_TIMESYNC_PKT_MANAGMENT:
                if (ING_1588_INGRESS_CTRLm_RX_TS_MSG_TYPE_13_TO_CPUf_GET(ing_ctrl)) {
                    ts_config->pkt_tocpu |= BCM_PORT_TIMESYNC_PKT_MANAGMENT;
                }
                break;
            default:
                break;
        }
    }

    return SYS_OK;
}

/*!
 * \brief Initializes timesync port profiles.
 *
 * This should be called after all ports have been probed and reset.
 *
 * \param [in] unit The chip unit number
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
bcm5607x_timesync_init(uint8 unit)
{
    uint16 lport;
    CLPORT_MAC_CONTROLr_t clport_mac_control;
    XLPORT_MODE_REGr_t xlport_mode;
    EGR_1588_PARSING_CONTROLr_t parsing_ctrl;
#ifndef CFG_BROADSYNC_INCLUDED
    IPROC_NS_TIMESYNC_TS0_FREQ_CTRL_FRAC_UPPERr_t freq_frac_upper;
    IPROC_NS_TIMESYNC_TS0_FREQ_CTRL_FRAC_LOWERr_t freq_frac_lower;
    IPROC_NS_TIMESYNC_TS0_COUNTER_ENABLEr_t counter_enable;
    IPROC_NS_TIMESYNC_COUNTER_CONFIG_SELECTr_t config_select;

    /* Use free running counter for debug purpose without BS FW. */
    READ_IPROC_NS_TIMESYNC_TS0_FREQ_CTRL_FRAC_UPPERr(unit, freq_frac_upper);
    /* 500 Mhz NCO - 2.000 ns */
    IPROC_NS_TIMESYNC_TS0_FREQ_CTRL_FRAC_UPPERr_UINCf_SET(freq_frac_upper,
                                                        0x4000);
    WRITE_IPROC_NS_TIMESYNC_TS0_FREQ_CTRL_FRAC_UPPERr(unit, freq_frac_upper);

    READ_IPROC_NS_TIMESYNC_TS0_FREQ_CTRL_FRAC_LOWERr(unit, freq_frac_lower);
    IPROC_NS_TIMESYNC_TS0_FREQ_CTRL_FRAC_LOWERr_LINCf_SET(freq_frac_lower, 0);
    WRITE_IPROC_NS_TIMESYNC_TS0_FREQ_CTRL_FRAC_LOWERr(unit, freq_frac_lower);

    READ_IPROC_NS_TIMESYNC_TS0_COUNTER_ENABLEr(unit, counter_enable);
    IPROC_NS_TIMESYNC_TS0_COUNTER_ENABLEr_ENABLEf_SET(counter_enable, 1);
    WRITE_IPROC_NS_TIMESYNC_TS0_COUNTER_ENABLEr(unit, counter_enable);

    READ_IPROC_NS_TIMESYNC_COUNTER_CONFIG_SELECTr(unit, config_select);
    IPROC_NS_TIMESYNC_COUNTER_CONFIG_SELECTr_ENABLE_COMMON_CONTROLf_SET(config_select, 1);
    WRITE_IPROC_NS_TIMESYNC_COUNTER_CONFIG_SELECTr(unit, config_select);

    /* Enable Timestamp Generation for debug purpose. */
    READ_IPROC_NS_TIMESYNC_TS0_FREQ_CTRL_FRAC_UPPERr(unit, freq_frac_upper);
    /* 500 Mhz NCO - 2.000 ns */
    IPROC_NS_TIMESYNC_TS0_FREQ_CTRL_FRAC_UPPERr_UINCf_SET(freq_frac_upper,
                                                        0x4000);
    WRITE_IPROC_NS_TIMESYNC_TS0_FREQ_CTRL_FRAC_UPPERr(unit, freq_frac_upper);

    READ_IPROC_NS_TIMESYNC_TS0_FREQ_CTRL_FRAC_LOWERr(unit, freq_frac_lower);
    IPROC_NS_TIMESYNC_TS0_FREQ_CTRL_FRAC_LOWERr_LINCf_SET(freq_frac_lower, 0);
    WRITE_IPROC_NS_TIMESYNC_TS0_FREQ_CTRL_FRAC_LOWERr(unit, freq_frac_lower);

    READ_IPROC_NS_TIMESYNC_TS0_COUNTER_ENABLEr(unit, counter_enable);
    IPROC_NS_TIMESYNC_TS0_COUNTER_ENABLEr_ENABLEf_SET(counter_enable, 1);
    WRITE_IPROC_NS_TIMESYNC_TS0_COUNTER_ENABLEr(unit, counter_enable);
#endif /* CFG_BROADSYNC_INCLUDED */

    /*
     * Use 48-bit timesamping mode since PCS 1588 doesn't support 32-bit
     * timestamp on PM4x25 gen3 port macro.
     */
    SOC_IF_ERROR_RETURN(READ_EGR_1588_PARSING_CONTROLr(unit, parsing_ctrl));
    EGR_1588_PARSING_CONTROLr_TIMESTAMPING_MODEf_SET(parsing_ctrl, 1);
    SOC_IF_ERROR_RETURN(WRITE_EGR_1588_PARSING_CONTROLr(unit, parsing_ctrl));

    SOC_LPORT_ITER(lport) {
        if (IS_XL_PORT(lport)) {
            SOC_IF_ERROR_RETURN(READ_XLPORT_MODE_REGr(unit, lport, xlport_mode));
            XLPORT_MODE_REGr_EGR_1588_TIMESTAMPING_MODEf_SET(xlport_mode, 1);
            XLPORT_MODE_REGr_EGR_1588_TIMESTAMPING_CMIC_48_ENf_SET(xlport_mode, 1);
            SOC_IF_ERROR_RETURN(WRITE_XLPORT_MODE_REGr(unit, lport, xlport_mode));
        }
        if (IS_CL_PORT(lport)) {
            /* Disable TS in MAC for PM4x25 */
            SOC_IF_ERROR_RETURN(READ_CLPORT_MAC_CONTROLr(unit, lport, clport_mac_control));
            CLPORT_MAC_CONTROLr_CLMAC_TS_DISABLEf_SET(clport_mac_control, 1);
            SOC_IF_ERROR_RETURN(WRITE_CLPORT_MAC_CONTROLr(unit, lport, clport_mac_control));
        }
    }

    return SYS_OK;
}

/*!
 * \brief Set timesync configurations for the port.
 *
 * \param [in] unit Unit number.
 * \param [in] lport Port number.
 * \param [in] config_count Count of timesync configurations.
 * \param [in] config_array Pointer to timesync configurations.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
bcm5607x_port_timesync_config_set(uint8 unit, uint8 lport,
                            int config_count, bcm_port_timesync_config_t *config_array)
{
    int cnt;
    bcm_port_timesync_config_t *ts_config;
    EGR_1588_INGRESS_CTRLr_t ing_ctrl;
    EGR_1588_EGRESS_CTRLr_t egr_ctrl;
    EGR_1588_SAm_t sa;
    PORTm_t port_tab;

    /* Validate Parameters */
    if ((config_count > TIMESYNC_MAX_PROFILE)
        || ((config_count > 0) && (NULL == config_array))
        || ((config_count == 0) && (NULL != config_array))) {
        return SYS_ERR_PARAMETER;
    }

    /* Set timesync profiles */
    for (cnt = 0; cnt < config_count; cnt++)
    {
        ts_config = (bcm_port_timesync_config_t*) (config_array + cnt);

        if (ts_config->flags & BCM_PORT_TIMESYNC_DEFAULT) {
            /* Set one-step timestamp configurations */
            if (ts_config->flags & BCM_PORT_TIMESYNC_ONE_STEP_TIMESTAMP) {
                /* Enable Correction updates for ingress and egress */
                SOC_IF_ERROR_RETURN(READ_EGR_1588_INGRESS_CTRLr(unit, lport, ing_ctrl));
                EGR_1588_INGRESS_CTRLr_CF_UPDATE_ENABLEf_SET(ing_ctrl, 1);
                SOC_IF_ERROR_RETURN(WRITE_EGR_1588_INGRESS_CTRLr(unit, lport, ing_ctrl));

                SOC_IF_ERROR_RETURN(READ_EGR_1588_EGRESS_CTRLr(unit, lport, egr_ctrl));
                if (ts_config->flags &
                        BCM_PORT_TIMESYNC_TIMESTAMP_CFUPDATE_ALL) {
                    EGR_1588_EGRESS_CTRLr_CF_UPDATE_MODEf_SET(egr_ctrl, 1);
                } else {
                    /* TR3: The ingress port is default enabled for correction
                     * updates for one-step timestamping and hence the
                     * EGR update mode is set to ingress based correction
                     * updates.
                     */
                    EGR_1588_EGRESS_CTRLr_CF_UPDATE_MODEf_SET(egr_ctrl, 2);
                }
               SOC_IF_ERROR_RETURN(WRITE_EGR_1588_EGRESS_CTRLr(unit, lport, egr_ctrl));

                /* Set sign extension from timestamp field, for ingress
                 * Front Panel port or HG Proxy port
                 */
                SOC_IF_ERROR_RETURN(READ_EGR_1588_INGRESS_CTRLr(unit, lport, ing_ctrl));
                EGR_1588_INGRESS_CTRLr_FORCE_ITS_SIGN_FROM_TSf_SET(ing_ctrl, 1);
                SOC_IF_ERROR_RETURN(WRITE_EGR_1588_INGRESS_CTRLr(unit, lport, ing_ctrl));

                /* Enable Source Address update for corrections */
                if (!BCM_MAC_IS_ZERO(ts_config->src_mac_addr)) {
                    SOC_IF_ERROR_RETURN(READ_EGR_1588_EGRESS_CTRLr(unit, lport, egr_ctrl));
                    EGR_1588_EGRESS_CTRLr_SA_UPDATE_ENABLEf_SET(egr_ctrl, 1);
                    SOC_IF_ERROR_RETURN(WRITE_EGR_1588_EGRESS_CTRLr(unit, lport, egr_ctrl));

                    EGR_1588_SAm_CLR(sa);
                    EGR_1588_SAm_SAf_SET(sa, (uint32 *)ts_config->src_mac_addr);
                    SOC_IF_ERROR_RETURN(WRITE_EGR_1588_SAm(unit, lport, sa));
                }
            }

            if (ts_config->flags & BCM_PORT_TIMESYNC_TWO_STEP_TIMESTAMP) {
                /* Set two-step timestamping in mac for all event pkts */
                SOC_IF_ERROR_RETURN(READ_EGR_1588_EGRESS_CTRLr(unit, lport, egr_ctrl));
                EGR_1588_EGRESS_CTRLr_TX_TS_SYNCf_SET(egr_ctrl, 1);
                EGR_1588_EGRESS_CTRLr_TX_TS_DELAY_REQf_SET(egr_ctrl, 1);
                EGR_1588_EGRESS_CTRLr_TX_TS_PDELAY_REQf_SET(egr_ctrl, 1);
                EGR_1588_EGRESS_CTRLr_TX_TS_PDELAY_RESPf_SET(egr_ctrl, 1);
                SOC_IF_ERROR_RETURN(WRITE_EGR_1588_EGRESS_CTRLr(unit, lport, egr_ctrl));
            }

            if (ts_config->pkt_drop & BCM_PORT_TIMESYNC_PKT_INVALID) {
                SOC_IF_ERROR_RETURN(READ_EGR_1588_EGRESS_CTRLr(unit, lport, egr_ctrl));
                EGR_1588_EGRESS_CTRLr_DROP_INVALID_1588_PKTf_SET(egr_ctrl, 1);
                SOC_IF_ERROR_RETURN(WRITE_EGR_1588_EGRESS_CTRLr(unit, lport, egr_ctrl));
            }

            /* Use lport number as timesync control profile index. */
            SOC_IF_ERROR_RETURN(
                _bcm_port_timesync_control_profile_entry_add(unit, ts_config, lport));

            SOC_IF_ERROR_RETURN(READ_PORTm(unit, lport, port_tab));
            PORTm_CTRL_PROFILE_INDEX_1588f_SET(port_tab, lport);
            /* Enable Timesync for the port */
            PORTm_IEEE_802_1AS_ENABLEf_SET(port_tab, 1);
            SOC_IF_ERROR_RETURN(WRITE_PORTm(unit, lport, port_tab));
        }
    }

    /* Disable timesync for NULL timesync configuration */
    if (config_count == 0) {
        /* Disable one step and two step timesync */
        EGR_1588_EGRESS_CTRLr_CLR(egr_ctrl);
        SOC_IF_ERROR_RETURN(WRITE_EGR_1588_EGRESS_CTRLr(unit, lport, egr_ctrl));

        EGR_1588_INGRESS_CTRLr_CLR(ing_ctrl);
        SOC_IF_ERROR_RETURN(WRITE_EGR_1588_INGRESS_CTRLr(unit, lport, ing_ctrl));

        /* Disable Timesync for the port */
        SOC_IF_ERROR_RETURN(READ_PORTm(unit, lport, port_tab));
        PORTm_IEEE_802_1AS_ENABLEf_SET(port_tab, 0);
        SOC_IF_ERROR_RETURN(WRITE_PORTm(unit, lport, port_tab));
    }

    return SYS_OK;
}

/*!
 * \brief Get timesync configurations for the port.
 *
 * \param [in] lport Port number.
 * \param [in] array_size Required Count of timesync configurations.
 * \param [in/out] config_array Pointer to timesync configurations.
 * \param [out] array_count Pointer to timesync configuration array count.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
bcm5607x_port_timesync_config_get(uint8 unit, uint8 lport,
                                int array_size,
                                bcm_port_timesync_config_t *config_array,
                                int *array_count)
{
    int cnt, config_cnt =0;
    uint32 index, value;
    bcm_port_timesync_config_t *ts_config;
    EGR_1588_INGRESS_CTRLr_t ing_ctrl;
    EGR_1588_EGRESS_CTRLr_t egr_ctrl;
    EGR_1588_SAm_t sa;
    PORTm_t port_tab;
    uint32 ts_index[TIMESYNC_MAX_PROFILE];

    /* Check for array_count */
    if ((NULL == array_count) ||
       (array_size > 0 && NULL == config_array)) {
        return SYS_ERR_PARAMETER;
    }

    /* Read Port Table, do limited operations, holding port tab memory lock*/
    sal_memset(ts_index, 0, sizeof(ts_index));
    ts_index[TS_PORT_INDEX] = 0xffffffff;

    SOC_IF_ERROR_RETURN(READ_PORTm(unit, lport, port_tab));

    index = PORTm_CTRL_PROFILE_INDEX_1588f_GET(port_tab);

    /* Store all 1588 port indexes for later use */
    if (ts_index[TS_PORT_INDEX] == 0xffffffff) {
        ts_index[TS_PORT_INDEX] = index;
        config_cnt++;
    }

    /* update config cnt */
    *array_count = config_cnt;

    /* if config_array is null, return array count */
    if ((NULL == config_array)) {
        return SYS_OK;
    }

    for (cnt = 0; (cnt < array_size) && (array_size <= config_cnt); cnt++)
    {
        ts_config = (bcm_port_timesync_config_t*) (config_array + cnt);
        sal_memset(ts_config, 0, sizeof(bcm_port_timesync_config_t));
        /* Get timesync port control profile */
        if ((cnt == TS_PORT_INDEX) && (ts_index[TS_PORT_INDEX] != 0xffffffff)) {
            ts_config->flags |= BCM_PORT_TIMESYNC_DEFAULT;
            index = ts_index[TS_PORT_INDEX];

            SOC_IF_ERROR_RETURN(READ_EGR_1588_EGRESS_CTRLr(unit, lport, egr_ctrl));
            SOC_IF_ERROR_RETURN(READ_EGR_1588_INGRESS_CTRLr(unit, lport, ing_ctrl));

            value = 0;
            value |= EGR_1588_EGRESS_CTRLr_TX_TS_SYNCf_GET(egr_ctrl);
            value |= EGR_1588_EGRESS_CTRLr_TX_TS_DELAY_REQf_GET(egr_ctrl);
            value |= EGR_1588_EGRESS_CTRLr_TX_TS_PDELAY_REQf_GET(egr_ctrl);
            value |= EGR_1588_EGRESS_CTRLr_TX_TS_PDELAY_RESPf_GET(egr_ctrl);
            if (value) {
                ts_config->flags |= BCM_PORT_TIMESYNC_TWO_STEP_TIMESTAMP;
            }

            if (EGR_1588_EGRESS_CTRLr_DROP_INVALID_1588_PKTf_GET(egr_ctrl)) {
                ts_config->pkt_drop |= BCM_PORT_TIMESYNC_PKT_INVALID;
            }

            value = EGR_1588_EGRESS_CTRLr_CF_UPDATE_MODEf_GET(egr_ctrl);
            if (value == 1) {
                ts_config->flags |= BCM_PORT_TIMESYNC_TIMESTAMP_CFUPDATE_ALL;
            }
            if (EGR_1588_INGRESS_CTRLr_CF_UPDATE_ENABLEf_GET(ing_ctrl)) {
                ts_config->flags |=  BCM_PORT_TIMESYNC_ONE_STEP_TIMESTAMP;
            }

            if (EGR_1588_EGRESS_CTRLr_SA_UPDATE_ENABLEf_GET(egr_ctrl)) {
                READ_EGR_1588_SAm(unit, lport, sa);
                sal_memcpy(ts_config->src_mac_addr, &sa._egr_1588_sa, SHR_MAC_ADDR_LEN);
            }

            SOC_IF_ERROR_RETURN(
                _bcm_port_timesync_control_profile_entry_get(unit, ts_config, index));
        }
    }

    return SYS_OK;
}

/*!
 * \brief Set PHY/PCS timesync configurations for the port.
 *
 * \param [in] unit Unit number.
 * \param [in] lport Port number.
 * \param [in] en value for enable/disable.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
bcm5607x_port_phy_timesync_enable_set(uint8 unit, uint8 lport, int en)
{
    return pcm_phy_timesync_enable_set(unit, lport, en);
}

/*!
 * \brief Set PHY/PCS timesync configurations for the port.
 *
 * \param [in] unit Unit number.
 * \param [in] lport Port number.
 * \param [out] en value for enable/disable.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
bcm5607x_port_phy_timesync_enable_get(uint8 unit, uint8 lport, int *en)
{
    return pcm_phy_timesync_enable_get(unit, lport, en);
}

/*!
 * \brief Set timesync PHY/PCS features for the port.
 *
 * \param [in] unit Unit number.
 * \param [in] lport Port number.
 * \param [in] type Port feature enumerator.
 * \param [in] value Value to configure for the feature.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
bcm5607x_port_control_phy_timesync_set(uint8 unit, uint8 lport,
                            bcm_port_control_phy_timesync_t type,
                            uint64 value)
{
    sys_error_t rv = SYS_OK;

    switch (type) {
        case bcmPortControlPhyTimesyncTimestampOffset:
            rv = pcm_phy_timesync_ctrl_set(unit, lport,
                                     PCM_PHY_TIMESYNC_TIMESTAMP_OFFSET, value);
            break;
        case bcmPortControlPhyTimesyncTimestampAdjust:
            rv = pcm_phy_timesync_ctrl_set(unit, lport,
                                     PCM_PHY_TIMESYNC_TIMESTAMP_ADJUST, value);
            break;
        default:
            return SYS_ERR_NOT_IMPLEMENTED;
    }

    return rv;
}

/*!
 * \brief Get 1588 packet's transmit information for the port.
 *
 * \param [in] uport Port number.
 * \param [in/out] tx_info Pointer to structure to get timesync tx information.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
bcm5607x_port_timesync_tx_info_get(uint8 unit, uint8 lport,
                            bcm_port_timesync_tx_info_t *tx_info)
{
    pcm_port_timesync_tx_info_t info;

    sal_memset(&info, 0, sizeof(pcm_port_timesync_tx_info_t));

    SOC_IF_ERROR_RETURN(pcm_port_timesync_tx_info_get(unit, lport, &info));

    COMPILER_64_SET(tx_info->timestamp, info.timestamps_in_fifo_hi, info.timestamps_in_fifo);
    tx_info->sequence_id = info.sequence_id;
    tx_info->sub_ns      = info.sub_ns;

    return SYS_OK;
}
#endif /* CFG_SWITCH_TIMESYNC_INCLUDED */
