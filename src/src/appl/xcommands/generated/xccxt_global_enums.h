/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _XCCXT_GLOBAL_ENUMS_H_
#define _XCCXT_GLOBAL_ENUMS_H_

#include "system.h"

#ifdef CFG_XCOMMAND_INCLUDED

/*
 * All possible command paths
 */
enum {

    /* show system  */
    XCPATH_GLOBAL_H2_SHOW_SYSTEM,

    /* enable block_unknown_multicast  */
    XCPATH_GLOBAL_H2_ENABLE_BLOCK_UNKNOWN_MULTICAST,

    /* disable igmpsnoop  */
    XCPATH_GLOBAL_H2_DISABLE_IGMPSNOOP,

    /* config igmpsnoop version <VER_ID:integer>  */
    XCPATH_GLOBAL_H3_CONFIG_IGMPSNOOP_VERSION,

    /* disable mirror  */
    XCPATH_GLOBAL_H2_DISABLE_MIRROR,

    /* config mirror destination_port <PORT_ID:integer>  */
    XCPATH_GLOBAL_H3_CONFIG_MIRROR_DESTINATION_PORT,

    /* enable mirror  */
    XCPATH_GLOBAL_H2_ENABLE_MIRROR,

    /* config mirror source_port_list <PORT_LIST>  */
    XCPATH_GLOBAL_H3_CONFIG_MIRROR_SOURCE_PORT_LIST,

    /* enable igmpsnoop vlan <VID:integer>  */
    XCPATH_GLOBAL_H2_ENABLE_IGMPSNOOP,

    /* exit  */
    XCPATH_GLOBAL_H1_EXIT,

    /* config port <PORT_ID:integer> <PORT_NAME:line>  */
    XCPATH_GLOBAL_H2_CONFIG_PORT,

    /* disable block_unknown_multicast  */
    XCPATH_GLOBAL_H2_DISABLE_BLOCK_UNKNOWN_MULTICAST,

    /* config igmpsnoop all  */
    XCPATH_GLOBAL_H3_CONFIG_IGMPSNOOP_ALL,

    /* config igmpsnoop query_interval <SECS:integer>  */
    XCPATH_GLOBAL_H3_CONFIG_IGMPSNOOP_QUERY_INTERVAL,

    /* show counter  */
    XCPATH_GLOBAL_H2_SHOW_COUNTER,

    /* config igmpsnoop robustness_variable <TIMES:integer>  */
    XCPATH_GLOBAL_H3_CONFIG_IGMPSNOOP_ROBUSTNESS_VARIABLE,

};


/*
 * Variant parameter IDs for command "enable"
 */
enum {
    XCID_GLOBAL_ENABLE_VID,
};


/*
 * Variant parameter IDs for command "config"
 */
enum {
    XCID_GLOBAL_CONFIG_PORT_ID,
    XCID_GLOBAL_CONFIG_PORT_NAME,
    XCID_GLOBAL_CONFIG_VER_ID,
    XCID_GLOBAL_CONFIG_SECS,
    XCID_GLOBAL_CONFIG_TIMES,
    XCID_GLOBAL_CONFIG_PORT_LIST,
};


/*
 * All callbacks function that should be implemented
 */
extern XCMD_ERROR xchandler_global_exit(int, XCMD_HANDLE);
extern XCMD_ERROR xchandler_global_disable(int, XCMD_HANDLE);
extern XCMD_ERROR xchandler_global_enable(int, XCMD_HANDLE);
extern XCMD_ERROR xchandler_global_show(int, XCMD_HANDLE);
extern XCMD_ERROR xchandler_global_config(int, XCMD_HANDLE);
extern XCMD_ACTION xcbuilder_global_disable(int, unsigned int, XCMD_HANDLE);
extern XCMD_ACTION xcbuilder_global_enable(int, unsigned int, XCMD_HANDLE);
extern XCMD_ACTION xcbuilder_global_config(int, unsigned int, XCMD_HANDLE);

#endif /* CFG_XCOMMAND_INCLUDED */

#endif /* _XCCXT_GLOBAL_ENUMS_H_ */
