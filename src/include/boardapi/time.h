/*! \file time.h
 *
 * Time board APIs.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_TIME_H_
#define _BOARDAPI_TIME_H_

#include "soc/soc.h"

#ifdef CFG_SWITCH_SYNCE_INCLUDED
/*!
 * \brief Get syncE clock source control option.
 *
 * This function is used to get syncE clock control option.
 *
 * \param [in] clk_src_config clock source config.
 * \param [in] control SyncE source.
 * \param [out] value control value.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_time_synce_clock_source_control_get(
                          bcm_time_synce_clock_source_config_t *clk_src_config,
                          bcm_time_synce_clock_source_control_t control,
                          int *value) REENTRANT;

/*!
 * \brief Set syncE clock source control option.
 *
 * This function is used to set syncE clock control option.
 *
 * \param [in] clk_src_config clock source config.
 * \param [in] control SyncE source.
 * \param [in] value control value.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_time_synce_clock_source_control_set(
                          bcm_time_synce_clock_source_config_t *clk_src_config,
                          bcm_time_synce_clock_source_control_t control,
                          int value) REENTRANT;

#endif /* CFG_SWITCH_SYNCE_INCLUDED*/
#endif /* _BOARDAPI_TIME_H_ */
