/*! \file m0ssq_mbox.c
 *
 * M0SSQ MBOX(mailbox) driver.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include <system.h>

#include <utils/shr/shr_debug.h>

#include <m0ssq_internal.h>
#include <m0ssq_mem.h>
#include <m0ssq_fifo_internal.h>
#include <m0ssq_mbox_internal.h>

/*******************************************************************************
 * Local definitions
 */

#define MBOX_HDR_SIZE                      sizeof(m0ssq_mbox_int_msg_t)
#define MBOX_ENT_SIZE                      sizeof(uint32)
#define MBOX_MAX_RX_MSGS_PER_CALL          200
#define MBOX_TX_TIMEOUT_CNT                1000

/*!
 * Mbox configuration is stored M0SSQ SRAM.
 * Offset of each mbox configuration are listed below
 */
#define U0_MBOX_BASE_OFFSET                 0x0
#define U0_MBOX_SIZE_OFFSET                 0x4
#define U1_MBOX_BASE_OFFSET                 0x8
#define U1_MBOX_SIZE_OFFSET                 0xc
#define U2_MBOX_BASE_OFFSET                 0x10
#define U2_MBOX_SIZE_OFFSET                 0x14
#define U3_MBOX_BASE_OFFSET                 0x18
#define U3_MBOX_SIZE_OFFSET                 0x1c
#define MBOX_INTERRUPT_MODE_OFFSET          0x20
#define MAX_MBOX_MBOX_PER_CORE_OFFSET       0x24

/*! Size of MBOX configuration space. */
#define MBOX_SRAM_FWREG_SIZE                256

/*! Total size of all MBOXs. */
#define MBOX_SRAM_TOTAL_SIZE                (4 * 4096)

/*! Invalid uC number. */
#define MBOX_INVALID_UC                     ((uint32) -1)

/*******************************************************************************
 * Local variables
 */

/*! Device data and information. */
static m0ssq_mbox_dev_t *m0ssq_mbox_dev[CONFIG_MAX_UNITS] = { NULL };

/*******************************************************************************
 * Private functions
 */

static void
m0ssq_mbox_msg_resp_free(m0ssq_mbox_int_msg_t **int_msg,
                         m0ssq_mbox_int_msg_t **int_resp)
{
    if ((int_msg != NULL) && (*int_msg != NULL))
    {
        sal_free(*int_msg);
    }

    if ((int_resp != NULL) && (*int_resp != NULL))
    {
        sal_free(*int_resp);
    }
}

static int
m0ssq_mbox_msg_resp_alloc(m0ssq_mbox_int_msg_t **int_msg,
                          m0ssq_mbox_int_msg_t **int_resp,
                          uint32 size)
{
    if (int_msg == NULL) {
        return SYS_ERR;
    }

    *int_msg = sal_alloc(size, "bcmbdM0ssqRecvBuff");
    if (*int_msg == NULL) {
        return SYS_ERR;
    }

    if (int_resp != NULL) {
        *int_resp = sal_alloc(size, "bcmbdM0ssqRespBuff");
        if (*int_resp == NULL) {
            sal_free(*int_msg);
            *int_msg = NULL;
            return SYS_ERR;
        }
    }

    return SYS_OK;
}

/*!
 * \brief Built-in software interrupt handler for MBOX.
 */
static void
m0ssq_mbox_built_in_rxmsg_intr(uint32 param)
{
    uint32 id;
    mbox_t *mbox;
    m0ssq_mbox_dev_t *dev;
    uint32 uc = param & 0xFF;
    uint32 unit = (param >> 8) & 0xFF;
    if (!UNIT_VALID(unit)) {
        return;
    }

    dev = m0ssq_mbox_dev[unit];
    if (dev == NULL ||
        dev->init_done == 0)
    {
        return;
    }

    for (id = 0; id < dev->num_of_mbox; id++)
    {
        mbox = &dev->mbox[id];
        if (mbox->uc == MBOX_INVALID_UC ||
            mbox->uc != uc ||
            mbox->inuse == 0 ||
            mbox->recv_msg_handler == NULL)
        {
            continue;
        }

        m0ssq_mbox_process_recv_msgs(unit, id);
    }
}

/*******************************************************************************
 * Public functions shared by M0SSQ and MBOX drivers.
 */

