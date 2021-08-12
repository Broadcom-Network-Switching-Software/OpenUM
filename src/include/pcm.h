/*
 * $Id: pcm.h,v 1.2 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _PCM_H_
#define _PCM_H_

#define PORT_LINK_UP                    (1)
#define PORT_LINK_DOWN                  (0)

/* for SDK header file include */
#define _SOC_TYPES_H                 /* prventing SDK types definition */
#define _SAL_TYPES_H                 /* prventing SDK types definition */
#define _SOC_DEFS_H                  /* prventing SOC_MAX_NUM_DEVICES define*/
#define _SAL_LIBC_H                  /* preventing SAL layer include */
#define _SAL_IO_H                    /* preventing SAL layer include */
#define _SOC_PHYCTRL_H_              /* for preventing include phyctrl.h in ll.h */
typedef uint8 soc_port_t;           /* for soc/ll.h */
typedef uint8 sal_mac_addr_t[6];   /*  for  soc/ll.h */
#define SOC_MAX_NUM_DEVICES 1
#define PHYMOD_CONFIG_DEFINE_INT8_T 0
#define PHYMOD_CONFIG_DEFINE_INT32_T 0

/*
 * PCM loopback mode
 */
#define PCM_PORT_LOOPBACK_NONE                  0
#define PCM_PORT_LOOPBACK_MAC                   1
#define PCM_PORT_LOOPBACK_PHY                   2
#define PCM_PORT_LOOPBACK_PHY_REMOTE            3

/*
 * Duplex mode
 */
#define PCM_PORT_DUPLEX_HALF                    0
#define PCM_PORT_DUPLEX_FULL                    1

#include <soc/portmode.h>            /*  for  soc/port_ability.h */
#include <soc/port_ability.h>        /*  for _shr_port_ability_t */
#include <shared/port.h>             /*  for _shr_port_if_t */
#include <soc/ll.h>                  /*  for bcmxxxx.h:mac_driver_t define */
#include <boardapi/port.h>           /*  for port_cable_diag_t */

typedef _shr_port_ability_t          pcm_port_ability_t;
typedef _shr_port_cable_diag_t       pcm_port_cable_diag_t;
typedef _shr_port_if_t               pcm_port_if_t;

/*
 * phy_addr encoding
 * bit7, 1: internal MDIO bus, 0: external MDIO bus
 * bit9,8,6,5, mdio bus number
 * bit4-0,   mdio addresses
 */
#define PCM_PHY_ID_BUS_UPPER_MASK     0x300
#define PCM_PHY_ID_BUS_UPPER_SHIFT    0x6
#define PCM_PHY_ID_BUS_LOWER_MASK     0x60
#define PCM_PHY_ID_BUS_LOWER_SHIFT    5
#define PCM_PHY_ID_BUS_NUM(_id)   ((((_id) & PCM_PHY_ID_BUS_UPPER_MASK) >> \
        PCM_PHY_ID_BUS_UPPER_SHIFT) | (((_id) & PCM_PHY_ID_BUS_LOWER_MASK) >> \
        PCM_PHY_ID_BUS_LOWER_SHIFT))
#define PCM_PHY_ID_PHY_ADDR(_id)     ((_id) & 0x1F)
#define PCM_PHY_ID_IS_INTERNAL_BUS(_id)  ((_id) & (1 << 7))

#define PCM_PHY_ID_PHY_ADDR_SET(phy_addr, id)               (((phy_addr) & ~(0x1F)) | ((id) & 0x1F))
#define PCM_PHY_ID_INTERNAL_BUS_SET(phy_addr, internal)     (((phy_addr) & ~(1 << 7)) | (((internal) & 0x1) << 7))
#define PCM_PHY_ID_BUS_NUM_SET(phy_addr, bus_num)           (((phy_addr) & ~((PCM_PHY_ID_BUS_UPPER_MASK >> PCM_PHY_ID_BUS_UPPER_SHIFT) | (PCM_PHY_ID_BUS_LOWER_MASK >> PCM_PHY_ID_BUS_LOWER_SHIFT))) \
                                                              | ((((bus_num) << PCM_PHY_ID_BUS_UPPER_SHIFT) & PCM_PHY_ID_BUS_UPPER_MASK) | (((bus_num) << PCM_PHY_ID_BUS_LOWER_SHIFT) & PCM_PHY_ID_BUS_LOWER_MASK)))

