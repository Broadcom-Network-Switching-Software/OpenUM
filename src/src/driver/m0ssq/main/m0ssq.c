/*! \file m0ssq.c
 *
 * M0SSQ (ARM Cortex-M0 based Sub-System Quad) driver.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include <system.h>
#include <stddef.h>

#include <m0ssq_internal.h>
#include <m0ssq_mem.h>
#include <m0ssq_mbox_internal.h>
#include <utils/shr/shr_debug.h>

/*******************************************************************************
 * Local definition.
 */
#define DRV_CALL(_d, _f, _a) \
        ((_d) == NULL ? SYS_ERR_UNAVAIL: \
              ((_d)->_f == NULL ? SYS_ERR_UNAVAIL: (_d)->_f _a))

#define M0SSQ_INIT(_u) \
        DRV_CALL(bd_m0ssq_drv[_u], init, ((_u)))

#define M0SSQ_UC_START_SET(_u, _uc, _start) \
        DRV_CALL(bd_m0ssq_drv[_u], uc_start_set, \
                     ((_u), (_uc), (_start)))

#define M0SSQ_UC_START_GET(_u, _uc, _start) \
        DRV_CALL(bd_m0ssq_drv[_u], uc_start_get, \
                 ((_u), (_uc), (_start)))

/*******************************************************************************
 * Local variables.
 */
static const m0ssq_drv_t *bd_m0ssq_drv[CONFIG_MAX_UNITS] = { NULL };
static m0ssq_dev_t *bd_m0ssq_dev[CONFIG_MAX_UNITS] = { NULL };

/*******************************************************************************
 * Internal functions.
 */

/*******************************************************************************
 * Public functions
 */
int
m0ssq_drv_init(int unit, const m0ssq_drv_t *drv)
{
    SHR_FUNC_ENTER(unit);

    if (!UNIT_VALID(unit)) {
        SHR_ERR_EXIT(SYS_ERR);
    }

    bd_m0ssq_drv[unit] = drv;

exit:
    SHR_FUNC_EXIT();
}

int
m0ssq_dev_get(int unit, m0ssq_dev_t **dev)
{
    SHR_FUNC_ENTER(unit);

    if (!UNIT_VALID(unit)) {
        SHR_ERR_EXIT(SYS_ERR);
    }

    *dev = bd_m0ssq_dev[unit];
    SHR_NULL_CHECK(*dev, SYS_ERR_UNAVAIL);

exit:
    SHR_FUNC_EXIT();
}

int
m0ssq_uc_num_get(int unit)
{
    m0ssq_dev_t *dev;

    if (!UNIT_VALID(unit)) {
        return 0;
    }

    dev = bd_m0ssq_dev[unit];
    if (dev) {
        return dev->num_of_uc;
    }

    return 0;
}

int
m0ssq_uc_swintr_handler_set(int unit, uint32 uc,
                            board_intr_f handler,
                            uint32 param)
{
    m0ssq_dev_t *dev;

    SHR_FUNC_ENTER(unit);

    if (!UNIT_VALID(unit)) {
        SHR_ERR_EXIT(SYS_ERR);
    }

    dev = bd_m0ssq_dev[unit];
    SHR_NULL_CHECK(dev, SYS_ERR_UNAVAIL);

    if (uc >= dev->num_of_uc) {
        SHR_ERR_EXIT(SYS_ERR_UNAVAIL);
    }

    dev->uc_swintr_handler[uc] = handler;
    dev->uc_swintr_param[uc] = param;

exit:
    SHR_FUNC_EXIT();
}

int
m0ssq_uc_swintr_enable_set(int unit, uint32 uc,
                                 bool enable)
{
    m0ssq_dev_t *dev;

    SHR_FUNC_ENTER(unit);

    if (!UNIT_VALID(unit)) {
        SHR_ERR_EXIT(SYS_ERR);
    }

    dev = bd_m0ssq_dev[unit];
    SHR_NULL_CHECK(dev, SYS_ERR_UNAVAIL);

    if (uc >= dev->num_of_uc) {
        SHR_ERR_EXIT(SYS_ERR_UNAVAIL);
    }

    /* Setting enable flag. */
    dev->uc_swintr_enable[uc] = enable;

exit:
    SHR_FUNC_EXIT();
}

int
m0ssq_init(int unit)
{
    m0ssq_dev_t *dev = NULL;

    SHR_FUNC_ENTER(unit);

    if (!UNIT_VALID(unit)) {
        SHR_ERR_EXIT(SYS_ERR);
    }

    if (bd_m0ssq_drv[unit] == NULL) {
        SHR_EXIT();
    }

    /* Allocate device-specific data. */
    dev = sal_alloc(sizeof(m0ssq_dev_t), "bcmbdM0ssqDev");
    SHR_NULL_CHECK(dev, SYS_ERR_OUT_OF_RESOURCE);

    /* Install device-specific data. */
    bd_m0ssq_dev[unit] = dev;

    /* Initialize chip specific part. */
    SHR_IF_ERR_EXIT(M0SSQ_INIT(unit));

    /*
     * Initialize M0SSQ MBOX driver for 2 applications,
     * led and linkscan.
     */
    SHR_IF_ERR_EXIT(m0ssq_mbox_init(unit, &(dev->sram), 2));

exit:
    if (SHR_FUNC_ERR()) {

        /* Uninstall device-specific data. */
        if (UNIT_VALID(unit)) {
            bd_m0ssq_dev[unit] = NULL;
        }
        sal_free(dev);
    }
    SHR_FUNC_EXIT();
}

