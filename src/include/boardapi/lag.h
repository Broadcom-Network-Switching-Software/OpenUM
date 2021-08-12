/*! \file lag.h
 *
 * LAG board APIs.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_LAG_H_
#define _BOARDAPI_LAG_H_

/*!
 * \brief Set the lag to enable or disable.
 *
 * \param [in] enable:
 *    \li TRUE = Enable lag.
 *    \li FALSE = Disable lag.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_lag_set(uint8 enable) REENTRANT;

/*!
 * \brief Get the lag status.
 *
 * \param [out] enable
 *    \li TRUE = Lag is enabled
 *    \li FALSE = Lag is disabled.
 */
extern void board_lag_get(uint8 *enable) REENTRANT;

/*!
 * \brief Set the lag group members.
 *
 * \param [in] lagid Lag ID number.
 * \param [in] enable
 *    \li TRUE = Enable the lag group.
 *    \li FALSE = Disable the lag group.
 * \param [in] uplist Lag members list.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_lag_group_set(uint8 lagid, 
                                    uint8 enable, uint8 *uplist) REENTRANT;

/*!
 * \brief Get the lag group members.
 *
 * \param [in] lagid Lag ID number.
 * \param [out] enable
 *    \li TRUE = Lag group is enabled.
 *    \li FALSE = Lag group is disabled.
 * \param [out] uplist  Lag members list.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_lag_group_get(uint8 lagid, 
                                    uint8 *enable, uint8 *uplist) REENTRANT;

/*!
 * \brief Get the max lag numbers.
 *
 * \param [out] num Max group numbers.
 */
extern void board_lag_group_max_num(uint8 *num) REENTRANT;

/*!
 * \brief Link change callback function to change lag member.
 *
 * \param [in] uport User port number.
 * \param [in] link Link state.
 * \param [in] arg User argument.
 */
extern void board_lag_linkchange(uint16 uport, BOOL link, void *arg) REENTRANT;

#endif /* _BOARDAPI_LAG_H_ */
