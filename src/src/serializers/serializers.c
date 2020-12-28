/*
 * $Id: serializers.c,v 1.26 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#include "appl/persistence.h"

#if CFG_PERSISTENCE_SUPPORT_ENABLED

/* Forwards */
extern void serializers_init(void);

#ifdef CFG_PERSISTENCE_STRING_SUPPORT_INCLUDED
/*
 * Convenient macro for string serializer
 */
#define DEF_STRING_SERL(name,group,func) do { \
    extern int32 func(SERIALIZE_OP op, SERL_MEDIUM_STR *medium) REENTRANT; \
    register_string_serializer(name, group, func, FALSE); \
} while(0)
#define DEF_STRING_SERL_WITH_DEFAULTS(name,group,func) do { \
    extern int32 func(SERIALIZE_OP op, SERL_MEDIUM_STR *medium) REENTRANT; \
    register_string_serializer(name, group, func, TRUE); \
} while(0)
#endif /* CFG_PERSISTENCE_STRING_SUPPORT_INCLUDED */

/*
 * Convenient macro for binary serializer
 */
#define DEF_BINARY_SERL(name,group,func) do { \
    extern int32 func(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT; \
    register_binary_serializer(name, group, func, FALSE); \
} while(0)
#define DEF_BINARY_SERL_WITH_DEFAULTS(name,group,func) do { \
    extern int32 func(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT; \
    register_binary_serializer(name, group, func, TRUE); \
} while(0)

/*
 * REGISTER YOUR SERIALIZERS HERE!
 */
void
serializers_init(void)
{
    /*
     * Switch features 
     */

#ifdef CFG_SWITCH_QOS_INCLUDED
    DEF_BINARY_SERL_WITH_DEFAULTS("qos", SERL_GRP_SWITCH, qos_serializer);
#endif /* CFG_SWITCH_QOS_INCLUDED */

#ifdef CFG_SWITCH_RATE_INCLUDED
    DEF_BINARY_SERL_WITH_DEFAULTS("rate", SERL_GRP_SWITCH, rate_serializer);
    DEF_BINARY_SERL_WITH_DEFAULTS("storm", SERL_GRP_SWITCH, storm_serializer);
#endif /* CFG_SWITCH_RATE_INCLUDED */

#ifdef CFG_SWITCH_MIRROR_INCLUDED
    DEF_BINARY_SERL_WITH_DEFAULTS("mirror", SERL_GRP_SWITCH, mirror_serializer);
#endif /* CFG_SWITCH_MIRROR_INCLUDED */

#ifdef CFG_SWITCH_VLAN_INCLUDED
    DEF_BINARY_SERL_WITH_DEFAULTS("vlan", SERL_GRP_SWITCH, vlan_serializer);
    DEF_BINARY_SERL_WITH_DEFAULTS("pvid", SERL_GRP_SWITCH, pvid_serializer);
#endif /* CFG_SWITCH_VLAN_INCLUDED */

#ifdef CFG_SWITCH_MCAST_INCLUDED
    DEF_BINARY_SERL_WITH_DEFAULTS("mcast", SERL_GRP_SWITCH, mcast_serializer);
/*
  * IGMP snoop features 
  */
    DEF_BINARY_SERL_WITH_DEFAULTS("igmpsnoop", SERL_GRP_SWITCH, igmpsnoop_serializer);

#endif /* CFG_SWITCH_MCAST_INCLUDED */

#ifdef CFG_SWITCH_LAG_INCLUDED
    DEF_BINARY_SERL_WITH_DEFAULTS("lag", SERL_GRP_SWITCH, lag_serializer);
#endif /* CFG_SWITCH_LAG_INCLUDED */

#if CFG_UIP_STACK_ENABLED
    /* 
     * Networking 
     */
    DEF_BINARY_SERL_WITH_DEFAULTS("ethconfig", SERL_GRP_NETWORK_IP, eth_serializer);
#endif /* CFG_UIP_STACK_ENABLED */

    /*
     * Other system configuration
     */
#ifdef CFG_SYSTEM_PASSWORD_INCLUDED
    DEF_BINARY_SERL_WITH_DEFAULTS("password", SERL_GRP_NETWORK, password_serializer);
#endif /* CFG_SYSTEM_PASSWORD_INCLUDED */
    DEF_BINARY_SERL_WITH_DEFAULTS("name", SERL_GRP_NETWORK, system_name_serializer);
#ifdef CFG_PRODUCT_REGISTRATION_INCLUDED    
    DEF_BINARY_SERL_WITH_DEFAULTS("registration", SERL_GRP_NETWORK, 
        registration_status_serializer);
#endif /* CFG_PRODUCT_REGISTRATION_INCLUDED */

#ifdef CFG_ZEROCONF_AUTOIP_INCLUDED
    DEF_BINARY_SERL_WITH_DEFAULTS("autoip", SERL_GRP_NETWORK, 
        autoip_serializer);
#endif /* CFG_ZEROCONF_AUTOIP_INCLUDED */

#ifdef CFG_ZEROCONF_MDNS_INCLUDED
    DEF_BINARY_SERL_WITH_DEFAULTS("mdns", SERL_GRP_PROTOCOL, 
        mdns_serializer);
#endif /* CFG_ZEROCONF_MDNS_INCLUDED */

#if CFG_UIP_IPV6_ENABLED 
    DEF_BINARY_SERL_WITH_DEFAULTS("ipv6", SERL_GRP_NETWORK, 
        ipv6_serializer);
#endif /* CFG_UIP_IPV6_ENABLED */

#ifdef CFG_SWITCH_LOOPDETECT_INCLUDED
    DEF_BINARY_SERL_WITH_DEFAULTS("loopdetect", SERL_GRP_SWITCH, 
        loopdetect_serializer);
#endif
    DEF_BINARY_SERL_WITH_DEFAULTS("portdesc", SERL_GRP_NETWORK, port_desc_serializer);

/* #ifdef CFG_UM_BCMSIM remove this port enable descriptor */
    DEF_BINARY_SERL_WITH_DEFAULTS("portenable", SERL_GRP_NETWORK, port_enable_serializer);


#if CFG_UIP_STACK_ENABLED
    DEF_BINARY_SERL_WITH_DEFAULTS("accesscontrol", SERL_GRP_NETWORK, access_control_serializer);
    DEF_BINARY_SERL_WITH_DEFAULTS("adminpriv", SERL_GRP_NETWORK, admin_privilege_serializer);
#endif /* CFG_UIP_STACK_ENABLED */
}
#endif /* CFG_PERSISTENCE_SUPPORT_ENABLED */

