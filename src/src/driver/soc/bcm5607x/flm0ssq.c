/*! \file flm0ssq.c
 *
 * Firelight device-specific M0SSQ driver.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#include <system.h>
#include <m0ssq.h>
#include <m0ssq_internal.h>
#include <cmicx_m0ssq_internal.h>

#include <utils/shr/shr_debug.h>
/*******************************************************************************
 * Local definitions
 */

/* Base address of M0SSQ block. */
#define M0SSQ_BASE_ADDR           0x1300000

/* Device id. */
#define BCM5607X_DEVICE_ID        0xb070

/*******************************************************************************
 * Local data
 */

/* Device-specific M0SSQ driver. */
static m0ssq_drv_t bcm5607x_m0ssq_drv;

/*******************************************************************************
 * Firelight M0SSQ driver.
 */
static int
bcm5607x_m0ssq_init(int unit)
{
    m0ssq_dev_t *dev;
    const m0ssq_drv_t *base_drv;
    int uc;

    SHR_FUNC_ENTER(unit);

    SHR_IF_ERR_EXIT
        (m0ssq_dev_get(unit, &dev));

    /* Setup chip-specific data. */
    dev->base_addr = M0SSQ_BASE_ADDR;

    /* Create mem objects from template. */
    SHR_IF_ERR_EXIT
        (cmicx_m0ssq_mem_create(unit));

    for (uc = 0; uc < dev->num_of_uc; uc ++) {
        dev->fwconfig[uc].devid = BCM5607X_DEVICE_ID;
    }

    /* Invoke function of base driver to init M0SSQ block. */
    base_drv = cmicx_m0ssq_base_drv_get(unit);
    SHR_NULL_CHECK(base_drv, SYS_ERR);
    SHR_IF_ERR_EXIT
        (base_drv->init(unit));

exit:
    SHR_FUNC_EXIT();
}

/*******************************************************************************
 * Public Functions.
 */
int
bcm5607x_m0ssq_drv_init(int unit)
{
    const m0ssq_drv_t *base_drv;

    SHR_FUNC_ENTER(unit);

    if (!UNIT_VALID(unit)) {
        return SYS_ERR_UNAVAIL;
    }

    /* Get base driver. */
    base_drv = cmicx_m0ssq_base_drv_get(unit);
    SHR_NULL_CHECK(base_drv, SYS_ERR);

    /* Inherit driver methods from M0SSQ base driver. */
    sal_memcpy(&bcm5607x_m0ssq_drv, base_drv, sizeof(m0ssq_drv_t));

    /* Override chip-specific method. */
    bcm5607x_m0ssq_drv.init = bcm5607x_m0ssq_init;

    SHR_IF_ERR_EXIT
        (m0ssq_drv_init(unit, &bcm5607x_m0ssq_drv));

    m0ssq_init(unit);

exit:
    SHR_FUNC_EXIT();
}

int
bcm5607x_m0ssq_drv_cleanup(int unit)
{
    return m0ssq_drv_init(unit, NULL);
}
