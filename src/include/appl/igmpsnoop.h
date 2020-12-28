/*
 * $Id: igmpsnoop.h,v 1.18 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _IGMP_H_
#define _IGMP_H_
#include "system.h"

typedef struct igmp_dbg_count_s {
    uint16  total_receive;
    uint16  report_v1;
    uint16  report_v2;
    uint16  report_v3;
    uint16  leave;
    uint16  query;
    uint16  unkonwn_igmp_type;
    uint16  unkonwn_igmpv3_record_type;
    uint16  non_igmp;
    uint16  relay_report_and_leave;
    uint16  relay_query;
    uint16  relay_specific_query;
    uint16  relay_fail;
    uint16  relay_success;
    uint16  database_update_fail;
    uint16  leave_q_operation_fail;
    uint16  invalid_vlan_group;
} igmp_dbg_count_t;

extern uint16 IGMPsnoop_vid;
extern void igmpsnoop_dbg_timer_get(uint16 *tick_timer, uint16 *router_port_tick_timer);
extern void igmpsnoop_dbg_count_get(igmp_dbg_count_t *count);
extern void igmpsnoop_dbg_count_reset(void);

extern BOOL igmpsnoop_enable_set(uint8 enable);
extern void igmpsnoop_enable_get(uint8 *enable);
extern void igmpsnoop_database_init(void);
extern void igmpsnoop_vid_get(uint16 *vid);
extern sys_error_t igmpsnoop_vid_set(uint16 vid);
extern uint16 igmpsnoop_get_vid_by_index(int16 index);

#if (CFG_CLI_ENABLED)
extern uint16 igmpsnoop_group_count_get(void);
extern BOOL igmpsnoop_group_member_get(uint16 index,
                    uint32 *gip, uint16 *vid, uint8 *uplist);
extern uint16 igmpsnoop_dynamic_router_port_count_get(void);
extern BOOL igmpsnoop_dynamic_router_port_get(uint8 index,
                              uint16 *vid, uint8 *uplist);
#endif /* (CFG_CLI_ENABLED) */

#endif /* _IGMP_H_ */
