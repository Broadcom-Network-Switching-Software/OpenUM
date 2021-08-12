/*! \file m0ssq_mbox.h
 *
 * M0SSQ MBOX(mailbox) driver public API.
 *
 *     MBOX(mailbox) driver provides bi-directional communication channel
 * between host processor and uC FW. Users can build their own protocol
 * on top of MBOX communication.
 *     Variant length of message(mail) could be conveyed over MBOX channel.
 * Per message optionally can requests a reponse of message from RX side.
 * To use API in this file, user can
 *     - Alloc/free a MBOX channel.
 *     - Send message through a MBOX channel.
 *     - Recieve message in interrupt mode or polling mode.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef M0SSQ_MBOX_H
#define M0SSQ_MBOX_H

/*******************************************************************************
 * M0SSQ-MBOX public definition
 */
/*!
 *  Message for M0SSQ-MBOX communication.
 */
typedef struct m0ssq_mbox_msg_s {

    /*! Message id. */
    uint32 id;

    /*! Byte length of message payload. The length should be any mutiple of 4. */
    uint32  datalen;

    /*! Message payload. */
    uint32  *data;

} m0ssq_mbox_msg_t;

/*!
 *  Response of Message for M0SSQ-MBOX communication.
 */
typedef struct m0ssq_mbox_resp_s {

    /*!
     * Byte length of response payload which must be any mutiple of 4.
     * The size of reponse payload must less or equal than message
     * payload.
     */
    uint32  datalen;

    /*! Response payload. */
    uint32  *data;

} m0ssq_mbox_resp_t;


/*!
 * \brief Customer RX message handler.
 *
 *    This function is defined by customer. It is used to process
 *  the recieved message and optionally fill out message response
 *  according to TX request.
 *
 *    1. A message handler should be registered through
 *  \ref m0ssq_mbox_msg_handler_set before start to
 *  use it.
 *
 *    2. m0ssq_mbox_process_recv_msgs should be invoked by
 *  interrupt service routine(ISR) or a periodic polling thread to
 *  recieve message from MBOX.
 *    if any valid message is recieved,
 *  m0ssq_mbox_process_recv_msgs will call customer RX
 *  message handler to process message.
 *
 *    3. The usage of message and response depends on the protocol
 *  between TX and RX. TX side can send message which doesn't need
 *  any RX response. And TX also can send a message and request a
 *  RX response. The RX response could only return a successful flag
 *  or carry a payload back to TX side. The length of response payload
 *  should be less than or eqaul to the length of TX message payload.
 *
 *    4. If user want to use software ISR to recieve message.
 *  MBOX driver provide a API \ref m0ssq_mbox_uc_swintr_attach
 *  to install MBOX built-in software ISR then attach message
 *  handler to ISR.
 *
 *    5. User could define its own software interrupt handler invoke
 *  \ref m0ssq_mbox_process_recv_msgs within interrupt handler.
 *  Here is a exmple.
 *  \code{.c}
 *  Example:
 *  void
 *  uc0_mbox_init(void)
 *  {
 *      ...
 *      m0ssq_mbox_message_handle_set(unit, 0, mbox_id, uc0_swintr);
 *      m0ssq_swintr_handler_set(unit, 0, uc0_swintr, 0);
 *  }
 *  void
 *  uc0_swintr(unit, uint32 param) {
 *      ...
 *      m0ssq_mbox_process_recv_msgs(mbox_id);
 *  }
 *  \endcode
 *
 *  \param [in] unit Unit number.
 *  \param [in] msg  Recieved message.
 *  \param [in,out]  resp  Response of message.
 *                   !NULL: Transmitter request a response, the
 *                          hanlder has to fill out the content of response.
 *                   NULL: The respose is not required.
 *
 *  \retval SYS_OK No errors.
 *  \retval !SYS_OK Fail reason.
 */
typedef int
(*m0ssq_mbox_msg_handler_f)(int unit,
                            m0ssq_mbox_msg_t *msg,
                            m0ssq_mbox_resp_t *resp);

/*******************************************************************************
 * Public APIs
 */

/*!
 * \brief Allocate a MBOX channel.
 *
 * This function is used to allocate a MBOX channel.
 *
 * \param [in] unit Unit number.
 * \param [in] mbox_name Name of MBOX such as "led", "linkscan".
 * \param [out] mbox_id MBOX id.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX allocation fails.
 */
extern int
m0ssq_mbox_alloc(int unit, const char *mbox_name,
                       uint32 *mbox_id);

/*!
 * \brief Free a MBOX channel.
 *
 * This function is used to free a MBOX channel.
 *
 * \param [in] unit Unit number.
 * \param [in] mbox_id MBOX number.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX free operation fails.
 */
extern int
m0ssq_mbox_free(int unit, uint32 mbox_id);

/*!
 * \brief Install customer RX message handler.
 *
 * This function is to install a RX message handler for a MBOX.
 *
 * \param [in] unit Unit number.
 * \param [in] mbox_id MBOX id.
 * \param [in] handler Message handler.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX Installation failed.
 */
extern int
m0ssq_mbox_msg_handler_set(int unit, uint32 mbox_id,
                           m0ssq_mbox_msg_handler_f handler);

/*!
 * \brief Attach a MBOX to a uC software interrupt.
 *
 * This function is to attach a MBOX onto a built-in software ISR
 * of a uC for message recieving.
 *
 * uC FW can send a message to MBOX and issue a interrupt.
 * Then the built-in swintr handler will call
 * \ref m0ssq_mbox_process_recv_msgs to receive and process
 * the message.
 *
 *
 * \param [in] unit Unit number.
 * \param [in] mbox_id MBOX id.
 * \param [in] uc uC number.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX Attach failed.
 */
extern int
m0ssq_mbox_uc_swintr_attach(int unit, uint32 mbox_id, uint32 uc);

/*!
 * \brief Process recieved messages.
 *
 * This function will
 *  1. Recieve messages from a MBOX.
 *  2. Invoke registered message handler to process messaage.
 *  3. Reply message reponse if required.
 *
 * If user want to receive message, a message handler should be
 *     regitered by \ref m0ssq_mbox_msg_handler_set.
 *
 * In interrupt mode, this function should be invoked in
 * interrupt handler. User can use m0ssq_mbox_uc_swintr_attach
 * to install a MBOX built-in interrupt handler to uC swintr.
 *
 * In polling mode this function should be invoked
 * once for a while for recieving and processing.
 *
 * \param [in] unit Unit number.
 * \param [in] mbox_id mbox number.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX if operation fail.
 */
extern int
m0ssq_mbox_process_recv_msgs(int unit, uint32 mbox_id);

/*!
 * \brief Send a message to a MBOX and optionally get response.
 *
 * This function is used to send a allocate a specific MBOX from
 * m0ssq MBOX pool.
 *
 * \param [in] unit Unit number.
 * \param [in] mbox_id mbox number.
 * \param [in] msg Message needed to be sent.
 * \param [in,out] resp Response of message.
 *                 !NULL: Response is expected.
 *                 NULL: No response is required.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX if operation fails.
 */
extern int
m0ssq_mbox_msg_send(int unit, uint32 mbox_id,
                    m0ssq_mbox_msg_t *msg,
                    m0ssq_mbox_resp_t *resp);

#endif /* M0SSQ_MBOX_H */
