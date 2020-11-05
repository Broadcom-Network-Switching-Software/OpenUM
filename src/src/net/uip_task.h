/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __UIP_TASK_H__
#define __UIP_TASK_H__

/* TCP listen port definitions */
#define HTTP_TCP_PORT       80

/**
 * \var #define UIP_APPCALL
 *
 * The name of the application function that uIP should call in
 * response to TCP/IP events.
 */
#ifndef UIP_CONF_IPV6
#define UIP_CONF_IPV6                   0
#endif /* !UIP_CONF_IPV6 */
#if UIP_CONF_IPV6

/* For IPv6 stack to link with different function names */
void uip6_appcall(void);
void udp6_appcall(void);
#define UIP_APPCALL     uip6_appcall
#define UIP_UDP_APPCALL udp6_appcall

#else /* !UIP_CONF_IPV6 */

/* IPv4 stack */
void uip_appcall(void);
void udp_appcall(void);
#define UIP_APPCALL     uip_appcall
#define UIP_UDP_APPCALL udp_appcall

#endif /* !UIP_CONF_IPV6 */

/*
 * \var typedef uip_tcp_appstate_t
 *
 * The type of the application state that is to be stored in the
 * uip_conn structure. This usually is typedef:ed to a struct holding
 * application state information.
 */
typedef uint8 uip_tcp_appstate_t;
typedef uint8 uip_udp_appstate_t;

void uip_task_init(void);

#endif /* __UIP_TASK_H__ */
