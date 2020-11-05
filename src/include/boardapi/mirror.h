/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_MIRROR_H_
#define _BOARDAPI_MIRROR_H_

/*
 * Select mirror-to-port
 */
extern sys_error_t board_mirror_to_set(uint16 uport) REENTRANT;
extern sys_error_t board_mirror_to_get(uint16 *uport) REENTRANT;

/*
 * Enable or disable mirroring per port
 */
extern sys_error_t board_mirror_port_set(uint16 uport, uint8 enable) REENTRANT;
extern sys_error_t board_mirror_port_get(uint16 uport, uint8 *enable) REENTRANT;

#endif /* _BOARDAPI_MIRROR_H_ */
