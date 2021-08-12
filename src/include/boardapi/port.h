/*! \file port.h
 *
 * Port board APIs.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_PORT_H_
#define _BOARDAPI_PORT_H_

/*!
 * \brief Get link speed of a port.
 *
 * \param [in] uport User port number.
 * \param [out] speed Port speed in Mbsp.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_get_port_link_speed(uint16 uport, int16 *speed);

/*!
 * \brief Get the status of auto neg.
 *
 * \param [in] uport User port number.
 * \param [out] an Autoneg enabled.
 * \param [out] an_done Autoneg done.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_get_port_phy_autoneg(uint16 uport, BOOL *an, BOOL *an_done);

/*!
 * \brief Get the remote EEE capability.
 *
 * \param [in] uport User port number.
 * \param [out] types EEE abilities.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t board_get_port_phy_eee_ability_remote(uint16 uport, uint8 *types);

/*!
 * \brief Get the minimum PHY EEE transition wake time.
 *
 * \param [in] uport User port number.
 * \param [in] type EEE type.
 * \param [out] wake_time Wake time.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_get_port_phy_eee_min_wake_time(uint16 uport, uint8 type,
                                     uint16 *wake_time);

/*!
 * \brief Set the EEE tx wake timer.
 *
 * \param [in] uport User port number.
 * \param [in] type EEE type.
 * \param [in] wake_time Wake time.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_set_port_phy_eee_tx_wake_time(uint16 uport, uint8 type, uint16 wake_time);

/*!
 * Port status.
 */
typedef enum port_mode_s {
    /*! Link down. */
    PM_LINKDOWN = 0,

    /*! 10 Mbps half duplex. */
    PM_10MB_HD = 10,

    /*! 10 Mbps full duplex. */
    PM_10MB_FD = 11,

    /*! 100 Mbps half duplex. */
    PM_100MB_HD = 100,

    /*! 100 Mbps full duplex. */
    PM_100MB_FD = 101,

    /*! 1000 Mbps. */
    PM_1000MB = 1000,

    /*! 2500 Mbps. */
    PM_2500MB = 2500,

    /*! 5000 Mbps. */
    PM_5000MB = 5000,

    /*! 10 Gbps. */
    PM_10000MB = 10000,

    /*! 25 Gbps. */
    PM_25000MB = 25000,

    /*! 40 Gbps. */
    PM_40000MB = 40000,

    /*! 50 Gbps. */
    PM_50000MB = 50000,

    /*! 100 Gbps. */
    PM_100000MB = 100000,

    /*! Autoneg. */
    PM_AUTO = -1,
} port_mode_t;

/*!
 * \brief Get the port mode for the specific user port.
 *
 * \param [in] uport User port number.
 * \param [out] mode Port mode.
 *    \li PM_LINKDOWN
 *    \li PM_10MB_HD
 *    \li PM_10MB_FD
 *    \li ...
 *    \li PM_50000MB
 *    \li PM_100000MB
 *    \li PM_AUTO
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_mode_get(uint16 uport, port_mode_t *mode);

/*!
 * Cable diag state.
 */
typedef enum {
    /*! No cable connection or unknown. */
    PORT_CABLE_STATE_NO_CABLE,

    /*! OK. */
    PORT_CABLE_STATE_OK,

    /*! Open.*/
    PORT_CABLE_STATE_OPEN,

    /*! Short. */
    PORT_CABLE_STATE_SHORT,

    /*! Open/Short. */
    PORT_CABLE_STATE_OPENSHORT,

    /*! Crosstalk. */
    PORT_CABLE_STATE_CROSSTALK

} port_cable_state_t;

/*!
 * Cable diag status structure.
 */
typedef struct port_cable_diag_s {
    /*! State. */
    port_cable_state_t state;

    /*! Length in meters. */
    int32 length;
} port_cable_diag_t;

/*!
 * \brief Run cable diagnostics on the port.
 *
 * \param [in] uport User port number.
 * \param [out] status Cable diag status.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_cable_diag(uint16 uport, port_cable_diag_t *status);

/*!
 * \brief Check cable diagnostics is supported by the port.
 *
 * \param [in] uport User port number.
 * \param [out] support Cable diag is supported.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_get_cable_diag_support_by_port(uint16 uport, BOOL *support);

/*!
 * \brief Get the number of ports which support cable diagnostics.
 *
 * \return The number of ports which support cable diag.
 */
extern uint16
board_cable_diag_port_count(void);

/*!
 * MIB information
 */
