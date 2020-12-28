/*
 * $Id: port.h,v 1.22 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_PORT_H_
#define _BOARDAPI_PORT_H_

/*! Get link speed of a port */
extern sys_error_t
board_get_port_link_speed(uint16 uport, int16 *speed);

/*! Get the status of auto neg */
extern sys_error_t
board_get_port_phy_autoneg(uint16 uport, BOOL *an, BOOL *an_done);

/*! Get the remote EEE capability */
sys_error_t board_get_port_phy_eee_ability_remote(uint16 uport, uint8 *types);

/*! Get the minimum PHY EEE transition wake time */
extern sys_error_t
board_get_port_phy_eee_min_wake_time(uint16 uport, uint8 type, uint16 *wake_t);

/*! Set the EEE tx wake timer */
extern sys_error_t
board_set_port_phy_eee_tx_wake_time(uint16 uport, uint8 type, uint16 wake_t);





/*
 * Port status
 */
typedef enum port_mode_s {
    PM_LINKDOWN = 0,
    PM_10MB_HD = 10,
    PM_10MB_FD = 11,
    PM_100MB_HD = 100,
    PM_100MB_FD = 101,
    PM_1000MB = 1000,
    PM_2500MB = 2500,
    PM_5000MB = 5000,
    PM_10000MB = 10000,
    PM_25000MB = 25000,
    PM_40000MB = 40000,
    PM_50000MB = 50000,
    PM_100000MB = 100000,
    PM_AUTO = -1,
} port_mode_t;

/*!
 * \brief Get the port mode for the specific user port.
 * \param [in] uport User port number.
 * \param [out] mode Port mode.
 *    \li PM_LINKDOWN
 *    \li PM_10MB_HD
 *    \li PM_10MB_FD
 *    \li ...
 *    \li PM_50000MB
 *    \li PM_100000MB
 *    \li PM_AUTO
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_mode_get(uint16 uport,
                                       port_mode_t *mode);

/*
 * Cable Diagnostics
 */
typedef enum {
    PORT_CABLE_STATE_NO_CABLE,
    PORT_CABLE_STATE_OK,
    PORT_CABLE_STATE_OPEN,
    PORT_CABLE_STATE_SHORT,
    PORT_CABLE_STATE_OPENSHORT,
    PORT_CABLE_STATE_CROSSTALK
} port_cable_state_t;

typedef struct port_cable_diag_s {
    port_cable_state_t  state;   /*! state */
    int32               length;  /*! length in metres */
} port_cable_diag_t;

/*!
 * \brief Run cable diagnostics on the port.
 * \param [in] uport Port number.
 * \param [in] status Cable diag status structure.
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_port_cable_diag(uint16 uport,
                                         port_cable_diag_t *status);

extern sys_error_t
board_get_cable_diag_support_by_port(uint16 uport,
                                         BOOL *support);
extern uint16 board_cable_diag_port_count(void);
/*
 * MIB information
 */
typedef struct port_stat_s {
    /*! Byte Counter */
    uint32  RxOctets_lo;  /*! Rx byte count lo */
    uint32  RxOctets_hi;  /*! Rx byte count hi */
    uint32  TxOctets_lo;  /*! Tx byte count lo */
    uint32  TxOctets_hi;  /*! Tx byte count hi */
    /*! Frame counter */
    uint32  RxPkts_lo;
    uint32  RxPkts_hi;
    uint32  TxPkts_lo;
    uint32  TxPkts_hi;
    /*! Rx FCS Error Frame Counter */
    uint32  CRCErrors_lo;
    uint32  CRCErrors_hi;
    /*! Unicast Frame Counter */
    uint32  RxUnicastPkts_lo;
    uint32  RxUnicastPkts_hi;
    uint32  TxUnicastPkts_lo;
    uint32  TxUnicastPkts_hi;
    /*! Multicast Frame Counter */
    uint32  RxMulticastPkts_lo;
    uint32  RxMulticastPkts_hi;
    uint32  TxMulticastPkts_lo;
    uint32  TxMulticastPkts_hi;
    /*! Broadcast Frame Counter */
    uint32  RxBroadcastPkts_lo;
    uint32  RxBroadcastPkts_hi;
    uint32  TxBroadcastPkts_lo;
    uint32  TxBroadcastPkts_hi;
    /*! Pause Frame Counter */
    uint32  RxPauseFramePkts_lo;
    uint32  RxPauseFramePkts_hi;
    uint32  TxPauseFramePkts_lo;
    uint32  TxPauseFramePkts_hi;
    /*! Oversized Frame Counter */
    uint32  RxOversizePkts_lo;
    uint32  RxOversizePkts_hi;
    uint32  TxOversizePkts_lo;
    uint32  TxOversizePkts_hi;

     /*! IPIPE HOLD Drop Counter */
    uint32  EgressDropPkts_lo;
    uint32  EgressDropPkts_hi;

    /*! EEE LPI counter */
    uint32  RxLPIPkts_hi;
    uint32  RxLPIDuration_hi;
    uint32  TxLPIPkts_hi;
    uint32  TxLPIDuration_hi;
    uint32  RxLPIPkts_lo;
    uint32  RxLPIDuration_lo;
    uint32  TxLPIPkts_lo;
    uint32  TxLPIDuration_lo;

} port_stat_t;

