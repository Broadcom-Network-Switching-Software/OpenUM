/*! \file m0ssq.h
 *
 *  APIs for M0SSQ (ARM Cortex-M0 based Sub-System Quad).
 *
 *      M0SSQ contains four ARM Cortex-M0 uCs and a shared
 *  SRAM.
 *      User can use API in this file to
 *          - Download M0 firmware.
 *          - Start/stop M0 uc running.
 *          - Install/uninstall software interrupt handler of M0 uCs.
 *          - Enable/Disable software interrupt of M0 uCs.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef M0SSQ_H
#define M0SSQ_H

/*******************************************************************************
 * M0SSQ public APIs.
 */
/*!
 * \brief Load firmware into a uC of M0SSQ.
 *
 * This function loads firmware into uC memory.
 *
 * \param [in] unit Unit number.
 * \param [in] uc uC controller number.
 * \param [in] offset Byte offset of uC memory.
 * \param [in] buf Firmware buffer.
 * \param [in] len Length of firmware buffer.
 *
 * \retval SYS_OK Firmware successfully loaded.
 * \retval SYS_ERR_XXX Failed to load firmware.
 */
extern int
m0ssq_uc_fw_load(int unit, uint32 uc, uint32 offset,
                 const uint8 *buf, uint32 len);

/*!
 * \brief Get number of uC in M0SSQ.
 *
 * \param [in] unit Unit number.
 *
 * \retval Number of uC.
 */
extern int
m0ssq_uc_num_get(int unit);

/*!
 * \brief Start a M0SSQ uC.
 *
 * \param [in]  unit Unit number.
 * \param [in]  uc uC number.
 *
 * \retval SYS_OK Start a uC successfully.
 * \retval SYS_ERR_XXX Fail to start a uC.
 */
extern int
m0ssq_uc_start(int unit, uint32 uc);

/*!
 * \brief Stop a M0SSQ uC.
 *
 * \param [in]  unit Unit number.
 * \param [in]  uc uC number.
 *
 * \retval SYS_OK Stop a uC successfully.
 * \retval SYS_ERR_XXX Fail to stop a uC.
 */
extern int
m0ssq_uc_stop(int unit, uint32 uc);

/*!
 * \brief Check if uC is started or not.
 *
 * \param [in]  unit Unit number.
 * \param [in]  uc uC number.
 * \param [out] start Indicate uC is started or not.
 *
 * \return SYS_OK if successful.
 */
extern int
m0ssq_uc_start_get(int unit, uint32 uc, bool *start);

/*!
 * \brief Install uC interrupt hanlder.
 *
 * This function is used to install uC's software interrupt
 * handler.
 *
 * \param [in] unit Unit number.
 * \param [in] uc uC number.
 * \param [in] handler Pointer of handler.
 *                     NULL : uninstall uC interrupt handler.
 * \param [in] param User defined parameter of interrupt handler.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX Interriupt handler setup failed.
 */
extern int
m0ssq_uc_swintr_handler_set(int unit, uint32 uc,
                            sys_intr_f handler,
                            uint32 param);

/*!
 * \brief uC interrupt enable/disable.
 *
 * This function is used to enable/disable uC software
 * interrupt.
 *
 * \param [in] unit Unit number.
 * \param [in] uc uC number.
 * \param [in] enable !0 to enable interrupt.
 *                    0 to disable.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX Interriupt handler setup failed.
 */
extern int
m0ssq_uc_swintr_enable_set(int unit, uint32 uc, bool enable);

#endif /* M0SSQ_H */