typedef struct port_stat_s {
    /*! Rx byte count low. */
    uint32 RxOctets_lo;

    /*! Rx byte count high. */
    uint32 RxOctets_hi;

    /*! Tx byte count low */
    uint32 TxOctets_lo;

    /*! Tx byte count high */
    uint32 TxOctets_hi;

    /*! Rx packet count low. */
    uint32 RxPkts_lo;

    /*! Rx packet count high. */
    uint32 RxPkts_hi;

    /*! Tx packet count low */
    uint32 TxPkts_lo;

    /*! Tx packet count high. */
    uint32 TxPkts_hi;

    /*! Rx FCS Error Frame Counter low. */
    uint32 CRCErrors_lo;

    /*! Rx FCS Error Frame Counter high. */
    uint32 CRCErrors_hi;

    /*! Rx Unicast Frame Counter low. */
    uint32 RxUnicastPkts_lo;

    /*! Rx Unicast Frame Counter high. */
    uint32 RxUnicastPkts_hi;

    /*! Tx Unicast Frame Counter low. */
    uint32 TxUnicastPkts_lo;

    /*! Tx Unicast Frame Counter high. */
    uint32 TxUnicastPkts_hi;

    /*! Rx Multicast Frame Counter low. */
    uint32 RxMulticastPkts_lo;

    /*! Rx Multicast Frame Counter high. */
    uint32 RxMulticastPkts_hi;

    /*! Tx Multicast Frame Counter low. */
    uint32 TxMulticastPkts_lo;

    /*! Tx Multicast Frame Counter high. */
    uint32 TxMulticastPkts_hi;

    /*! Rx Broadcast Frame Counter low. */
    uint32 RxBroadcastPkts_lo;

    /*! Rx Broadcast Frame Counter high. */
    uint32 RxBroadcastPkts_hi;

    /*! Tx Broadcast Frame Counter low. */
    uint32 TxBroadcastPkts_lo;

    /*! Tx Broadcast Frame Counter high. */
    uint32 TxBroadcastPkts_hi;

    /*! Rx Pause Frame Counter low. */
    uint32 RxPauseFramePkts_lo;

    /*! Rx Pause Frame Counter high. */
    uint32 RxPauseFramePkts_hi;

    /*! Tx Pause Frame Counter low. */
    uint32 TxPauseFramePkts_lo;

    /*! Tx Pause Frame Counter high. */
    uint32 TxPauseFramePkts_hi;

    /*! Rx Oversized Frame Counter low. */
    uint32 RxOversizePkts_lo;

    /*! Rx Oversized Frame Counter high. */
    uint32 RxOversizePkts_hi;

    /*! Tx Oversized Frame Counter low. */
    uint32 TxOversizePkts_lo;

    /*! Tx Oversized Frame Counter high. */
    uint32 TxOversizePkts_hi;

    /*! IPIPE HOLD Drop Counter low. */
    uint32 EgressDropPkts_lo;

    /*! IPIPE HOLD Drop Counter high. */
    uint32 EgressDropPkts_hi;

    /*! Rx EEE LPI counter high. */
    uint32 RxLPIPkts_hi;

    /*! Rx EEE LPI duration high. */
    uint32 RxLPIDuration_hi;

    /*! Tx EEE LPI counter high. */
    uint32 TxLPIPkts_hi;

    /*! Tx EEE LPI duration high. */
    uint32 TxLPIDuration_hi;

    /*! Rx EEE LPI counter low. */
    uint32 RxLPIPkts_lo;

    /*! Rx EEE LPI duration low. */
    uint32 RxLPIDuration_lo;

    /*! Tx EEE LPI counter low. */
    uint32 TxLPIPkts_lo;

    /*! Tx EEE LPI duration low. */
    uint32 TxLPIDuration_lo;
} port_stat_t;

/*!
 * \brief Get the statistic value for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] stat Statistics value.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_stat_get(uint16 uport, port_stat_t *stat);

/*!
 * \brief Clear the statistic value for a given port.
 *
 * \param [in] uport Port number.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_stat_clear(uint16 uport);

/*!
 * \brief Clear the statistic value for all the ports.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_stat_clear_all(void);

/*!
 * \brief Port re-init.
 *
 * This function is used to reinit for a given port.
 *
 * \param [in] uport Port number.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_reinit(uint16 uport);

/*!
 * \brief Setting the port speed
 *
 * This function is used to set the speed for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] speed Speed alue in megabits/sec (10, 100, etc)
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_speed_set(uint16 uport, int speed);

/*!
 * \brief Getting the port speed
 *
 * This function is used to get the speed for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] speed Speed alue in megabits/sec (10, 100, etc)
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_speed_get(uint16 uport, int *speed);

/*!
 * \brief Defines interface type between MAC and PHY.
 */
typedef int port_if_t;