#define PCM_PHY_INTERNAL        0x1
#define PCM_PHY_EXTERNAL        0x0
#define PCM_EEE_NONE            0x0
#define PCM_EEE_802_3           0x1
#define PCM_EEE_AUTO_GREEN      0x2

/*! Timesync control types. */
typedef enum pcm_phy_timesync_ctrl_e {
    PCM_PHY_TIMESYNC_TIMESTAMP_OFFSET,
    PCM_PHY_TIMESYNC_TIMESTAMP_ADJUST,
    PCM_PHY_TIMESYNC_COUNT
} pcm_phy_timesync_ctrl_t;

/*! Timesync Tx information. */
typedef struct pcm_port_timesync_tx_info_s {
    /*! Low 32bit of Timestamp in FIFO. */
    uint32 timestamps_in_fifo;

    /*! High 32 bit of timestamp in FIFO. */
    uint32 timestamps_in_fifo_hi;

    /*! Sequence id of sent PTP packet. */
    uint32 sequence_id;

    /*! Timestamp sub-nanosecond */
    uint32 sub_ns;
} pcm_port_timesync_tx_info_t;

/*! Timesync Tx information. */
typedef enum pcm_port_control_e {
    /*! Enable controls frames on port. */
    pcmPortControlPassControlFrames,

    /*! Enable/disable spanning tree and Vlan membership checks on ingress
        Ethernet and higig packets. */
    pcmPortControlDoNotCheckVlan
} pcm_port_control_t ;


