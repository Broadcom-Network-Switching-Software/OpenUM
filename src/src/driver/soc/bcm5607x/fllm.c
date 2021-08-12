/*! \file fllm.c
 *
 * Linkscan chip-specific driver for BCM5607X.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#include <system.h>

#include <lm_drv_internal.h>
#include <lm_fw_linkscan.h>

#include <utils/shr/shr_debug.h>

static lm_drv_t bcm5607x_lm_drv;

/*******************************************************************************
 * Public functions
 */

/*!
 * \brief Allocate resources and attach specific operations.
 *
 * \param [in] drv Pointer to driver structure.
 *
 * \retval SHR_E_NONE No errors.
 */
int
bcm5607x_lm_drv_init(int unit)
{
    SHR_FUNC_ENTER(unit);

    bcm5607x_lm_drv.unit = unit;
    SHR_IF_ERR_EXIT
        (cmicx_fw_linkscan_drv_attach(unit, &bcm5607x_lm_drv));

    lm_hw_init(unit);

exit:
    SHR_FUNC_EXIT();
}

/*!
 * \brief Release resources.
 *
 * \param [in] drv Pointer to driver structure.
 *
 * \retval SHR_E_NONE No errors.
 */
int
bcm5607x_lm_drv_detach(int unit)
{
    return cmicx_fw_linkscan_drv_detach(unit, &bcm5607x_lm_drv);
}

