/*! \file cmicx_fw_linkscan.c
 *
 * Link Manager CMICx firmware linkscan handler.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include <system.h>

#include <lm_drv_internal.h>
#include <lm_fw_linkscan.h>
#include <cmicx_m0ssq_internal.h>
#include <cmicx_link_defs.h>
#include <m0ssq_mbox.h>
#include <m0ssq_mem.h>

#include <utils/shr/shr_debug.h>

/*******************************************************************************
 * Local definitions.
 */

/* uC number for firmware linkscan. */
#define FWLINKSCAN_UC   0

/* MBOX message id for firmware linkscan. */
typedef enum ls_msgtype_e {

    /* Configure ports which enable firmware linkscan. */
    LS_HW_CONFIG = 1,

    /* Check if FW is alive by getting FW version. */
    LS_HW_HEARTBEAT,

    /* Pause firmware linkscan. */
    LS_PAUSE,

    /* Continue/Start firmware linkscan. */
    LS_CONTINUE,

    /* Event to notify link status change. */
    LS_LINK_STATUS_CHANGE

} ls_msgtype_t;

/* Physical port bit map for FW linkscan MBOX communication. */
#define LS_PHY_PBMP_PORT_MAX     384
#define LS_PHY_PBMP_WORD_MAX     ((LS_PHY_PBMP_PORT_MAX + 31) / 32)
typedef struct ls_phy_pbmp_s {
    uint32_t  pbits[LS_PHY_PBMP_WORD_MAX];
} ls_phy_pbmp_t;

/* Minimun physical port bit map. */
#define MIN_PBMP_SIZE  (sizeof(ls_phy_pbmp_t) > sizeof(pbmp_t) ? \
                        sizeof(pbmp_t) : sizeof(ls_phy_pbmp_t))

/* Condition to bypass firmware linkscan driver. */
#define FW_LINKSCAN_BYPASS_CHK(unit)   do { } while(0)

/*! Local define. */
#define LS_SHMEM(unit)  ls_shmem[0]
/*******************************************************************************
 * Local variables.
 */

/* Linkscan firmware object */
static lm_fw_drv_t lm_fw_drv[1];
static m0ssq_mem_t *ls_shmem[1];

/*******************************************************************************
 * Internal Functions.
 */
static int
cmicx_fw_linkscan_msg_handler(int unit,
                              m0ssq_mbox_msg_t *msg,
                              m0ssq_mbox_resp_t *resp)
{
    int size = MIN_PBMP_SIZE;
    pbmp_t link_pbmp;
    lm_fw_drv_t *fw_drv = &lm_fw_drv[unit];

    SHR_FUNC_ENTER(unit);
    SHR_NULL_CHECK(msg, SYS_ERR);

    switch (msg->id) {
    case LS_LINK_STATUS_CHANGE:

        /* Clear link pbmp. */
        PBMP_CLEAR(link_pbmp);

        /* Update cached link pbmp. */
        sal_memcpy(&link_pbmp, msg->data, size);
        PBMP_ASSIGN(fw_drv->link_stat, link_pbmp);

        /* Notify LM state changed. */
        if (fw_drv->intr_func) {
            fw_drv->intr_func(fw_drv->unit);
        }

        SHR_LOG_DEBUG("Link Change\n");
        break;
    default:
        SHR_LOG_ERROR("Linkscan FW unknown message\n");
        SHR_ERR_EXIT(SYS_ERR);
        break;
    }

exit:
    SHR_FUNC_EXIT();
}

/*!
 * \biref Initialize linkscan firmware config.
 *
 * \param [in] unit Unit number..
 *
 * \retval SYS_OK No errors.
 */
static int
cmicx_fw_linkscan_config_init(int unit) {

    FWLINKSCAN_CTRLr_t ctrl;

    SHR_FUNC_ENTER(unit);

    /* Get shared memory of firmware linkscan. */
    SHR_IF_ERR_EXIT
        (m0ssq_mem_get(unit, FWLINKSCAN_UC,
                       "linkscan", &LS_SHMEM(unit)));

    /* Clear shared memory of firmware linkscan as zero. */
    SHR_IF_ERR_EXIT
        (m0ssq_mem_clear(LS_SHMEM(unit)));

    /* Pause firmware linkscan. */
    FWLINKSCAN_CTRLr_CLR(ctrl);
    FWLINKSCAN_CTRLr_PAUSEf_SET(ctrl, 1);
    SHR_IF_ERR_EXIT
        (WRITE_FWLINKSCAN_CTRLr(LS_SHMEM(unit), ctrl));

exit:
    SHR_FUNC_EXIT();

}