extern sys_error_t pcm_software_init(int unit);
extern sys_error_t pcm_port_config_init(int unit);
extern sys_error_t pcm_phy_reg_sbus_get(int unit, uint32 phy_addr, uint32 phy_reg, uint32 *phy_data);
extern sys_error_t pcm_phy_reg_sbus_set(int unit, uint32 phy_addr, uint32 phy_reg, uint32 phy_data);
extern sys_error_t pcm_port_probe_init(int unit, pbmp_t lpbmp, pbmp_t *okay_lpbmp);
extern sys_error_t pcm_port_probe(int unit, pbmp_t pbmp, pbmp_t *okay_pbmp);
extern sys_error_t pcm_port_reinit(int unit, int port);
extern sys_error_t pcm_port_loopback_set(int unit, int port, int loopback);
extern sys_error_t pcm_port_loopback_get(int unit, int port, int *loopback);
extern sys_error_t pcm_port_control_get(int unit, int lport, int type, int *value);
extern sys_error_t pcm_port_control_set(int unit, int lport, int type, int value);
extern sys_error_t pcm_port_speed_set(int unit, int port, int speed);
extern sys_error_t pcm_port_speed_get(int unit, int port, int *speed);
extern sys_error_t pcm_port_link_status_get(int unit, int lport, int *up);
extern sys_error_t pcm_port_autoneg_get(int unit, int lport, int *autoneg);
extern sys_error_t pcm_port_autoneg_set(int unit, int port, int autoneg);
extern sys_error_t pcm_port_pause_set(int unit, int port, int pause_tx, int pause_rx);
extern sys_error_t pcm_port_pause_get(int unit, int lport, int *pause_tx, int *pause_rx);
extern sys_error_t pcm_port_duplex_set(int unit, int port, int duplex);
extern sys_error_t pcm_port_duplex_get(int unit, int lport, int *duplex);
extern sys_error_t pcm_port_enable_set(int unit, int port, int enable);
extern sys_error_t pcm_port_enable_get(int unit, int port, int *enable);
extern sys_error_t pcm_port_cable_diag(int unit, int lport, pcm_port_cable_diag_t* status);
extern sys_error_t pcm_port_cable_diag_support(int unit, int lport, int *support);
extern sys_error_t pcm_port_notify(int unit, int lport, int event, int value);
extern sys_error_t pcm_port_update(int unit, int lport, int link);
extern sys_error_t pcm_port_ability_advert_set(int unit, int lport, pcm_port_ability_t *adv);
extern sys_error_t pcm_port_ability_remote_get(int unit, int lport, pcm_port_ability_t *adv);
extern sys_error_t pcm_port_ability_get(int unit, int lport, pcm_port_ability_t *adv);
extern const char *pcm_phy_driver_name_get(int unit, int lport, uint32 index);
extern sys_error_t pcm_phy_addr_get(int unit, int lport, uint32 *iaddr, uint32 *addr);
extern sys_error_t pcm_phy_reg_set(int unit, int lport, uint32 index, uint32 phy_reg_addr, uint32 phy_data);
extern sys_error_t pcm_phy_reg_get(int unit, int lport, uint32 index, uint32 phy_reg_addr, uint32 *phy_data);
extern sys_error_t pcm_port_eee_enable_set(int unit, int lport, int enable, int *mode);
extern sys_error_t pcm_port_interface_set(int unit, int lport, int intf);
extern sys_error_t pcm_port_interface_get(int unit, int lport, int *intf);
extern sys_error_t pcm_port_pause_addr_get(int unit, int port, sal_mac_addr_t mac);
extern sys_error_t pcm_port_pause_addr_set(int unit, int port, sal_mac_addr_t mac);
extern sys_error_t pcm_port_frame_max_get(int unit, int port, int *size);
extern sys_error_t pcm_port_frame_max_set(int unit, int port, int size);
extern sys_error_t pcm_port_ifg_get(int unit, int port, int *ifg);
extern sys_error_t pcm_port_ifg_set(int unit, int port, int ifg);
extern sys_error_t pcm_port_class_get(int unit, int lport, port_class_t pclass, uint32 *pclass_id);
extern sys_error_t pcm_port_class_set(int unit, int lport, port_class_t pclass, uint32 pclass_id);
extern sys_error_t pcm_phy_synce_clock_get(int unit, int lport, uint32 *mode0, uint32 *mode1, uint32 *sdm_value);
extern sys_error_t pcm_phy_synce_clock_set(int unit, int lport, uint32 mode0, uint32 mode1, uint32 sdm_value);
extern sys_error_t pcm_phy_timesync_enable_get(int unit, int lport, int *en);
extern sys_error_t pcm_phy_timesync_enable_set(int unit, int lport, int en);
extern sys_error_t pcm_phy_timesync_ctrl_get(int unit, int lport, pcm_phy_timesync_ctrl_t type, uint64 *value);
extern sys_error_t pcm_phy_timesync_ctrl_set(int unit, int lport, pcm_phy_timesync_ctrl_t type, uint64 value);
extern sys_error_t pcm_port_timesync_tx_info_get(int unit, int lport, pcm_port_timesync_tx_info_t *info);
extern sys_error_t pcm_port_fault_status_get(int unit, int lport, board_port_fault_st_t *st);
extern sys_error_t pcm_port_fault_ctrl_get(int unit, int lport, board_port_fault_ctrl_t *ctrl);
extern sys_error_t pcm_port_fault_ctrl_set(int unit, int lport, board_port_fault_ctrl_t *ctrl);
extern sys_error_t pcm_phy_fec_mode_set(int unit, int lport, board_phy_fec_mode_t mode);
extern sys_error_t pcm_phy_fec_mode_get(int unit, int lport, board_phy_fec_mode_t *mode);
extern sys_error_t pcm_phy_fec_status_get(int unit, int lport, board_phy_fec_st_t *st);
extern sys_error_t pcm_phy_power_set(int unit, int lport, int power);
extern sys_error_t pcm_phy_power_get(int unit, int lport, int *power);
extern sys_error_t pcm_phy_tx_ctrl_set(int unit, int lport, board_phy_tx_ctrl_t *ctrl);
extern sys_error_t pcm_phy_tx_ctrl_get(int unit, int lport, board_phy_tx_ctrl_t *ctrl);
extern sys_error_t pcm_phy_rx_status_get(int unit, int lport, board_phy_rx_st_t *st);
extern sys_error_t pcm_phy_prbs_ctrl_set(int unit, int lport, int flags, board_phy_prbs_ctrl_t *ctrl);
extern sys_error_t pcm_phy_prbs_ctrl_get(int unit, int lport, int flags, board_phy_prbs_ctrl_t *ctrl);
extern sys_error_t pcm_phy_prbs_enable_set(int unit, int lport, int flags, int en);
extern sys_error_t pcm_phy_prbs_enable_get(int unit, int lport, int flags, int *en);
extern sys_error_t pcm_phy_prbs_status_get(int unit, int lport, board_phy_prbs_st_t *st);
extern sys_error_t pcm_phy_stats_get(int unit, int lport, board_phy_stats_t *stats);
extern void pcm_port_link_scan(void *param);

extern pbmp_t phy_external_mode;

#endif /* _PCM_H_ */