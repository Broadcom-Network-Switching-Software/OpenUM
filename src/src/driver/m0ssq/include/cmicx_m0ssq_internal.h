/*! \file cmicx_m0ssq_internal.h
 *
 *  CMICx M0SSQ base driver.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef CMICX_M0SSQ_INTERNAL_H
#define CMICX_M0SSQ_INTERNAL_H

#include <m0ssq_internal.h>

/*!
 * \brief Create M0SSQ memory objects for CMICx and CMICx2 device.
 *
 * \param [in] unit Unit number.
 *
 * \retval SYS_OK No errors.
 */
extern int
cmicx_m0ssq_mem_create(int unit);

/*!
 * \brief Get CMICx-M0SSQ base driver.
 *
 * \param [in] unit Unit number.
 *
 * \retval Pointer to the base driver.
 */
extern const m0ssq_drv_t *
cmicx_m0ssq_base_drv_get(int unit);

/*!
 * \brief Initialze Broadcom M0 CMICx-Linkscan firmware.
 *
 * This function will download CMICx-Linkscan firmware and then
 * let the uC start running.
 *
 * \param [in] unit Unit number.
 * \param [in] uc uC number.
 *
 * \retval SYS_OK Intialization successed.
 * \retval SYS_ERR Intialization failed.
 */
extern int
cmicx_m0ssq_fw_linkscan_init(int unit, int uc);

/*!
 * \brief Cleanup Broadcom M0 CMICx-Linkscan firmware.
 *
 * This function will try to stop the uC for CMICx-Linkscan firmware.
 *
 * \param [in] unit Unit number.
 * \param [in] uc uC number.
 *
 * \retval SYS_OK Cleanup successed.
 * \retval SYS_ERR Cleanup failed.
 */
extern int
cmicx_m0ssq_fw_linkscan_cleanup(int unit, int uc);

/*!
 * \brief Initialze Broadcom M0 CMICx-LED firmware.
 *
 * This function will download CMICx-LED firmware and then
 * let the uC start running.
 *
 * \param [in] unit Unit number.
 * \param [in] uc uC number.
 *
 * \retval SYS_OK Intialization successed.
 * \retval SYS_ERR Intialization failed.
 */
extern int
cmicx_m0ssq_fw_led_init(int unit, int uc);

/*!
 * \brief Cleanup Broadcom M0 CMICx-LED firmware.
 *
 *  This function will try to stop the uC for run CMICx-LED firmware.
 *
 * \param [in] unit Unit number.
 * \param [in] uc uC number.
 *
 * \retval SYS_OK Cleanup successed.
 * \retval SYS_ERR Cleanup failed.
 */
extern int
cmicx_m0ssq_fw_led_cleanup(int unit, int uc);


#endif /* CMICX_M0SSQ_INTERNAL_H */
