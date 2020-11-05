/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _BRDIMPL_VLAN_H_
#define _BRDIMPL_VLAN_H_

#ifdef CFG_SWITCH_VLAN_INCLUDED
/*
 * SW VLAN shadow table :
 *      - Used to serve the Port-based and 1Q-based VLAN
 *
 * Note :
 *  1. This shadow table will be at reset status when VLAN type changed.
 *  2. Member ports is represented in logical port list format.
 */
typedef struct vlan_list_s {
    uint16  vlan_id;    /* be the VLAN group index in PVLAN and VID in QVLAN */
    uint8   uplist[MAX_UPLIST_WIDTH]; /* the member port list in this VLAN group */
    struct vlan_list_s *next;
} vlan_list_t;

/* Integrated VLAN SW information :
 *  - All related VLAN configuration or informaiton for both Port-based
 *      and 1Q-based VLAN.
 */
typedef struct vlan_info_s {
    vlan_type_t type;   /* the current working VLAN type */
    uint16      count;  /* the number of the created VLAN groups */
    uint16      max_count;  /* the allowed MAX VLAN groups */
    vlan_list_t *head; /* head of vlan list */
    vlan_list_t *tail; /* tail of vlan list */
} vlan_info_t;

#define BOARD_VLAN_GROUP_EXIST_CHK(_vlan_list)      ((_vlan_list) != NULL)

#define MAX_QVLAN_ID    4094

/*
 * VLAN reset
 */
extern sys_error_t brdimpl_vlan_reset(void) REENTRANT;

/*
 * VLAN type
 */
extern sys_error_t brdimpl_vlan_type_set(vlan_type_t type) REENTRANT;
extern sys_error_t brdimpl_vlan_type_get(vlan_type_t *type) REENTRANT;

/*
 * VLAN creation/deletion
 */
extern sys_error_t brdimpl_vlan_create(uint16 vlan_id) REENTRANT;
extern sys_error_t brdimpl_vlan_destroy(uint16 vlan_id) REENTRANT;

/*
 * Port-base VLAN port set/get
 */
extern sys_error_t brdimpl_pvlan_port_set(uint16  vlan_id,
                                          uint8 *uplist) REENTRANT;
extern sys_error_t brdimpl_pvlan_port_get(uint16  vlan_id,
                                          uint8 *uplist) REENTRANT;

                                          /*
 * Port-base VLAN egress ports get
 */
extern sys_error_t brdimpl_pvlan_egress_get(uint16 uport,
                                            uint8 *uplist) REENTRANT;

/*
 * 8021Q VLAN port set/get
 */
extern sys_error_t brdimpl_qvlan_port_set(uint16  vlan_id, uint8 *uplist,
                                           uint8 *taguplist) REENTRANT;
extern sys_error_t brdimpl_qvlan_port_get(uint16  vlan_id, uint8 *uplist,
                                           uint8 *taguplist) REENTRANT;

/*
 * retrieve total vlan numbers
 */
extern uint16 brdimpl_vlan_count(void) REENTRANT;


/*
 * retrieve vlan members with index
 */
extern sys_error_t brdimpl_pvlan_get_by_index(uint16  index, uint16 *vlan_id,
                                           uint8 *uplist) REENTRANT;
extern sys_error_t brdimpl_qvlan_get_by_index(uint16  index, uint16 *vlan_id,
                   uint8 *uplist, uint8 *taguplist, BOOL get_uplist) REENTRANT;

/* _brdimpl_vlan_init() is used for system bootup process only. */
extern sys_error_t _brdimpl_vlan_init(void) REENTRANT;
#ifdef BRD_VLAN_DEBUG
/* _brdimpl_dump_vlan_info() is used for debug purpose. */
extern void _brdimpl_dump_vlan_info(void) REENTRANT;
#endif  /* BRD_VLAN_DEBUG */

#endif  /* CFG_SWITCH_VLAN_INCLUDED */

#endif /* _BRDIMPL_VLAN_H_ */
