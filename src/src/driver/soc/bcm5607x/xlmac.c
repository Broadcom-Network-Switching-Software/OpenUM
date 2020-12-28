/*
 * $Id: xlmac.c,v 1.5 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#include "utils/system.h"
#undef SOC_IF_ERROR_RETURN
#include <soc/error.h>
#undef _SOC_PHYCTRL_H_
#include <soc/phyctrl.h>

/* from sdk/include/sal/compiler.h */
#define COMPILER_REFERENCE(_a)    ((void)(_a))
#define COMPILER_ATTRIBUTE(_a)    __attribute__ (_a)
#ifndef FUNCTION_NAME
#define FUNCTION_NAME() (__FUNCTION__)
#endif

#include <shared/bsl.h>
#include <shared/bslenum.h>
#include <shared/bsltypes.h>

//#define BSL_LS_SOC_XLMAC_DEBUG
//#define UM_READBACK_DEBUG

extern int
soc_mmu_flush_enable(int unit, uint8 lport, int enable);
extern int
soc_port_mmu_buffer_enable(int unit, uint8 lport, int enable);
extern int
soc_egress_drain_cells(int unit, uint8 lport, uint32 drain_timeout);
extern int
soc_port_egress_buffer_sft_reset(int unit, uint8 lport, int reset);
extern int
soc_port_speed_update(int unit, uint8 lport, int speed);
extern int
soc_port_epc_link_set(int unit, uint8 lport, int link);
extern int
soc_port_epc_link_get(int unit, uint8 lport, int *link);

#if defined(LOG_VERBOSE) && defined(BSL_LS_SOC_XLMAC_DEBUG)
/* Outout the debug message with LOG_VERBOSE() */
static char *mac_cl_encap_mode[] = SOC_ENCAP_MODE_NAMES_INITIALIZER;
//static char *mac_cl_port_if_names[] = SOC_PORT_IF_NAMES_INITIALIZER;
#else
#undef LOG_VERBOSE
#define LOG_VERBOSE(ls_, stuff_)
#endif

#define UM_PHY_NOTIFY  1

//#define CFG_TIMESTAMP_MAC_DELAY 1
#ifdef CFG_TIMESTAMP_MAC_DELAY
/* define default spn_TIMESTAMP_ADJUST value if not "0" */
#define SPN_TIMESTAMP_ADJUST    0

/* 250Mhz TS PLL implies 4ns resolution */
#define SOC_TIMESYNC_PLL_CLOCK_NS(unit) (1/250 * 1000)
#define XLMAC_TIMESTAMP_ADJUST__TS_OSTS_ADJUST__MASK ((1<<9)-1)
#endif

/*
 * Forward Declarations
 */
mac_driver_t soc_mac_xl;

#define XLMAC_RUNT_THRESHOLD_IEEE  0x40
#define XLMAC_RUNT_THRESHOLD_HG1   0x48
#define XLMAC_RUNT_THRESHOLD_HG2   0x4c
#define XLMAC_RUNT_THRESHOLD_MIN   0x11
#define XLMAC_RUNT_THRESHOLD_MAX   0x60

/*
 * XLMAC Register field definitions.
 */

/* Max legal value (per regsfile) */
#define JUMBO_MAXSZ 0x3fe8

#define SOC_XLMAC_SPEED_10     0x0
#define SOC_XLMAC_SPEED_100    0x1
#define SOC_XLMAC_SPEED_1000   0x2
#define SOC_XLMAC_SPEED_2500   0x3
#define SOC_XLMAC_SPEED_10000  0x4

#define SOC_XLMAC_XLPORT_MODE_SINGLE    4
#define SOC_XLMAC_XLPORT_MODE_QUAD      0

/* Transmit CRC Modes */
#define XLMAC_CRC_APPEND        0x0
#define XLMAC_CRC_KEEP          0x1
#define XLMAC_CRC_REPLACE       0x2
#define XLMAC_CRC_PER_PKT_MODE  0x3

#define FD_XE_IPG   96
#define FD_HG_IPG   64
#define FD_HG2_IPG  96

struct {
    int speed;
    uint32 clock_rate;
}_mac_xl_clock_rate[] = {
    { 40000, 312 },
    { 20000, 156 },
    { 10000, 78  },
    { 5000,  78  },
    { 2500,  312 },
    { 1000,  125 },
    { 0,     25  },
};

static int
soc_port_eee_timers_init(int unit, soc_port_t port, int speed)
{
    /* Do nothing for FL */
    return SYS_OK;
}

static int
soc_port_eee_timers_setup(int unit, soc_port_t port, int speed)
{
    /* Do nothing for FL */
    return SYS_OK;
}

static int
soc_port_thdo_rx_enable_set(int unit, soc_port_t port, int enable)
{
    /* Do nothing for FL */
    return SYS_OK;
}

static int
soc_port_fifo_reset(int unit, uint8 lport)
{
    /* Do nothing for FL */
    return SYS_OK;
}

static int
xlport_credit_reset(int unit, uint8 lport)
{
    int phy_port;
    int bindex;
    PGW_XL_TXFIFO_CTRLr_t pgw_xl_txfifo_ctrl;
    XLPORT_ENABLE_REGr_t xlport_enable_reg;
    EGR_PORT_CREDIT_RESETm_t egr_port_credit_reset;

    phy_port = SOC_PORT_L2P_MAPPING(lport);
    bindex = bcm5607x_xlport_pport_to_index_in_block[phy_port];

    SOC_IF_ERROR_RETURN
        (READ_XLPORT_ENABLE_REGr(unit, lport, xlport_enable_reg));
    if (bindex == 0) {
        XLPORT_ENABLE_REGr_PORT0f_SET(xlport_enable_reg, 0);
    } else if (bindex == 1) {
        XLPORT_ENABLE_REGr_PORT1f_SET(xlport_enable_reg, 0);
    } else if (bindex == 2) {
        XLPORT_ENABLE_REGr_PORT2f_SET(xlport_enable_reg, 0);
    } else if (bindex == 3) {
        XLPORT_ENABLE_REGr_PORT3f_SET(xlport_enable_reg, 0);
    }
    SOC_IF_ERROR_RETURN
        (WRITE_XLPORT_ENABLE_REGr(unit, lport, xlport_enable_reg));

    /* To clear the port credit (per physical port) */
    EGR_PORT_CREDIT_RESETm_CLR(egr_port_credit_reset);
    EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 1);
    SOC_IF_ERROR_RETURN
        (WRITE_EGR_PORT_CREDIT_RESETm(unit, phy_port, egr_port_credit_reset));

    SOC_IF_ERROR_RETURN
        (READ_PGW_XL_TXFIFO_CTRLr(unit, lport, pgw_xl_txfifo_ctrl));
    PGW_XL_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(pgw_xl_txfifo_ctrl, 1);
    PGW_XL_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(pgw_xl_txfifo_ctrl, 1);
    SOC_IF_ERROR_RETURN
        (WRITE_PGW_XL_TXFIFO_CTRLr(unit, lport, pgw_xl_txfifo_ctrl));

    sal_usleep(1000);

    EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 0);
    SOC_IF_ERROR_RETURN
        (WRITE_EGR_PORT_CREDIT_RESETm(unit, phy_port, egr_port_credit_reset));

    PGW_XL_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(pgw_xl_txfifo_ctrl, 0);
    PGW_XL_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(pgw_xl_txfifo_ctrl, 0);
    SOC_IF_ERROR_RETURN
        (WRITE_PGW_XL_TXFIFO_CTRLr(unit, lport, pgw_xl_txfifo_ctrl));

    if (bindex == 0) {
        XLPORT_ENABLE_REGr_PORT0f_SET(xlport_enable_reg, 1);
    } else if (bindex == 1) {
        XLPORT_ENABLE_REGr_PORT1f_SET(xlport_enable_reg, 1);
    } else if (bindex == 2) {
        XLPORT_ENABLE_REGr_PORT2f_SET(xlport_enable_reg, 1);
    } else if (bindex == 3) {
        XLPORT_ENABLE_REGr_PORT3f_SET(xlport_enable_reg, 1);
    }
    SOC_IF_ERROR_RETURN
        (WRITE_XLPORT_ENABLE_REGr(unit, lport, xlport_enable_reg));

    return SYS_OK;
}

static int
_mac_xl_drain_cells(int unit, uint8 lport, int notify_phy, int queue_enable)
{
    int rv;
    int pause_tx = 0, pause_rx = 0, pfc_rx = 0, llfc_rx = 0;
    XLMAC_CTRLr_t xlmac_ctrl;
    XLMAC_TX_CTRLr_t xlmac_tx_ctrl;
    XLMAC_TXFIFO_CELL_CNTr_t txfifo_cell_cnt;
    uint32 drain_timeout, fval;

#if CONFIG_EMULATION
    drain_timeout = 250000000;
#else
    drain_timeout = 250000;
#endif

    /* Disable pause/pfc/llfc function */
    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_pause_get(unit, lport, &pause_tx, &pause_rx));
    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_pause_set(unit, lport, pause_tx, 0));

    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_control_get(unit, lport, SOC_MAC_CONTROL_PFC_RX_ENABLE,
                                   &pfc_rx));
    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_control_set(unit, lport, SOC_MAC_CONTROL_PFC_RX_ENABLE,
                                   0));

    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_control_get(unit, lport, SOC_MAC_CONTROL_LLFC_RX_ENABLE,
                                   &llfc_rx));
    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_control_set(unit, lport, SOC_MAC_CONTROL_LLFC_RX_ENABLE,
                                   0));

    /* Assert SOFT_RESET before DISCARD just in case there is no credit left */
    SOC_IF_ERROR_RETURN
        (READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
    XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 1);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

    /* Drain data in TX FIFO without egressing */
    SOC_IF_ERROR_RETURN
        (READ_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
    XLMAC_TX_CTRLr_DISCARDf_SET(xlmac_tx_ctrl, 1);
    XLMAC_TX_CTRLr_EP_DISCARDf_SET(xlmac_tx_ctrl, 1);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));

    /* Reset EP credit before de-assert SOFT_RESET */
    SOC_IF_ERROR_RETURN
        (xlport_credit_reset(unit, lport));

    /* De-assert SOFT_RESET to let the drain start */
    SOC_IF_ERROR_RETURN
        (READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
    XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 0);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

#ifdef UM_PHY_NOTIFY
    if (notify_phy) {
        /* Notify PHY driver */
        SOC_IF_ERROR_RETURN
            (soc_phyctrl_notify(unit, lport, phyEventStop, PHY_STOP_DRAIN));
    }
#endif

    /* Wait until mmu cell count is 0 */
    rv = soc_egress_drain_cells(unit, lport, drain_timeout);
    if (SYS_OK == rv) {
        /* Wait until TX fifo cell count is 0 */
        for (;;) {
            rv = READ_XLMAC_TXFIFO_CELL_CNTr(unit, lport, txfifo_cell_cnt);
            if (SYS_OK != rv) {
                break;
            }

            fval = XLMAC_TXFIFO_CELL_CNTr_CELL_CNTf_GET(txfifo_cell_cnt);
            if (fval == 0) {
                break;
            }

            sal_usleep(1000);
            drain_timeout -= 1000;
            if (drain_timeout <= 0) {
                sal_printf("%s..: unit %d lport %d drain_timeout\n",
                           __func__, unit, lport);
                rv = SYS_ERR;
                break;
            }
        }
    }

#ifdef UM_PHY_NOTIFY
    if (notify_phy) {
        /* Notify PHY driver */
        SOC_IF_ERROR_RETURN
            (soc_phyctrl_notify(unit, lport, phyEventResume, PHY_STOP_DRAIN));
    }
