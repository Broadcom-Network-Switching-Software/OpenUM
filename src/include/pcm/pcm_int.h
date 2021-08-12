/*! \file pcm_int.h
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _PCM_INT_H_
#define _PCM_INT_H_

#define BMD_PHY_M_EEE_OFF       0
#define BMD_PHY_M_EEE_802_3     1
#define BMD_PHY_M_EEE_AUTO      2

int bmd_phy_eee_set(int unit, int lport, int mode);

typedef struct {
    const char * (*phy_driver_name_get) (int , int, uint32);
    sys_error_t (*phy_reg_get) (int , int,  uint32 , uint32 , uint32* );
    sys_error_t (*phy_reg_set) (int , int,  uint32 , uint32 , uint32 );
    sys_error_t (*phy_addr_get) (int , int,  uint32 *, uint32 *);
    sys_error_t (*phy_cable_diag)(int , int , pcm_port_cable_diag_t *);
    sys_error_t (*phy_cable_diag_support)(int , int , int *);
    sys_error_t (*software_init) (int);
    sys_error_t (*phy_reg_sbus_get)(int , uint32 , uint32 , uint32*);
    sys_error_t (*phy_reg_sbus_set)(int , uint32 , uint32 , uint32);
    sys_error_t (*port_probe_init) (int, pbmp_t, pbmp_t *);
    sys_error_t (*port_loopback_set) (int , int , int);
    sys_error_t (*port_loopback_get) (int , int , int *);
    sys_error_t (*port_control_get)  (int , int, int , int *);
    sys_error_t (*port_control_set) (int , int , int , int);
    sys_error_t (*port_speed_set)(int , int , int);
    sys_error_t (*port_speed_get)(int , int , int *);
    sys_error_t (*port_interface_set)(int , int , int);
    sys_error_t (*port_interface_get)(int , int , int *);
    sys_error_t (*port_ability_remote_get)(int , int , pcm_port_ability_t *);
    sys_error_t (*port_ability_local_get)(int , int, pcm_port_ability_t *);
    sys_error_t (*port_ability_advert_set)(int , int , pcm_port_ability_t *);
    sys_error_t (*port_link_status_get) (int, int, int *);
    sys_error_t (*port_autoneg_get)(int , int, int *);
    sys_error_t (*port_autoneg_set)(int , int, int);
    sys_error_t (*port_pause_set)(int , int, int , int);
    sys_error_t (*port_pause_get)(int , int , int *, int *);
    sys_error_t (*port_duplex_set)(int , int , int);
    sys_error_t (*port_duplex_get)(int , int , int *);
    sys_error_t (*port_enable_set)(int , int , int);
    sys_error_t (*port_enable_get)(int , int , int *);
    sys_error_t (*port_eee_enable_set) (int unit, int port, int enable, int *mode);
    sys_error_t (*port_notify)(int , int , int , uint32 );
    sys_error_t (*port_update) (int , int, int);
    sys_error_t (*port_pause_addr_get) (int unit, int port, sal_mac_addr_t mac);
    sys_error_t (*port_pause_addr_set) (int unit, int port, sal_mac_addr_t mac);
    sys_error_t (*port_frame_max_get) (int, int, int *);
    sys_error_t (*port_frame_max_set) (int, int, int);
    sys_error_t (*port_ifg_get) (int, int, int *);
    sys_error_t (*port_ifg_set) (int, int, int);
    sys_error_t (*port_class_get) (int, int, port_class_t, uint32 *);
    sys_error_t (*port_class_set) (int, int, port_class_t, uint32);
    sys_error_t (*port_reinit) (int, int);
    sys_error_t (*phy_synce_clock_get)(int unit, int lport, uint32 *mode0, uint32 *mode1, uint32 *sdm_value);
    sys_error_t (*phy_synce_clock_set)(int unit, int lport, uint32 mode0, uint32 mode1, uint32 sdm_value);
    sys_error_t (*phy_timesync_enable_get)(int unit, int lport, int *en);
    sys_error_t (*phy_timesync_enable_set)(int unit, int lport, int en);
    sys_error_t (*phy_timesync_ctrl_get)(int unit, int lport, pcm_phy_timesync_ctrl_t type, uint64 *value);
    sys_error_t (*phy_timesync_ctrl_set)(int unit, int lport, pcm_phy_timesync_ctrl_t type, uint64 value);
    sys_error_t (*port_timesync_tx_info_get)(int unit, int lport, pcm_port_timesync_tx_info_t *info);
    sys_error_t (*port_fault_status_get)(int unit, int lport, board_port_fault_st_t *st);
    sys_error_t (*port_fault_ctrl_get)(int unit, int lport, board_port_fault_ctrl_t *ctrl);
    sys_error_t (*port_fault_ctrl_set)(int unit, int lport, board_port_fault_ctrl_t *ctrl);
    sys_error_t (*phy_fec_mode_set)(int unit, int lport, board_phy_fec_mode_t mode);
    sys_error_t (*phy_fec_mode_get)(int unit, int lport, board_phy_fec_mode_t *mode);
    sys_error_t (*phy_fec_status_get)(int unit, int lport, board_phy_fec_st_t *st);
    sys_error_t (*phy_power_set)(int unit, int lport, int power);
    sys_error_t (*phy_power_get)(int unit, int lport, int *power);
    sys_error_t (*phy_tx_ctrl_set)(int unit, int lport, board_phy_tx_ctrl_t *ctrl);
    sys_error_t (*phy_tx_ctrl_get)(int unit, int lport, board_phy_tx_ctrl_t *ctrl);
    sys_error_t (*phy_rx_status_get)(int unit, int lport, board_phy_rx_st_t *st);
    sys_error_t (*phy_prbs_ctrl_set)(int unit, int lport, int flags, board_phy_prbs_ctrl_t *ctrl);
    sys_error_t (*phy_prbs_ctrl_get)(int unit, int lport, int flags, board_phy_prbs_ctrl_t *ctrl);
    sys_error_t (*phy_prbs_enable_set)(int unit, int lport, int flags, int en);
    sys_error_t (*phy_prbs_enable_get)(int unit, int lport, int flags, int *en);
    sys_error_t (*phy_prbs_status_get)(int unit, int lport, board_phy_prbs_st_t *st);
    sys_error_t (*phy_stats_get)(int unit, int lport, board_phy_stats_t *stats);
    void (*port_link_scan)(void *param);
} pcm_port_funtion_t;

typedef struct {
    const pcm_port_funtion_t *f;
} pcm_port_ctrl_t;

extern pcm_port_ctrl_t pcm_port_ctrl[SOC_MAX_NUM_PORTS];
extern const pcm_port_funtion_t pcm_phyctrl;
extern const pcm_port_funtion_t mdk_port;

extern int _firmware_helper(void *ctx, uint32 offset, uint32 size, void *data);

extern void  bsl_debug_init(void);

/* PCM-PHYCTRL port API */
extern sys_error_t pcm_phyctrl_port_interface_set(int unit, int lport, int intf);
extern sys_error_t pcm_phyctrl_port_interface_get(int unit, int port, int *intf);
extern sys_error_t pcm_phyctrl_port_autoneg_get(int unit, int lport, int *autoneg);
extern sys_error_t pcm_phyctrl_port_autoneg_set(int unit, int lport, int autoneg);
extern sys_error_t pcm_phyctrl_port_duplex_get(int unit, int lport, int *duplex);
extern sys_error_t pcm_phyctrl_port_duplex_set(int unit, int lport, int duplex);
extern sys_error_t pcm_phyctrl_port_enable_set(int unit, int lport, int enable);
extern sys_error_t pcm_phyctrl_port_enable_get(int unit, int lport, int *enable);
extern sys_error_t pcm_phyctrl_port_loopback_set(int unit, int lport, int loopback);
extern sys_error_t pcm_phyctrl_port_loopback_get(int unit, int lport, int *loopback);
extern sys_error_t pcm_phyctrl_port_control_get(int unit, int lport, int type, int *value);
extern sys_error_t pcm_phyctrl_port_control_set(int unit, int lport, int type, int value);
extern sys_error_t pcm_phyctrl_port_update(int unit, int lport, int link);
extern sys_error_t pcm_phyctrl_phy_cable_diag(int unit, int lport, pcm_port_cable_diag_t *status);
extern sys_error_t pcm_phyctrl_phy_cable_diag_support(int unit, int lport, int *support);
extern sys_error_t pcm_phyctrl_phy_reg_get(int unit, int lport, uint32 index, uint32 phy_reg_addr, uint32 *data);
extern sys_error_t pcm_phyctrl_phy_reg_set(int unit, int lport, uint32 flags, uint32 phy_reg_addr, uint32 data);
extern const char * pcm_phyctrl_phy_get_driver_name(int unit, int lport, uint32 internal);
extern sys_error_t pcm_phyctrl_phy_addr_get(int unit, int lport, uint32 *iaddr, uint32 *addr);
extern sys_error_t pcm_phyctrl_port_eee_enable_set(int unit, int lport, int enable, int* mode);
extern sys_error_t pcm_phyctrl_port_probe_init(int unit, pbmp_t lpbmp, pbmp_t *okay_lpbmp);
extern sys_error_t pcm_phyctrl_port_reinit(int unit, int lport);
extern sys_error_t pcm_phyctrl_port_ability_local_get(int unit, int lport, pcm_port_ability_t *ability_mask);
extern sys_error_t pcm_phyctrl_port_ability_remote_get(int unit, int lport, pcm_port_ability_t *ability_mask);
extern sys_error_t pcm_phyctrl_port_ability_advert_set(int unit, int lport, pcm_port_ability_t *ability_mask);
extern sys_error_t pcm_phyctrl_port_speed_set(int unit, int lport, int speed);
extern sys_error_t pcm_phyctrl_port_speed_get(int unit, int lport, int *speed);
extern sys_error_t pcm_phyctrl_port_link_status_get(int unit, int lport, int *up);
extern sys_error_t pcm_phyctrl_port_pause_addr_get(int unit, int lport, sal_mac_addr_t mac);
extern sys_error_t pcm_phyctrl_port_pause_addr_set(int unit, int lport, sal_mac_addr_t mac);
extern sys_error_t pcm_phyctrl_phy_notify(int unit, int port, int event, uint32 value);
extern sys_error_t pcm_phyctrl_phy_timesync_enable_get(int unit, int lport, int *en);
extern sys_error_t pcm_phyctrl_phy_timesync_enable_set(int unit, int lport, int en);
extern sys_error_t pcm_phyctrl_phy_timesync_ctrl_get(int unit, int lport, pcm_phy_timesync_ctrl_t type, uint64 *value);
extern sys_error_t pcm_phyctrl_phy_timesync_ctrl_set(int unit, int lport, pcm_phy_timesync_ctrl_t type, uint64 value);
extern sys_error_t pcm_portctrl_port_ifg_set(int unit, int lport, int ifg);
extern sys_error_t pcm_portctrl_port_ifg_get(int unit, int lport, int *ifg);
extern sys_error_t pcm_portctrl_port_frame_max_set(int unit, int lport, int size);
extern sys_error_t pcm_portctrl_port_frame_max_get(int unit, int lport, int *size);
extern sys_error_t pcm_portctrl_port_pause_set(int unit, int lport, int pause_tx, int pause_rx);
extern sys_error_t pcm_portctrl_port_pause_get(int unit, int lport, int *pause_tx, int *pause_rx);
extern sys_error_t pcm_phyctrl_port_fault_status_get(int unit, int lport, board_port_fault_st_t *st);
extern sys_error_t pcm_phyctrl_phy_fec_status_get(int unit, int lport, board_phy_fec_st_t *st);
extern sys_error_t pcm_phyctrl_phy_power_set(int unit, int lport, int power);
extern sys_error_t pcm_phyctrl_phy_power_get(int unit, int lport, int *power);
extern sys_error_t pcm_phyctrl_phy_tx_ctrl_set(int unit, int lport, board_phy_tx_ctrl_t *ctrl);
extern sys_error_t pcm_phyctrl_phy_tx_ctrl_get(int unit, int lport, board_phy_tx_ctrl_t *ctrl);
extern sys_error_t pcm_phyctrl_phy_rx_status_get(int unit, int lport, board_phy_rx_st_t *st);
extern sys_error_t pcm_phyctrl_phy_prbs_ctrl_set(int unit, int lport, int flags, board_phy_prbs_ctrl_t *ctrl);
extern sys_error_t pcm_phyctrl_phy_prbs_ctrl_get(int unit, int lport, int flags, board_phy_prbs_ctrl_t *ctrl);
extern sys_error_t pcm_phyctrl_phy_prbs_enable_set(int unit, int lport, int flags, int en);
extern sys_error_t pcm_phyctrl_phy_prbs_enable_get(int unit, int lport, int flags, int *en);
extern sys_error_t pcm_phyctrl_phy_prbs_status_get(int unit, int lport, board_phy_prbs_st_t *st);