int
m0ssq_uc_fw_load(int unit, uint32 uc, uint32 offset,
                 const uint8_t *buf, uint32 len)
{
    m0ssq_mem_t *mem;
    m0ssq_dev_t *dev = NULL;

    SHR_FUNC_ENTER(unit);

    dev = bd_m0ssq_dev[unit];
    SHR_NULL_CHECK(dev, SYS_ERR_UNAVAIL);

    /* Get uC program memory object. */
    mem = &dev->m0_tcm[uc];

    /* Write firmware binary into program memory. */
    SHR_IF_ERR_EXIT
        (m0ssq_mem_write(mem, offset, buf, len));

exit:
    SHR_FUNC_EXIT();
}

int
m0ssq_uc_start(int unit, uint32 uc)
{
    m0ssq_mem_t *mem;
    m0ssq_dev_t *dev;
    uint32 chksum;

    SHR_FUNC_ENTER(unit);

    dev = bd_m0ssq_dev[unit];
    SHR_NULL_CHECK(dev, SYS_ERR_UNAVAIL);

    if (uc >= dev->num_of_uc) {
        SHR_ERR_EXIT(SYS_ERR_UNAVAIL);
    }

    /* Calculate checksum of firmware config. */
    SHR_IF_ERR_EXIT
        (m0ssq_fwconfig_chksum(dev->fwconfig[uc], &chksum));
    dev->fwconfig[uc].chksum = chksum;

    /* Fill out the content of firmware config memory. */
    mem = &dev->fwconfig_mem[uc];
    SHR_IF_ERR_EXIT
        (m0ssq_mem_write(mem, 0, (const uint8 *)&dev->fwconfig[uc],
                      sizeof(fwconfig_t)));

    /* Let uc start running. */
    SHR_IF_ERR_EXIT
        (M0SSQ_UC_START_SET(unit, uc, 1));

exit:
    SHR_FUNC_EXIT();
}

int
m0ssq_uc_stop(int unit, uint32 uc)
{
    m0ssq_dev_t *dev;

    SHR_FUNC_ENTER(unit);

    if (!UNIT_VALID(unit)) {
        SHR_ERR_EXIT(SYS_ERR);
    }

    dev = bd_m0ssq_dev[unit];
    SHR_NULL_CHECK(dev, SYS_ERR_UNAVAIL);

    if (uc >= dev->num_of_uc) {
        SHR_ERR_EXIT(SYS_ERR_UNAVAIL);
    }

    /* Let uc stop running. */
    SHR_IF_ERR_EXIT(M0SSQ_UC_START_SET(unit, uc, 0));

exit:
    SHR_FUNC_EXIT();
}

int
m0ssq_uc_start_get(int unit, uint32 uc, bool *start)
{
    SHR_FUNC_ENTER(unit);

    if (!UNIT_VALID(unit)) {
        SHR_ERR_EXIT(SYS_ERR);
    }

    SHR_IF_ERR_EXIT(M0SSQ_UC_START_GET(unit, uc, start));

exit:
    SHR_FUNC_EXIT();
}

int
m0ssq_cleanup(int unit)
{
    m0ssq_dev_t *dev = NULL;

    SHR_FUNC_ENTER(unit);

    if (!UNIT_VALID(unit)) {
        SHR_ERR_EXIT(SYS_ERR);
    }

    if (bd_m0ssq_drv[unit] == NULL) {
        SHR_EXIT();
    }

    dev = bd_m0ssq_dev[unit];
    SHR_NULL_CHECK(dev, SYS_OK);

    /* Cleanup MBOX driver. */
    SHR_IF_ERR_CONT
        (m0ssq_mbox_cleanup(unit));

    /* Cleanup M0 driver. */
    bd_m0ssq_dev[unit] = NULL;

exit:
    /* Free device-specific data. */
    sal_free(dev);
    SHR_FUNC_EXIT();
}

int
m0ssq_fwconfig_chksum(fwconfig_t fwcfg, uint32 *pchksum)
{
    int chksum = 0;
    uint32 *ptr = (uint32 *) &fwcfg;
    uint32 i;

    if (pchksum == NULL) {
        return SYS_ERR_PARAMETER;
    }

    for (i = 0;
         i < offsetof(fwconfig_t, chksum); i += 4)
    {
        chksum += ptr[i / 4];
    }

    *pchksum = chksum;

    return SYS_OK;
}
