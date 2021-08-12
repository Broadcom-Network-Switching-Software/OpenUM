/*! \file flmcs.c
 *
 * bcm5607x device-specific MCS driver.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include <system.h>
#include <utils/shr/shr_debug.h>
#include <mcs.h>
#include <mcs_internal.h>

/*******************************************************************************
 * Local definitions
 */

/* Devic-specific driver for bcm5607x. */
static mcs_drv_t bcm5607x_drv;

/* Base address of MCS block. */
#define MCS_BASE_ADDR           0x1100000

/*******************************************************************************
 * Internal functions.
 */
static int
bcm5607x_mcs_init(int unit)
{
    mcs_dev_t *dev;
    const mcs_drv_t *base_drv;

    SHR_FUNC_ENTER(unit);

    SHR_IF_ERR_EXIT
        (mcs_dev_get(unit, &dev));

    /* Setup chip-specific data. */
    dev->base_addr = MCS_BASE_ADDR;

    /* Initial dev objects data from template. */
    SHR_IF_ERR_EXIT
        (cmicx_mcs_dev_init(unit));

    /* Invoke function of base driver to init MCS block. */
    base_drv = cmicx_mcs_base_drv_get(unit);
    SHR_NULL_CHECK(base_drv, SYS_ERR_UNAVAIL);

    SHR_IF_ERR_EXIT
        (base_drv->init(unit));

exit:
    SHR_FUNC_EXIT();
}

/*******************************************************************************
 * Public Functions.
 */
int
bcm5607x_mcs_drv_init(int unit)
{
    const mcs_drv_t *base_drv;

    SHR_FUNC_ENTER(unit);

    /* Get base driver. */
    base_drv = cmicx_mcs_base_drv_get(unit);
    SHR_NULL_CHECK(base_drv, SYS_ERR_UNAVAIL);

    sal_memcpy(&bcm5607x_drv, base_drv, sizeof(mcs_drv_t));

    bcm5607x_drv.init = bcm5607x_mcs_init;

    SHR_IF_ERR_EXIT
        (mcs_drv_init(unit, &bcm5607x_drv));

    /* Init MCS system. */
    mcs_init(unit);

exit:
    SHR_FUNC_EXIT();
}

int
bcm5607x_mcs_drv_cleanup(int unit)
{
    return mcs_drv_init(unit, NULL);
}