int
m0ssq_mbox_dev_get(int unit, m0ssq_mbox_dev_t **dev)
{
    if (m0ssq_mbox_dev[unit] == NULL ||
        m0ssq_mbox_dev[unit]->init_done == 0)
    {
        return SYS_ERR;
    }

    *dev = m0ssq_mbox_dev[unit];

    return SYS_OK;
}

int
m0ssq_mbox_init(int unit, m0ssq_mem_t *mem, uint32 num_of_mbox)
{
    uint32 type;
    uint32 mbox_base, mbox_size, mbox_total_size;
    uint32 mbox_id;
    int rv;
    m0ssq_mbox_dev_t *dev;
    m0ssq_mem_t new_mem;
    m0ssq_fifo_t *fifo;

    if (!UNIT_VALID(unit)) {
        return SYS_ERR_UNAVAIL;
    }

    if (m0ssq_mbox_dev[unit] != NULL &&
        m0ssq_mbox_dev[unit]->init_done)
    {
        return SYS_OK;
    }

    dev = sal_alloc(sizeof(m0ssq_mbox_dev_t), "bcmbdM0ssqMboxDev");
    if (dev == NULL) {
        return SYS_ERR_OUT_OF_RESOURCE;
    }

    dev->num_of_mbox = num_of_mbox;
    mbox_total_size = MBOX_SRAM_TOTAL_SIZE;
    mbox_base = mem->base + MBOX_SRAM_FWREG_SIZE;
    mbox_size = mbox_total_size / num_of_mbox / MBOX_CHN_TYPE_MAX;

    /* Write the mailbox information into M0SSQ shared SRAM. */
    m0ssq_mem_write32(mem, U0_MBOX_BASE_OFFSET, mbox_base);
    m0ssq_mem_write32(mem, U0_MBOX_SIZE_OFFSET, mbox_total_size);
    m0ssq_mem_write32(mem, MAX_MBOX_MBOX_PER_CORE_OFFSET, num_of_mbox);
    m0ssq_mem_write32(mem, MBOX_INTERRUPT_MODE_OFFSET, 0);

    /*
     * Chunk input memory block into smaller ones and
     * init each smaller memory block as fifo.
     */

    /* Create a new memory object by inherit the input memroy object. */
    sal_memcpy(&new_mem, mem, sizeof(m0ssq_mem_t));

    for (mbox_id = 0; mbox_id < dev->num_of_mbox; mbox_id++) {
        for (type = 0; type < MBOX_CHN_TYPE_MAX; type++) {
             fifo = &(dev->mbox[mbox_id].chan[type]);

             /* Initialize a new memory object with new base and size. */
             new_mem.base  = mbox_base;
             new_mem.size  = mbox_size;

             /* Initialize a FIFO with the new memory object and entry size. */
             rv = m0ssq_fifo_init(fifo, &new_mem, MBOX_ENT_SIZE);
             if (SHR_FAILURE(rv)) {
                 SHR_LOG_ERROR("FIFO init fail for mailbox %d channel %d\n", mbox_id, type);
                 return SYS_ERR;
             };

             /* Increase the base address for next memory object. */
             mbox_base += mbox_size;
        }
        dev->mbox[mbox_id].recv_msg_handler = NULL;
        dev->mbox[mbox_id].uc = MBOX_INVALID_UC;
        dev->mbox[mbox_id].inuse = 0;
    }

    /* Install MBOX driver-specific data and mark it is done. */
    dev->mbox_size = mbox_size;
    dev->init_done = 1;
    m0ssq_mbox_dev[unit] = dev;

    return SYS_OK;
}

int
m0ssq_mbox_cleanup(int unit)
{
    uint32 mbox_id, type;
    m0ssq_mbox_dev_t *dev = m0ssq_mbox_dev[unit];
    m0ssq_fifo_t *fifo;

    for (mbox_id = 0; mbox_id < dev->num_of_mbox; mbox_id++) {
        if (dev->mbox[mbox_id].uc != MBOX_INVALID_UC) {
            /* Stop uCs' software interrupt. */
            m0ssq_uc_swintr_enable_set(unit, dev->mbox[mbox_id].uc, 0);

            /* Uninstall uCs' software interrupt handler. */
            m0ssq_uc_swintr_handler_set(unit, dev->mbox[mbox_id].uc,
                                              NULL, MBOX_INVALID_UC);
        }

        /* Uninstall MBOX RX message handler. */
        m0ssq_mbox_msg_handler_set(unit, mbox_id, NULL);

        /* Cleanup FIFO resources. */
        for (type = 0; type < MBOX_CHN_TYPE_MAX; type++) {
            fifo = &(dev->mbox[mbox_id].chan[type]);
            m0ssq_fifo_cleanup(fifo);
        }
    }

    sal_free(m0ssq_mbox_dev[unit]);

    return SYS_OK;
}