#endif

    /* Stop TX FIFO drainging */
    SOC_IF_ERROR_RETURN
        (READ_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
    XLMAC_TX_CTRLr_DISCARDf_SET(xlmac_tx_ctrl, 0);
    XLMAC_TX_CTRLr_EP_DISCARDf_SET(xlmac_tx_ctrl, 0);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));

    /* Restore original pause/pfc/llfc configuration */
    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_pause_set(unit, lport, pause_tx, pause_rx));
    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_control_set(unit, lport,
                                   SOC_MAC_CONTROL_PFC_RX_ENABLE,
                                   pfc_rx));
    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_control_set(unit, lport,
                                   SOC_MAC_CONTROL_LLFC_RX_ENABLE,
                                   llfc_rx));

    return SYS_OK;
}

/*
 * Function:
 *      _mac_xl_timestamp_byte_adjust_set
 * Purpose:
 *      Set timestamp byte adjust values.
 * Parameters:
 *      unit - XGS unit #.
 *      port - Port number on unit.
 * Returns:
 *      SYS_ERR_XXX
 */
static int
_mac_xl_timestamp_byte_adjust_set(int unit, uint8 lport)
{
    XLMAC_TIMESTAMP_BYTE_ADJUSTr_t ts_adj;

    if (IS_GE_PORT(lport)) {
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:unit=%d lport %d IS_GE_PORT\n",
                     __func__, unit, lport));

        /* for viper and eagle ports on SB2 B0 */
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_TIMESTAMP_BYTE_ADJUSTr(unit, lport, ts_adj));
        XLMAC_TIMESTAMP_BYTE_ADJUSTr_TX_TIMER_BYTE_ADJUSTf_SET(ts_adj, 8);
        SOC_IF_ERROR_RETURN
            (WRITE_XLMAC_TIMESTAMP_BYTE_ADJUSTr(unit, lport, ts_adj));

        SOC_IF_ERROR_RETURN
            (READ_XLMAC_TIMESTAMP_BYTE_ADJUSTr(unit, lport, ts_adj));
        XLMAC_TIMESTAMP_BYTE_ADJUSTr_TX_TIMER_BYTE_ADJUST_ENf_SET(ts_adj, 1);
        SOC_IF_ERROR_RETURN
            (WRITE_XLMAC_TIMESTAMP_BYTE_ADJUSTr(unit, lport, ts_adj));

        SOC_IF_ERROR_RETURN
            (READ_XLMAC_TIMESTAMP_BYTE_ADJUSTr(unit, lport, ts_adj));
        XLMAC_TIMESTAMP_BYTE_ADJUSTr_RX_TIMER_BYTE_ADJUSTf_SET(ts_adj, 8);
        SOC_IF_ERROR_RETURN
            (WRITE_XLMAC_TIMESTAMP_BYTE_ADJUSTr(unit, lport, ts_adj));

        SOC_IF_ERROR_RETURN
            (READ_XLMAC_TIMESTAMP_BYTE_ADJUSTr(unit, lport, ts_adj));
        XLMAC_TIMESTAMP_BYTE_ADJUSTr_RX_TIMER_BYTE_ADJUST_ENf_SET(ts_adj, 1);
        SOC_IF_ERROR_RETURN
            (WRITE_XLMAC_TIMESTAMP_BYTE_ADJUSTr(unit, lport, ts_adj));
    }

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_init
 * Purpose:
 *      Initialize Xlmac into a known good state.
 * Parameters:
 *      unit - XGS unit #.
 *      port - Port number on unit.
 * Returns:
 *      SYS_ERR_XXX
 * Notes:
 */
static int
mac_xl_init(int unit, uint8 lport)
{
    XLMAC_CTRLr_t xlmac_ctrl;
    XLMAC_RX_CTRLr_t xlmac_rx_ctrl;
    XLMAC_TX_CTRLr_t xlmac_tx_ctrl;
    XLMAC_PFC_CTRLr_t xlmac_pfc_ctrl;
    XLMAC_RX_MAX_SIZEr_t xlmac_rx_max_size;
    XLMAC_MODEr_t xlmac_mode;
    XLMAC_RX_LSS_CTRLr_t xlmac_rx_lss_ctrl;
    XLPORT_MAC_RSV_MASKr_t xlport_mac_rsv_mask;
    int ipg;
    int mode;
    int runt;
    int encap = 0;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d\n",
                 __func__, unit, lport));

    /* Disable Tx/Rx, assume that MAC is stable (or out of reset) */
    SOC_IF_ERROR_RETURN
        (READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

    /* Reset EP credit before de-assert SOFT_RESET */
    if (XLMAC_CTRLr_SOFT_RESETf_GET(xlmac_ctrl)) {
        SOC_IF_ERROR_RETURN
            (xlport_credit_reset(unit, lport));
    }

    XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 0);
    XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, 0);
    XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, 0);
    XLMAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(xlmac_ctrl,
                                             IS_HG_PORT(lport) ? 1 : 0);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

    if (IS_ST_PORT(lport)) {
        soc_mac_xl.md_pause_set(unit, lport, FALSE, FALSE);
    } else {
        soc_mac_xl.md_pause_set(unit, lport, TRUE, TRUE);
    }

    SOC_IF_ERROR_RETURN
        (READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
    XLMAC_PFC_CTRLr_PFC_REFRESH_ENf_SET(xlmac_pfc_ctrl, 1);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));

    /* Set jumbo max size (16360 byte payload) */
    XLMAC_RX_MAX_SIZEr_CLR(xlmac_rx_max_size);
    XLMAC_RX_MAX_SIZEr_RX_MAX_SIZEf_SET(xlmac_rx_max_size, JUMBO_MAXSZ);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_RX_MAX_SIZEr(unit, lport, xlmac_rx_max_size));

    XLMAC_MODEr_CLR(xlmac_mode);
#if 0
    if (!IS_XE_PORT(unit, lport) && !IS_GE_PORT(unit, lport)) {
        mode = soc_property_port_get(unit, port, spn_HIGIG2_HDR_MODE,
               soc_feature(unit, soc_feature_no_higig_plus) ? 1 : 0) ? 2 : 1;
        soc_reg64_field32_set(unit, XLMAC_MODEr, &rval, HDR_MODEf, mode);
        encap = mode;
    }
#endif
    switch (SOC_PORT_SPEED_MAX(lport)) {
    case 10:
        mode = SOC_XLMAC_SPEED_10;
        break;
    case 100:
        mode = SOC_XLMAC_SPEED_100;
        break;
    case 1000:
        mode = SOC_XLMAC_SPEED_1000;
        break;
    case 2500:
        mode = SOC_XLMAC_SPEED_2500;
        break;
    default:
        mode = SOC_XLMAC_SPEED_10000;
        break;
    }
    XLMAC_MODEr_SPEED_MODEf_SET(xlmac_mode, mode);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_MODEr(unit, lport, xlmac_mode));

    /*
     * 1. Initialize mask for purging packet data received from the MAC
     * 2. Diable length check
     */
    XLPORT_MAC_RSV_MASKr_CLR(xlport_mac_rsv_mask);
    XLPORT_MAC_RSV_MASKr_MASKf_SET(xlport_mac_rsv_mask, 0x58);
    WRITE_XLPORT_MAC_RSV_MASKr(unit, lport, xlport_mac_rsv_mask);

    /* init IPG and RUNT_THRESHOLD after port encap mode been established. */
    if (encap == 1) {
        ipg = FD_HG_IPG;
        runt = XLMAC_RUNT_THRESHOLD_HG1;
    } else if (encap == 2) {
        ipg = FD_HG2_IPG;
        runt = XLMAC_RUNT_THRESHOLD_HG2;
    } else {
        ipg = FD_XE_IPG;
        runt = XLMAC_RUNT_THRESHOLD_IEEE;
    }

    SOC_IF_ERROR_RETURN
        (READ_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
    XLMAC_TX_CTRLr_AVERAGE_IPGf_SET(xlmac_tx_ctrl, ((ipg / 8) & 0x1f));
    XLMAC_TX_CTRLr_CRC_MODEf_SET(xlmac_tx_ctrl, XLMAC_CRC_PER_PKT_MODE);
    XLMAC_TX_CTRLr_TX_THRESHOLDf_SET(xlmac_tx_ctrl, 6);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));

    SOC_IF_ERROR_RETURN
        (READ_XLMAC_RX_CTRLr(unit, lport, xlmac_rx_ctrl));
    XLMAC_RX_CTRLr_STRIP_CRCf_SET(xlmac_rx_ctrl, 0);
    XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(xlmac_rx_ctrl,
                                        SOC_PORT_SPEED_MAX(lport) >= 10000 &&
                                        IS_XE_PORT(lport) ? 1 : 0);

    /* assigning RUNT_THRESHOLD (per encap mode) */
    XLMAC_RX_CTRLr_RUNT_THRESHOLDf_SET(xlmac_rx_ctrl, runt);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_RX_CTRLr(unit, lport, xlmac_rx_ctrl));

    SOC_IF_ERROR_RETURN
        (soc_port_eee_timers_init(unit, lport, SOC_PORT_SPEED_MAX(lport)));

    SOC_IF_ERROR_RETURN
        (READ_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));
    XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LOCAL_FAULTf_SET(xlmac_rx_lss_ctrl, 1);
    XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_REMOTE_FAULTf_SET(xlmac_rx_lss_ctrl, 1);
    XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LINK_INTERRUPTf_SET(xlmac_rx_lss_ctrl, 1);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));

    /* Disable loopback and bring XLMAC out of reset */
    SOC_IF_ERROR_RETURN
        (READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
    XLMAC_CTRLr_LOCAL_LPBKf_SET(xlmac_ctrl, 0);
    XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, 1);
    XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, 1);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

    SOC_IF_ERROR_RETURN
        (_mac_xl_timestamp_byte_adjust_set(unit, lport));

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_egress_queue_drain
 * Purpose:
 *      Drain the egress queues with out bringing down the port
 * Parameters:
 *      unit - XGS unit #.
 *      port - Port number on unit.
 * Returns:
 *      SYS_ERR_XXX
 */
static int
mac_xl_egress_queue_drain(int unit, uint8 lport)
{
    int rx_enable = 0;
    int is_active = 0;
    XLMAC_CTRLr_t xlmac_ctrl, xlmac_ctrl_orig;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d\n",
                 __func__, unit, lport));

    SOC_IF_ERROR_RETURN
        (READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
    sal_memcpy(&xlmac_ctrl_orig, &xlmac_ctrl, sizeof(xlmac_ctrl_orig));

    rx_enable = XLMAC_CTRLr_RX_ENf_GET(xlmac_ctrl);
    /* Don't disable TX since it stops egress and hangs if CPU sends */
    XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, 1);
    XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, 0);
    /* Disable RX */
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

    /* Remove port from EPC_LINK */
    SOC_IF_ERROR_RETURN
        (soc_port_epc_link_get(unit, lport, &is_active));
    if (is_active) {
        SOC_IF_ERROR_RETURN
            (soc_port_epc_link_set(unit, lport, 0));
    }

    /* Drain cells */
    SOC_IF_ERROR_RETURN
        (_mac_xl_drain_cells(unit, lport, 0, TRUE));

    /* Reset port FIFO */
    SOC_IF_ERROR_RETURN
        (soc_port_fifo_reset(unit, lport));

    /* Put port into SOFT_RESET */
    XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 1);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

    /* Reset EP credit before de-assert SOFT_RESET */
    SOC_IF_ERROR_RETURN
        (xlport_credit_reset(unit, lport));
    SOC_IF_ERROR_RETURN
        (READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
    XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, rx_enable ? 1 : 0);

    /* Enable both TX and RX, deassert SOFT_RESET */
    XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 0);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

    /* Restore xlmac_ctrl to original value */
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl_orig));

    /* Add port to EPC_LINK */
    if(is_active) {
        SOC_IF_ERROR_RETURN
            (soc_port_epc_link_set(unit, lport, 1));
    }

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_enable_set
 * Purpose:
 *      Enable or disable MAC
 * Parameters:
 *      unit - XGS unit #.
 *      port - Port number on unit.
 *      enable - TRUE to enable, FALSE to disable
 * Returns:
 *      SYS_ERR_XXX
 */