/*! No physical connection. */
#define PORT_IF_NOCXN       SOC_PORT_IF_NOCXN
/*! Pass-through connection without PHY. */
#define PORT_IF_NULL        SOC_PORT_IF_NULL
/*! Media-Independent Interface. */
#define PORT_IF_MII         SOC_PORT_IF_MII
/*! Gigabit MII. */
#define PORT_IF_GMII        SOC_PORT_IF_GMII
/*! Serial Gigabit MII. */
#define PORT_IF_SGMII       SOC_PORT_IF_SGMII
/*! Ten Bit Interface. */
#define PORT_IF_TBI         SOC_PORT_IF_TBI
/*! 10-Gigabit MII. */
#define PORT_IF_XGMII       SOC_PORT_IF_XGMII
/*! Reduced Gigabit MII. */
#define PORT_IF_RGMII       SOC_PORT_IF_RGMII
/*! Serdes Framer Interface. */
#define PORT_IF_SFI         SOC_PORT_IF_SFI
/*! XFP Interface. */
#define PORT_IF_XFI         SOC_PORT_IF_XFI
/*! Backplane Interface with 64B/66B PCS. */
#define PORT_IF_KR          SOC_PORT_IF_KR
/*! Backplane 2x10GbE 64B/66B interface */
#define PORT_IF_KR2         SOC_PORT_IF_KR2
/*! Backplane 4x10 GbE 64B/66B interface */
#define PORT_IF_KR4         SOC_PORT_IF_KR4
/*! Copper 10 GbE 64B/66B interface */
#define PORT_IF_CR          SOC_PORT_IF_CR
/*! Copper 2x10GbE 64B/66B interface */
#define PORT_IF_CR2         SOC_PORT_IF_CR2
/*! Copper 4x10 GbE 64B/66B interface. */
#define PORT_IF_CR4         SOC_PORT_IF_CR4
/*! 40 Gigabit Attachment Unit Interface */
#define PORT_IF_XLAUI       SOC_PORT_IF_XLAUI
/*! 40 Gigabit Attachment Unit Interface over 2 lanes */
#define PORT_IF_XLAUI2      SOC_PORT_IF_XLAUI2
/*! Reduced XAUI. */
#define PORT_IF_RXAUI       SOC_PORT_IF_RXAUI
/*! 10-gigabit Attachment Unit Interface. */
#define PORT_IF_XAUI        SOC_PORT_IF_XAUI
/*! SPAUI interface */
#define PORT_IF_SPAUI       SOC_PORT_IF_SPAUI
/*! Qual Serial Gigabit MII. */
#define PORT_IF_QSGMII      SOC_PORT_IF_QSGMII
/*! ILKN interface */
#define PORT_IF_ILKN        SOC_PORT_IF_ILKN
/*! RCY interface */
#define PORT_IF_RCY         SOC_PORT_IF_RCY
/*! PIPE interface */
#define PORT_IF_FAT_PIPE    SOC_PORT_IF_FAT_PIPE
/*! Fiber SR/LR 64B/66B interface */
#define PORT_IF_SR          SOC_PORT_IF_SR
/*! Fiber 2x10GbE 64B/66B interface. */
#define PORT_IF_SR2         SOC_PORT_IF_SR2
/*! CAUI interface */
#define PORT_IF_CAUI        SOC_PORT_IF_CAUI
/*! Fiber LR 64B/66B interface */
#define PORT_IF_LR          SOC_PORT_IF_LR
/*! Fiber LR4 64B/66B interface */
#define PORT_IF_LR4         SOC_PORT_IF_LR4
/*! Fiber SR4 64B/66B interface */
#define PORT_IF_SR4         SOC_PORT_IF_SR4
/*! Backplane Interface with 8B/10B PCS. */
#define PORT_IF_KX          SOC_PORT_IF_KX
/*! Fiber ZR 64B/66B interface */
#define PORT_IF_ZR          SOC_PORT_IF_ZR
/*! Fiber SR10 64B/66B interface. */
#define PORT_IF_SR10        SOC_PORT_IF_SR10
/*! Copper CR10 64B/66B interface. */
#define PORT_IF_CR10        SOC_PORT_IF_CR10
/*! Backplane KR10 64B/66B interface. */
#define PORT_IF_KR10        SOC_PORT_IF_KR10
/*! Fiber LR10 64B/66B interface. */
#define PORT_IF_LR10        SOC_PORT_IF_LR10
/*! Fiber 4x25 GbE OTL interface */
#define PORT_IF_OTL         SOC_PORT_IF_OTL
/*! CPU interface */
#define PORT_IF_CPU         SOC_PORT_IF_CPU
/*! Fiber ER 64B/66B interface */
#define PORT_IF_ER          SOC_PORT_IF_ER
/*! Fiber ER2 2x10G 64B/66B interface */
#define PORT_IF_ER2         SOC_PORT_IF_ER2
/*! Fiber ER4 4x10G 64B/66B interface */
#define PORT_IF_ER4         SOC_PORT_IF_ER4
/*! Copper 10G 10B/8B interface */
#define PORT_IF_CX          SOC_PORT_IF_CX
/*! Copper 2x10G 10B/8B interface. */
#define PORT_IF_CX2         SOC_PORT_IF_CX2
/*! Copper 4x10G 10B/8B interface */
#define PORT_IF_CX4         SOC_PORT_IF_CX4
/*! CAUI 100G C2C interface */
#define PORT_IF_CAUI_C2C    SOC_PORT_IF_CAUI_C2C
/*! CAUI 100G C2M interface */
#define PORT_IF_CAUI_C2M    SOC_PORT_IF_CAUI_C2M
/*! Fiber VSR 10G 64B/66B interface */
#define PORT_IF_VSR         SOC_PORT_IF_VSR
/*! Fiber 2x10Gbe LR 64B/66B interface */
#define PORT_IF_LR2         SOC_PORT_IF_LR2
/*! Fiber LRM multipoint 64B/66B interface */
#define PORT_IF_LRM         SOC_PORT_IF_LRM
/*! 40G parallel physical interface */
#define PORT_IF_XLPPI       SOC_PORT_IF_XLPPI
/*! Link bonding interface */
#define PORT_IF_LBG         SOC_PORT_IF_LBG
/*! CAUI4 100G interface */
#define PORT_IF_CAUI4       SOC_PORT_IF_CAUI4
/*! OAMP interface */
#define PORT_IF_OAMP        SOC_PORT_IF_OAMP
/*! OLP interface */
#define PORT_IF_OLP         SOC_PORT_IF_OLP
/*! ERP interface */
#define PORT_IF_ERP         SOC_PORT_IF_ERP
/*! SAT interface */
#define PORT_IF_SAT         SOC_PORT_IF_SAT
/*! Eventor interface */
#define PORT_IF_EVENTOR         SOC_PORT_IF_EVENTOR
/*! NIF interface */
#define PORT_IF_NIF_ETH         SOC_PORT_IF_NIF_ETH
/*! FlexE client */
#define PORT_IF_FLEXE_CLIENT    SOC_PORT_IF_FLEXE_CLIENT
/*! Virtual FlexE client */
#define PORT_IF_VIRTUAL_FLEXE_CLIENT    SOC_PORT_IF_VIRTUAL_FLEXE_CLIENT
/*! SCH interface */
#define PORT_IF_SCH         SOC_PORT_IF_SCH
/*! CRPS interface */
#define PORT_IF_CRPS        SOC_PORT_IF_CRPS