/* BMDPC port API */
extern int bmdpc_software_init(int unit);
extern const char *bmdpc_phy_driver_name_get(int unit, int lport, uint32 flags);
extern int bmdpc_port_interface_set(int unit, int lport, pcm_port_if_t intf);
extern int bmdpc_port_interface_get(int unit, int port, pcm_port_if_t *intf);
extern int bmdpc_port_autoneg_get(int unit, int lport, int *autoneg);
extern int bmdpc_port_autoneg_set(int unit, int lport, int autoneg);
extern int bmdpc_port_pause_set(int unit, int lport, int pause_tx, int pause_rx);
extern int bmdpc_port_pause_get(int unit, int lport, int *pause_tx, int *pause_rx);
extern int bmdpc_port_duplex_get(int unit, int lport, int *duplex);
extern int bmdpc_port_duplex_set(int unit, int lport, int duplex);
extern int bmdpc_port_enable_set(int unit, int lport, int enable);
extern int bmdpc_port_enable_get(int unit, int lport, int *enable);
extern int bmdpc_port_loopback_set(int unit, int lport, int loopback);
extern int bmdpc_port_loopback_get(int unit, int lport, int *loopback);
extern int bmdpc_port_control_get(int unit, int lport, int type, int *value);
extern int bmdpc_port_control_set(int unit, int lport, int type, int value);
extern int bmdpc_port_update(int unit, int lport, int link);
extern int bmdpc_phy_cable_diag(int unit, int lport, pcm_port_cable_diag_t *status);
extern int bmdpc_phy_cable_diag_support(int unit, int lport, int *support);
extern int bmdpc_phy_reg_get(int unit, int lport, uint32 index, uint32 phy_reg_addr, uint32 *data);
extern int bmdpc_phy_reg_set(int unit, int lport, uint32 flags, uint32 phy_reg_addr, uint32 data);
extern const char * bmdpc_phy_get_driver_name(int unit, int lport, uint32 internal);
extern int bmdpc_phy_addr_get(int unit, int lport, uint32 *iaddr, uint32 *addr);
extern int bmdpc_port_eee_enable_set(int unit, int lport, int enable, int* mode);
extern int bmdpc_port_probe_init(int unit, pbmp_t lpbmp, pbmp_t *okay_lpbmp);
extern int bmdpc_port_ability_local_get(int unit, int lport, pcm_port_ability_t *ability_mask);
extern int bmdpc_port_ability_remote_get(int unit, int lport, pcm_port_ability_t *ability_mask);
extern int bmdpc_port_ability_advert_set(int unit, int lport, pcm_port_ability_t *ability_mask);
extern int bmdpc_port_speed_set(int unit, int lport, int speed);
extern int bmdpc_port_speed_get(int unit, int lport, int *speed);
extern const char *bmdpc_phy_driver_name_get(int unit, int lport, uint32 flags);
extern int bmdpc_port_link_status_get(int unit, int lport, int *up);
extern int bmdpc_port_cable_diag(int unit, int lport, pcm_port_cable_diag_t* status);
extern int bmdpc_port_cable_diag_support(int unit,int lport, int *support);

#endif
