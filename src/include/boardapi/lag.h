/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_LAG_H_
#define _BOARDAPI_LAG_H_

#ifdef CFG_SWITCH_LAG_INCLUDED

extern sys_error_t board_lag_set(uint8 enable) REENTRANT;
extern void board_lag_get(uint8 *enable) REENTRANT;
extern sys_error_t board_lag_group_set(uint8 lagid, 
                                    uint8 enable, uint8 *uplist) REENTRANT;
extern sys_error_t board_lag_group_get(uint8 lagid, 
                                    uint8 *enable, uint8 *uplist) REENTRANT;

extern void board_lag_group_max_num(uint8 *num) REENTRANT;
extern void board_lag_linkchange(uint16 uport, BOOL link, void *arg) REENTRANT;

#endif /* CFG_SWITCH_LAG_INCLUDED */

#endif /* _BOARDAPI_LAG_H_ */
