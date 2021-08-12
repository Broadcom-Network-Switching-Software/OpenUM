/*! \file cmicx_led.c
 *
 * CMICx LED host base driver.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include <system.h>

#include <led_internal.h>
#include <cmicx_m0ssq_internal.h>
#include <m0ssq_mbox.h>
#include <m0ssq_mem.h>
#include <cmicx_led_defs.h>
#include <cmicx_led_internal.h>
#include <cmicx_led.h>

#include <utils/shr/shr_debug.h>

/*******************************************************************************
 * Local variables.
 */
/* CMICx-LED driver specific data. */
static cmicx_led_dev_t *cmicx_led_devs[CONFIG_MAX_UNITS] = { NULL };

/*******************************************************************************
 * CMICx-LED host driver to control LED FW by MBOX(mailbox).
 */
static int
cmicx_led_control_data_write(int unit, int led_uc, int offset, uint8 data)
{
    CMICX_LED_CONTROL_DATAr_t led_control_data;
    cmicx_led_dev_t *cmicx_led_dev;

    SHR_FUNC_ENTER(unit);

    if (offset >= CMICX_LED_CONTROL_DATA_SIZE) {
        return SYS_ERR_PARAMETER;
    }

    cmicx_led_dev = cmicx_led_devs[unit];
    SHR_NULL_CHECK(cmicx_led_dev, SYS_ERR_UNAVAIL);

    SHR_IF_ERR_EXIT
        (READ_CMICX_LED_CONTROL_DATAr(cmicx_led_dev->led_shmem, offset / 4, &led_control_data));

    switch (offset % 4) {
    case 0:
         CMICX_LED_CONTROL_DATAr_BYTE_0f_SET(led_control_data, data);
    break;
    case 1:
         CMICX_LED_CONTROL_DATAr_BYTE_1f_SET(led_control_data, data);
    break;
    case 2:
         CMICX_LED_CONTROL_DATAr_BYTE_2f_SET(led_control_data, data);
    break;
    case 3:
    default:
         CMICX_LED_CONTROL_DATAr_BYTE_3f_SET(led_control_data, data);
    break;
    }

    SHR_IF_ERR_EXIT
        (WRITE_CMICX_LED_CONTROL_DATAr(cmicx_led_dev->led_shmem, offset / 4, led_control_data));

exit:
    SHR_FUNC_EXIT();
}

static int
cmicx_led_control_data_read(int unit, int led_uc, int offset, uint8 *data)
{
    CMICX_LED_CONTROL_DATAr_t led_control_data;
    cmicx_led_dev_t *cmicx_led_dev;

    SHR_FUNC_ENTER(unit);

    if (offset >= CMICX_LED_CONTROL_DATA_SIZE) {
        return SYS_ERR_PARAMETER;
    }

    cmicx_led_dev = cmicx_led_devs[unit];
    SHR_NULL_CHECK(cmicx_led_dev, SYS_ERR_UNAVAIL);

    SHR_IF_ERR_EXIT
        (READ_CMICX_LED_CONTROL_DATAr(cmicx_led_dev->led_shmem, offset / 4, &led_control_data));

    switch (offset % 4) {
    case 0:
         *data = CMICX_LED_CONTROL_DATAr_BYTE_0f_GET(led_control_data);
    break;
    case 1:
         *data = CMICX_LED_CONTROL_DATAr_BYTE_1f_GET(led_control_data);
    break;
    case 2:
         *data = CMICX_LED_CONTROL_DATAr_BYTE_2f_GET(led_control_data);
    break;
    case 3:
    default:
         *data = CMICX_LED_CONTROL_DATAr_BYTE_3f_GET(led_control_data);
    break;
    }

exit:
    SHR_FUNC_EXIT();
}