static int
mac_xl_enable_set(int unit, uint8 lport, int enable)
{
    XLMAC_CTRLr_t xlmac_ctrl, xlmac_ctrl_orig;
#ifdef CFG_TIMESTAMP_MAC_DELAY
    int speed = 1000;
#endif

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d enable=%d\n",
                 __func__, unit, lport, enable));

    SOC_IF_ERROR_RETURN
        (READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl) );
    sal_memcpy(&xlmac_ctrl_orig, &xlmac_ctrl, sizeof(xlmac_ctrl_orig));

    /* Don't disable TX since it stops egress and hangs if CPU sends */
    XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, 1);
    XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, enable ? 1 : 0);

    if (!sal_memcmp(&xlmac_ctrl, &xlmac_ctrl_orig, sizeof(xlmac_ctrl_orig))) {
        if (enable) {
            return SYS_OK;
        } else {
            if (XLMAC_CTRLr_SOFT_RESETf_GET(xlmac_ctrl)) {
                return SYS_OK;
            }
        }
    }

    if (enable) {
        /* Reset EP credit before de-assert SOFT_RESET */
        SOC_IF_ERROR_RETURN
            (xlport_credit_reset(unit, lport));

        /* Deassert SOFT_RESET */
        XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 0);

        /* Deassert EGR_XLPORT_BUFFER_SFT_RESET */
        SOC_IF_ERROR_RETURN
            (soc_port_egress_buffer_sft_reset(unit, lport, 0));

        /* Enable both TX and RX */
        SOC_IF_ERROR_RETURN
            (WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

        /* Enable MMU port */
        SOC_IF_ERROR_RETURN
            (soc_port_mmu_buffer_enable(unit, lport, TRUE));

        /* Add port to EPC_LINK */
        SOC_IF_ERROR_RETURN
            (soc_port_epc_link_set(unit, lport, 1));

        /* Enable output threshold RX */
        SOC_IF_ERROR_RETURN
            (soc_port_thdo_rx_enable_set(unit, lport, 1));

#ifdef CFG_TIMESTAMP_MAC_DELAY
        /* set timestamp adjust delay */
        mac_xl_speed_get(unit, lport, &speed);
        _mac_xl_timestamp_delay_set(unit, lport, speed);
#endif
    } else {
        /* Disable MAC RX */
        SOC_IF_ERROR_RETURN
            (WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

        /* Remove port from EPC_LINK */
        SOC_IF_ERROR_RETURN
            (soc_port_epc_link_set(unit, lport, 0));

        /* Delay to ensure EOP is received at Ingress */
        sal_usleep(1000);

        /*
         * Disable MMU port
         * for some devices where EPC_LINK can not block the SOBMH from cpu and
         * no mechanism is avalible to reset EDATABUF.
         */
        SOC_IF_ERROR_RETURN(soc_port_mmu_buffer_enable(unit, lport, FALSE));

        /* Drain cells */
        SOC_IF_ERROR_RETURN(_mac_xl_drain_cells(unit, lport, 1, FALSE));

        /* Reset egress_buffer */
        SOC_IF_ERROR_RETURN(soc_port_egress_buffer_sft_reset(unit, lport, 1));

        /* Reset port FIFO */
        SOC_IF_ERROR_RETURN(soc_port_fifo_reset(unit, lport));

        /* Put port into SOFT_RESET */
        XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 1);
        SOC_IF_ERROR_RETURN
            (WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

        /* Disable output threshold RX */
        SOC_IF_ERROR_RETURN
            (soc_port_thdo_rx_enable_set(unit, lport, 0));
    }

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_enable_get
 * Purpose:
 *      Get MAC enable state
 * Parameters:
 *      unit - XGS unit #.
 *      port - Port number on unit.
 *      enable - (OUT) TRUE if enabled, FALSE if disabled
 * Returns:
 *      SYS_ERR_XXX
 */
static int
mac_xl_enable_get(int unit, uint8 lport, int *enable)
{
    XLMAC_CTRLr_t xlmac_ctrl;

    SOC_IF_ERROR_RETURN
        (READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

    *enable = XLMAC_CTRLr_RX_ENf_GET(xlmac_ctrl);

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d *enable=%d\n",
                 __func__, unit, lport, *enable));

    return SYS_OK;
}

#ifdef CFG_TIMESTAMP_MAC_DELAY
static void
_mac_xl_speed_to_clock_rate(int unit, uint8 lport, int speed,
                            uint32 *clock_rate)
{
    int idx;

    for (idx = 0;
         idx < sizeof(_mac_xl_clock_rate) / sizeof(_mac_xl_clock_rate[0]);
         idx++) {
        if (speed >=_mac_xl_clock_rate[idx].speed) {
            *clock_rate = _mac_xl_clock_rate[idx].clock_rate;
            return;
        }
    }
    *clock_rate = 0;
}

/*
 * Function:
 *      _mac_xl_timestamp_delay_set
 * Purpose:
 *      Set Timestamp delay for one-step to account for lane and pipeline delay.
 * Parameters:
 *      unit - XGS unit #.
 *      port - Port number on unit.
 *      speed - Speed
 *      phy_mode - single/dual/quad phy mode
 * Returns:
 *      SYS_ERR_XXX
 */
static int
_mac_xl_timestamp_delay_set(int unit, uint8 lport, int speed)
{
    uint32 clk_rate, tx_clk_ns;
    int osts_delay;
    int divisor;
    XLMAC_TIMESTAMP_ADJUSTr_t xlmac_timestamp_adjust;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d speed=%dMb\n",
                 __func__, unit, lport, speed));

    SOC_IF_ERROR_RETURN(
        READ_XLMAC_TIMESTAMP_ADJUSTr(unit, lport, xlmac_timestamp_adjust));

    _mac_xl_speed_to_clock_rate(unit, lport, speed, &clk_rate);
    /* Tx clock rate for single/dual/quad phy mode */
    if ((speed >= 5000) && (speed <= 40000)) {
        divisor = speed > 20000 ? 1 : speed > 10000 ? 2 : 4;
        /* tx clock rate in ns */
        tx_clk_ns = ((1000 / clk_rate) / divisor);
    } else {
        /* Same tx clk rate for < 10G  for all phy modes*/
        tx_clk_ns = 1000 / clk_rate;
    }

    
    
    /*
     * MAC pipeline delay for XGMII/XGMII mode is:
     *          = (5.5 * TX line clock period) + (Timestamp clock period)
     */
    /* signed value of pipeline delay in ns */
    osts_delay = SPN_TIMESTAMP_ADJUST;
    if (osts_delay == 0) {
        osts_delay = SOC_TIMESYNC_PLL_CLOCK_NS(unit) - ((11 * tx_clk_ns ) / 2);
    }
    osts_delay &= XLMAC_TIMESTAMP_ADJUST__TS_OSTS_ADJUST__MASK;
    XLMAC_TIMESTAMP_ADJUSTr_TS_OSTS_ADJUSTf_SET(xlmac_timestamp_adjust,
                                                osts_delay);

#if 0 
    /*
     * Lane delay for xlmac lanes
     *   Lane_0(0-3)  : 1 * TX line clock period
     *   Lane_1(4-7)  : 2 * TX line clock period
     *   Lane_2(8-11) : 3 * TX line clock period
     *   Lane_3(12-15): 4 * TX line clock period
     */
    /* unsigned value of lane delay in ns */
    delay = 1 * tx_clk_ns;
    soc_reg64_field32_set(unit, XLMAC_OSTS_TIMESTAMP_ADJUSTr, &ctrl,
                          TS_ADJUST_DEMUX_DELAY_0f, delay );
    delay = 2 * tx_clk_ns;
    soc_reg64_field32_set(unit, XLMAC_OSTS_TIMESTAMP_ADJUSTr, &ctrl,
                          TS_ADJUST_DEMUX_DELAY_1f, delay );
    delay = 3 * tx_clk_ns;
    soc_reg64_field32_set(unit, XLMAC_OSTS_TIMESTAMP_ADJUSTr, &ctrl,
                          TS_ADJUST_DEMUX_DELAY_2f, delay );
    delay = 4 * tx_clk_ns;
    soc_reg64_field32_set(unit, XLMAC_OSTS_TIMESTAMP_ADJUSTr, &ctrl,
                          TS_ADJUST_DEMUX_DELAY_3f, delay );
#endif
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_TIMESTAMP_ADJUSTr(unit, lport, xlmac_timestamp_adjust));

    /* Set Timestamp byte adjust values */
    SOC_IF_ERROR_RETURN
        (_mac_xl_timestamp_byte_adjust_set(unit, lport));

    return SYS_OK;
}
#endif //ifdef CFG_TIMESTAMP_MAC_DELAY

/*
 * Function:
 *      mac_xl_duplex_set
 * Purpose:
 *      Set XLMAC in the specified duplex mode.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      duplex - Boolean: true --> full duplex, false --> half duplex.
 * Returns:
 *      SYS_ERR_XXX
 * Notes:
 */
static int
mac_xl_duplex_set(int unit, uint8 lport, int duplex)
{
    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d duplex=%d\n",
                 __func__, unit, lport, duplex));

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_duplex_get
 * Purpose:
 *      Get XLMAC duplex mode.
 * Parameters:
 *      unit - XGS unit #.
 *      duplex - (OUT) Boolean: true --> full duplex, false --> half duplex.
 * Returns:
 *      SYS_ERR_XXX
 */
