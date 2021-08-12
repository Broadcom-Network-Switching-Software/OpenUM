/*
 * $Id: tx.c,v 1.4 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifdef __C51__
#ifdef CODE_USERCLASS
#pragma userclass (code = krntx)
#endif /* CODE_USERCLASS */
#endif /* __C51__ */

#include "system.h"

#if (CFG_RXTX_SUPPORT_ENABLED && !defined(__BOOTLOADER__))
 
/* Forwards */
void APIFUNC(sys_tx_init)(void) REENTRANT;
APISTATIC void sys_tx_sync_cbk(sys_pkt_t *pkt, sys_error_t status) REENTRANT;

APISTATIC void
APIFUNC(sys_tx_sync_cbk)(sys_pkt_t *pkt, sys_error_t status) REENTRANT
{
    SAL_ASSERT(pkt != NULL);
    if (pkt == NULL) {
        return;
    }

    /* Mark packet as done */    
    pkt->internal0 = (uint32)status;
}

sys_error_t
APIFUNC(sys_tx)(sys_pkt_t *pkt, SYS_TX_CALLBACK cbk) REENTRANT
{
    sys_error_t r;
    
    if (pkt == NULL || pkt->pkt_data == NULL || pkt->pkt_len == 0) {
        return SYS_ERR_PARAMETER;
    }
    
    if (cbk == NULL) {
        
        /* We make use of this field to mark packet being processing */
        pkt->internal0 = 1;
        
        /* TX out */
        r = board_tx(pkt, sys_tx_sync_cbk);
        if (r == SYS_OK) {
            for(;;) {
                POLL();
                if (pkt->internal0 != 1) {
                    r = (sys_error_t)pkt->internal0;
                    break;
                }
                /* We rely on BOARD-layer tx to timeout if it takes too long */
            }
        }

        return r;

    } else {

        /* TX out */
        r = board_tx(pkt, (BOARD_TX_CALLBACK)cbk);

        return r;
    }
}

void
APIFUNC(sys_tx_init)(void) REENTRANT
{
}

#endif /* CFG_RXTX_SUPPORT_ENABLED && !defined(__BOOTLOADER__) */