static int
cmicx_fw_linkscan_init(int unit)
{
    lm_fw_drv_t *fw_drv = (lm_fw_drv_t *)&lm_fw_drv[unit];
    uint32_t mbox_id, uc = FWLINKSCAN_UC;

    SHR_FUNC_ENTER(unit);

    FW_LINKSCAN_BYPASS_CHK(unit);

    if (fw_drv->fw_init) {
        SHR_EXIT();
    }

    /* Allocate a mbox for Linkscan. */
    SHR_IF_ERR_EXIT
        (m0ssq_mbox_alloc(unit, "linkscan", &mbox_id));

    /* Config linkscan shared memory. */
    SHR_IF_ERR_EXIT
        (cmicx_fw_linkscan_config_init(unit));

    /* Download and start linkscan firmware. */
    SHR_IF_ERR_EXIT
        (cmicx_m0ssq_fw_linkscan_init(unit, uc));

    /* Install linkscan rx msg handler. */
    SHR_IF_ERR_EXIT
        (m0ssq_mbox_msg_handler_set(unit, mbox_id,
                                    cmicx_fw_linkscan_msg_handler));

    /* Attach linkscan MBOX to uC0's swintr. */
    SHR_IF_ERR_EXIT
        (m0ssq_mbox_uc_swintr_attach(unit, mbox_id, uc));

    fw_drv->unit = unit;
    fw_drv->mbox_id = mbox_id;
    fw_drv->fw_init = 1;

    SHR_LOG_DEBUG("Linkscan FW init\n");

exit:
    SHR_FUNC_EXIT();
}

static int
cmicx_fw_linkscan_cleanup(int unit)
{
    lm_fw_drv_t *fw_drv = (lm_fw_drv_t *)&lm_fw_drv[unit];
    uint32_t uc = FWLINKSCAN_UC;

    SHR_FUNC_ENTER(unit);

    FW_LINKSCAN_BYPASS_CHK(unit);

    if (!fw_drv->fw_init) {
        SHR_EXIT();
    }

    /* Disable a uC software interrupt */
    SHR_IF_ERR_EXIT
        (m0ssq_uc_swintr_enable_set(unit, uc, 0));

    /* Free mailbox resource. */
    SHR_IF_ERR_EXIT
        (m0ssq_mbox_free(unit, fw_drv->mbox_id));

    fw_drv->fw_init = 0;

    SHR_LOG_DEBUG("Linkscan FW cleanup\n");

exit:
    SHR_FUNC_EXIT();

}

static int
cmicx_fw_linkscan_config(int unit, pbmp_t pbm)
{
    lm_fw_drv_t *fw_drv = (lm_fw_drv_t *)&lm_fw_drv[unit];
    m0ssq_mbox_msg_t msg;
    m0ssq_mbox_resp_t resp;
    ls_phy_pbmp_t ls_pbmp, ls_link_pbmp;
    int size = MIN_PBMP_SIZE;

    SHR_FUNC_ENTER(unit);

    FW_LINKSCAN_BYPASS_CHK(unit);

    if (!fw_drv->fw_init) {
        SHR_ERR_EXIT(SYS_ERR_UNAVAIL);
    }

    sal_memset(&ls_pbmp, 0, sizeof(ls_pbmp));
    sal_memcpy(&ls_pbmp, &pbm, size);

    msg.id = LS_HW_CONFIG;
    msg.datalen = sizeof(ls_phy_pbmp_t);
    msg.data = (uint32_t *) &ls_pbmp;
    resp.datalen = sizeof(ls_phy_pbmp_t);
    resp.data = (uint32_t *) &ls_link_pbmp;

    /* Send "configure" message to setup valid port bit map. */
    SHR_IF_ERR_EXIT
        (m0ssq_mbox_msg_send(unit, fw_drv->mbox_id, &msg, &resp));

    /*
     * Response contains current link status.
     * Copy the response (link status) to cached link status.
     */
    sal_memset(&pbm, 0, sizeof(pbmp_t));
    sal_memcpy(&pbm, resp.data, size);
    PBMP_ASSIGN(fw_drv->link_stat, pbm);

    SHR_LOG_DEBUG("Linkscan FW config\n");

exit:
    SHR_FUNC_EXIT();

}

