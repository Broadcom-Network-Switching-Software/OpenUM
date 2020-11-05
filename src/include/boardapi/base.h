/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_BASE_H_
#define _BOARDAPI_BASE_H_

/* Number of units (should bed defined in board.h) */
#define board_unit_count() BOARD_NUM_OF_UNITS

/* Max number of ports */
#define board_max_port_count() BOARD_MAX_NUM_OF_PORTS

/* Board name */
extern const char *board_name(void) REENTRANT;

/* Get the number of user ports */
extern uint8 board_uport_count(void) REENTRANT;

/* Convert user port to unit and chip internal logical port */
extern sys_error_t board_uport_to_lport(uint16 uport, 
                                          uint8 *unit, 
                                          uint8 *lport) REENTRANT;

/* Convert unit and chip internal logical port to user port */
extern sys_error_t board_lport_to_uport(uint8 unit, 
                                         uint8 lport, 
                                         uint16 *uport) REENTRANT;

/* Convert user port list to  chip internal logical port bitmap for specified unit */
extern sys_error_t board_uplist_to_lpbmp(uint8 *uplist, uint8 unit,
                                          pbmp_t *lpbmp) REENTRANT;

/* Convert unit and  chip internal logical port bitmap to user port list */
extern sys_error_t board_lpbmp_to_uplist(uint8 unit, pbmp_t lpbmp,
                                           uint8 *uplist) REENTRANT;

/* Get SOC instance by unit */
extern soc_switch_t *board_get_soc_by_unit(uint8 unit) REENTRANT;


#ifndef CFG_PCM_SUPPORT_INCLUDED
/* Get phy driver by user port */
extern phy_driver_t *board_get_phy_drv(uint16 uport) REENTRANT;
#endif
#endif /* _BOARDAPI_BASE_H_ */