/*!
 * \brief Setting the interface type
 *
 * This function is used to set the interface type for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] intf PORT_IF_*
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_interface_set(uint16 uport, port_if_t intf);

/*!
 * \brief Getting the interface type
 *
 * This function is used to get the interface type for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] intf PORT_IF_*
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_interface_get(uint16 uport, port_if_t *intf);

/*!
 * \brief Setting the autonegotiation state
 *
 * This function is used to set the autonegotiation state for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] an Boolean value for autonegotiation state
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_an_set(uint16 uport, BOOL an);

/*!
 * \brief Getting the autonegotiation state of the port
 *
 * This function is used to get the autonegotiation state for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] an Boolean value for autonegotiation state
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_an_get(uint16 uport, BOOL *an);

/*!
 * \brief Enumeration of port duplex mode.
 */
typedef enum port_duplex_e {

    /*! Half duplex */
    PORT_DUPLEX_HALF,

    /*! Full duplex */
    PORT_DUPLEX_FULL,

    /*! Count for duplex mode */
    PORT_DUPLEX_COUNT

} port_duplex_t;

/*!
 * \brief Setting the port duplex state
 *
 * This function is used to set the duplex mode for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] duplex Duplex mode
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_duplex_set(uint16 uport, port_duplex_t duplex);

/*!
 * \brief Getting the port duplex state
 *
 * This function is used to get the duplex mode for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] duplex Duplex mode
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_duplex_get(uint16 uport, port_duplex_t *duplex);

/*!
 * \brief Setting the autonegotiation state
 *
 * This function is used to set the autonegotiation state for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] an Boolean value for autonegotiation state
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_an_set(uint16 uport, BOOL an);

/*!
 * \brief Getting the autonegotiation state of the port
 *
 * This function is used to get the autonegotiation state for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] an Boolean value for autonegotiation state
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_an_get(uint16 uport, BOOL *an);

/*!
 * \brief Physically enable/disable the port
 *
 * This function is used to enable/disable the MAC/PHY for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] enable Boolean value for enable/disable
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_enable_set(uint16 uport, BOOL enable);

/*!
 * \brief Getting the enable state
 *
 * This function is used to get the enable/disable state for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] enable Boolean value for enable/disable.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_enable_get(uint16 uport, BOOL *enable);

/*!
 * \brief Enumeration of port loopback mode.
 */
typedef enum port_loopback_e {

    /*! Loopback disable */
    PORT_LOOPBACK_NONE = 0,

    /*! MAC loopback */
    PORT_LOOPBACK_MAC,

    /*! PHY loopback */
    PORT_LOOPBACK_PHY,

    /*! Remote PHY loopback */
    PORT_LOOPBACK_PHY_REMOTE,

    /*! Count for loopback mode */
    PORT_LOOPBACK_COUNT

} port_loopback_t;

