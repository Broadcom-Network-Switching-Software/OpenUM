/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
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

extern int pcm_software_init(int unit);
extern int pcm_port_config_init(int unit);
extern int pcm_port_probe_init(int unit, pbmp_t lpbmp, pbmp_t *okay_lpbmp);
extern int pcm_port_probe(int unit, pbmp_t pbmp, pbmp_t *okay_pbmp);
extern int pcm_port_loopback_set(int unit, int port, int mode);
extern int pcm_port_control_get(int unit, int lport, int type, int *value);
extern int pcm_port_control_set(int unit, int lport, int type, int value);
extern int pcm_port_speed_set(int unit, int port, int speed);
extern int pcm_port_speed_get(int unit, int port, int *speed);
extern int pcm_port_link_status_get(int unit, int lport, int *up);
extern int pcm_port_autoneg_get(int unit, int lport, int *autoneg);
extern int pcm_port_autoneg_set(int unit, int port, int autoneg);
extern int pcm_port_pause_set(int unit, int port, int pause_tx, int pause_rx);
extern int pcm_port_pause_get(int unit, int lport, int *pause_tx, int *pause_rx);
extern int pcm_port_duplex_set(int unit, int port, int duplex);
extern int pcm_port_duplex_get(int unit, int lport, int *duplex);
extern int pcm_port_enable_set(int unit, int port, int enable);
extern int pcm_port_enable_get(int unit, int port, int *enable);
extern int pcm_port_cable_diag(int unit, int lport, pcm_port_cable_diag_t* status);
extern int pcm_port_cable_diag_support(int unit, int lport, int *support);
extern int pcm_port_notify(int unit, int lport, int event, int value);
extern int pcm_port_update(int unit, int lport, int link);
extern int pcm_port_ability_advert_set(int unit, int lport, pcm_port_ability_t *adv);
extern int pcm_port_ability_remote_get(int unit, int lport, pcm_port_ability_t *adv);
extern int pcm_port_ability_get(int unit, int lport, pcm_port_ability_t *adv);
extern const char *pcm_phy_driver_name_get(int unit, int lport, uint32 index);
extern int pcm_phy_addr_get(int unit, int lport, uint32 *iaddr, uint32 *addr);
extern int pcm_phy_reg_set(int unit, int lport, uint32 index, uint32 phy_reg_addr, uint32 phy_data);
extern int pcm_phy_reg_get(int unit, int lport, uint32 index, uint32 phy_reg_addr, uint32 *phy_data);
extern int pcm_port_eee_enable_set(int unit, int lport, int enable, int *mode);
extern int pcm_port_interface_set(int unit, int lport,  pcm_port_if_t intf);
extern int pcm_port_interface_get(int unit, int lport,  pcm_port_if_t *intf);
extern int pcm_port_pause_addr_get(int unit, int port, sal_mac_addr_t mac);
extern int pcm_port_pause_addr_set(int unit, int port, sal_mac_addr_t mac);


extern pbmp_t phy_external_mode;
#endif /* _PCM_H_ */
















