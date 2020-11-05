/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_MCAST_H_
#define _BOARDAPI_MCAST_H_

extern sys_error_t board_mcast_addr_add(uint8 *mac_addr,
                                        uint16 vlan_id, uint8 *uplist);
extern sys_error_t board_mcast_addr_remove(uint8 *mac_addr,
                                           uint16 vlan_id);
extern sys_error_t board_mcast_port_add(uint8 *mac_addr, uint16 vlan_id,
                                        uint16 uport);
extern sys_error_t board_mcast_port_remove(uint8 *mac_addr, uint16 vlan_id,
                                           uint16 uport);
extern sys_error_t board_mcast_port_get(uint8 *mac_addr, uint16 vlan_id,
                                        uint8 *uplist);

extern sys_error_t board_igmp_snoop_enable_set(uint8 enable) REENTRANT;
extern sys_error_t board_igmp_snoop_enable_get(uint8 *enable) REENTRANT;

extern sys_error_t board_block_unknown_mcast_set(uint8 enable) REENTRANT;
extern sys_error_t board_block_unknown_mcast_get(uint8 *enable) REENTRANT;

#endif /* _BOARDAPI_MCAST_H_ */

