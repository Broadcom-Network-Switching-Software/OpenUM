/*! \file timesync.h
 *
 * Timesync board APIs.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_TIMESYNC_H_
#define _BOARDAPI_TIMESYNC_H_

#ifdef CFG_SWITCH_TIMESYNC_INCLUDED

#include <utils/shr/shr_types.h>

#ifndef CFG_PCM_SUPPORT_INCLUDED
#include <shared/port.h>        /*  for _shr_port_if_t */
#endif

/* Timesync Configuration flags. */
#define BCM_PORT_TIMESYNC_DEFAULT           0x00000001 /*!< Port Timesync
                                                          configuration */
#define BCM_PORT_TIMESYNC_MPLS              0x00000002 /*!< Port Timesync MPLS
                                                          configuration */
#define BCM_PORT_TIMESYNC_ONE_STEP_TIMESTAMP 0x00000004 /*!< Port one step
                                                          timestamp
                                                          configuration */
#define BCM_PORT_TIMESYNC_TWO_STEP_TIMESTAMP 0x00000008 /*!< Port two step
                                                          timestamp
                                                          configuration */
#define BCM_PORT_TIMESYNC_TIMESTAMP_CFUPDATE_ALL 0x00000010 /*!< All 1588 packets will
                                                          be egress timestamped.
                                                          By default, only
                                                          ingress timestamped
                                                          packets are egress
                                                          timestamped */
#define BCM_PORT_TIMESYNC_EXTERNAL_MAC_ENABLE 0x00000020 /*!< enable external mac
                                                          for the port */

/* Timesync Packet type bitmaps. */
#define BCM_PORT_TIMESYNC_PKT_SYNC          0x00000001 /*!< Sync Event Packet type */
#define BCM_PORT_TIMESYNC_PKT_DELAY_REQ     0x00000002 /*!< Delay Request Event
                                                          Packet type */
#define BCM_PORT_TIMESYNC_PKT_PDELAY_REQ    0x00000004 /*!< Pdelay Request Event
                                                          Packet type */
#define BCM_PORT_TIMESYNC_PKT_PDELAY_RESP   0x00000008 /*!< Pdelay Response Event
                                                          Packet type */
#define BCM_PORT_TIMESYNC_PKT_FOLLOWUP      0x00000100 /*!< Followup General
                                                          Packet type */
#define BCM_PORT_TIMESYNC_PKT_DELAY_RESP    0x00000200 /*!< Delay_resp General
                                                          Packet type */
#define BCM_PORT_TIMESYNC_PKT_PDELAY_RESP_FOLLOWUP 0x00000400 /*!< Pdelay_resp_followup
                                                          General Packet type */
#define BCM_PORT_TIMESYNC_PKT_ANNOUNCE      0x00000800 /*!< Announce General
                                                          Packet type */
#define BCM_PORT_TIMESYNC_PKT_SIGNALLING    0x00001000 /*!< Signalling General
                                                          Packet type */
#define BCM_PORT_TIMESYNC_PKT_MANAGMENT     0x00002000 /*!< Management General
                                                          Packet type */
#define BCM_PORT_TIMESYNC_PKT_INVALID       0x00004000 /*!< Invalid Packet type */

/*!
 * Timesync configuration parameters.
 */
typedef struct bcm_port_timesync_config_s {
    uint32 flags;           /*!< Timesync Configuration flags */
    uint32 pkt_drop;        /*!< Pakcet Drop bitmap of event and general  packet
                               types */
    uint32 pkt_tocpu;       /*!< CPU forward bitmap of event and general  packet
                               types */
    int mpls_min_label;     /*!< Timesync over MPLS min label */
    int mpls_max_label;     /*!< Timesync over MPLS max label */
    shr_mac_t src_mac_addr; /*!< Source mac address for one-step timestamp update */
    int user_trap_id;       /*!< 1588 user trap id */
} bcm_port_timesync_config_t;

