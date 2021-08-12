/*! \file mirror.h
 *
 * Mirror board APIs.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_MIRROR_H_
#define _BOARDAPI_MIRROR_H_

/*
 * Select mirror-to-port
 */
/*!
 * \brief Set the mirror-to-port.
 *
 * \param [in] uport Port number.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_mirror_to_set(uint16 uport) REENTRANT;

/*!
 * \brief Get the mirror-to-port.
 *
 * \param [out] uport Port number.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_mirror_to_get(uint16 *uport) REENTRANT;

/*
 * Enable or disable mirroring per port
 */
/*!
 * \brief Set the port to be mirrored or disable.
 *
 * \param [in] uport - Port number.
 * \param [in] enable
 *    \li TRUE = Set the port to be mirrored.
 *    \li FALSE = Set the port not to be mirrored.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_mirror_port_set(uint16 uport, uint8 enable) REENTRANT;

/*!
 * \brief Get if the port is set to be mirrored.
 *
 * \param [in] uport Port number.
 * \param [out] enable
 *    \li TRUE = The port is mirrored.
 *    \li FALSE = The port is not mirrored.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_mirror_port_get(uint16 uport, uint8 *enable) REENTRANT;

#endif /* _BOARDAPI_MIRROR_H_ */