/*******************************************************************************
 * Public Functions
 */

int
m0ssq_mbox_alloc(int unit, const char *mbox_name, uint32 *mbox_id)
{
    m0ssq_mbox_dev_t *dev;
    uint32 id;

    SHR_FUNC_ENTER(unit);

    if (!UNIT_VALID(unit)) {
        return SYS_ERR_UNAVAIL;
    }

    dev = m0ssq_mbox_dev[unit];
    SHR_NULL_CHECK(dev, SYS_ERR);

    /* Static allocation */
    if (sal_strncmp(mbox_name, "led", sal_strlen("led")) == 0) {
        id = 0;
    } else if (sal_strncmp(mbox_name, "linkscan", sal_strlen("linkscan")) == 0) {
        id = 1;
    } else {
        
        SHR_ERR_EXIT(SYS_ERR);
    }

    if (dev->mbox[id].inuse) {
        SHR_ERR_EXIT(SYS_ERR);
    }

    *mbox_id = id;
    dev->mbox[id].inuse = 1;

exit:
    SHR_FUNC_EXIT();
}

int
m0ssq_mbox_free(int unit, uint32 mbox_id)
{
    m0ssq_mbox_dev_t *dev;

    SHR_FUNC_ENTER(unit);

    if (!UNIT_VALID(unit)) {
        return SYS_ERR_UNAVAIL;
    }

    dev = m0ssq_mbox_dev[unit];
    SHR_NULL_CHECK(dev, SYS_ERR);

    if (dev->mbox[mbox_id].inuse) {
        dev->mbox[mbox_id].inuse = 0;
    }

exit:
    SHR_FUNC_EXIT();
}

int
m0ssq_mbox_msg_handler_set(int unit,
                                 uint32 mbox_id,
                                 m0ssq_mbox_msg_handler_f msg_handler)
{
    m0ssq_mbox_dev_t *dev;

    if (!UNIT_VALID(unit)) {
        return SYS_ERR_UNAVAIL;
    }

    dev = m0ssq_mbox_dev[unit];
    if (dev == NULL) {
        return SYS_ERR;
    }

    /* Install mbox message handler */
    dev->mbox[mbox_id].recv_msg_handler = msg_handler;

    return SYS_OK;
}

int
m0ssq_mbox_uc_swintr_attach(int unit, uint32 mbox_id,
                                  uint32 uc)
{
    m0ssq_mbox_dev_t *dev;

    if (!UNIT_VALID(unit)) {
        return SYS_ERR_UNAVAIL;
    }

    dev = m0ssq_mbox_dev[unit];
    if (dev == NULL) {
        return SYS_ERR;
    }

    /* Attach uC into mbox */
    dev->mbox[mbox_id].uc = uc;

    /* Install built-in software interrupt handler for MBOX. */
    return m0ssq_uc_swintr_handler_set(unit, uc,
                                       m0ssq_mbox_built_in_rxmsg_intr, (unit << 8) | uc);
}