/*!
 * \brief Setting the loopback
 *
 * This function is used to set the loopback for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] loopback Loopback value
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_loopback_set(uint16 uport, port_loopback_t loopback);

/*!
 * \brief Getting the loopback
 *
 * This function is used to get the loopback for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] loopback Loopback value
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_loopback_get(uint16 uport, port_loopback_t *loopback);

/*!
 * \brief Setting the new ifg (Inter-frame gap) value
 *
 * This function is used to set the new ifg (Inter-frame gap) value for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] ifg Inter-frame gap in bit-times.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_ifg_set(uint16 uport, int ifg);

/*!
 * \brief Getting the new ifg (Inter-frame gap) value
 *
 * This function is used to get the new ifg (Inter-frame gap) value for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] ifg Inter-frame gap in bit-times.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_ifg_get(uint16 uport, int *ifg);

/*!
 * \brief Setting the maximum receive frame size
 *
 * This function is used to set the maximum receive frame size for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] size Maximum frame size in bytes.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_frame_max_set(uint16 uport, int size);

/*!
 * \brief Getting the maximum receive frame size
 *
 * This function is used to get the maximum receive frame size for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] size Maximum frame size in bytes.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_frame_max_get(uint16 uport, int *size);

/*!
 * \brief Setting the pause state
 *
 * This function is used to set the pause state for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] pause_tx Boolean value for TX pause state.
 * \param [in] pause_rx Boolean value for RX pause state.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_pause_set(uint16 uport, BOOL pause_tx, BOOL pause_rx);

/*!
 * \brief Getting the pause state
 *
 * This function is used to get the pause state for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] pause_tx Boolean value for TX pause state.
 * \param [out] pause_rx Boolean value for RX pause state.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_pause_get(uint16 uport, BOOL *pause_tx, BOOL *pause_rx);

/*!
 * \brief Enumeration of port class type.
 */
typedef enum port_class_e {

    /*! Class for field stage Lookup */
    PortClassFieldLookup = 0,

    /*! Class for field stage Ingress */
    PortClassFieldIngress = 1,

    /*! Class for field stage Egress */
    PortClassFieldEgress = 2,

    /*! Class for egress VLAN translation */
    PortClassVlanTranslateEgress = 3,

    /*! Class for ingress basis grouping */
    PortClassIngress = 4,

    /*! Class for mapping local-port to vlan-domain. vlan domain is used in mapping packet VID to Vlan */
    PortClassId = 5,

    /*! Packet processing port Class lower 32 bits for field stage Ingress */
    PortClassFieldIngressPacketProcessing = 6,

    /*! Packet processing port Class for field stage Egress */
    PortClassFieldEgressPacketProcessing = 7,

    /*! Class for L2 lookup */
    PortClassL2Lookup = 8,

    /*! Class for ingress forwarding */
    PortClassForwardIngress = 9,

    /*! Class for egress forwarding */
    PortClassForwardEgress = 10,

    /*! Class for field stage Ingress Vlan Translation */
    PortClassFieldIngressVlanTranslation = 11,

    /*! Class for field stage Ingress Tunnel Terminated */
    PortClassFieldIngressTunnelTerminated = 12,

    /*! Class for Egress Programmable Editor properties */
    PortClassProgramEditorEgressPacketProcessing = 13,

    /*! EGR_PORT class ID field for field stage Egress */
    PortClassEgress = 14,

    /*! Per port Class ID for field stage Ingress group selection */
    PortClassFieldIngressGroupSel = 15,

    /*! virtual port instance of SGLP in IFP IPBM key */
    PortClassFieldIngressSystemPort = 16,

    /*! virtual port instance of SVP in IFP IPBM key */
    PortClassFieldIngressSourceGport = 17,

    /*! virtual port instance of device port in IFP IPBM key */
    PortClassFieldIngressDevicePort = 18,

    /*! Class for ingress VLAN translation */
    PortClassVlanTranslateIngress = 19,

    /*! Traffic management port Class for field stage Egress */
    PortClassProgramEditorEgressTrafficManagement = 20,

    /*! Packet processing port Class high 32 bits for field stage Ingress */
    PortClassFieldIngressPacketProcessingHigh = 21,

    /*! Class for VLAN Membership */
    PortClassVlanMember = 22,

    /*! Class for LIF encoded as Gport in the Field Ingres stages */
    PortClassFieldIngressVport = 23,

    /*! Class for LIF encoded as Gport in the Field stage ePMF */
    PortClassFieldEgressVport = 24,

    /*! Class for packet processing port in Field stage iPMF1 */
    PortClassFieldIngressPMF1PacketProcessingPort = 25,

    /*! Context selection class for packet processing port in Field stage iPMF1 */
    PortClassFieldIngressPMF1PacketProcessingPortCs = 26,

    /*! Class for packet processing port in Field stage iPMF3 */
    PortClassFieldIngressPMF3PacketProcessingPort = 27,

    /*! Context selection class for packet processing port in Field stage iPMF3 */
    PortClassFieldIngressPMF3PacketProcessingPortCs = 28,

    /*! Class for packet processing port in Field stage ePMF */
    PortClassFieldEgressPacketProcessingPort = 29,

    /*! Context selection class for packet processing port in Field stage ePMF */
    PortClassFieldEgressPacketProcessingPortCs = 30,

    /*! Context selection class for packet processing port in Field stage external */
    PortClassFieldExternalPacketProcessingPortCs = 31,

    /*! Class for traffic management port in Field stage iPMF1 */
    PortClassFieldIngressPMF1TrafficManagementPort = 32,

    /*! Context selection class for traffic management port in Field stage iPMF1 */
    PortClassFieldIngressPMF1TrafficManagementPortCs = 33,

    /*! Class for traffic management port in Field stage iPMF3 */
    PortClassFieldIngressPMF3TrafficManagementPort = 34,

    /*! Context selection class for traffic management port in Field stage iPMF3 */
    PortClassFieldIngressPMF3TrafficManagementPortCs = 35,

    /*! Class for traffic management processing port in Field stage ePMF */
    PortClassFieldEgressTrafficManagementPort = 36,

/*! Context selection class for traffic management port in Field stage external */
    PortClassFieldExternalTrafficManagementPortCs = 37,

    /*! Extra constant to assign to a packet processing port, up to 32lsb. Field stage iPMF1. */
    PortClassFieldIngressPMF1PacketProcessingPortGeneralData = 38,

    /*! Extra constant to assign to a packet processing port, beyond the first 32 bits. Field stage iPMF1. */
    PortClassFieldIngressPMF1PacketProcessingPortGeneralDataHigh = 39,

    /*! Extra constant to assign to a packet processing port, first 32 bits. Field stage iPMF3. */
    PortClassFieldIngressPMF3PacketProcessingPortGeneralData = 40,

    /*! Extra constant to assign to a packet processing port, beyond the first 32 bits. Field stage iPMF3. */
    PortClassFieldIngressPMF3PacketProcessingPortGeneralDataHigh = 41,

    /*! Opaque control ID available for IFP key selection */
    PortClassOpaqueCtrlId = 42,

    /*! Opaque control ID available for EFP key selection */
    PortClassEgressOpaqueCtrlId = 43,

    /*! Opaque control ID available for VFP key selection */
    PortClassOpaqueCtrlId1 = 44,

    /*! Opaque control ID A for flex digest and VFP key selection. */
    PortClassOpaqueCtrlId2 = 45,

    /*! Opaque control ID B for flex digest and VFP key selection. */
    PortClassOpaqueCtrlId3 = 46,

    /*! Must be the last. */
    PortClassCount = 47

} port_class_t;