static int
mac_xl_duplex_get(int unit, uint8 lport, int *duplex)
{
    *duplex = TRUE; /* Always full duplex */

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d *duplex=%d\n",
                 __func__, unit, lport, *duplex));

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_pause_set
 * Purpose:
 *      Configure XLMAC to transmit/receive pause frames.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      pause_tx - Boolean: transmit pause or -1 (don't change)
 *      pause_rx - Boolean: receive pause or -1 (don't change)
 * Returns:
 *      SYS_ERR_XXX
 */
static int
mac_xl_pause_set(int unit, uint8 lport, int pause_tx, int pause_rx)
{
    XLMAC_PAUSE_CTRLr_t xlmac_pause_ctrl;

    SOC_IF_ERROR_RETURN(
        READ_XLMAC_PAUSE_CTRLr(unit, lport, xlmac_pause_ctrl));
    if (pause_tx) {
        XLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_SET(xlmac_pause_ctrl, 1);
    } else {
        XLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_SET(xlmac_pause_ctrl, 0);
    }

    if (pause_rx) {
        XLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_SET(xlmac_pause_ctrl, 1);
    } else {
        XLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_SET(xlmac_pause_ctrl, 0);
    }
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_PAUSE_CTRLr(unit, lport, xlmac_pause_ctrl));

#if UM_READBACK_DEBUG
    /* Read back after setting xlmac_pause_ctrl for checking */
    SOC_IF_ERROR_RETURN(READ_XLMAC_PAUSE_CTRLr(unit, lport, xlmac_pause_ctrl));

    sal_printf("read back after setting xlmac_pause_ctrl: "
               "lport %d RX=%d TX=%d\n",
               lport,
               XLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_GET(xlmac_pause_ctrl),
               XLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_GET(xlmac_pause_ctrl));
#endif

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_pause_get
 * Purpose:
 *      Return the pause ability of XLMAC
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      pause_tx - Boolean: transmit pause
 *      pause_rx - Boolean: receive pause
 *      pause_mac - MAC address used for pause transmission.
 * Returns:
 *      SYS_ERR_XXX
 */
static int
mac_xl_pause_get(int unit, uint8 lport, int *pause_tx, int *pause_rx)
{
    XLMAC_PAUSE_CTRLr_t xlmac_pause_ctrl;

    SOC_IF_ERROR_RETURN(READ_XLMAC_PAUSE_CTRLr(unit, lport, xlmac_pause_ctrl));

    *pause_rx = XLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_GET(xlmac_pause_ctrl);
    *pause_tx = XLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_GET(xlmac_pause_ctrl);

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("xlmac_pause_ctrl: lport %d RX=%s TX=%s\n",
                 lport,
                 *pause_rx ? "on" : "off",
                 *pause_tx ? "on" : "off"));

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_speed_set
 * Purpose:
 *      Set XLMAC in the specified speed.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      speed - 100000, 120000.
 * Returns:
 *      SYS_ERR_XXX
 */
static int
mac_xl_speed_set(int unit, uint8 lport, int speed)
{
    uint32 mode;
    int enable;
    XLMAC_MODEr_t xlmac_mode;
    XLMAC_RX_CTRLr_t xlmac_rx_ctrl;
    XLMAC_RX_LSS_CTRLr_t xlmac_rx_lss_ctrl;
    XLPORT_FAULT_LINK_STATUSr_t xlport_fault_link_st;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d speed=%dMb\n",
                 __func__, unit, lport, speed));

    switch (speed) {
    case 10:
        mode = SOC_XLMAC_SPEED_10;
        break;
    case 100:
        mode = SOC_XLMAC_SPEED_100;
        break;
    case 1000:
        mode = SOC_XLMAC_SPEED_1000;
        break;
    case 2500:
        mode = SOC_XLMAC_SPEED_2500;
        break;
    case 5000:
        mode = SOC_XLMAC_SPEED_10000;
        break;
    case 0:
        return SYS_OK;              /* Support NULL PHY */
    default:
        if (speed < 10000) {
            return SYS_ERR_PARAMETER;
        }
        mode = SOC_XLMAC_SPEED_10000;
        break;
    }


    SOC_IF_ERROR_RETURN
        (mac_xl_enable_get(unit, lport, &enable));
    if (enable) {
        /* Turn off TX/RX enable */
        SOC_IF_ERROR_RETURN(mac_xl_enable_set(unit, lport, 0));
    }

    /* Update the speed */
    SOC_IF_ERROR_RETURN
        (READ_XLMAC_MODEr(unit, lport, xlmac_mode));
    XLMAC_MODEr_SPEED_MODEf_SET(xlmac_mode, mode);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_MODEr(unit, lport, xlmac_mode));

    SOC_IF_ERROR_RETURN
        (READ_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));
    if (speed < 5000) {
        XLMAC_RX_LSS_CTRLr_LOCAL_FAULT_DISABLEf_SET(xlmac_rx_lss_ctrl, 1);
        XLMAC_RX_LSS_CTRLr_REMOTE_FAULT_DISABLEf_SET(xlmac_rx_lss_ctrl, 1);
    } else {
        XLMAC_RX_LSS_CTRLr_LOCAL_FAULT_DISABLEf_SET(xlmac_rx_lss_ctrl, 0);
        XLMAC_RX_LSS_CTRLr_REMOTE_FAULT_DISABLEf_SET(xlmac_rx_lss_ctrl, 0);
    }
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));

    /*
     * Not set REMOTE_FAULT/LOCAL_FAULT for 1G ports,
     * else HW Linkscan interrupt would be suppressed.
     */
    SOC_IF_ERROR_RETURN
        (READ_XLPORT_FAULT_LINK_STATUSr(unit, lport, xlport_fault_link_st));
    if (speed <= 1000) {
        XLPORT_FAULT_LINK_STATUSr_LOCAL_FAULTf_SET(xlport_fault_link_st, 0);
        XLPORT_FAULT_LINK_STATUSr_REMOTE_FAULTf_SET(xlport_fault_link_st, 0);
    } else {
        XLPORT_FAULT_LINK_STATUSr_LOCAL_FAULTf_SET(xlport_fault_link_st, 1);
        XLPORT_FAULT_LINK_STATUSr_REMOTE_FAULTf_SET(xlport_fault_link_st, 1);
    }
    SOC_IF_ERROR_RETURN
        (WRITE_XLPORT_FAULT_LINK_STATUSr(unit, lport, xlport_fault_link_st));

    /* Update port speed related setting in components other than MAC/SerDes*/
    SOC_IF_ERROR_RETURN(soc_port_speed_update(unit, lport, speed));

    /* Update port strict preamble */
    SOC_IF_ERROR_RETURN
        (READ_XLMAC_RX_CTRLr(unit, lport, xlmac_rx_ctrl));
    /* Enable STRICT_PREAMBLEf[Bit 3] if speed >= 10000 */
    if (speed >= 10000) {
        /* && IS_XE_PORT(unit, port) */
        XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(xlmac_rx_ctrl, 1);
    } else {
        XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(xlmac_rx_ctrl, 0);
    }
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_RX_CTRLr(unit, lport, xlmac_rx_ctrl));

#ifdef UM_PHY_NOTIFY
    /*
     * Notify internal PHY driver of speed change in case it is being
     * used as pass-through to an external PHY.
     */
    SOC_IF_ERROR_RETURN
        (soc_phyctrl_notify(unit, lport, phyEventSpeed, speed));
#endif

    if (enable) {
        /* Re-enable transmitter and receiver */
        SOC_IF_ERROR_RETURN(mac_xl_enable_set(unit, lport, 1));
    }

#ifdef CFG_TIMESTAMP_MAC_DELAY
    /* Set Timestamp Mac Delays */
    _mac_xl_timestamp_delay_set(unit, port, speed);
#endif

    /* assigning proper setting for Native EEE per speed */
    SOC_IF_ERROR_RETURN
        (soc_port_eee_timers_setup(unit, lport, speed));

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_speed_get
 * Purpose:
 *      Get XLMAC speed
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      speed - (OUT) speed in Mb
 * Returns:
 *      SYS_ERR_XXX
 */
static int
mac_xl_speed_get(int unit, uint8 lport, int *speed)
{
    XLMAC_MODEr_t xlmac_mode;

    SOC_IF_ERROR_RETURN
        (READ_XLMAC_MODEr(unit, lport, xlmac_mode));

    switch (XLMAC_MODEr_SPEED_MODEf_GET(xlmac_mode)) {
    case SOC_XLMAC_SPEED_10:
        *speed = 10;
        break;
    case SOC_XLMAC_SPEED_100:
        *speed = 100;
        break;
    case SOC_XLMAC_SPEED_1000:
        *speed = 1000;
        break;
    case SOC_XLMAC_SPEED_2500:
        *speed = 2500;
        break;
    case SOC_XLMAC_SPEED_10000:
    default:
        *speed = 10000;
        break;
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..: unit %d lport %d speed=%dMb\n",
                 __func__, unit, lport, *speed));

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_loopback_set
 * Purpose:
 *      Set a XLMAC into/out-of loopback mode
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS unit # on unit.
 *      loopback - Boolean: true -> loopback mode, false -> normal operation
 * Note:
 *      On Xlmac, when setting loopback, we enable the TX/RX function also.
 *      Note that to test the PHY, we use the remote loopback facility.
 * Returns:
 *      SYS_ERR_XXX
 */
static int
mac_xl_loopback_set(int unit, uint8 lport, int lb)
{
    XLMAC_CTRLr_t xlmac_ctrl;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d loopback=%d\n",
                 __func__, unit, lport, lb));

#ifdef UM_PHY_NOTIFY
    /* need to enable clock compensation for applicable serdes device */
    (void)soc_phyctrl_notify(unit, lport, phyEventMacLoopback, lb? 1: 0);
#endif

    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_control_set(unit, lport,
                                   SOC_MAC_CONTROL_FAULT_LOCAL_ENABLE,
                                   lb ? 0 : 1));
    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_control_set(unit, lport,
                                   SOC_MAC_CONTROL_FAULT_REMOTE_ENABLE,
                                   lb ? 0 : 1));
    SOC_IF_ERROR_RETURN
        (READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
    XLMAC_CTRLr_LOCAL_LPBKf_SET(xlmac_ctrl, lb ? 1 : 0);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_loopback_get
 * Purpose:
 *      Get current XLMAC loopback mode setting.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      loopback - (OUT) Boolean: true = loopback, false = normal
 * Returns:
 *      SYS_ERR_XXX
 */
static int
mac_xl_loopback_get(int unit, uint8 lport, int *lb)
{
    XLMAC_CTRLr_t xlmac_ctrl;

    SOC_IF_ERROR_RETURN
        (READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

    *lb = XLMAC_CTRLr_LOCAL_LPBKf_GET(xlmac_ctrl);

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d loopback=%d\n",
                 __func__, unit, lport, *lb));

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_pause_addr_set
 * Purpose:
 *      Configure PAUSE frame source address.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      pause_mac - (OUT) MAC address used for pause transmission.
 * Returns:
 *      SYS_ERR_XXX
 */
static int
mac_xl_pause_addr_set(int unit, uint8 lport, sal_mac_addr_t mac)
{
    uint32 values[2];
    XLMAC_TX_MAC_SAr_t xlmac_tx_mac_sa;
    XLMAC_RX_MAC_SAr_t xlmac_rx_mac_sa;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d MAC=%02x:%02x:%02x:%02x:%02x:%02x\n",
                 __func__, unit, lport,
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]));

    values[0] = (mac[0] << 8) | mac[1];
    values[1] = (mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5];

    XLMAC_TX_MAC_SAr_SA_LOf_SET(xlmac_tx_mac_sa, values[1]);
    XLMAC_TX_MAC_SAr_SA_HIf_SET(xlmac_tx_mac_sa, values[0]);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_TX_MAC_SAr(unit, lport, xlmac_tx_mac_sa));

    XLMAC_RX_MAC_SAr_SA_LOf_SET(xlmac_rx_mac_sa, values[1]);
    XLMAC_RX_MAC_SAr_SA_HIf_SET(xlmac_rx_mac_sa, values[0]);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_RX_MAC_SAr(unit, lport, xlmac_rx_mac_sa));

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_pause_addr_get
 * Purpose:
 *      Retrieve PAUSE frame source address.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      pause_mac - (OUT) MAC address used for pause transmission.
 * Returns:
 *      SYS_ERR_XXX
 * NOTE: We always write the same thing to TX & RX SA
 *       so, we just return the contects on RX_MAC_SA.
 */
static int
mac_xl_pause_addr_get(int unit, uint8 lport, sal_mac_addr_t mac)
{
    uint32 values[2];
    XLMAC_RX_MAC_SAr_t xlmac_rx_mac_sa;

    SOC_IF_ERROR_RETURN(
        READ_XLMAC_RX_MAC_SAr(unit, lport, xlmac_rx_mac_sa));

    values[1] = XLMAC_RX_MAC_SAr_SA_LOf_GET(xlmac_rx_mac_sa);
    values[0] = XLMAC_RX_MAC_SAr_SA_HIf_GET(xlmac_rx_mac_sa);

    mac[0] = (values[0] & 0x0000ff00) >> 8;
    mac[1] = values[0] & 0x000000ff;
    mac[2] = (values[1] & 0xff000000) >> 24;
    mac[3] = (values[1] & 0x00ff0000) >> 16;
    mac[4] = (values[1] & 0x0000ff00) >> 8;
    mac[5] = values[1] & 0x000000ff;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d MAC=%02x:%02x:%02x:%02x:%02x:%02x\n",
                 __func__, unit, lport,
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]));

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_interface_set
 * Purpose:
 *      Set a XLMAC interface type
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      pif - one of SOC_PORT_IF_*
 * Returns:
 *      SYS_OK
 *      SYS_ERR_UNAVAIL - requested mode not supported.
 * Notes:
 */
static int
mac_xl_interface_set(int unit, uint8 lport, soc_port_if_t pif)
{
    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d pif=%d\n",
                 __func__, unit, lport, pif));

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_interface_get
 * Purpose:
 *      Retrieve XLMAC interface type
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      pif - (OUT) one of SOC_PORT_IF_*
 * Returns:
 *      SYS_OK
 */