static int
cmicx_fw_linkscan_link_get(int unit, pbmp_t *pbm)
{
    lm_fw_drv_t *fw_drv = (lm_fw_drv_t *)&lm_fw_drv[unit];

    SHR_FUNC_ENTER(unit);

    FW_LINKSCAN_BYPASS_CHK(unit);

    /* Get firmware link status from cached link status. */
    PBMP_ASSIGN(*pbm, fw_drv->link_stat);

    SHR_LOG_DEBUG("Linkscan FW states get\n");

    SHR_FUNC_EXIT();
}

static int
cmicx_fw_linkscan_intr_func_set(int unit, sys_intr_f intr_func)
{
    lm_fw_drv_t *fw_drv = (lm_fw_drv_t *)&lm_fw_drv[unit];

    SHR_FUNC_ENTER(unit);

    fw_drv->intr_func = intr_func;

    SHR_EXIT();

exit:
    SHR_FUNC_EXIT();
}

static int
cmicx_fw_linkscan_stop(int unit)
{
    lm_fw_drv_t *fw_drv = (lm_fw_drv_t *)&lm_fw_drv[unit];
    m0ssq_mbox_msg_t msg;
    m0ssq_mbox_resp_t resp;
    uint32_t ret, uc = FWLINKSCAN_UC;

    SHR_FUNC_ENTER(unit);

    FW_LINKSCAN_BYPASS_CHK(unit);

    if (!fw_drv->fw_init) {
        SHR_ERR_EXIT(SYS_ERR_UNAVAIL);
    }

    /* Disable a uC software interrupt */
    SHR_IF_ERR_EXIT
        (m0ssq_uc_swintr_enable_set(unit, uc, 0));

    msg.id = LS_PAUSE;
    msg.datalen = sizeof(ret);
    msg.data = &ret;
    resp.datalen = 0;
    resp.data = &ret;

    /* Send "pause" message to FW. */
    SHR_IF_ERR_EXIT
        (m0ssq_mbox_msg_send(unit, fw_drv->mbox_id, &msg, &resp));

    /* Purge the mailbox. */
    m0ssq_mbox_process_recv_msgs(unit, fw_drv->mbox_id);

    SHR_LOG_DEBUG("Linkscan FW stop\n");

exit:
    SHR_FUNC_EXIT();
}

static int
cmicx_fw_linkscan_start(int unit)
{
    lm_fw_drv_t *fw_drv = (lm_fw_drv_t *)&lm_fw_drv[unit];
    m0ssq_mbox_msg_t msg;
    m0ssq_mbox_resp_t resp;
    uint32_t ret, uc = FWLINKSCAN_UC;

    SHR_FUNC_ENTER(unit);

    FW_LINKSCAN_BYPASS_CHK(unit);

    if (!fw_drv->fw_init) {
        SHR_ERR_EXIT(SYS_ERR_UNAVAIL);
    }

    /* Enable a uC software interrupt */
    SHR_IF_ERR_EXIT
        (m0ssq_uc_swintr_enable_set(unit, uc, 1));

    msg.id = LS_CONTINUE;
    msg.datalen = sizeof(ret);
    msg.data = &ret;
    resp.datalen = 0;
    resp.data = &ret;

    /* Send "continue" message to FW. */
    SHR_IF_ERR_EXIT
        (m0ssq_mbox_msg_send(unit, fw_drv->mbox_id, &msg, &resp));

    SHR_LOG_DEBUG("Linkscan FW start\n");

exit:
    SHR_FUNC_EXIT();
}

/*******************************************************************************
 * Public Functions.
 */
int
cmicx_fw_linkscan_drv_attach(int unit, lm_drv_t *drv)
{
    drv->hw_init = cmicx_fw_linkscan_init;
    drv->hw_cleanup = cmicx_fw_linkscan_cleanup;
    drv->hw_config = cmicx_fw_linkscan_config;
    drv->hw_link_get = cmicx_fw_linkscan_link_get;
    drv->hw_intr_cb_set = cmicx_fw_linkscan_intr_func_set;
    drv->hw_scan_stop = cmicx_fw_linkscan_stop;
    drv->hw_scan_start = cmicx_fw_linkscan_start;

    return lm_drv_attach(unit, drv);
}

int
cmicx_fw_linkscan_drv_detach(int unit, lm_drv_t *drv)
{
    return lm_drv_attach(unit, NULL);
}