int
m0ssq_mbox_process_recv_msgs(int unit, uint32 mbox_id)
{
    uint32 size;
    m0ssq_mbox_dev_t *dev;
    mbox_t *mbox;
    m0ssq_fifo_t *fifo;
    m0ssq_mbox_int_msg_t *int_msg = NULL;
    m0ssq_mbox_int_msg_t *int_resp = NULL;
    m0ssq_mbox_msg_t msg;
    m0ssq_mbox_resp_t resp;
    uint32 msg_cnt;
    int ret, rv;
    uint32 resp_pos;

    SHR_FUNC_ENTER(unit);

    if (!UNIT_VALID(unit))
    {
        return SYS_ERR_UNAVAIL;
    }

    dev = m0ssq_mbox_dev[unit];
    if (dev == NULL) {
        return SYS_ERR_UNAVAIL;
    }

    /* Get MBOX by ID. */
    mbox = &dev->mbox[mbox_id];

    /* Get MBOX RX channel. */
    fifo = &mbox->chan[MBOX_CHN_TYPE_RX];

    /* One FIFO can be used by only one thread. Lock it before use.*/
    m0ssq_fifo_lock(fifo);

    /* Allocate memory for mailbox message and response. */
    SHR_IF_ERR_EXIT
        (m0ssq_mbox_msg_resp_alloc(&int_msg, &int_resp,
                                   fifo->size));

    /* Start to receive messages. Up to MBOX_MAX_RX_MSGS_PER_CALL messages
     * can be received per call.
     */
    for (msg_cnt = 0; msg_cnt < MBOX_MAX_RX_MSGS_PER_CALL; msg_cnt++)
    {
        /* Read message header. */
        rv = m0ssq_fifo_peek_read(fifo,
                                  int_msg,
                                  MBOX_HDR_SIZE);
        if (rv == SYS_ERR_EMPTY) {

            /* FIFO is empty. */
            SHR_ERR_EXIT(SYS_OK);
        } else {

            /* Return if there is any other error. */
            SHR_IF_ERR_EXIT(rv);
        }

        size = int_msg->size * MBOX_ENT_SIZE + MBOX_HDR_SIZE;

        if (int_msg->flags & MBOX_RESP_REQUIRED) {
            resp_pos = m0ssq_fifo_rp_get(fifo);
        } else {
            resp_pos = 0;
        }

        /* Read entire message. */
        SHR_IF_ERR_EXIT
            (m0ssq_fifo_peek_read(fifo, int_msg, size));

        msg.id = int_msg->id;
        msg.datalen = int_msg->size * MBOX_ENT_SIZE;
        msg.data = int_msg->data;
        resp.datalen = 0;
        resp.data = int_resp->data;

        if (resp_pos) { /* If message response is required. */

            /* Invoke customer handler to process message. */
            ret = SYS_ERR;
            if (mbox->recv_msg_handler != NULL) {
                ret = mbox->recv_msg_handler(unit, &msg, &resp);
            }

            /* Prepare response to send back. */
            int_resp->id = int_msg->id;
            int_resp->flags = int_msg->flags;
            int_resp->size = (resp.datalen + MBOX_ENT_SIZE - 1) / MBOX_ENT_SIZE;

            if (int_msg->size >= int_resp->size && (ret == SYS_OK)) {
                int_resp->size = (resp.datalen + MBOX_ENT_SIZE - 1) / MBOX_ENT_SIZE;
                sal_memcpy(int_resp->data, resp.data, resp.datalen);

                /* Write the reponse data into FIFO */
                size = MBOX_HDR_SIZE + int_resp->size * MBOX_ENT_SIZE;
                SHR_IF_ERR_EXIT(m0ssq_fifo_pos_write(fifo,
                                                        resp_pos,
                                                        int_resp,
                                                        size));

                /* Mark the response flag as success and ready. */
                int_resp->flags &= ~(MBOX_RESP_REQUIRED);
                int_resp->flags |= (MBOX_RESP_SUCCESS | MBOX_RESP_READY);
            } else {
                if (msg.datalen >= MBOX_ENT_SIZE) {
                    int_resp->size = 1;
                    int_resp->data[0] = ret;
                } else {
                    int_resp->size = 0;
                }

                /* Write the reponse data into FIFO */
                size = MBOX_HDR_SIZE + int_resp->size * MBOX_ENT_SIZE;
                SHR_IF_ERR_EXIT(m0ssq_fifo_pos_write(fifo, resp_pos,
                                                           int_resp,
                                                           size));

                /* Mark the response flag as ready only. */
                int_resp->flags &= ~(MBOX_RESP_REQUIRED);
                int_resp->flags |= (MBOX_RESP_READY);
            }

            /* Update the flag field in last step of response. */
            SHR_IF_ERR_EXIT(m0ssq_fifo_pos_write(fifo,
                                                       resp_pos,
                                                       int_resp,
                                                       sizeof(uint32)));

        } else {  /* If message response is not required. */

            /* Invoke customer handler to process message. */
            if (mbox->recv_msg_handler != NULL) {
                if (SHR_FAILURE(mbox->recv_msg_handler(unit, &msg, NULL))) {
                    SHR_LOG_ERROR("Messgage handling failed for mbox %d msg id %d\n",  mbox_id, msg.id);
                };
            }
        };

        /* Message process complete, make rp move forward. */
        size = MBOX_HDR_SIZE + int_msg->size * MBOX_ENT_SIZE;
        m0ssq_fifo_rp_forward(fifo, size);
    }

exit:
    /* Free allocated memory in this function. */
    m0ssq_mbox_msg_resp_free(&int_msg, &int_resp);

    /* Release FIFO. */
    m0ssq_fifo_unlock(fifo);

    SHR_FUNC_EXIT();
}