static int
mac_xl_interface_get(int unit, uint8 lport, soc_port_if_t *pif)
{
    *pif = SOC_PORT_IF_MII;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d *pif=%d\n",
                 __func__, unit, lport, *pif));

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_frame_max_set
 * Description:
 *      Set the maximum receive frame size for the port
 * Parameters:
 *      unit - Device number
 *      port - Port number
 *      size - Maximum frame size in bytes
 * Return Value:
 *      BCM_E_XXX
 */
static int
mac_xl_frame_max_set(int unit, uint8 lport, int size)
{
    XLMAC_RX_MAX_SIZEr_t xlmac_rx_max_size;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d size=%d\n",
                 __func__, unit, lport, size));

    if (IS_CE_PORT(lport) || IS_XE_PORT(lport)) {
        /* For VLAN tagged packets */
        size += 4;
    }

    SOC_IF_ERROR_RETURN
        (READ_XLMAC_RX_MAX_SIZEr(unit, lport, xlmac_rx_max_size));
    XLMAC_RX_MAX_SIZEr_RX_MAX_SIZEf_SET(xlmac_rx_max_size, size);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_RX_MAX_SIZEr(unit, lport, xlmac_rx_max_size));

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_frame_max_get
 * Description:
 *      Set the maximum receive frame size for the port
 * Parameters:
 *      unit - Device number
 *      port - Port number
 *      size - Maximum frame size in bytes
 * Return Value:
 *      BCM_E_XXX
 */
static int
mac_xl_frame_max_get(int unit, uint8 lport, int *size)
{
    XLMAC_RX_MAX_SIZEr_t xlmac_rx_max_size;

    SOC_IF_ERROR_RETURN
        (READ_XLMAC_RX_MAX_SIZEr(unit, lport, xlmac_rx_max_size));

    *size = XLMAC_RX_MAX_SIZEr_RX_MAX_SIZEf_GET(xlmac_rx_max_size);
    if (IS_CE_PORT(lport) || IS_XE_PORT(lport)) {
        /* For VLAN tagged packets */
        *size -= 4;
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d *size=%d\n",
                 __func__, unit, lport, *size));

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_ifg_set
 * Description:
 *      Sets the new ifg (Inter-frame gap) value
 * Parameters:
 *      unit   - Device number
 *      port   - Port number
 *      speed  - the speed for which the IFG is being set
 *      duplex - the duplex for which the IFG is being set
 *      ifg - number of bits to use for average inter-frame gap
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      The function makes sure the IFG value makes sense and updates the
 *      IPG register in case the speed/duplex match the current settings
 */
static int
mac_xl_ifg_set(int unit, uint8 lport, int speed,
               soc_port_duplex_t duplex, int ifg)
{
    int cur_speed;
    int cur_duplex;
    int real_ifg;
    soc_port_ability_t ability;
    uint32 pa_flag;
    XLMAC_TX_CTRLr_t xlmac_tx_ctrl;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d speed=%dMb duplex=%s ifg=%d\n",
                 __func__, unit, lport, speed, duplex ? "True" : "False", ifg));

    pa_flag = SOC_PA_SPEED(speed);
    soc_mac_xl.md_ability_local_get(unit, lport, &ability);
    if (!(pa_flag & ability.speed_full_duplex)) {
        return SYS_ERR;
    }

    /* Silently adjust the specified ifp bits to valid value */
    /* valid value: 8 to 64 bytes (i.e. multiple of 8 bits) */
    real_ifg = ifg < 64 ? 64 : (ifg > 512 ? 512 : (ifg + 7) & (0x7f << 3));

    SOC_IF_ERROR_RETURN
        (mac_xl_duplex_get(unit, lport, &cur_duplex));
    SOC_IF_ERROR_RETURN
        (mac_xl_speed_get(unit, lport, &cur_speed));

    /* XLMAC_MODE supports only 4 speeds with 4 being max as LINK_10G_PLUS */
    if ((speed > 10000) && (cur_speed == 10000)) {
        cur_speed = speed;
    }

    if (cur_speed == speed &&
        cur_duplex == (duplex == SOC_PORT_DUPLEX_FULL ? TRUE : FALSE)) {
        int ipg = real_ifg / 8;

        SOC_IF_ERROR_RETURN
            (READ_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
        if (XLMAC_TX_CTRLr_AVERAGE_IPGf_GET(xlmac_tx_ctrl) != ipg) {
            XLMAC_TX_CTRLr_AVERAGE_IPGf_SET(xlmac_tx_ctrl, ipg);
            SOC_IF_ERROR_RETURN
                (WRITE_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
        }
    }

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_ifg_get
 * Description:
 *      Sets the new ifg (Inter-frame gap) value
 * Parameters:
 *      unit   - Device number
 *      port   - Port number
 *      speed  - the speed for which the IFG is being set
 *      duplex - the duplex for which the IFG is being set
 *      size - Maximum frame size in bytes
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      The function makes sure the IFG value makes sense and updates the
 *      IPG register in case the speed/duplex match the current settings
 */
static int
mac_xl_ifg_get(int unit, uint8 lport, int speed,
               soc_port_duplex_t duplex, int *ifg)
{
    soc_port_ability_t ability;
    uint32 pa_flag;
    XLMAC_TX_CTRLr_t xlmac_tx_ctrl;

    if (!duplex) {
        return SYS_ERR;
    }

    pa_flag = SOC_PA_SPEED(speed);
    soc_mac_xl.md_ability_local_get(unit, lport, &ability);
    if (!(pa_flag & ability.speed_full_duplex)) {
        return SYS_ERR;
    }

    SOC_IF_ERROR_RETURN
        (READ_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));

    *ifg = XLMAC_TX_CTRLr_AVERAGE_IPGf_GET(xlmac_tx_ctrl);
    *ifg = (*ifg) * (8);

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d speed=%dMb duplex=%s *ifg=%d\n",
                 __func__, unit, lport, speed,
                 duplex ? "True" : "False", *ifg));

    return SYS_OK;
}

/*
 * Function:
 *      _mac_xl_port_mode_update
 * Purpose:
 *      Set the XLMAC port encapsulation mode.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      to_hg_port - (TRUE/FALSE)
 * Returns:
 *      SYS_ERR_XXX
 */
static int
_mac_xl_port_mode_update(int unit, uint8 lport, int hg_mode)
{
    /*
     * Currently not implemanted since mac_xl_encap_set
     * should not be called at runtime.
     */
    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d to_hg_port=%d\n",
                 __func__, unit, lport, hg_mode));

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_encap_set
 * Purpose:
 *      Set the XLMAC port encapsulation mode.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      mode - (IN) encap bits (defined above)
 * Returns:
 *      SYS_ERR_XXX
 */
static int
mac_xl_encap_set(int unit, uint8 lport, int mode)
{
    int enable, encap, runt;
    XLMAC_MODEr_t xlmac_mode;
    XLMAC_RX_CTRLr_t xlmac_rx_ctrl;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d encapsulation=%s\n",
                 __func__, unit, lport, mac_xl_encap_mode[mode]));

    switch (mode) {
    case SOC_ENCAP_IEEE:
    case SOC_ENCAP_HIGIG2_LITE:
        encap = 0;
        break;
    case SOC_ENCAP_HIGIG:
        encap = 1;
        break;
    case SOC_ENCAP_HIGIG2:
        encap = 2;
        break;
    default:
        return SYS_ERR_PARAMETER;
    }

    SOC_IF_ERROR_RETURN(mac_xl_enable_get(unit, lport, &enable));
    if (enable) {
        /* Turn off TX/RX enable */
        SOC_IF_ERROR_RETURN(mac_xl_enable_set(unit, lport, 0));
    }

    /* mode update for all encap mode change! */
    SOC_IF_ERROR_RETURN(_mac_xl_port_mode_update(unit, lport, mode));

    /* Update the encapsulation mode */
    SOC_IF_ERROR_RETURN
        (READ_XLMAC_MODEr(unit, lport, xlmac_mode));
    XLMAC_MODEr_HDR_MODEf_SET(xlmac_mode, encap);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_MODEr(unit, lport, xlmac_mode));

    SOC_IF_ERROR_RETURN
        (READ_XLMAC_RX_CTRLr(unit, lport, xlmac_rx_ctrl));

    /* assigning runt value per encap setting */
    if ((mode == SOC_ENCAP_HIGIG2) || (mode == SOC_ENCAP_HIGIG2_LITE)) {
        runt = XLMAC_RUNT_THRESHOLD_HG2;
    } else if (mode == SOC_ENCAP_HIGIG) {
        runt = XLMAC_RUNT_THRESHOLD_HG1;
    } else {
        runt = XLMAC_RUNT_THRESHOLD_IEEE;
    }
    XLMAC_RX_CTRLr_RUNT_THRESHOLDf_SET(xlmac_rx_ctrl, runt);

    /* assigning strict preamble per encap setting */
    XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(xlmac_rx_ctrl,
                                        mode == SOC_ENCAP_IEEE ? 1 : 0);
    SOC_IF_ERROR_RETURN
        (WRITE_XLMAC_RX_CTRLr(unit, lport, xlmac_rx_ctrl));

    if (enable) {
        /* Re-enable transmitter and receiver */
        SOC_IF_ERROR_RETURN(mac_xl_enable_set(unit, lport, 1));
    }

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_encap_get
 * Purpose:
 *      Get the XLMAC port encapsulation mode.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      mode - (INT) encap bits (defined above)
 * Returns:
 *      SYS_ERR_XXX
 */
static int
mac_xl_encap_get(int unit, uint8 lport, int *mode)
{
    XLMAC_MODEr_t xlmac_mode;

    if (!mode) {
        return SYS_ERR;
    }

    SOC_IF_ERROR_RETURN(READ_XLMAC_MODEr(unit, lport, xlmac_mode));
    switch (XLMAC_MODEr_HDR_MODEf_GET(xlmac_mode)) {
    case 0:
        *mode = SOC_ENCAP_IEEE;
        break;
    case 1:
        *mode = SOC_ENCAP_HIGIG;
        break;
    case 2:
        *mode = SOC_ENCAP_HIGIG2;
        break;
    default:
        *mode = SOC_ENCAP_COUNT;
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d encapsulation=%s\n",
                 __func__, unit, lport, mac_xl_encap_mode[*mode]));

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_expected_rx_latency_get
 * Purpose:
 *      Get the XLMAC port expected Rx latency
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      latency - (OUT) Latency in NS
 * Returns:
 *      SYS_ERR_XXX
 */
static int
mac_xl_expected_rx_latency_get(int unit, uint8 lport, int *latency)
{
    int speed = 0;

    SOC_IF_ERROR_RETURN(mac_xl_speed_get(unit, lport, &speed));

    switch (speed) {
    case 1000:  /* GigE */
        *latency = 510; /* From SDK-69340 */
        break;

    case 10000:  /* 10G */
        *latency = 230; /* From SDK-69340 */
        break;

    default:
        *latency = 0;
        break;
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d *latency=%d\n",
                 __func__, unit, lport, *latency));

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_control_set
 * Purpose:
 *      To configure MAC control properties.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      type - MAC control property to set.
 *      int  - New setting for MAC control.
 * Returns:
 *      SYS_ERR_XXX
 */