/*!
 * \brief Setting the ports class ID.
 *
 * This function is used to set the ports class ID for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] pclass Classification type.
 * \param [in] pclass_id New class ID of the port.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_class_set(uint16 uport, port_class_t pclass, uint32 pclass_id);

/*!
 * \brief Getting the ports class ID.
 *
 * This function is used to get the ports class ID for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] pclass Classification type.
 * \param [out] pclass_id New class ID of the port.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_class_get(uint16 uport, port_class_t pclass, uint32 *pclass_id);

/*!
 * \brief Get the link status
 *
 * This function is used to get the link status for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] link Link status.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_link_status_get(uint16 uport, BOOL *link);

/*!
 * \brief Get the link status
 *
 * This function is used to get the current speed status for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] speed Speed alue in megabits/sec (10, 100, etc).
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_speed_status_get(uint16 uport, int *speed);

/*!
 * \brief Get the link status
 *
 * This function is used to get the current autoneg status for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] an Boolean value for autonegotiation state.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_an_status_get(uint16 uport, BOOL *an);

/*!
 * \brief Get the duplex status
 *
 * This function is used to get the current duplex status for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] duplex Duplex mode.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_duplex_status_get(uint16 uport, port_duplex_t *duplex);

/*!
 * \brief Port fault status type.
 */
typedef struct board_port_fault_st_s {

    /*!
     * Local fault state.
     * Set to indicate local fault detected.
     */
    int local_fault;

    /*!
     * Remote fault state.
     * Set to indicate remote fault detected.
     */
    int remote_fault;

} board_port_fault_st_t;

/*!
 * \brief Get the fault status.
 *
 * This function is used to get the fault status for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] fault_st Fault status.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_fault_status_get(uint16 uport, board_port_fault_st_t *fault_st);

/*!
 * \brief Port fault control type.
 */