/*!
 * \brief Get the statistic value for a given port.
 * \param [in] uport Port number.
 * \param [out] stat Statistics value.
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_port_stat_get(uint16 uport, port_stat_t *stat);

/*
 * Reset counters
 */
/*!
 * \brief Clear the statistic value for a given port.
 * \param [in] uport Port number.
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_stat_clear(uint16 uport);

/*!
 * \brief Clear the statistic value for all the ports.
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
 * \brief Enumeration of port interface.
 */
typedef enum port_if_e {

    /*! No physical connection */
    PORT_IF_NOCXN,

    /*! Pass-through connection without PHY */
    PORT_IF_NULL,

    /*! Media-Independent Interface. */
    PORT_IF_MII,

    /*! Gigabit MII. */
    PORT_IF_GMII,

    /*! Serial Gigabit MII. */
    PORT_IF_SGMII,

    /*! Ten Bit Interface. */
    PORT_IF_TBI,

    /*! 10-Gigabit MII. */
    PORT_IF_XGMII,

    /*! Reduced Gigabit MII. */
    PORT_IF_RGMII,

    /*! Reverse MII. */
    PORT_IF_RVMII,

    /*! Serdes Framer Interface. */
    PORT_IF_SFI,

    /*! XFP Interface. */
    PORT_IF_XFI,

    /*! Backplane Interface with 64B/66B PCS. */
    PORT_IF_KR,

    /*! Backplane 2x10GbE 64B/66B interface */
    PORT_IF_KR2,

    /*! Backplane 4x10 GbE 64B/66B interface */
    PORT_IF_KR4,

    /*! Copper 10 GbE 64B/66B interface */
    PORT_IF_CR,

    /*! Copper 2x10GbE 64B/66B interface */
    PORT_IF_CR2,

    /*! Copper 4x10 GbE 64B/66B interface. */
    PORT_IF_CR4,

    /*! 40 Gigabit Attachment Unit Interface */
    PORT_IF_XLAUI,

    /*! 40 Gigabit Attachment Unit Interface over 2 lanes */
    PORT_IF_XLAUI2,

    /*! Reduced XAUI. */
    PORT_IF_RXAUI,

    /*! 10-gigabit Attachment Unit Interface. */
    PORT_IF_XAUI,

    /*! SPAUI interface */
    PORT_IF_SPAUI,

    /*! Qual Serial Gigabit MII. */
    PORT_IF_QSGMII,

    /*! ILKN interface */
    PORT_IF_ILKN,

    /*! RCY interface */
    PORT_IF_RCY,

    /*! PIPE interface */
    PORT_IF_FAT_PIPE,

    /*! Fiber SR/LR 64B/66B interface */
    PORT_IF_SR,

    /*! Fiber 2x10GbE 64B/66B interface. */
    PORT_IF_SR2,

    /*! CAUI interface */
    PORT_IF_CAUI,

    /*! Fiber LR 64B/66B interface */
    PORT_IF_LR,

    /*! Fiber LR4 64B/66B interface */
    PORT_IF_LR4,

    /*! Fiber SR4 64B/66B interface */
    PORT_IF_SR4,

    /*! Backplane Interface with 8B/10B PCS. */
    PORT_IF_KX,

    /*! Fiber ZR 64B/66B interface */
    PORT_IF_ZR,

    /*! Fiber SR10 64B/66B interface. */
    PORT_IF_SR10,

    /*! Copper CR10 64B/66B interface. */
    PORT_IF_CR10,

    /*! Backplane KR10 64B/66B interface. */
    PORT_IF_KR10,

    /*! Fiber LR10 64B/66B interface. */
    PORT_IF_LR10,

    /*! Fiber 4x25 GbE OTL interface */
    PORT_IF_OTL,

    /*! CPU interface */
    PORT_IF_CPU,

    /*! Fiber ER 64B/66B interface */
    PORT_IF_ER,

    /*! Fiber ER2 2x10G 64B/66B interface */
    PORT_IF_ER2,

    /*! Fiber ER4 4x10G 64B/66B interface */
    PORT_IF_ER4,

    /*! Copper 10G 10B/8B interface */
    PORT_IF_CX,

    /*! Copper 2x10G 10B/8B interface. */
    PORT_IF_CX2,

    /*! Copper 4x10G 10B/8B interface */
    PORT_IF_CX4,

    /*! CAUI 100G C2C interface */
    PORT_IF_CAUI_C2C,

    /*! CAUI 100G C2M interface */
    PORT_IF_CAUI_C2M,

    /*! Fiber VSR 10G 64B/66B interface */
    PORT_IF_VSR,

    /*! Fiber 2x10Gbe LR 64B/66B interface */
    PORT_IF_LR2,

    /*! Fiber LRM multipoint 64B/66B interface */
    PORT_IF_LRM,

    /*! 40G parallel physical interface */
    PORT_IF_XLPPI,

    /*! Link bonding interface */
    PORT_IF_LBG,

    /*! CAUI4 100G interface */
    PORT_IF_CAUI4,

    /*! OAMP interface */
    PORT_IF_OAMP,

    /*! OLP interface */
    PORT_IF_OLP,

    /*! ERP interface */
    PORT_IF_ERP,

    /*! SAT interface */
    PORT_IF_SAT,

    /*! RCY Mirror interface */
    PORT_IF_RCY_MIRROR,

    /*! Eventor interface */
    PORT_IF_EVENTOR,

    /*! NIF interface */
    PORT_IF_NIF_ETH,

    /*! FlexE client */
    PORT_IF_FLEXE_CLIENT,

    /*! Virtual FlexE client */
    PORT_IF_VIRTUAL_FLEXE_CLIENT,

    /*! SCH interface */
    PORT_IF_SCH,

    /*! CRPS interface */
    PORT_IF_CRPS,

    /*! Must be the last. */
    PORT_IF_COUNT

} port_if_t;

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
board_port_enable_set(uint16 uport, BOOL portenable);