/*!
 * \brief Set timesync configurations for the port.
 *
 * The port timesync configuration must specify one-step or two-step
 * timestamp type and its packet drop and CPU forward bitmaps.
 * To disable timesync configuration on a particular port,
 * bcm_port_timesync_config_set() API needs to be called with zero config
 * count and NULL pointer needs to be passed to bcm_port_timesync_config_t
 * structure pointer
 *
 * \param [in] uport Port number.
 * \param [in] config_count Count of timesync configurations.
 * \param [in] config_array Pointer to timesync configurations.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_timesync_config_set(uint16 uport,
                            int config_count, bcm_port_timesync_config_t *config_array);

/*!
 * \brief Get timesync configurations for the port.
 *
 * This function returns timesync configurations configured and
 * its configuration array count for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] array_size Required Count of timesync configurations.
 * \param [in,out] config_array Pointer to timesync configurations.
 * \param [out] array_count Pointer to timesync configuration array count.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_timesync_config_get(uint16 uport, int array_size,
                            bcm_port_timesync_config_t *config_array,
                            int *array_count);

/*! Fast call actions */
typedef enum bcm_port_control_phy_timesync_e {
    bcmPortControlPhyTimesyncCaptureTimestamp = _SHR_PORT_CONTROL_PHY_TIMESYNC_CAPTURE_TIMESTAMP, /*!< Captured TS from packets */
    bcmPortControlPhyTimesyncHeartbeatTimestamp = _SHR_PORT_CONTROL_PHY_TIMESYNC_HEARTBEAT_TIMESTAMP, /*!< Heartbeat TS at framesync */
    bcmPortControlPhyTimesyncNCOAddend = _SHR_PORT_CONTROL_PHY_TIMESYNC_NCOADDEND, /*!< NCO addend (for non DPLL mode) */
    bcmPortControlPhyTimesyncFrameSync = _SHR_PORT_CONTROL_PHY_TIMESYNC_FRAMESYNC, /*!< Cause immediate framesync */
    bcmPortControlPhyTimesyncLocalTime = _SHR_PORT_CONTROL_PHY_TIMESYNC_LOCAL_TIME, /*!< Set local time (44 bits) */
    bcmPortControlPhyTimesyncLoadControl = _SHR_PORT_CONTROL_PHY_TIMESYNC_LOAD_CONTROL, /*!< Load control and sticky bits */
    bcmPortControlPhyTimesyncInterrupt = _SHR_PORT_CONTROL_PHY_TIMESYNC_INTERRUPT, /*!< 1588 PHY interrupt status */
    bcmPortControlPhyTimesyncInterruptMask = _SHR_PORT_CONTROL_PHY_TIMESYNC_INTERRUPT_MASK, /*!< 1588 PHY interrupt mask */
    bcmPortControlPhyTimesyncTxTimestampOffset = _SHR_PORT_CONTROL_PHY_TIMESYNC_TX_TIMESTAMP_OFFSET, /*!< TX AFE delay */
    bcmPortControlPhyTimesyncRxTimestampOffset = _SHR_PORT_CONTROL_PHY_TIMESYNC_RX_TIMESTAMP_OFFSET, /*!< RX AFE delay */
    bcmPortControlPhyTimesyncNSETimeOffset = _SHR_PORT_CONTROL_PHY_TIMESYNC_NSE_TIME_OFFSET, /*!< En_adj_80 << 15 | En_adj_48 << 14 |
                                           14 bit 2's comp value */
    bcmPortControlPhyTimesyncTimestampOffset = _SHR_PORT_CONTROL_PHY_TIMESYNC_TIMESTAMP_OFFSET, /*!< the delay from CMIC to the PCS */
    bcmPortControlPhyTimesyncTimestampAdjust = _SHR_PORT_CONTROL_PHY_TIMESYNC_TIMESTAMP_ADJUST, /*!< the adjustment of TS */
    bcmPortControlPhyTimesyncOneStepEnable = _SHR_PORT_CONTROL_PHY_TIMESYNC_ONE_STEP_ENABLE /*!< One step proecssing mode for TS */
} bcm_port_control_phy_timesync_t;

/*! AM norm modes for aggregated port speeds */
typedef enum bcm_port_phy_timesync_compensation_mode_e {
    bcmPortPhyTimesyncCompensationModeNone = _SHR_PORT_PHY_TIMESYNC_COMPENSATION_MODE_NONE, /*!< No compensation */
    bcmPortPhyTimesyncCompensationModeEarliestLane = _SHR_PORT_PHY_TIMESYNC_COMPENSATION_MODE_EARLIEST_LANE, /*!< Compensate based on the earliest lane */
    bcmPortPhyTimesyncCompensationModeLatestlane = _SHR_PORT_PHY_TIMESYNC_COMPENSATION_MODE_LATEST_LANE /*!< Compensate based on the latest lane. */
} bcm_port_phy_timesync_compensation_mode_t;

/*!
 * \brief Configure PHY/PCS timesync for the port.
 *
 * This function is to enable/disable PHY/PCS timesync configurations for the port.
 *
 * \param [in] uport Port number.
 * \param [in] en value for enable/disable.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_phy_timesync_enable_set(uint16 uport, int en);

/*!
 * \brief Get PHY/PCS timesync configurations for the port.
 *
 * This function returns PHY/PCS timesync configurations configured
 * for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] en value for enable/disable.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_phy_timesync_enable_get(uint16 uport, int *en);

/*!
 * \brief Set timesync PHY/PCS features for the port.
 *
 * This function sets specific timesync feature for the port.
 *
 * \param [in] uport Port number.
 * \param [in] type Port feature enumerator.
 * \param [in] value Value to configure for the feature.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_control_phy_timesync_set(uint16 uport,
                            bcm_port_control_phy_timesync_t type,
                            uint64 value);

/*!
 * \brief Get timesync PHY/PCS features for the port.
 *
 * This function returns specific timesync feature configured
 * for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] type Port feature enumerator.
 * \param [out] value Value configured for the feature.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_control_phy_timesync_get(uint16 uport,
                            bcm_port_control_phy_timesync_t type,
                            uint64 *value);

/*!
 * 1588 packet's transmit information.
 */
typedef struct bcm_port_timesync_tx_info_s {
    uint64 timestamp;   /*!< transmit timestamp of ptp packet */
    uint32 sequence_id; /*!< sequence id of transmitted ptp packet */
    uint32 sub_ns;      /*!< timestamp sub-nanosecond */
} bcm_port_timesync_tx_info_t;

/*!
 * \brief Get 1588 packet's transmit information form the port.
 *
 * In 1588 two-step mode, get timestamp and sequence id of ptp packet after
 * sending 1588 packet.
 *
 * \param [in] uport Port number.
 * \param [in,out] tx_info Pointer to structure to get timesync tx informatio.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_timesync_tx_info_get(uint16 uport,
                            bcm_port_timesync_tx_info_t *tx_info);
#endif /* CFG_SWITCH_TIMESYNC_INCLUDED*/
#endif /* _BOARDAPI_TIMESYNC_H_ */
