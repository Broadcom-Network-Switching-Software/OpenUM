/*! \file mcs.h
 *
 *  APIs for MCS (Multiple Controller System).
 *
 *      MCS contains four ARM Cortex-R5 and a shared SRAM
 *      User can use APIs to
 *          - Download CR5 firmware.
 *          - Start/reset CR5 uc.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef __MCS_H__
#define __MCS_H__

#include <system.h>

/*******************************************************************************
 * MCS public APIs.
 */
/*!
 * \brief Load firmware into a uC of MCS.
 *
 * This function loads firmware into uC memory.
 *
 * \param [in] uc uC controller number.
 * \param [in] offset Byte offset of uC memory.
 * \param [in] buf Firmware buffer.
 * \param [in] len Length of firmware buffer.
 *
 * \retval SYS_OK Firmware successfully loaded.
 * \retval SYS_ERR_XXX Failed to load firmware.
 */
extern int
board_mcs_uc_fw_load(uint32 uc, uint32 offset, const uint8 *buf, uint32 len);

/*!
 * \brief Get number of uC in MCS.
 *
 * \retval Number of uC.
 */
extern int
board_mcs_uc_num_get(void);

/*!
 * \brief Start a MCS uC.
 *
 * \param [in]  uc uC number.
 *
 * \retval SYS_OK Start a uC successfully.
 * \retval SYS_ERR_XXX Fail to start a uC.
 */
extern int
board_mcs_uc_start(uint32 uc);

/*!
 * \brief Reset a MCS uC.
 *
 * \param [in]  uc uC number.
 *
 * \retval SYS_OK Reset a uC successfully.
 * \retval SYS_ERR_XXX Fail to stop a uC.
 */
extern int
board_mcs_uc_reset(uint32 uc);

/*!
 * \brief Check if uC is started or not.
 *
 * \param [in]  uc uC number.
 * \param [out] start Indicate uC is started or not.
 *
 * \return SYS_OK if successful.
 */
extern int
board_mcs_uc_start_get(uint32 uc, bool *start);

#endif /* __MCS_H__ */
