/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_L2_H_
#define _BOARDAPI_L2_H_

#define BOARD_L2_STATIC   (0x1 << 0)


#if defined(CFG_SWITCH_L2_ADDR_INCLUDED)
typedef enum l2_cmd_s {
    L2_CMD_ADD = 0,
    L2_CMD_DELETE,
    L2_CMD_DELETE_BY_PORT,
    L2_CMD_DELETE_BY_VLAN,
    L2_CMD_DELETE_BY_TRUNK,
    L2_CMD_DELETE_ALL,
    L2_CMD_LOOKUP,
    L2_CMD_GET_FIRST,
    L2_CMD_GET_NEXT,
    L2_CMD_GET_LAST,
    L2_CMD_GET_ALL,
    L2_CMD_COUNT
} l2_cmd_t;

typedef struct board_l2_addr_s {
  uint16 flags;   /* BOARD_L2_xxx flags. */
  uint8  mac[6];  /* 802.3 MAC address. */
  uint16 vid;     /* VLAN identifier. */
  uint16 uport;   /* user port number. */
  uint16 mcidx;   /* L2MC pointer. */
  uint8  tgid;    /* Trunk ID value. */
  uint8  is_trunk;  /* Trunk bit. */
} board_l2_addr_t;

extern sys_error_t board_l2_addr_add(board_l2_addr_t *l2addr);

extern sys_error_t board_l2_addr_delete(board_l2_addr_t *l2addr);

extern sys_error_t board_l2_addr_delete_by_port(uint16 uport);

extern sys_error_t board_l2_addr_delete_by_vlan(uint16 vid);

extern sys_error_t board_l2_addr_delete_by_trunk(uint8 lagid);

extern sys_error_t board_l2_addr_delete_all(void);

extern sys_error_t board_l2_addr_lookup(board_l2_addr_t *l2addr, 
                                                         uint16 *index);

extern sys_error_t board_l2_addr_get_first(board_l2_addr_t *l2addr, 
                                                            uint16 *index);

extern sys_error_t board_l2_addr_get_next(board_l2_addr_t *l2addr, 
                                                            uint16 *index);

extern sys_error_t board_l2_addr_get_last(board_l2_addr_t *l2addr, 
                                                            uint16 *index);
#endif /* CFG_SWITCH_L2_ADDR_INCLUDED */
#endif /* _BOARDAPI_L2_H_ */
