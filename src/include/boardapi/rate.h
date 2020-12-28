/*
 * $Id: rate.h,v 1.3 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_RATE_H_
#define _BOARDAPI_RATE_H_

/*
 * Ingress/Egress rate limiting
 */
/*!
 * \brief Set the ingress rate for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] pps Rate
 *    \li 0: No limit
 *    \li 512000
 *    \li 1024000
 *    \li ...
 *    \li 524288000
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_port_rate_ingress_set(uint16 uport,
                                               uint32 pps) REENTRANT;

/*!
 * \brief Get the rate ingress setting for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] pps Rate
 *    \li 0: No limit
 *    \li Rate value
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_port_rate_ingress_get(uint16 uport,
                                               uint32 *pps) REENTRANT;

/*!
 * \brief Set the egress rate for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] pps Rate
 *    \li 0: No limit
 *    \li 512000
 *    \li 1024000
 *    \li ...
 *    \li 524288000
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_port_rate_egress_set(uint16 uport,
                                              uint32 pps) REENTRANT;

/*!
 * \brief Get the rate egress setting for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] pps Rate
 *    \li 0: No limit
 *    \li Rate value
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_port_rate_egress_get(uint16 uport,
                                              uint32 *pps) REENTRANT;

/*
 * Storm control type flags
 */

#define STORM_RATE_NONE   0x00
#define STORM_RATE_BCAST  0x01
#define STORM_RATE_MCAST  0x02
#define STORM_RATE_DLF    0x04
#define STORM_RATE_ALL    0xFF

/*
 * Selecting storm control types for all ports
 */
/*!
 * \brief Set the storm control type.
 *
 * \param [in] flags
 *    \li STORM_RATE_NONE
 *    \li STORM_RATE_BCAST
 *    \li STORM_RATE_MCAST
 *    \li STORM_RATE_DLF
 *    \li STORM_RATE_ALL
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_rate_type_set(uint8 flags) REENTRANT;

/*!
 * \brief Get the storm control type.
 *
 * \param [out] flags
 *    \li STORM_RATE_NONE
 *    \li STORM_RATE_BCAST
 *    \li STORM_RATE_MCAST
 *    \li STORM_RATE_DLF
 *    \li STORM_RATE_ALL
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_rate_type_get(uint8 *flags)  REENTRANT;

/*
 * Enable/disable storm control
 */
/*!
 * \brief Set the storm control setting rate for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] pps Rate
 *    \li 0: No limit
 *    \li Rate value
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_rate_set(uint16 uport, uint32 pps)  REENTRANT;

/*!
 * \brief Get the storm control setting rate for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] pps Rate
 *    \li 0: No limit
 *    \li Rate value
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_rate_get(uint16 uport, uint32 *pps)  REENTRANT;

#endif /* _BOARDAPI_RATE_H_ */