static int
mac_xl_control_set(int unit, uint8 lport, soc_mac_control_t type,
                  int value)
{
    uint32 fval0;
#if UM_READBACK_DEBUG
    uint32 fval1;
#endif
    XLMAC_CTRLr_t xlmac_ctrl;
    XLMAC_TX_CTRLr_t xlmac_tx_ctrl;
    XLMAC_RX_CTRLr_t xlmac_rx_ctrl;
    XLMAC_PFC_CTRLr_t xlmac_pfc_ctrl;
    XLMAC_PFC_TYPEr_t xlmac_pfc_type;
    XLMAC_PFC_OPCODEr_t xlmac_pfc_opcode;
    XLMAC_PFC_DAr_t xlmac_pfc_da;
    XLMAC_LLFC_CTRLr_t xlmac_llfc_ctrl;
    XLMAC_EEE_CTRLr_t xlmac_eee_ctrl;
    XLMAC_EEE_TIMERSr_t xlmac_eee_timers;
    XLMAC_RX_LSS_CTRLr_t xlmac_rx_lss_ctrl;
    XLMAC_RX_VLAN_TAGr_t xlmac_rx_vlan_tag;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d type=%d value=%d\n",
                 __func__, unit, lport, type, value));

    switch (type) {
    case SOC_MAC_CONTROL_RX_SET:
        SOC_IF_ERROR_RETURN(READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
        XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, (value ? 1 : 0));
        SOC_IF_ERROR_RETURN(WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN(READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
        value = XLMAC_CTRLr_RX_ENf_GET(xlmac_ctrl);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_CONTROL_RX_SET: value=%d\n",
                     __func__, value));
#endif
        break;

    case SOC_MAC_CONTROL_TX_SET:
        SOC_IF_ERROR_RETURN(READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
        XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, (value ? 1 : 0));
        SOC_IF_ERROR_RETURN(WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN(READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
        value = XLMAC_CTRLr_TX_ENf_GET(xlmac_ctrl);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_CONTROL_TX_SET: value=%d\n",
                     __func__, value));
#endif
        break;

    case SOC_MAC_CONTROL_FRAME_SPACING_STRETCH:
        if (value < 0 || value > 255) {
            return SOC_E_PARAM;
        }

        SOC_IF_ERROR_RETURN(READ_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
        if (value >= 8) {
            XLMAC_TX_CTRLr_THROT_DENOMf_SET(xlmac_tx_ctrl, value);
            XLMAC_TX_CTRLr_THROT_NUMf_SET(xlmac_tx_ctrl, 1);
        } else {
            XLMAC_TX_CTRLr_THROT_DENOMf_SET(xlmac_tx_ctrl, 0);
            XLMAC_TX_CTRLr_THROT_NUMf_SET(xlmac_tx_ctrl, 0);
        }
        SOC_IF_ERROR_RETURN(WRITE_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN(READ_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
        value = XLMAC_TX_CTRLr_THROT_DENOMf_GET(xlmac_tx_ctrl);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_CONTROL_FRAME_SPACING_STRETCH: "
                     "THROT_DENOM value=%d\n",
                     __func__, value));
        value = XLMAC_TX_CTRLr_THROT_NUMf_GET(xlmac_tx_ctrl);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_CONTROL_FRAME_SPACING_STRETCH: "
                     "THROT_NUM value=%d\n",
                     __func__, value));
#endif
        break;

    case SOC_MAC_PASS_CONTROL_FRAME:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_RX_CTRLr(unit, lport, xlmac_rx_ctrl));
        XLMAC_RX_CTRLr_RX_PASS_PAUSEf_SET(xlmac_rx_ctrl, value ? 1 : 0);
        SOC_IF_ERROR_RETURN
            (WRITE_XLMAC_RX_CTRLr(unit, lport, xlmac_rx_ctrl));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_RX_CTRLr(unit, lport, xlmac_rx_ctrl));
        value = XLMAC_RX_CTRLr_RX_PASS_PAUSEf_GET(xlmac_rx_ctrl);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_PASS_CONTROL_FRAME: RX_PAUSE_EN value=%d\n",
                     __func__, value));
#endif
        break;

    case SOC_MAC_CONTROL_PFC_TYPE:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_PFC_TYPEr(unit, lport, xlmac_pfc_type));
        XLMAC_PFC_TYPEr_PFC_ETH_TYPEf_SET(xlmac_pfc_type, value);
        SOC_IF_ERROR_RETURN
            (WRITE_XLMAC_PFC_TYPEr(unit, lport, xlmac_pfc_type));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_TYPEr(unit, lport, xlmac_pfc_type));
        value = XLMAC_PFC_TYPEr_PFC_ETH_TYPEf_GET(xlmac_pfc_type);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_CONTROL_PFC_TYPE: value=%d\n",
                     __func__, value));
#endif
        break;

    case SOC_MAC_CONTROL_PFC_OPCODE:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_PFC_OPCODEr(unit, lport, xlmac_pfc_opcode));
        XLMAC_PFC_OPCODEr_PFC_OPCODEf_SET(xlmac_pfc_opcode, value);
        SOC_IF_ERROR_RETURN
            (WRITE_XLMAC_PFC_OPCODEr(unit, lport, xlmac_pfc_opcode));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_PFC_OPCODEr(unit, lport, xlmac_pfc_opcode));
        value = XLMAC_PFC_OPCODEr_PFC_OPCODEf_GET(xlmac_pfc_opcode);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_CONTROL_PFC_OPCODE: value=%d\n",
                     __func__, value));
#endif
        break;

    case SOC_MAC_CONTROL_PFC_CLASSES:
        if (value != 8) {
            return SYS_ERR_PARAMETER;
        }
        break;

    case SOC_MAC_CONTROL_PFC_MAC_DA_OUI:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_DAr(unit, lport, xlmac_pfc_da));
        fval0 = XLMAC_PFC_DAr_PFC_MACDA_LOf_GET(xlmac_pfc_da);
        fval0 &= 0x00ffffff;
        fval0 |= (value & 0xff) << 24;
        XLMAC_PFC_DAr_PFC_MACDA_LOf_SET(xlmac_pfc_da, fval0);
        XLMAC_PFC_DAr_PFC_MACDA_HIf_SET(xlmac_pfc_da, (value >> 8));
        SOC_IF_ERROR_RETURN(WRITE_XLMAC_PFC_DAr(unit, lport, xlmac_pfc_da));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_DAr(unit, lport, xlmac_pfc_da));
        fval0 = XLMAC_PFC_DAr_PFC_MACDA_LOf_GET(xlmac_pfc_da);
        fval1 = XLMAC_PFC_DAr_PFC_MACDA_HIf_GET(xlmac_pfc_da);
        value = (fval0 >> 24) | (fval1 << 8);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_CONTROL_PFC_MAC_DA_OUI: value=0x%08x\n",
                     __func__, value));
#endif
        break;

    case SOC_MAC_CONTROL_PFC_MAC_DA_NONOUI:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_DAr(unit, lport, xlmac_pfc_da));
        fval0 = XLMAC_PFC_DAr_PFC_MACDA_LOf_GET(xlmac_pfc_da) & 0xff000000;
        fval0 |= value;
        XLMAC_PFC_DAr_PFC_MACDA_LOf_SET(xlmac_pfc_da, fval0);
        SOC_IF_ERROR_RETURN(WRITE_XLMAC_PFC_DAr(unit, lport, xlmac_pfc_da));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_DAr(unit, lport, xlmac_pfc_da));
        value = XLMAC_PFC_DAr_PFC_MACDA_LOf_GET(xlmac_pfc_da) & 0x00ffffff;
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_CONTROL_PFC_MAC_DA_NONOUI: value=0x%08x\n",
                     __func__, value));
#endif
        break;

    case SOC_MAC_CONTROL_PFC_RX_PASS:
        /* this is always true */
        break;

    case SOC_MAC_CONTROL_PFC_RX_ENABLE:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
        XLMAC_PFC_CTRLr_RX_PFC_ENf_SET(xlmac_pfc_ctrl, value ? 1 : 0);
        SOC_IF_ERROR_RETURN(WRITE_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
        value = XLMAC_PFC_CTRLr_RX_PFC_ENf_GET(xlmac_pfc_ctrl);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_CONTROL_PFC_RX_ENABLE: value=0x%08x\n",
                     __func__, value));
#endif
        break;

    case SOC_MAC_CONTROL_PFC_TX_ENABLE:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
        XLMAC_PFC_CTRLr_TX_PFC_ENf_SET(xlmac_pfc_ctrl, value ? 1 : 0);
        SOC_IF_ERROR_RETURN(WRITE_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
        value = XLMAC_PFC_CTRLr_TX_PFC_ENf_GET(xlmac_pfc_ctrl);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_CONTROL_PFC_TX_ENABLE: value=0x%08x\n",
                     __func__, value));
#endif
        break;

    case SOC_MAC_CONTROL_PFC_FORCE_XON:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
        XLMAC_PFC_CTRLr_FORCE_PFC_XONf_SET(xlmac_pfc_ctrl, (value ? 1 : 0));
        SOC_IF_ERROR_RETURN(WRITE_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
        value = XLMAC_PFC_CTRLr_FORCE_PFC_XONf_GET(xlmac_pfc_ctrl);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_CONTROL_PFC_FORCE_XON: value=0x%08x\n",
                     __func__, value));
#endif
        break;

    case SOC_MAC_CONTROL_PFC_STATS_ENABLE:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
        XLMAC_PFC_CTRLr_PFC_STATS_ENf_SET(xlmac_pfc_ctrl, (value ? 1 : 0));
        SOC_IF_ERROR_RETURN(WRITE_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
        value = XLMAC_PFC_CTRLr_PFC_STATS_ENf_GET(xlmac_pfc_ctrl);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_CONTROL_PFC_STATS_ENABLE: value=0x%08x\n",
                     __func__, value));
#endif
        break;

    case SOC_MAC_CONTROL_PFC_REFRESH_TIME:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
        XLMAC_PFC_CTRLr_PFC_REFRESH_TIMERf_SET(xlmac_pfc_ctrl, value);
        SOC_IF_ERROR_RETURN(WRITE_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
        value = XLMAC_PFC_CTRLr_PFC_REFRESH_TIMERf_GET(xlmac_pfc_ctrl);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_CONTROL_PFC_REFRESH_TIME: value=0x%08x\n",
                     __func__, value));
#endif
        break;

    case SOC_MAC_CONTROL_PFC_XOFF_TIME:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
        XLMAC_PFC_CTRLr_PFC_XOFF_TIMERf_SET(xlmac_pfc_ctrl, value);
        SOC_IF_ERROR_RETURN(WRITE_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
        value = XLMAC_PFC_CTRLr_PFC_XOFF_TIMERf_GET(xlmac_pfc_ctrl);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_CONTROL_PFC_XOFF_TIME: value=0x%08x\n",
                     __func__, value));
#endif
        break;

    case SOC_MAC_CONTROL_LLFC_RX_ENABLE:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_LLFC_CTRLr(unit, lport, xlmac_llfc_ctrl));
        XLMAC_LLFC_CTRLr_RX_LLFC_ENf_SET(xlmac_llfc_ctrl, (value ? 1 : 0));
        SOC_IF_ERROR_RETURN(WRITE_XLMAC_LLFC_CTRLr
            (unit, lport, xlmac_llfc_ctrl));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_LLFC_CTRLr(unit, lport, xlmac_llfc_ctrl));
        value = XLMAC_LLFC_CTRLr_RX_LLFC_ENf_GET(xlmac_llfc_ctrl);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_CONTROL_PFC_XOFF_TIME: value=0x%08x\n",
                     __func__, value));
#endif
        break;

    case SOC_MAC_CONTROL_LLFC_TX_ENABLE:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_LLFC_CTRLr(unit, lport, xlmac_llfc_ctrl));
        XLMAC_LLFC_CTRLr_TX_LLFC_ENf_SET(xlmac_llfc_ctrl, (value ? 1 : 0));
        SOC_IF_ERROR_RETURN
            (WRITE_XLMAC_LLFC_CTRLr(unit, lport, xlmac_llfc_ctrl));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_LLFC_CTRLr(unit, lport, xlmac_llfc_ctrl));
        value = XLMAC_LLFC_CTRLr_TX_LLFC_ENf_GET(xlmac_llfc_ctrl);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_CONTROL_LLFC_TX_ENABLE: value=0x%08x\n",
                     __func__, value));