static int
cmicx_led_enable(int unit, bool enable)
{
    m0ssq_mbox_msg_t msg;
    m0ssq_mbox_resp_t resp;
    uint32 enable32;
    cmicx_led_dev_t *cmicx_led_dev;

    SHR_FUNC_ENTER(unit);

    cmicx_led_dev = cmicx_led_devs[unit];
    SHR_NULL_CHECK(cmicx_led_dev, SYS_ERR_UNAVAIL);
    if (enable) {
        enable32 = (LED_SW_LINK_DISABLE << 1) | (1);
    } else {
        enable32 = 0;
    }

    /* Prepare the message. */
    msg.id = LED_MSG_ENABLE;
    msg.datalen = sizeof(enable32);
    msg.data = &enable32;

    /* Request response without payload. */
    resp.data = NULL;
    resp.datalen = 0;

    SHR_IF_ERR_EXIT
        (m0ssq_mbox_msg_send(unit, cmicx_led_dev->mbox_id,
                                   &msg, &resp));

exit:
    SHR_FUNC_EXIT();
}

/*******************************************************************************
 * CMICx-LED Driver.
 */
static int
cmicx_led_uc_num_get(int unit, int *uc_num)
{
    SHR_FUNC_ENTER(unit);

    SHR_NULL_CHECK(uc_num, SYS_ERR_PARAMETER);

    *uc_num = CMICX_MAX_LEDUPS;

exit:
    SHR_FUNC_EXIT();
}

static int
cmicx_led_fw_start_get(int unit, int led_uc, int *start)
{
    cmicx_led_dev_t *cmicx_led_dev;

    SHR_FUNC_ENTER(unit);

    cmicx_led_dev = cmicx_led_devs[unit];
    SHR_NULL_CHECK(cmicx_led_dev, SYS_ERR);
    SHR_NULL_CHECK(start, SYS_ERR_PARAMETER);

    /* Get the LED enable state. */
    *start = cmicx_led_dev->enable;

exit:
    SHR_FUNC_EXIT();
}

static int
cmicx_led_fw_start_set(int unit, int led_uc, int start)
{
    cmicx_led_dev_t *cmicx_led_dev;
    bool startb = (start != 0);

    SHR_FUNC_ENTER(unit);

    cmicx_led_dev = cmicx_led_devs[unit];
    SHR_NULL_CHECK(cmicx_led_dev, SYS_ERR);

    /* Make sure Broadcom LED firmware is running. */
    SHR_IF_ERR_EXIT(cmicx_m0ssq_fw_led_init(unit, LED_UC));

    /* Enable CMICx LED. */
    SHR_IF_ERR_EXIT(cmicx_led_enable(unit, startb));

    /* Store the state of LED enable. */
    cmicx_led_dev->enable = startb;

exit:
    SHR_FUNC_EXIT();
}

static int
cmicx_led_config_init(int unit, int uc)
{
    cmicx_led_dev_t *cmicx_led_dev;

    SHR_FUNC_ENTER(unit);

    if (!UNIT_VALID(unit)) {
        SHR_ERR_EXIT(SYS_ERR_PARAMETER);
    }

    cmicx_led_dev = cmicx_led_devs[unit];
    SHR_NULL_CHECK(cmicx_led_dev, SYS_ERR);

    /* Get shared memory of LED. */
    SHR_IF_ERR_EXIT
        (m0ssq_mem_get(unit, uc, "led", &(cmicx_led_dev->led_shmem)));

    /* Get shared memory of LED. */
    SHR_IF_ERR_EXIT
        (m0ssq_mem_clear(cmicx_led_dev->led_shmem));

exit:
    SHR_FUNC_EXIT();
}

