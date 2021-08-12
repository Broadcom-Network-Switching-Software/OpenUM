/*! \file cmicx_miim.h
 *
 * MIIM APIs.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef CMICX_MIIM_H
#define CMICX_MIIM_H

#include <system.h>

/*! Types of MIIM operations. */
typedef enum cmicx_miim_opcode_e {
    CMICX_MIIM_OPC_NONE = 0,
    CMICX_MIIM_OPC_CL22_READ,
    CMICX_MIIM_OPC_CL22_WRITE,
    CMICX_MIIM_OPC_CL45_READ,
    CMICX_MIIM_OPC_CL45_WRITE,
    CMICX_MIIM_OPC_COUNT
} cmicx_miim_opcode_t;

/*! Generic MIIM operation type. */
typedef struct cmicx_miim_op_s {

    /*! Type of MIIM operation to perform. */
    cmicx_miim_opcode_t opcode;

    /*! MIIM hardware channel (use -1 to auto-select). */
    int chan;

    /*! Set to true to select internal MIIM bus(es). */
    bool internal;

    /*! Bus number (unused if \c busmap is non-zero). */
    uint32_t busno;

    /*!
     * Bitmap of buses on which to perform the operation. Used for
     * broadcasting to multiple PHYs. Should only be used for write
     * operations. Overrides \c busno if non-zero.
     */
    uint32_t busmap;

    /*! PHY address on MIIM bus. */
    uint32_t phyad;

    /*! Clause 45 PHY device number. */
    uint32_t devad;

    /*! PHY register address. */
    uint32_t regad;

} cmicx_miim_op_t;

/*!
 * The MIIM data rate is calculated as a fraction of a base frequency,
 * and this structure defines the dividend and divisor of this
 * fraction.
 */
typedef struct cmicx_miim_rate_config_s {

    /*!
     * The MIIM data rate is calculated as a fraction of a base
     * frequency, and this field defines the dividend of this fraction.
     *
     * If this field is 0, then a default value of 1 will be used
     * instead.
     */
    uint32_t dividend;

    /*!
     * The MIIM data rate is calculated as a fraction of a base
     * frequency, and this field defines the divisor of this fraction.
     *
     * If this field is 0, then a default value of 1 will be used
     * instead.
     */
    uint32_t divisor;

} cmicx_miim_rate_config_t;

/*!
 * \brief Perform MIIM operation.
 *
 * This function will perform a read or write access on the specified
 * MIIM bus using either clause 22 or clause 45 access.
 *
 * \param [in] unit Unit number.
 * \param [in] op Structure with MIIM operation parameters.
 * \param [in,out] data Data to write or data read.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR Hardware access returned an error.
 * \retval SYS_ERR_TIMEOUT Operation did not complete within normal time.
 */
extern int
cmicx_miim_op(int unit, cmicx_miim_op_t *op, uint32_t *data);

/*!
 * \brief Configure MIIM data rate parameters.
 *
 * If \c busno is -1, then the configuration will be applied to all
 * MIIM buses. Some device types only support this option as all buses
 * share the same hardware configuration.
 *
 * Note that the data rate calculation is device-specific based on
 * available clock sources and dividers.
 *
 * \param [in] unit Unit number.
 * \param [in] internal Apply configuration to internal MIIM bus.
 * \param [in] busno MIIM bus number (use -1 for all).
 * \param [in] ratecfg MIIM data rate configuration.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_PARAMETER Unsupported MIIM bus parameter.
 */
extern int
cmicx_miim_rate_config_set(int unit, bool internal, int busno,
                           cmicx_miim_rate_config_t *ratecfg);

#endif /* CMICX_MIIM_H */
