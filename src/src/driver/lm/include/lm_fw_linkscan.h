/*! \file lm_fw_linkscan.h
 *
 * Link Manager firmware linkscan definitions.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef LM_FW_LINKSCAN_H
#define LM_FW_LINKSCAN_H

#include <lm_drv_internal.h>

/*!
 * \brief Link Manager firmware driver object.
 *
 * The Link Manager firware driver object is used to initialize and access
 * firmware linkscan for a switch device.
 */
typedef struct lm_fw_drv_s {

    /*! Unit number. */
    int unit;

    /*! Cached link status. */
    pbmp_t link_stat;

    /*! Interrupt callback for letting LM to handle link change event. */
    sys_intr_f intr_func;

    /*! Mailbox ID to communicate with linkscan FW. */
    uint32 mbox_id;

    /*! Flag to indicate FW linkscan driver is initialized or not. */
    bool fw_init;

} lm_fw_drv_t;

/*!
 * \brief Mapping exception bits within linkscan registers.
 *
 * By default, the link bit offset within linkscan register is equal to
 * (physical port number - 1). But sometimes there are some exceptions
 * due to hardware design. This structure is to define those exceptions.
 */
typedef struct lm_fw_pport_to_lsbit_s {

    /*! Physical port number. */
    int pport;

    /*! Bits offset for port within linkscan register. */
    int lsbit;

} lm_fw_pport_to_lsbit_t;

/*!
 * \brief Allocate resources and attach firmware linkscan operations.
 *
 * \param [in] unit Unit number.
 * \param [in] drv Pointer to driver structure.
 *
 * \retval SHR_E_NONE No errors.
 */
int
cmicx_fw_linkscan_drv_attach(int unit, lm_drv_t *drv);

/*!
 * \brief Release resources.
 *
 * \param [in] unit Unit number.
 * \param [in] drv Pointer to driver structure.
 *
 * \retval SHR_E_NONE No errors.
 */
int
cmicx_fw_linkscan_drv_detach(int unit, lm_drv_t *drv);

#endif /* LM_FW_LINKSCAN_H */