static int
cmicx_led_init(int unit)
{
    int port;
    led_dev_t *dev;
    cmicx_led_dev_t *cmicx_led_dev;
    uint32 mbox_id;
    U0_LED_ACCU_CTRLr_t ctrl;

    SHR_FUNC_ENTER(unit);

    if (!UNIT_VALID(unit)) {
        SHR_ERR_EXIT(SYS_ERR_PARAMETER);
    }

    /* Get LED driver-specific data. */
    SHR_IF_ERR_EXIT
        (led_dev_get(unit, &dev));

    /* Allocate a MBOX for CMICx-LED */
    SHR_IF_ERR_EXIT
        (m0ssq_mbox_alloc(unit, "led", &mbox_id));

    /* Stop LED accumulation. */
    READ_U0_LED_ACCU_CTRLr(unit, ctrl);
    U0_LED_ACCU_CTRLr_LED_ACCU_ENf_SET(ctrl, 0);
    WRITE_U0_LED_ACCU_CTRLr(unit, ctrl);

    sal_usleep(100);

    /* Let Broadcom LED firmware run. */
    SHR_IF_ERR_EXIT
        (cmicx_m0ssq_fw_led_init(unit, LED_UC));

    /* Allocate CMICx-LED driver-specific data */
    cmicx_led_dev = sal_alloc(sizeof(cmicx_led_dev_t), "bcmbdCmicxLedDev");
    SHR_NULL_CHECK(cmicx_led_dev, SYS_ERR_OUT_OF_RESOURCE);

    /* Reset the internal led port mapping  */
    for (port = 0; port < CONFIG_MAX_PORTS; port++) {
        dev->pport_to_led_uc[port] = LED_UC_INVALID;
        dev->pport_to_led_uc_port[port] = 0;
    }

    /* Initialize the internal led port mapping  */
    SOC_PPORT_ITER(port) {
        dev->pport_to_led_uc_port[port] = port - 1;
        dev->pport_to_led_uc[port] = 0;
    }

    /* Offset of available "control_data" space is zero. */
    dev->control_data_start[0] = 0x0;

    /* Initialize CMICx-LED driver specific data. */
    cmicx_led_dev->enable = 0;
    cmicx_led_dev->mbox_id = mbox_id;
    cmicx_led_devs[unit] = cmicx_led_dev;

    /* Config Broadcom LED firmware. */
    SHR_IF_ERR_EXIT
        (cmicx_led_config_init(unit, LED_UC));

exit:
    SHR_FUNC_EXIT();
}

static int
cmicx_led_cleanup(int unit)
{
    U0_LED_ACCU_CTRLr_t ctrl;

    SHR_FUNC_ENTER(unit);

    if (!UNIT_VALID(unit)) {
        SHR_ERR_EXIT(SYS_ERR_PARAMETER);
    }

    /* Stop LED accumulation. */
    READ_U0_LED_ACCU_CTRLr(unit, ctrl);
    U0_LED_ACCU_CTRLr_LED_ACCU_ENf_SET(ctrl, 0);
    WRITE_U0_LED_ACCU_CTRLr(unit, ctrl);

    sal_usleep(100);

    /* Stop Broadcom LED firmware. */
    SHR_IF_ERR_EXIT
        (cmicx_m0ssq_fw_led_cleanup(unit, LED_UC));

    sal_free(cmicx_led_devs[unit]);

exit:
    SHR_FUNC_EXIT();
}

static int
cmicx_led_fw_load(int unit, int led_uc, const uint8 *buf, int len)
{

    SHR_FUNC_ENTER(unit);

    if (len > CMICX_CUSTOMER_LED_FW_SIZE_MAX) {
        SHR_ERR_EXIT(SYS_ERR_PARAMETER);
    }

    /* Load customer LED firmware. */
    SHR_IF_ERR_EXIT
        (m0ssq_uc_fw_load(unit, LED_UC, LED_FW_OFFSET, buf, len));

exit:
    SHR_FUNC_EXIT();
}

static int
cmicx_led_pport_to_led_uc_port(int unit, int pport,
                               int *led_uc, int *led_uc_port)
{
    led_dev_t *dev = NULL;

    SHR_FUNC_ENTER(unit);

    SHR_IF_ERR_EXIT
        (led_dev_get(unit, &dev));

    *led_uc = dev->pport_to_led_uc[pport];
    *led_uc_port = dev->pport_to_led_uc_port[pport];

    if (*led_uc == LED_UC_INVALID) {
        /* The map does not exist. */
        SHR_ERR_EXIT(SYS_ERR);
    }

exit:
    SHR_FUNC_EXIT();
}