/*!
 * \brief Getting the enable state
 *
 * This function is used to get the enable/disable state for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] enable Boolean value for enable/disable
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_enable_get(uint16 uport, BOOL *portenable);

/*!
 * \brief Enumeration of port loopback mode.
 */
typedef enum port_loopback_e {

    /*! Loopback disable */
    PORT_LOOPBACK_NONE = 0,

    /*! MAC loopback */
    PORT_LOOPBACK_MAC  = 1,

    /*! PHY loopback */
    PORT_LOOPBACK_PHY  = 2,

    /*! Remote PHY loopback */
    PORT_LOOPBACK_PHY_REMOTE = 3,

    /*! Must be the last. */
    PORT_LOOPBACK_COUNT = 4

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
 * \brief Setting the new ifg (Inter-frame gap) value
 *
 * This function is used to set the new ifg (Inter-frame gap) value for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] speed The speed for which the IFG is being set.
 * \param [in] duplex The duplex for which the IFG is being set.
 * \param [in] ifg Inter-frame gap in bit-times.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_ifg_set(uint16 uport, int speed, port_duplex_t duplex, int ifg);

/*!
 * \brief Getting the new ifg (Inter-frame gap) value
 *
 * This function is used to get the new ifg (Inter-frame gap) value for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] speed The speed for which the IFG is being set.
 * \param [in] duplex The duplex for which the IFG is being set.
 * \param [out] ifg Inter-frame gap in bit-times.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_ifg_get(uint16 uport, int speed, port_duplex_t duplex, int *ifg);

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

#endif /* _BOARDAPI_PORT_H_ */
