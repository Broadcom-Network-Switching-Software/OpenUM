/*! \file rxtx.h
 *
 * Rx/Tx board APIs.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_RXTX_H_
#define _BOARDAPI_RXTX_H_
    
#if CFG_RXTX_SUPPORT_ENABLED

/*!
 * Set (the one and only) RX handler. 
 * If intr is TRUE, the handler will be called in interrupt context 
 */
typedef void (*BOARD_RX_HANDLER)(sys_pkt_t *) REENTRANT;

/*!
 * \brief Set handler for receiving packets.
 *
 * \param [in] fn handler function.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_rx_set_handler(BOARD_RX_HANDLER fn) REENTRANT;

/*!
 * \brief Fill (or refill) one packet buffer to RX engine.
 *
 * You can fill more than one buffers until it returns SYS_ERR_FULL
 *
 * \param [in] pkt Pointer to packet buffer.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_rx_fill_buffer(sys_pkt_t *pkt) REENTRANT;

/*!
 * Packet transmit.
 * Note: cbk cannot be NULL (only async mode is supported).
 */
typedef void (*BOARD_TX_CALLBACK)(sys_pkt_t *pkt, sys_error_t status) REENTRANT;

/*!
 * \brief Transmit the packet.
 *
 * \param [in] pkt Pointer to packet buffer.
 * \param [in] cbk Callback.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_tx(sys_pkt_t *pkt, BOARD_TX_CALLBACK cbk) REENTRANT;

/*!
 * \brief Stop packet transmit/receive.
 */
extern void board_rxtx_stop(void) REENTRANT;

#endif /* CFG_RXTX_SUPPORT_ENABLED */

#endif /* _BOARDAPI_RXTX_H_ */
