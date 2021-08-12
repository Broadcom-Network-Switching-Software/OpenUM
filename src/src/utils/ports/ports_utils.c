/*
 * $Id: ports_utils.c,v 1.10 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifdef __C51__
#ifdef CODE_USERCLASS
#pragma userclass (code = putl)
#endif /* CODE_USERCLASS */
#endif /* __C51__ */

#include "system.h"
#include "utils/ports.h"

/*
 * PORT description
 */

static char port_desc[BOARD_MAX_NUM_OF_PORTS][WEB_PORT_DESC_LEN+1] = {{0}};

sys_error_t
APIFUNC(uplist_is_empty)(uint8 *uplist) REENTRANT
{
    uint8 index;
    if (uplist == NULL) {
        return SYS_ERR_PARAMETER;
    }
    for(index=0; index<MAX_UPLIST_WIDTH; index++) {
        if (uplist[index] != 0) {
            return SYS_ERR_FALSE;
        }
    }
    return SYS_OK;
}

sys_error_t
APIFUNC(uplist_clear)(uint8 *uplist) REENTRANT
{
    uint8 index;
    if (uplist == NULL) {
        return SYS_ERR_PARAMETER;
    }
    for(index=0; index<MAX_UPLIST_WIDTH; index++) {
        uplist[index] = 0;
    }
    return SYS_OK;
}

sys_error_t
APIFUNC(uplist_port_matched)(const uint8 *uplist, uint16 uport) REENTRANT
{
    if (uplist == NULL) {
        return SYS_ERR_PARAMETER;
    }
    if (SAL_UPORT_IS_NOT_VALID(uport)) {
        return SYS_ERR_PARAMETER;
    }

    uport = SAL_UPORT_TO_ZUPORT(uport);
    
#if (MAX_UPLIST_WIDTH == 1) /* Shortcut to avoid mul/div */
    if (uplist[0] & (1 << uport)) {
        return SYS_OK;
    }
#else
    if (uplist[uport / 8] & (1 << (uport % 8))) {
        return SYS_OK;
    }
#endif
    
    return SYS_ERR_FALSE;
}

sys_error_t
APIFUNC(uplist_port_add)(uint8 *uplist, uint16 uport) REENTRANT
{
    if (uplist == NULL) {
        return SYS_ERR_PARAMETER;
    }
    if (SAL_UPORT_IS_NOT_VALID(uport)) {
        return SYS_ERR_PARAMETER;
    }

    uport = SAL_UPORT_TO_ZUPORT(uport);
    
#if (MAX_UPLIST_WIDTH == 1) /* Shortcut to avoid mul/div */
    uplist[0] |= (1 << uport);
#else
    uplist[uport / 8] |= (1 << (uport % 8));
#endif
    
    return SYS_OK;
}

sys_error_t
APIFUNC(uplist_port_remove)(uint8 *uplist, uint16 uport) REENTRANT
{
    if (uplist == NULL) {
        return SYS_ERR_PARAMETER;
    }
    if (SAL_UPORT_IS_NOT_VALID(uport)) {
        return SYS_ERR_PARAMETER;
    }

    uport = SAL_UPORT_TO_ZUPORT(uport);
#if (MAX_UPLIST_WIDTH == 1) /* Shortcut to avoid mul/div */
    uplist[0] &= ~(1 << uport);
#else
    uplist[uport / 8] &= ~(1 << (uport % 8));
#endif
    
    return SYS_OK;
}

sys_error_t
APIFUNC(uplist_manipulate)(uint8 *dst_uplist, uint8 *src_uplist, uplist_op_t op) REENTRANT
{
    uint8 uport;
    if (dst_uplist == NULL) {
        return SYS_ERR_PARAMETER;
    }

    if (src_uplist == NULL) {
        return SYS_ERR_PARAMETER;
    }

    for(uport=0; uport<MAX_UPLIST_WIDTH; uport++) {
        if(op == UPLIST_OP_COPY) {
            dst_uplist[uport] = src_uplist[uport];
        } else if (op == UPLIST_OP_OR) {
            dst_uplist[uport] |= src_uplist[uport];
        } else if (op == UPLIST_OP_AND) {
            dst_uplist[uport] &= src_uplist[uport];
        } else if (op == UPLIST_OP_EQU) {
            if (dst_uplist[uport] != src_uplist[uport]) {
                return SYS_ERR;
            }
        } else {
            return SYS_ERR_PARAMETER;
        }
    }
    return SYS_OK;
}






sys_error_t
get_port_desc(uint16 uport, char *buf, uint8 len)
{
    if (buf == NULL || len == 0) {
        return SYS_ERR_PARAMETER;
    }

    uport = SAL_UPORT_TO_ZUPORT(uport);
    
    if (sal_strlen(port_desc[uport]) > len) {
        return SYS_ERR_PARAMETER;
    }

    sal_strcpy(buf, port_desc[uport]);

    return SYS_OK;
}

sys_error_t
set_port_desc(uint16 uport, const char *desc)
{
    if (desc == NULL) {
        return SYS_ERR_PARAMETER;
    }

    uport = SAL_UPORT_TO_ZUPORT(uport);

    if (sal_strlen(desc) > WEB_PORT_DESC_LEN) {
        return SYS_ERR_PARAMETER;
    }
    sal_strcpy(port_desc[uport], desc);

    return SYS_OK;
}