typedef struct board_port_fault_ctrl_s {

    /*!
     * Enable/Disable local fault detection.
     * Set 1 to enable, 0 to disable.
     */
    int local_fault;

    /*!
     * Enable/Disable remote fault detection.
     * Set 1 to enable, 0 to disable.
     */
    int remote_fault;

    /*!
     * Enable/Disable forcibly sending remote fault.
     * Set 1 to enable, 0 to disable.
     */
    int forced_remote_fault;

} board_port_fault_ctrl_t;

/*!
 * \brief Set the fault configuration.
 *
 * This function is used to set the fault configuration for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] fault_ctrl Fault configuration.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_fault_ctrl_set(uint16 uport, board_port_fault_ctrl_t *fault_ctrl);

/*!
 * \brief Get the fault configuration.
 *
 * This function is used to get the fault configuration for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] fault_ctrl Fault configuration.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_fault_ctrl_get(uint16 uport, board_port_fault_ctrl_t *fault_ctrl);

/*!
 * \brief Set PHY power configuration.
 *
 * This function is used to set the PHY (Serdes) power configuration for a given
 * port.
 *
 * \param [in] uport Port number.
 * \param [in] power PHY power state. 1 for power on, 0 for power down.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_phy_power_set(uint16 uport, int power);

/*!
 * \brief Get PHY power configuration.
 *
 * This function is used to get the PHY (Serdes) power configuration for a given
 * port.
 *
 * \param [in] uport Port number.
 * \param [out] power PHY power state. 1 for power on, 0 for power down.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_phy_power_get(uint16 uport, int *power);

/*!
 * \brief FEC mode type.
 */
typedef enum board_phy_fec_mode_s {

    /*! No FEC. */
    BOARD_PHY_FEC_NONE,

    /*! FEC CL74. */
    BOARD_PHY_FEC_CL74,

    /*! FEC CL91. */
    BOARD_PHY_FEC_CL91,

    /*! FEC CL108. */
    BOARD_PHY_FEC_CL108

} board_phy_fec_mode_t;

/*!
 * \brief Set PHY FEC mode.
 *
 * This function is used to set the PHY (Serdes) FEC mode for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] fec_mode FEC mode.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_phy_fec_mode_set(uint16 uport, board_phy_fec_mode_t fec_mode);

/*!
 * \brief Get PHY FEC mode.
 *
 * This function is used to get the PHY (Serdes) FEC mode for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] fec_mode FEC mode.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_phy_fec_mode_get(uint16 uport, board_phy_fec_mode_t *fec_mode);

/*!
 * \brief PHY FEC status type.
 */
typedef struct board_phy_fec_st_s {

    /*! FEC corrected block counter for Base-R FEC. */
    uint32 corrected_blocks;

    /*! FEC uncorrected block counter for Base-R FEC. */
    uint32 uncorrected_blocks;

    /*! FEC corrected codeword counter for RS-FEC. */
    uint32 corrected_cws;

    /*! FEC uncorrected codeword counter for RS-FEC. */
    uint32 uncorrected_cws;

} board_phy_fec_st_t;

/*!
 * \brief Get PHY FEC status.
 *
 * This function is used to get the PHY (Serdes) FEC status for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] fec_st FEC status.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_phy_fec_status_get(uint16 uport, board_phy_fec_st_t *fec_st);

/*!
 * \brief PHY Tx control type.
 */
typedef struct board_phy_tx_ctrl_s {

    /*! Tx FIR pre cursor tap value. */
    int pre;

    /*! Tx FIR main cursor tap value. */
    int main;

    /*! Tx FIR post cursor tap value. */
    int post;

    /*! Tx FIR post2 cursor tap value. */
    int post2;

    /*! Tx FIR post3 cursor tap value. */
    int post3;

} board_phy_tx_ctrl_t;

/*!
 * \brief Set PHY Tx configuration.
 *
 * This function is used to set the PHY (Serdes) Tx parameters for a given
 * port.
 *
 * \param [in] uport Port number.
 * \param [in] tx_ctrl PHY tx configuration.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_phy_tx_ctrl_set(uint16 uport, board_phy_tx_ctrl_t *tx_ctrl);

/*!
 * \brief Get PHY Tx configuration.
 *
 * This function is used to get the PHY (Serdes) Tx parameters for a given
 * port.
 *
 * \param [in] uport Port number.
 * \param [out] tx_ctrl PHY tx configuration.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_phy_tx_ctrl_get(uint16 uport, board_phy_tx_ctrl_t *tx_ctrl);

/*! Number of DEF taps. */
#define BOARD_NUM_DFE_TAPS 18

/*!
 * \brief PHY Rx status type.
 */
typedef struct board_phy_rx_st_s {

    /*! Number of elements in DFE array. */
    uint32 num_of_dfe_taps;

    /*! DFE values. */
    int dfe[BOARD_NUM_DFE_TAPS];

    /*! VGA. */
    int vga;

    /*! PPM. */
    int ppm;

} board_phy_rx_st_t;