int
m0ssq_mbox_msg_send(int unit, uint32 mbox_id,
                          m0ssq_mbox_msg_t *msg,
                          m0ssq_mbox_resp_t *resp)
{
    uint32 size;
    int timeout_cnt = MBOX_TX_TIMEOUT_CNT;
    uint32 wp = 0;
    bool require_response = (resp != NULL);
    m0ssq_fifo_t *fifo;
    m0ssq_mbox_dev_t *dev;
    m0ssq_mbox_int_msg_t *int_msg = NULL;
    m0ssq_mbox_int_msg_t *int_resp = NULL;

    SHR_FUNC_ENTER(unit);

    if (!UNIT_VALID(unit)) {
        return SYS_ERR_UNAVAIL;
    }

    /* Response data length is less than or
     * equal to messsage data length.
     */
    if ((msg == NULL) || (msg->datalen % 4))
    {
        return SYS_ERR_PARAMETER;
    }

    dev = m0ssq_mbox_dev[unit];
    if (dev == NULL ||
        msg == NULL ||
        dev->mbox[mbox_id].inuse == 0)
    {
        return SYS_ERR;
    };

    /* Get MBOX TX channel. */
    fifo  = &dev->mbox[mbox_id].chan[MBOX_CHN_TYPE_TX];

    /* One FIFO can be used by only one thread. Lock it before use.*/
    m0ssq_fifo_lock(fifo);

    /* Check if there is enough free space to write. */
    size = MBOX_HDR_SIZE + msg->datalen;
    SHR_IF_ERR_EXIT
        (m0ssq_fifo_write_check(fifo, size));

    /* Allocate memory for mailbox message and response. */
    SHR_IF_ERR_EXIT
        (m0ssq_mbox_msg_resp_alloc(&int_msg, &int_resp, size));


    /* Convert user message to internal mailbox message. */
    int_msg->id = msg->id;
    int_msg->size = (msg->datalen) / sizeof(uint32);
    if (msg->data) {
        sal_memcpy(int_msg->data, msg->data, msg->datalen);
    }

    /* Setup message flags if response is required. */
    if (require_response) {
        /*  Response is required. Mark relative flags. */
        int_msg->flags = MBOX_SYNC_MSG | MBOX_RESP_REQUIRED;

        /* Get current write pointer for message response*/
        wp = m0ssq_fifo_wp_get(fifo);

    } else {
        /*  Response is not required. Mark relative flags. */
        int_msg->flags = MBOX_ASYNC_STATUS;
    }

    /* Send message out. */
    SHR_IF_ERR_EXIT
        (m0ssq_fifo_write(fifo, (uint32 *) int_msg, size));

    /* Exit if reponse is not required. */
    if (!require_response) {
        SHR_EXIT();
    }

    /* Wait for response with a timeout. */
    while (timeout_cnt) {

        /* Try read response from FIFO. */
        SHR_IF_ERR_EXIT
            (m0ssq_fifo_pos_read(fifo, wp,
                                       (uint32 *) int_resp,
                                       size));

        /* Check if the response is ready. */
        if (int_resp->flags & MBOX_RESP_READY) {
            if (int_resp->flags & MBOX_RESP_SUCCESS) {
                if (resp->data && resp->datalen) {
                   sal_memcpy(resp->data, int_resp->data, resp->datalen);
                }
                SHR_LOG_DEBUG("Response ready %d %x %d\n", int_msg->id, *int_resp->data, resp->datalen);
                SHR_EXIT();
            } else {
                SHR_LOG_ERROR("Response a failure for mbox %d msg id %d\n", mbox_id, int_msg->id);
                if (int_resp->size && int_resp->data[0] != SYS_OK) {
                    SHR_ERR_EXIT(int_resp->data[0]);
                } else {
                    SHR_ERR_EXIT(SYS_ERR);
                }
            }
        }

        sal_usleep(100);
        timeout_cnt--;
    }

    if (!timeout_cnt) {
        SHR_LOG_ERROR("No response for msg %d\n", msg->id);
        SHR_ERR_EXIT(SYS_ERR_TIMEOUT);
    }

exit:
    /* Free allocated memory in this function. */
    m0ssq_mbox_msg_resp_free(&int_msg, &int_resp);

    /* Release FIFO. */
    m0ssq_fifo_unlock(fifo);

    SHR_FUNC_EXIT();
}

