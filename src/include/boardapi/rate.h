/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_RATE_H_
#define _BOARDAPI_RATE_H_

/*
 * Ingress/Egress rate limiting
 */
extern sys_error_t board_port_rate_ingress_set(uint16 uport, 
                                               uint32 pps) REENTRANT;
extern sys_error_t board_port_rate_ingress_get(uint16 uport, 
                                               uint32 *pps) REENTRANT;
                                               
extern sys_error_t board_port_rate_egress_set(uint16 uport, 
                                              uint32 pps) REENTRANT;
extern sys_error_t board_port_rate_egress_get(uint16 uport, 
                                              uint32 *pps) REENTRANT;

/*
 * Storm control type flags
 */

#define STORM_RATE_NONE   0x00
#define STORM_RATE_BCAST  0x01
#define STORM_RATE_MCAST  0x02
#define STORM_RATE_DLF    0x04
#define STORM_RATE_ALL    0xFF

/*
 * Selecting storm control types for all ports
 */
extern sys_error_t board_rate_type_set(uint8 flags) REENTRANT;
extern sys_error_t board_rate_type_get(uint8 *flags)  REENTRANT;

/*
 * Enable/disable storm control
 */
extern sys_error_t board_rate_set(uint16 uport, uint32 pps)  REENTRANT;
extern sys_error_t board_rate_get(uint16 uport, uint32 *pps)  REENTRANT;

#endif /* _BOARDAPI_RATE_H_ */
