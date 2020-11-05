/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_VLAN_H_
#define _BOARDAPI_VLAN_H_

#ifdef CFG_SWITCH_VLAN_INCLUDED
/*
 * Select vlan types
 */
extern sys_error_t board_vlan_type_set(vlan_type_t type);
extern sys_error_t board_vlan_type_get(vlan_type_t *type);

/*
 * VLAN creation/deletion
 */
extern sys_error_t board_vlan_create(uint16 vlan_id) REENTRANT;
extern sys_error_t board_vlan_destroy(uint16 vlan_id) REENTRANT;

/*
 * Port-base VLAN port set/get
 */
extern sys_error_t board_pvlan_port_set(uint16 vlan_id, uint8 *uplist);
extern sys_error_t board_pvlan_port_get(uint16 vlan_id, uint8 *uplist);

/*
 * Port-base VLAN egress ports get
 */
extern sys_error_t board_pvlan_egress_get(uint16 uport, uint8 *uplist);

/*
 * Port default VLAN set/get
 */
extern sys_error_t board_untagged_vlan_set(uint16 uport, uint16 vlan_id) REENTRANT;
extern sys_error_t board_untagged_vlan_get(uint16 uport, uint16 *vlan_id) REENTRANT;
                                           
/*
 * 802.1Q VLAN port set/get
 */
extern sys_error_t board_qvlan_port_set(uint16 vlan_id, uint8 *uplist, 
                                      uint8 *tag_uplist);
extern sys_error_t board_qvlan_port_get(uint16 vlan_id, uint8 *uplist, 
                                      uint8 *tag_uplist);

/*
 * retrieve total vlan numbers
 */
extern uint16 board_vlan_count(void) REENTRANT;

/*
 * retrieve vlan members with index
 */
extern sys_error_t board_qvlan_get_by_index(uint16  index, uint16 *vlan_id, uint8 *uplist, uint8 *tag_uplist, BOOL get_uplist) REENTRANT;

#endif  /* CFG_SWITCH_VLAN_INCLUDED */

#endif /* _BOARDAPI_VLAN_H_ */
