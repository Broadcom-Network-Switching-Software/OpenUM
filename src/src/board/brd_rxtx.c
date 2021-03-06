/*
 * $Id: brd_rxtx.c,v 1.9 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
 
#ifdef __C51__
#ifdef CODE_USERCLASS
#pragma userclass (code = brdimpl)
#endif /* CODE_USERCLASS */
#endif /* __C51__ */

#include "system.h"
#include "utils/ports.h"

/*
 * Typical implementation of certain board functions 
 * 
 * Note: Only applicable for single-unit board.
 */

#if CFG_RXTX_SUPPORT_ENABLED

STATIC BOARD_RX_HANDLER rx_handler;
STATIC sys_pkt_t *rx_syspkt_list;
STATIC soc_rx_packet_t *rx_pkt_list;
STATIC soc_tx_packet_t *tx_pkt_list;

/* Forwards */
APISTATIC void brdimpl_rx_handler(soc_rx_packet_t *pkt) REENTRANT;
APISTATIC void brdimpl_tx_cbk(struct soc_tx_packet_s *pkt) REENTRANT;

APISTATIC void
APIFUNC(brdimpl_rx_handler)(soc_rx_packet_t *pkt) REENTRANT
{
    uint16 flags0, flags1;
    sys_pkt_t *wrap;
    
    SAL_ASSERT(pkt != NULL);
    if (pkt == NULL) {
        return;
    }
    
    wrap = rx_syspkt_list;
    SAL_ASSERT(wrap != NULL);
    if (wrap == NULL) {
        return;
    }
    rx_syspkt_list = wrap->next;

    flags0 = pkt->flags;

    wrap->pkt_data = pkt->buffer;
    wrap->pkt_len = pkt->pktlen;
    wrap->buf_len = pkt->buflen;
    board_lport_to_uport(0, pkt->lport, &wrap->rx_src_uport);
    wrap->next = NULL;
    flags1 = 0;
    if (flags0 & SOC_RX_FLAG_TIMESTAMP) {
        flags1 |= SYS_RX_FLAG_TIMESTAMP;
        wrap->rx_timestamp = pkt->timestamp;
    }
    if (flags0 & SOC_RX_FLAG_TRAFFIC_CLASS) {
        flags1 |= SYS_RX_FLAG_COS;
        wrap->cos = pkt->traffic_class;
    }
    if (flags0 & SOC_RX_FLAG_TRUNCATED) {
        flags1 |= SYS_RX_FLAG_TRUNCATED;
    }
    if (flags0 & SOC_RX_FLAG_ERROR_CRC) {
        flags1 |= SYS_RX_FLAG_ERROR_CRC;
    }
    if (flags0 & SOC_RX_FLAG_ERROR_OTHER) {
        flags1 |= SYS_RX_FLAG_ERROR_OTHER;
    }
    wrap->flags = flags1;
    
    pkt->next = rx_pkt_list;
    rx_pkt_list = pkt;
    
    (*rx_handler)(wrap);
}

sys_error_t
APIFUNC(brdimpl_rx_set_handler)(BOARD_RX_HANDLER fn) REENTRANT
{
    soc_switch_t *soc;

    SAL_ASSERT(board_unit_count() == 1);
    SAL_ASSERT(rx_handler == NULL);
    if (rx_handler != NULL) {
        return SYS_ERR_STATE;
    }
    
    if (fn == NULL) {
        return SYS_ERR_PARAMETER;
    }
    
    soc = board_get_soc_by_unit(0);
    SAL_ASSERT(soc != NULL);
    
    rx_handler = fn;
    return (*soc->rx_set_handler)(0, brdimpl_rx_handler, FALSE);
}
    
sys_error_t 
APIFUNC(brdimpl_rx_fill_buffer)(sys_pkt_t *pkt) REENTRANT
{
    soc_switch_t *soc;
    soc_rx_packet_t *spkt;
    
    SAL_ASSERT(board_unit_count() == 1);
    if (pkt == NULL) {
        return SYS_ERR_PARAMETER;
    }

    spkt = rx_pkt_list;
    if (spkt == NULL) {
        spkt = (soc_rx_packet_t *)sal_malloc(sizeof(soc_rx_packet_t));
        if (spkt == NULL) {
            return SYS_ERR_OUT_OF_RESOURCE;
        }
    } else {
        rx_pkt_list = spkt->next;
    }

    spkt->buffer = pkt->pkt_data;
    spkt->alloc_ptr = pkt->pkt_data;
    spkt->buflen = pkt->buf_len;

    pkt->next = rx_syspkt_list;
    rx_syspkt_list = pkt;

    soc = board_get_soc_by_unit(0);
    SAL_ASSERT(soc != NULL);

    return (*soc->rx_fill_buffer)(0, spkt);
}

