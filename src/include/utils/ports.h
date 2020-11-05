/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _UTILS_PORTS_H_
#define _UTILS_PORTS_H_


/* Port description */
#define WEB_PORT_DESC_LEN    (20)


/* Check if an uplist is empty; return SYS_ERR_FALSE if not */
extern sys_error_t uplist_is_empty(uint8 *uplist) REENTRANT;

/* Clear an uplist */
extern sys_error_t uplist_clear(uint8 *uplist) REENTRANT;

/* Check if a port is in an uplist; return SYS_ERR_FALSE if not */
extern sys_error_t uplist_port_matched(const uint8 *uplist, uint16 uport) REENTRANT;

/* Add a uport to an uplist */
extern sys_error_t uplist_port_add(uint8 *uplist, uint16 uport) REENTRANT;

/* Remove a uport from an uplist */
extern sys_error_t uplist_port_remove(uint8 *uplist, uint16 uport) REENTRANT;

typedef enum uplist_op_s{
    UPLIST_OP_COPY = 0,
    UPLIST_OP_OR,
    UPLIST_OP_AND,
    UPLIST_OP_EQU
} uplist_op_t;

/* COPY/OR/AND two uplists */
extern sys_error_t uplist_manipulate(uint8 *dst_uplist, uint8 *src_uplist, uplist_op_t op) REENTRANT;


extern sys_error_t get_port_desc(uint16 uport, char *buf, uint8 len);
extern sys_error_t set_port_desc(uint16 uport, const char *buf);



#endif /* _UTILS_PORTS_H_ */