#endif
        break;

    case SOC_MAC_CONTROL_EEE_ENABLE:
        SOC_IF_ERROR_RETURN(READ_XLMAC_EEE_CTRLr(unit, lport, xlmac_eee_ctrl));
        XLMAC_EEE_CTRLr_EEE_ENf_SET(xlmac_eee_ctrl, value);
        SOC_IF_ERROR_RETURN(WRITE_XLMAC_EEE_CTRLr(unit, lport, xlmac_eee_ctrl));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN(READ_XLMAC_EEE_CTRLr(unit, lport, xlmac_eee_ctrl));
        value = XLMAC_EEE_CTRLr_EEE_ENf_GET(xlmac_eee_ctrl);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_CONTROL_LLFC_TX_ENABLE: value=0x%08x\n",
                     __func__, value));
#endif
        break;

    case SOC_MAC_CONTROL_EEE_TX_IDLE_TIME:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_EEE_TIMERSr(unit, lport, xlmac_eee_timers));
        XLMAC_EEE_TIMERSr_EEE_DELAY_ENTRY_TIMERf_SET(xlmac_eee_timers, value);
        SOC_IF_ERROR_RETURN
            (WRITE_XLMAC_EEE_TIMERSr(unit, lport, xlmac_eee_timers));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_EEE_TIMERSr(unit, lport, xlmac_eee_timers));
        value = XLMAC_EEE_TIMERSr_EEE_DELAY_ENTRY_TIMERf_GET(xlmac_eee_timers);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_CONTROL_EEE_TX_IDLE_TIME: value=0x%08x\n",
                     __func__, value));
#endif
        break;

    case SOC_MAC_CONTROL_EEE_TX_WAKE_TIME:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_EEE_TIMERSr(unit, lport, xlmac_eee_timers));
        XLMAC_EEE_TIMERSr_EEE_WAKE_TIMERf_SET(xlmac_eee_timers, value);
        SOC_IF_ERROR_RETURN
            (WRITE_XLMAC_EEE_TIMERSr(unit, lport, xlmac_eee_timers));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_EEE_TIMERSr(unit, lport, xlmac_eee_timers));
        value = XLMAC_EEE_TIMERSr_EEE_WAKE_TIMERf_GET(xlmac_eee_timers);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
            ("%s..:SOC_MAC_CONTROL_EEE_TX_WAKE_TIME: value=0x%08x\n",
             __func__, value));
#endif
        break;

    case SOC_MAC_CONTROL_FAULT_LOCAL_ENABLE:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));
        XLMAC_RX_LSS_CTRLr_LOCAL_FAULT_DISABLEf_SET(xlmac_rx_lss_ctrl,
                                                    value ? 0 : 1);
        SOC_IF_ERROR_RETURN
            (WRITE_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));
        /*
         * If set, MAC will continue to transmit data irrespective of
         * LOCAL_FAULT_STATUS.
         */
        value = XLMAC_RX_LSS_CTRLr_LOCAL_FAULT_DISABLEf_GET(xlmac_rx_lss_ctrl) ? 0 : 1;
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_CONTROL_FAULT_LOCAL_ENABLE: value=0x%08x\n",
                     __func__, value));
#endif
        break;

    case SOC_MAC_CONTROL_FAULT_REMOTE_ENABLE:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));
        XLMAC_RX_LSS_CTRLr_REMOTE_FAULT_DISABLEf_SET(xlmac_rx_lss_ctrl,
                                                     value ? 0 : 1);
        SOC_IF_ERROR_RETURN
            (WRITE_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));

#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));
        /*
         * If set, MAC will continue to transmit data irrespective of
         * REMOTE_FAULT_STATUS.
         */
        value = XLMAC_RX_LSS_CTRLr_REMOTE_FAULT_DISABLEf_GET(xlmac_rx_lss_ctrl) ? 0 : 1;
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:SOC_MAC_CONTROL_FAULT_LOCAL_ENABLE: value=0x%08x\n",
                     __func__, value));
#endif
        break;

    case SOC_MAC_CONTROL_FAILOVER_RX_SET:
        break;

    case SOC_MAC_CONTROL_EGRESS_DRAIN:
        SOC_IF_ERROR_RETURN(mac_xl_egress_queue_drain(unit, lport));
        break;

    case SOC_MAC_CONTROL_RX_VLAN_TAG_OUTER_TPID:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_RX_VLAN_TAGr(unit, lport, xlmac_rx_vlan_tag));
        XLMAC_RX_VLAN_TAGr_OUTER_VLAN_TAGf_SET(xlmac_rx_vlan_tag, value);
        XLMAC_RX_VLAN_TAGr_OUTER_VLAN_TAG_ENABLEf_SET(xlmac_rx_vlan_tag,
                                                      value ? 1 : 0);
        SOC_IF_ERROR_RETURN
            (WRITE_XLMAC_RX_VLAN_TAGr(unit, lport, xlmac_rx_vlan_tag));
        break;

    case SOC_MAC_CONTROL_RX_VLAN_TAG_INNER_TPID:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_RX_VLAN_TAGr(unit, lport, xlmac_rx_vlan_tag));
        XLMAC_RX_VLAN_TAGr_INNER_VLAN_TAGf_SET(xlmac_rx_vlan_tag, value);
        XLMAC_RX_VLAN_TAGr_INNER_VLAN_TAG_ENABLEf_SET(xlmac_rx_vlan_tag,
                                                      value ? 1 : 0);
        SOC_IF_ERROR_RETURN
            (WRITE_XLMAC_RX_VLAN_TAGr(unit, lport, xlmac_rx_vlan_tag));
        break;

    default:
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    ("%s..:no such type: type=%d\n",
                     __func__, type));
        return SYS_ERR_UNAVAIL;
    }

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_control_get
 * Purpose:
 *      To get current MAC control setting.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      type - MAC control property to set.
 *      int  - New setting for MAC control.
 * Returns:
 *      SYS_ERR_XXX
 */
static int
mac_xl_control_get(int unit, uint8 lport, soc_mac_control_t type,
                  int *value)
{
    uint32 fval0, fval1;
    XLMAC_CTRLr_t xlmac_ctrl;
    XLMAC_RX_CTRLr_t xlmac_rx_ctrl;
    XLMAC_TX_CTRLr_t xlmac_tx_ctrl;
    XLMAC_TX_TIMESTAMP_FIFO_STATUSr_t xlmac_tx_ts_fifo_st;
    XLMAC_TX_TIMESTAMP_FIFO_DATAr_t xlmac_tx_ts_fifo_data;
    XLMAC_PFC_CTRLr_t xlmac_pfc_ctrl;
    XLMAC_PFC_TYPEr_t xlmac_pfc_type;
    XLMAC_PFC_OPCODEr_t xlmac_pfc_opcode;
    XLMAC_PFC_DAr_t xlmac_pfc_da;
    XLMAC_LLFC_CTRLr_t xlmac_llfc_ctrl;
    XLMAC_EEE_CTRLr_t xlmac_eee_ctrl;
    XLMAC_EEE_TIMERSr_t xlmac_eee_timers;
    XLMAC_RX_LSS_CTRLr_t xlmac_rx_lss_ctrl;
    XLMAC_RX_LSS_STATUSr_t xlmac_rx_lss_st;
    XLMAC_RX_VLAN_TAGr_t xlmac_rx_vlan_tag;

    if (value == NULL) {
        return SYS_ERR_PARAMETER;
    }

    switch (type) {
    case SOC_MAC_CONTROL_RX_SET:
        SOC_IF_ERROR_RETURN(READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
        *value = XLMAC_CTRLr_RX_ENf_GET(xlmac_ctrl);
        break;

    case SOC_MAC_CONTROL_TX_SET:
        SOC_IF_ERROR_RETURN(READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
        *value = XLMAC_CTRLr_TX_ENf_GET(xlmac_ctrl);
        break;

    case SOC_MAC_CONTROL_FRAME_SPACING_STRETCH:
        SOC_IF_ERROR_RETURN(READ_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
        *value = XLMAC_TX_CTRLr_THROT_DENOMf_GET(xlmac_tx_ctrl);
        break;

    case SOC_MAC_CONTROL_TIMESTAMP_TRANSMIT:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_TX_TIMESTAMP_FIFO_STATUSr(unit, lport,
                                                  xlmac_tx_ts_fifo_st));
        if (XLMAC_TX_TIMESTAMP_FIFO_STATUSr_ENTRY_COUNTf_GET(xlmac_tx_ts_fifo_st) == 0) {
            return SYS_ERR;
        }

        SOC_IF_ERROR_RETURN
            (READ_XLMAC_TX_TIMESTAMP_FIFO_DATAr(unit, lport,
                                                xlmac_tx_ts_fifo_data));
        *value = XLMAC_TX_TIMESTAMP_FIFO_DATAr_TIME_STAMPf_GET(xlmac_tx_ts_fifo_data);
        break;

    case SOC_MAC_PASS_CONTROL_FRAME:
        SOC_IF_ERROR_RETURN(READ_XLMAC_RX_CTRLr(unit, lport, xlmac_rx_ctrl));
        *value = XLMAC_RX_CTRLr_RX_PASS_PAUSEf_GET(xlmac_rx_ctrl);
        break;

    case SOC_MAC_CONTROL_PFC_TYPE:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_TYPEr(unit, lport, xlmac_pfc_type));
        *value = XLMAC_PFC_TYPEr_PFC_ETH_TYPEf_GET(xlmac_pfc_type);
        break;

    case SOC_MAC_CONTROL_PFC_OPCODE:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_PFC_OPCODEr(unit, lport, xlmac_pfc_opcode));
        *value = XLMAC_PFC_OPCODEr_PFC_OPCODEf_GET(xlmac_pfc_opcode);
        break;

    case SOC_MAC_CONTROL_PFC_CLASSES:
        *value = 8;
        break;

    case SOC_MAC_CONTROL_PFC_MAC_DA_OUI:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_DAr(unit, lport, xlmac_pfc_da));
        fval0 = XLMAC_PFC_DAr_PFC_MACDA_LOf_GET(xlmac_pfc_da);
        fval1 = XLMAC_PFC_DAr_PFC_MACDA_HIf_GET(xlmac_pfc_da);
        *value = (fval0 >> 24) | (fval1 << 8);
        break;

    case SOC_MAC_CONTROL_PFC_MAC_DA_NONOUI:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_DAr(unit, lport, xlmac_pfc_da));
        *value = XLMAC_PFC_DAr_PFC_MACDA_LOf_GET(xlmac_pfc_da) & 0x00ffffff;
        break;

    case SOC_MAC_CONTROL_PFC_RX_PASS:
        *value = TRUE;
        break;

    case SOC_MAC_CONTROL_PFC_RX_ENABLE:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
        *value = XLMAC_PFC_CTRLr_RX_PFC_ENf_GET(xlmac_pfc_ctrl);
        break;

    case SOC_MAC_CONTROL_PFC_TX_ENABLE:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
        *value = XLMAC_PFC_CTRLr_TX_PFC_ENf_GET(xlmac_pfc_ctrl);
        break;

    case SOC_MAC_CONTROL_PFC_FORCE_XON:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
        *value = XLMAC_PFC_CTRLr_FORCE_PFC_XONf_GET(xlmac_pfc_ctrl);
        break;

    case SOC_MAC_CONTROL_PFC_STATS_ENABLE:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
        *value = XLMAC_PFC_CTRLr_PFC_STATS_ENf_GET(xlmac_pfc_ctrl);
        break;

    case SOC_MAC_CONTROL_PFC_REFRESH_TIME:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
        *value = XLMAC_PFC_CTRLr_PFC_REFRESH_TIMERf_GET(xlmac_pfc_ctrl);
        break;

    case SOC_MAC_CONTROL_PFC_XOFF_TIME:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
        *value = XLMAC_PFC_CTRLr_PFC_XOFF_TIMERf_GET(xlmac_pfc_ctrl);
        break;

    case SOC_MAC_CONTROL_LLFC_RX_ENABLE:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_LLFC_CTRLr(unit, lport, xlmac_llfc_ctrl));
        *value = XLMAC_LLFC_CTRLr_RX_LLFC_ENf_GET(xlmac_llfc_ctrl);
        break;

    case SOC_MAC_CONTROL_LLFC_TX_ENABLE:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_LLFC_CTRLr(unit, lport, xlmac_llfc_ctrl));
        *value = XLMAC_LLFC_CTRLr_TX_LLFC_ENf_GET(xlmac_llfc_ctrl);
        break;

    case SOC_MAC_CONTROL_EEE_ENABLE:
        SOC_IF_ERROR_RETURN(READ_XLMAC_EEE_CTRLr(unit, lport, xlmac_eee_ctrl));
        *value = XLMAC_EEE_CTRLr_EEE_ENf_GET(xlmac_eee_ctrl);
        break;

    case SOC_MAC_CONTROL_EEE_TX_IDLE_TIME:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_EEE_TIMERSr(unit, lport, xlmac_eee_timers));
        *value = XLMAC_EEE_TIMERSr_EEE_DELAY_ENTRY_TIMERf_GET(xlmac_eee_timers);
        break;

    case SOC_MAC_CONTROL_EEE_TX_WAKE_TIME:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_EEE_TIMERSr(unit, lport, xlmac_eee_timers));
        *value = XLMAC_EEE_TIMERSr_EEE_WAKE_TIMERf_GET(xlmac_eee_timers);
        break;

    case SOC_MAC_CONTROL_FAULT_LOCAL_ENABLE:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));
        /*
         * If set, MAC will continue to transmit data irrespective of
         * LOCAL_FAULT_STATUS.
         */
        *value = XLMAC_RX_LSS_CTRLr_LOCAL_FAULT_DISABLEf_GET(xlmac_rx_lss_ctrl) ? 0 : 1;
        break;

    case SOC_MAC_CONTROL_FAULT_LOCAL_STATUS:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));
        if (XLMAC_RX_LSS_CTRLr_LOCAL_FAULT_DISABLEf_GET(xlmac_rx_lss_ctrl)) {
            *value = 0;
            break;
        }
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_RX_LSS_STATUSr(unit, lport, xlmac_rx_lss_st));
        *value = XLMAC_RX_LSS_STATUSr_LOCAL_FAULT_STATUSf_GET(xlmac_rx_lss_st);
        break;

    case SOC_MAC_CONTROL_FAULT_REMOTE_ENABLE:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));
        /*
         * If set, MAC will continue to transmit data irrespective of
         * REMOTE_FAULT_STATUS.
         */
        *value = XLMAC_RX_LSS_CTRLr_REMOTE_FAULT_DISABLEf_GET(xlmac_rx_lss_ctrl) ? 0 : 1;
        break;

    case SOC_MAC_CONTROL_FAULT_REMOTE_STATUS:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));
        if (XLMAC_RX_LSS_CTRLr_REMOTE_FAULT_DISABLEf_GET(xlmac_rx_lss_ctrl)) {
            *value = 0;
            break;
        }
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_RX_LSS_STATUSr(unit, lport, xlmac_rx_lss_st));
        *value = XLMAC_RX_LSS_STATUSr_REMOTE_FAULT_STATUSf_GET(xlmac_rx_lss_st);
        break;

    case SOC_MAC_CONTROL_RX_VLAN_TAG_OUTER_TPID:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_RX_VLAN_TAGr(unit, lport, xlmac_rx_vlan_tag));
        *value = XLMAC_RX_VLAN_TAGr_OUTER_VLAN_TAGf_GET(xlmac_rx_vlan_tag);
        break;

    case SOC_MAC_CONTROL_RX_VLAN_TAG_INNER_TPID:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_RX_VLAN_TAGr(unit, lport, xlmac_rx_vlan_tag));
        *value = XLMAC_RX_VLAN_TAGr_INNER_VLAN_TAGf_GET(xlmac_rx_vlan_tag);
        break;
    case SOC_MAC_CONTROL_EXPECTED_RX_LATENCY:
        SOC_IF_ERROR_RETURN(mac_xl_expected_rx_latency_get(unit, lport, value));
        break;
    case SOC_MAC_CONTROL_RX_RUNT_THRESHOLD:
        SOC_IF_ERROR_RETURN(READ_XLMAC_RX_CTRLr(unit, lport, xlmac_rx_ctrl));
        *value = XLMAC_RX_CTRLr_RUNT_THRESHOLDf_GET(xlmac_rx_ctrl);
        break;
    default:
        return SYS_ERR_UNAVAIL;
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit %d lport %d type=%d *value=%d\n",
                 __func__, unit, lport, type, *value));

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_ability_local_get
 * Purpose:
 *      Return the abilities of XLMAC
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      mode - (OUT) Supported operating modes as a mask of abilities.
 * Returns:
 *      SYS_ERR_XXX
 */