/* CMICx LED host base driver. */
static const led_drv_t cmicx_led_base_drv = {

    /*! Initialize LED driver. */
    .init = cmicx_led_init,

    /*! Clean up LED driver. */
    .cleanup = cmicx_led_cleanup,

    /*! LED firmware loader. */
    .fw_load = cmicx_led_fw_load,

    /*! Get LED uC number */
    .uc_num_get = cmicx_led_uc_num_get,

    /*! Stop/start LED uC. */
    .uc_start_set = cmicx_led_fw_start_set,

    /*! Get LED uC is started or not. */
    .uc_start_get = cmicx_led_fw_start_get,

    /*! Read LED uC control_data */
    .uc_control_data_read = cmicx_led_control_data_read,

    /*! Write LED uC control_data */
    .uc_control_data_write = cmicx_led_control_data_write,

    /*! Physical port to LED uc number and port */
    .pport_to_led_uc_port = cmicx_led_pport_to_led_uc_port,

};

/*******************************************************************************
 * Public Functions.
 */
const led_drv_t *
cmicx_led_base_drv_get(int unit)
{
    return &cmicx_led_base_drv;
}

int
cmicx_led_clk_freq_set(int unit, uint32 src_clk_freq,
                       uint32 led_clk_freq)
{
    uint32 clk_half_period;
    U0_LED_CLK_DIV_CTRLr_t clk_div_ctrl;

    SHR_FUNC_ENTER(unit);

    /* LEDCLK_HALF_PERIOD
     *  = [(required LED clock period in sec)/2]
     *    *(M0SS clock frequency in Hz)]
     *
     *  Where M0SS freqency is 858MHz and
     *  Typical LED clock period is 200ns(5MHz) = 2*10^-7
     */
    clk_half_period = (src_clk_freq + (led_clk_freq / 2)) / led_clk_freq;
    clk_half_period = clk_half_period / 2;

    SHR_IF_ERR_EXIT
        (READ_U0_LED_CLK_DIV_CTRLr(unit, clk_div_ctrl));
    U0_LED_CLK_DIV_CTRLr_LEDCLK_HALF_PERIODf_SET(clk_div_ctrl, clk_half_period);
    SHR_IF_ERR_EXIT
        (WRITE_U0_LED_CLK_DIV_CTRLr(unit, clk_div_ctrl));

exit:
    SHR_FUNC_EXIT();
}

int
cmicx_led_refresh_freq_set(int unit, uint32 src_clk_freq,
                           uint32 refresh_freq)
{
    uint32 refresh_period;
    U0_LED_REFRESH_CTRLr_t led_refresh_ctrl;

    SHR_FUNC_ENTER(unit);

    /* refresh period
     * = (required refresh period in sec)*(switch clock frequency in Hz)
     */
    refresh_period = (src_clk_freq + (refresh_freq / 2)) / refresh_freq;

    SHR_IF_ERR_EXIT
        (READ_U0_LED_REFRESH_CTRLr(unit, led_refresh_ctrl));
    U0_LED_REFRESH_CTRLr_REFRESH_CYCLE_PERIODf_SET
         (led_refresh_ctrl, refresh_period);
    SHR_IF_ERR_EXIT
        (WRITE_U0_LED_REFRESH_CTRLr(unit, led_refresh_ctrl));

exit:
    SHR_FUNC_EXIT();
}

int
cmicx_led_last_port_set(int unit, uint32 last_port)
{
    U0_LED_ACCU_CTRLr_t led_accu_ctrl;

    SHR_FUNC_ENTER(unit);

    /* Last port setting of LED */
    SHR_IF_ERR_EXIT
        (READ_U0_LED_ACCU_CTRLr(unit, led_accu_ctrl));
    U0_LED_ACCU_CTRLr_LAST_PORTf_SET(led_accu_ctrl, last_port);
    SHR_IF_ERR_EXIT
        (WRITE_U0_LED_ACCU_CTRLr(unit, led_accu_ctrl));

exit:
    SHR_FUNC_EXIT();
}