/*!
 * \brief Get PHY Rx status.
 *
 * This function is used to get the PHY (Serdes) Rx status for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] rx_st PHY rx status.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_phy_rx_status_get(uint16 uport, board_phy_rx_st_t *rx_st);

/*!
 * \name PRBS RX/TX selector flags.
 * \anchor BOARD_PHY_PRBS_CTRL_F_XXX
 */
/*! \{ */
/*! Config RX. */
#define BOARD_PHY_PRBS_CTRL_F_RX 0x1
/*! Config TX */
#define BOARD_PHY_PRBS_CTRL_F_TX 0x2
/*! \} */

/*!
 * PRBS polynomial.
 */
typedef enum board_phy_prbs_poly_e {
    BOARD_PHY_PRBS_POLY_7 = 0,
    BOARD_PHY_PRBS_POLY_9,
    BOARD_PHY_PRBS_POLY_11,
    BOARD_PHY_PRBS_POLY_15,
    BOARD_PHY_PRBS_POLY_23,
    BOARD_PHY_PRBS_POLY_31,
    BOARD_PHY_PRBS_POLY_58,
    BOARD_PHY_PRBS_POLY_49,
    BOARD_PHY_PRBS_POLY_10,
    BOARD_PHY_PRBS_POLY_20,
    BOARD_PHY_PRBS_POLY_13,
    BOARD_PHY_PRBS_POLY_Q13,
    BOARD_PHY_PRBS_POLY_Count
} board_phy_prbs_poly_t;

/*!
 * \brief PHY PRBS control type.
 */
typedef struct board_phy_prbs_ctrl_s {

    /*! PRBS polynomial. */
    board_phy_prbs_poly_t poly;

    /*! Invert. */
    uint32 invert;

} board_phy_prbs_ctrl_t;

/*!
 * \brief Set PHY PRBS configuration.
 *
 * This function is used to set the PHY (Serdes) PRBS configuration for a given
 * port.
 *
 * \param [in] uport Port number.
 * \param [in] flags Config flags \ref BOARD_PHY_PRBS_CTRL_F_XXX.
 * \param [in] prbs PHY PRBS configuration.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_phy_prbs_ctrl_set(uint16 uport, int flags,
                             board_phy_prbs_ctrl_t *prbs);

/*!
 * \brief Get PHY PRBS configuration.
 *
 * This function is used to get the PHY (Serdes) PRBS configuration for a given
 * port.
 *
 * \param [in] uport Port number.
 * \param [in] flags Config flags \ref BOARD_PHY_PRBS_CTRL_F_XXX.
 * \param [out] prbs PHY PRBS configuration.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_phy_prbs_ctrl_get(uint16 uport, int flags,
                             board_phy_prbs_ctrl_t *prbs);

/*!
 * \brief Enable/Disable PHY PRBS.
 *
 * This function is used to set the PHY (Serdes) PRBS enable state for a given
 * port.
 *
 * \param [in] uport Port number.
 * \param [in] flags Config flags \ref BOARD_PHY_PRBS_CTRL_F_XXX.
 * \param [in] en 1 to enable PRBS, 0 to disable.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_phy_prbs_enable_set(uint16 uport, int flags, int en);

/*!
 * \brief Get PHY PRBS enable state.
 *
 * This function is used to get the PHY (Serdes) PRBS enable state for a given
 * port.
 *
 * \param [in] uport Port number.
 * \param [in] flags Config flags \ref BOARD_PHY_PRBS_CTRL_F_XXX.
 * \param [out] en 1 indicates PRBS enabled, 0 indicates disabled.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_phy_prbs_enable_get(uint16 uport, int flags, int *en);

/*!
 * PRBS status.
 */
typedef struct board_phy_prbs_st_s {

    /*! Whether PRBS is currently locked. */
    uint32 prbs_lock;

    /*! Whether PRBS was unlocked since last call. */
    uint32 prbs_lock_loss;

    /*! PRBS errors count. */
    uint32 error_count;

} board_phy_prbs_st_t;

/*!
 * \brief Get PHY PRBS status.
 *
 * This function is used to get the PHY (Serdes) PRBS status for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] st PRBS status.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_phy_prbs_status_get(uint16 uport, board_phy_prbs_st_t *st);

/*! Maximum lanes per core. */
#define BOARD_MAX_LANES_PER_CORE 12

/*!
 * PHY error statistics.
 */
typedef struct board_phy_stats_s {

    /*! BIP error count per lane. */
    uint32 bip_count[BOARD_MAX_LANES_PER_CORE];

    /*! BER count per lane. */
    uint32 ber_count[BOARD_MAX_LANES_PER_CORE];

} board_phy_stats_t;

/*!
 * \brief Get PHY error statistics.
 *
 * This function is used to get the PHY (Serdes) statistic counters, such as BER
 * and BIP error count, for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] stats PHY error statistics.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_phy_stats_get(uint16 uport, board_phy_stats_t *stats);

#endif /* _BOARDAPI_PORT_H_ */
