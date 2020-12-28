/*
 * $Id: rxtx.h,v 1.2 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _BRDIMPL_RXTX_H_
#define _BRDIMPL_RXTX_H_

#if CFG_RXTX_SUPPORT_ENABLED

/*
 * Board TX/RX initialization
 */
extern void brdimpl_rxtx_init(void) REENTRANT;

/* 
 * Set (the one and only) RX handler. 
 * If intr is TRUE, the handler will be called in interrupt context 
 */
extern sys_error_t brdimpl_rx_set_handler(BOARD_RX_HANDLER fn) REENTRANT;

/* 
 * Fill (or refill) one packet buffer to RX engine.
 * You can fill more than one buffers until it returns SYS_ERR_FULL
 */
extern sys_error_t brdimpl_rx_fill_buffer(sys_pkt_t *pkt) REENTRANT;

/*
 * Packet transmit. 
 * Note: cbk cannot be NULL (only async mode is supported).
 */
extern sys_error_t brdimpl_tx(sys_pkt_t *pkt, BOARD_TX_CALLBACK cbk) REENTRANT;

#endif /* CFG_RXTX_SUPPORT_ENABLED */

#endif /* _BRDIMPL_RXTX_H_ */