static int
mac_xl_ability_local_get(int unit, uint8 lport,
                         soc_port_ability_t *ability)
{
    int bindex, port_speed_max, i;
    int phy_port, active_port;
    uint32 active_mask;

    if (NULL == ability) {
        return SYS_ERR_PARAMETER;
    }

    ability->speed_half_duplex  = SOC_PA_ABILITY_NONE;
    ability->pause     = SOC_PA_PAUSE | SOC_PA_PAUSE_ASYMM;
    ability->interface = SOC_PA_INTF_MII | SOC_PA_INTF_XGMII;
    ability->medium    = SOC_PA_ABILITY_NONE;
    ability->loopback  = SOC_PA_LB_MAC;
    ability->flags     = SOC_PA_ABILITY_NONE;
    ability->encap = SOC_PA_ENCAP_IEEE | SOC_PA_ENCAP_HIGIG |
                     SOC_PA_ENCAP_HIGIG2;

    /* Adjust port_speed_max according to the port config */
    port_speed_max = SOC_PORT_SPEED_MAX(lport);
    phy_port = SOC_PORT_L2P_MAPPING(lport);
    bindex = -1;
    if (IS_XL_PORT(lport)){
        bindex = bcm5607x_xlport_pport_to_index_in_block[phy_port];
    }

    if (port_speed_max > 10000) {
        active_mask = 0;
        for (i = bindex + 1; i <= 3; i++) {
            active_port = SOC_PORT_P2L_MAPPING(phy_port - bindex + i);
            /* (active_port != -1 means it's not disabled.) */
            if (active_port != -1  /* &&
                !SOC_PBMP_MEMBER(SOC_PORT_DISABLED_BITMAP(unit, all),
                                 active_port) */) {
                active_mask |= 1 << i;
            }
        }
        if (bindex == 0) { /* Lanes 0 */
            if (active_mask & 0x2) { /* lane 1 is in use */
                port_speed_max = 10000;
            } else if (port_speed_max > 20000 && active_mask & 0xc) {
                /* Lane 1 isn't in use, lane 2 or 3 (or both) is (are) in use */
                port_speed_max = 20000;
            }
        } else { /* (Must be) lanes 2 */
            if (active_mask & 0x8) { /* lane 3 is in use */
                port_speed_max = 10000;
            }
        }
    }

    /* Use current number of lanes per port to determine the supported speeds */
    if (IS_HL_PORT(lport)) {
        LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:error : IS_HL_PORT.\n", __func__));
    } else if (IS_HG_PORT(lport)) {
        LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:error : IS_HG_PORT.\n", __func__));
    } else {
        if (port_speed_max >= 40000) {
            ability->speed_full_duplex |= SOC_PA_SPEED_40GB;
        }
        if (port_speed_max >= 20000) {
            ability->speed_full_duplex |= SOC_PA_SPEED_20GB;
        }
        if (port_speed_max >= 10000) {
            ability->speed_full_duplex |= SOC_PA_SPEED_10GB;
        }
        if (port_speed_max >= 5000) {
            ability->speed_full_duplex |= SOC_PA_SPEED_5000MB;
            /* For 5G speed, MAC will actually be set to 10G */
        }
        /* Temp fix running regression, saber2 check
         * In Saber2, the xlmac is used for MXQ ports as well.
         */
        //if (soc_feature(unit, soc_feature_unified_port) || (SOC_IS_SABER2(unit)))
        {
            if (port_speed_max >= 2500) {
                ability->speed_full_duplex |= SOC_PA_SPEED_2500MB;
            }
            if (port_speed_max >= 1000) {
                ability->speed_full_duplex |= SOC_PA_SPEED_1000MB;
            }
            if (port_speed_max >= 100) {
                ability->speed_full_duplex |= SOC_PA_SPEED_100MB;
            }
            if (port_speed_max >= 10) {
                ability->speed_full_duplex |= SOC_PA_SPEED_10MB;
            }
        }
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
            ("%s..:mac_xl_ability_local_get: unit %d lport=%d "
             "speed_half=0x%x speed_full=0x%x encap=0x%x pause=0x%x "
             "interface=0x%x medium=0x%x loopback=0x%x flags=0x%x\n",
             __func__, unit, lport,
             ability->speed_half_duplex, ability->speed_full_duplex,
             ability->encap, ability->pause, ability->interface,
             ability->medium, ability->loopback, ability->flags));

    return SYS_OK;
}

static int
mac_xl_timesync_tx_info_get(int unit, uint8 lport,
                            soc_port_timesync_tx_info_t *tx_info)
{
    uint32 val;
    XLMAC_TX_TIMESTAMP_FIFO_STATUSr_t xlmac_tx_ts_fifo_st;
    XLMAC_TX_TIMESTAMP_FIFO_DATAr_t xlmac_tx_ts_fifo_data;

    SOC_IF_ERROR_RETURN
        (READ_XLMAC_TX_TIMESTAMP_FIFO_STATUSr(unit, lport,
                                              xlmac_tx_ts_fifo_st));
    val = XLMAC_TX_TIMESTAMP_FIFO_STATUSr_ENTRY_COUNTf_GET(xlmac_tx_ts_fifo_st);
    if (val == 0) {
        return SYS_ERR;
    }

    SOC_IF_ERROR_RETURN
        (READ_XLMAC_TX_TIMESTAMP_FIFO_DATAr(unit, lport,
                                            xlmac_tx_ts_fifo_data));
    tx_info->timestamps_in_fifo_hi = 0;
    tx_info->timestamps_in_fifo =
        XLMAC_TX_TIMESTAMP_FIFO_DATAr_TIME_STAMPf_GET(xlmac_tx_ts_fifo_data);
    tx_info->sequence_id =
        XLMAC_TX_TIMESTAMP_FIFO_DATAr_SEQUENCE_IDf_GET(xlmac_tx_ts_fifo_data);

    return SYS_OK;
}

/* Exported XLMAC driver structure */
mac_driver_t soc_mac_xl = {
    .drv_name = "XLMAC Driver",
    .md_init = mac_xl_init,
    .md_enable_set = mac_xl_enable_set,
    .md_enable_get = mac_xl_enable_get,
    .md_duplex_set = mac_xl_duplex_set,
    .md_duplex_get = mac_xl_duplex_get,
    .md_speed_set = mac_xl_speed_set,
    .md_speed_get = mac_xl_speed_get,
    .md_pause_set = mac_xl_pause_set,
    .md_pause_get = mac_xl_pause_get,
    .md_pause_addr_set = mac_xl_pause_addr_set,
    .md_pause_addr_get = mac_xl_pause_addr_get,
    .md_lb_set = mac_xl_loopback_set,
    .md_lb_get = mac_xl_loopback_get,
    .md_interface_set = mac_xl_interface_set,
    .md_interface_get = mac_xl_interface_get,
    .md_ability_get = NULL,
    .md_frame_max_set = mac_xl_frame_max_set,
    .md_frame_max_get = mac_xl_frame_max_get,
    .md_ifg_set = mac_xl_ifg_set,
    .md_ifg_get = mac_xl_ifg_get,
    .md_encap_set = mac_xl_encap_set,
    .md_encap_get = mac_xl_encap_get,
    .md_control_set = mac_xl_control_set,
    .md_control_get = mac_xl_control_get,
    .md_ability_local_get = mac_xl_ability_local_get,
    .md_timesync_tx_info_get = mac_xl_timesync_tx_info_get
 };
