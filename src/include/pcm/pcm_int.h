/*! \file pcm_int.h
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _PCM_INT_H_
#define _PCM_INT_H_




#define BMD_PHY_M_EEE_OFF       0
#define BMD_PHY_M_EEE_802_3     1
#define BMD_PHY_M_EEE_AUTO      2

int bmd_phy_eee_set(int unit, int lport, int mode);

typedef struct {   
	const char * (*phy_driver_name_get) (int , int, uint32);
	int (*phy_reg_get) (int , int,  uint32 , uint32 , uint32* );
	int (*phy_reg_set) (int , int,  uint32 , uint32 , uint32 );
	int (*phy_addr_get) (int , int,  uint32 *, uint32 *);
	int (*phy_cable_diag)(int , int , pcm_port_cable_diag_t *);
	int (*phy_cable_diag_support)(int , int , int *);
    int (*software_init) (int);
    int (*port_probe_init) (int, pbmp_t, pbmp_t *);
	int (*port_loopback_set) (int , int , int);
	int (*port_loopback_get) (int , int , int *);
	int (*port_control_get)  (int , int, int , int *);
	int (*port_control_set) (int , int , int , int );
	int (*port_speed_set)(int , int , int );
	int (*port_speed_get)(int , int , int *);
	int (*port_interface_set)(int , int , pcm_port_if_t );
	int (*port_interface_get)(int , int , pcm_port_if_t *);
	int (*port_ability_remote_get)(int , int , pcm_port_ability_t *);
	int (*port_ability_local_get)(int , int, pcm_port_ability_t *);
	int (*port_ability_advert_set)(int , int , pcm_port_ability_t *);
	int (*port_link_status_get) (int, int, int *);
	int (*port_autoneg_get)(int , int, int *);
	int (*port_autoneg_set)(int , int, int );
	int (*port_pause_set)(int , int, int , int );
	int (*port_pause_get)(int , int , int *, int *);
	int (*port_duplex_set)(int , int , int );
	int (*port_duplex_get)(int , int , int *);
	int (*port_enable_set)(int , int , int );
	int (*port_enable_get)(int , int , int *);
	int (*port_eee_enable_set) (int unit, int port, int enable, int *mode);
	int (*port_notify)(int , int , int , uint32 );
    int (*port_update) (int , int, int);
    int (*port_pause_addr_get) (int unit, int port, sal_mac_addr_t mac);
    int (*port_pause_addr_set) (int unit, int port, sal_mac_addr_t mac);
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
extern int pcm_phyctrl_port_interface_set(int unit, int lport, pcm_port_if_t intf);
extern int pcm_phyctrl_port_interface_get(int unit, int port, pcm_port_if_t *intf);
extern int pcm_phyctrl_port_autoneg_get(int unit, int lport, int *autoneg);
extern int pcm_phyctrl_port_autoneg_set(int unit, int lport, int autoneg);
extern int pcm_phyctrl_port_pause_set(int unit, int lport, int pause_tx, int pause_rx);
extern int pcm_phyctrl_port_pause_get(int unit, int lport, int *pause_tx, int *pause_rx);
extern int pcm_phyctrl_port_duplex_get(int unit, int lport, int *duplex);
extern int pcm_phyctrl_port_duplex_set(int unit, int lport, int duplex);
extern int pcm_phyctrl_port_enable_set(int unit, int lport, int enable);
extern int pcm_phyctrl_port_enable_get(int unit, int lport, int *enable);
extern int pcm_phyctrl_port_loopback_set(int unit, int lport, int loopback);
extern int pcm_phyctrl_port_loopback_get(int unit, int lport, int *loopback);
extern int pcm_phyctrl_port_control_get(int unit, int lport, int type, int *value);
extern int pcm_phyctrl_port_control_set(int unit, int lport, int type, int value);
extern int pcm_phyctrl_port_update(int unit, int lport, int link);
extern int pcm_phyctrl_phy_cable_diag(int unit, int lport, pcm_port_cable_diag_t *status);
extern int pcm_phyctrl_phy_cable_diag_support(int unit, int lport, int *support);
extern int pcm_phyctrl_phy_reg_get(int unit, int lport, uint32 index, uint32 phy_reg_addr, uint32 *data);
extern int pcm_phyctrl_phy_reg_set(int unit, int lport, uint32 flags, uint32 phy_reg_addr, uint32 data);
extern const char * pcm_phyctrl_phy_get_driver_name(int unit, int lport, uint32 internal);
extern int pcm_phyctrl_phy_addr_get(int unit, int lport, uint32 *iaddr, uint32 *addr);
extern int pcm_phyctrl_port_eee_enable_set(int unit, int lport, int enable, int* mode);
extern int pcm_phyctrl_port_probe_init(int unit, pbmp_t lpbmp, pbmp_t *okay_lpbmp);
extern int pcm_phyctrl_port_ability_local_get(int unit, int lport, pcm_port_ability_t *ability_mask);
extern int pcm_phyctrl_port_ability_remote_get(int unit, int lport, pcm_port_ability_t *ability_mask);
extern int pcm_phyctrl_port_ability_advert_set(int unit, int lport, pcm_port_ability_t *ability_mask);
extern int pcm_phyctrl_port_speed_set(int unit, int lport, int speed);
extern int pcm_phyctrl_port_speed_get(int unit, int lport, int *speed);
extern int pcm_phyctrl_port_link_status_get(int unit, int lport, int *up);
extern int pcm_phyctrl_port_pause_addr_get(int unit, int port, sal_mac_addr_t mac);
extern int pcm_phyctrl_port_pause_addr_set(int unit, int port, sal_mac_addr_t mac);
extern int pcm_phyctrl_phy_notify(int unit, int port, int event, uint32 value);

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