APISTATIC void
APIFUNC(brdimpl_tx_cbk)(struct soc_tx_packet_s *pkt) REENTRANT
{
    sys_pkt_t *kpkt;
    BOARD_TX_CALLBACK cbk;
    sys_error_t r;

    SAL_ASSERT(pkt != NULL);
    if (pkt == NULL) {
        return;
    }

    kpkt = (sys_pkt_t *)pkt->cookie;
    cbk = (BOARD_TX_CALLBACK)kpkt->internal1;

    /* Get status */
    r = pkt->status;

    /* Return pkt to pool */        
    pkt->next = tx_pkt_list;
    tx_pkt_list = pkt;

    /* Invoke callback */ 
    (*cbk)(kpkt, r);
}

sys_error_t
APIFUNC(brdimpl_tx)(sys_pkt_t *pkt, BOARD_TX_CALLBACK cbk) REENTRANT
{
    soc_tx_packet_t *spkt;
    soc_switch_t *soc;
    uint16 flags = 0;
    sys_error_t r;

    SAL_ASSERT(board_unit_count() == 1);
    
    if (pkt == NULL || cbk == NULL) {
        return SYS_ERR_PARAMETER;
    }
    
    soc = board_get_soc_by_unit(0);
    SAL_ASSERT(soc != NULL);

    spkt = tx_pkt_list;
    if (spkt == NULL) {
        spkt = (soc_tx_packet_t *)sal_malloc(sizeof(soc_tx_packet_t));
        if (spkt == NULL) {
            return SYS_ERR_OUT_OF_RESOURCE;
        }
    } else {
        tx_pkt_list = spkt->next;
    }

    sal_memset(spkt, 0, sizeof(soc_tx_packet_t));
    spkt->buffer = pkt->pkt_data;
    spkt->pktlen = pkt->pkt_len;
    board_uplist_to_lpbmp(pkt->tx_uplist, 0, &spkt->port_bitmap);
    spkt->traffic_class = pkt->cos;
    if (pkt->flags & SYS_TX_FLAG_TIMESTAMP_REQUEST) {
        flags |= SOC_TX_FLAG_TIMESTAMP_REQUEST;
    }
    if (pkt->flags & SYS_TX_FLAG_USE_UNTAG_PORT_LIST) {
        flags |= SOC_TX_FLAG_USE_UNTAG_PORT_BITMAP;
        board_uplist_to_lpbmp(pkt->tx_untag_uplist, 0, &spkt->untag_bitmap);
    } else {
        flags &= ~SOC_TX_FLAG_USE_UNTAG_PORT_BITMAP;
        PBMP_CLEAR(spkt->untag_bitmap);
    }
    if (pkt->flags & SYS_TX_FLAG_TIMESYNC) {
        flags |= SOC_TX_FLAG_TIMESYNC;
        if (pkt->flags & SYS_TX_FLAG_TIMESYNC_ONE_STEP) {
            flags |= SOC_TX_FLAG_TIMESYNC_ONE_STEP;
        }
        if (pkt->flags & SYS_TX_FLAG_TIMESYNC_INGRESS_SIGN) {
            flags |= SOC_TX_FLAG_TIMESYNC_INGRESS_SIGN;
        }
        if (pkt->flags & SYS_TX_FLAG_TIMESYNC_HDR_START_OFFSET) {
            flags |= SOC_TX_FLAG_TIMESYNC_HDR_START_OFFSET;
            spkt->timestamp_offset = pkt->timestamp_offset;
        }
        if (pkt->flags & SYS_TX_FLAG_TIMESYNC_REGEN_UDP_CHKSUM) {
            flags |= SOC_TX_FLAG_TIMESYNC_REGEN_UDP_CHKSUM;
        }
    }
    if (uplist_is_empty(pkt->tx_uplist) == SYS_OK) {
        switch(pkt->tx_tag_mode) {
        case SYS_TX_TAG_MODE_FOLLOW_SWITCH_RULES:
            spkt->tag_mode = SOC_TX_TAG_MODE_FOLLOW_SWITCH_RULES; 
            break;
        case SYS_TX_TAG_MODE_UNTAG_ALL:
            spkt->tag_mode = SOC_TX_TAG_MODE_UNTAG_ALL; 
            break;
        case SYS_TX_TAG_MODE_TAG_ALL:
            spkt->tag_mode = SOC_TX_TAG_MODE_TAG_ALL; 
            break;
        default: 
            spkt->tag_mode = SOC_TX_TAG_MODE_FOLLOW_SWITCH_RULES; 
            break;
        }
    }
    spkt->flags = flags;
    spkt->callback = brdimpl_tx_cbk;
    spkt->cookie = (void *)pkt;
    pkt->internal1 = (uint32)cbk;
    
    r = (*soc->tx)(0, spkt);
    if (r != SYS_OK) {
        spkt->next = tx_pkt_list;
        tx_pkt_list = spkt;
    }
    
    return r;
}

void
APIFUNC(brdimpl_rxtx_init)(void) REENTRANT
{
    rx_handler = NULL;
    rx_syspkt_list = NULL;
    rx_pkt_list = NULL;
    tx_pkt_list = NULL;
}

#endif /* CFG_RXTX_SUPPORT_ENABLED */
