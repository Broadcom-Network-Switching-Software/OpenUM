/*! \file base.h
 *
 * Base board APIs.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_BASE_H_
#define _BOARDAPI_BASE_H_

/*!
 * \brief Get the number of units.
 *
 * \return mumber of units. (should be defined in board.h)
 */
#define board_unit_count(void) BOARD_NUM_OF_UNITS

/*!
 * \brief Get the board's name.
 *
 * \return Board name.
 */
extern const char *board_name(void) REENTRANT;

/*!
 * \brief Get the port count of user ports.
 *
 * \return Port count.
 */
extern uint8 board_uport_count(void) REENTRANT;

/*!
 * \brief Map a user port to a chip logical port.
 *
 * \param [in] uport The user port
 * \param [out] unit The chip unit number
 * \param [out] lport The chip logical port
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_uport_to_lport(uint16 uport,
                                          uint8 *unit,
                                          uint8 *lport) REENTRANT;

/*!
 * \brief Map a chip logical port to a user port.
 *
 * \param [in] unit The chip unit number
 * \param [in] lport The chip logical port
 * \param [out] uport The user port
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_lport_to_uport(uint8 unit,
                                         uint8 lport,
                                         uint16 *uport) REENTRANT;

/*!
 * \brief Map the user port bitmap array (uplist) to the chip logical port bitmap (lpbmp) on selected chip unit .
 *
 * \param [in] uplist The user port bit map array which may cover many chips
 * \param [in] unit The selected chip unit number
 * \param [out] lpbmp The chip logical port bit map
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_uplist_to_lpbmp(uint8 *uplist, uint8 unit,
                                          pbmp_t *lpbmp) REENTRANT;

/*!
 * \brief Map the chip logical port bit map (lpbmp) to a user port bit map array (uplist) on selected chip unit.
 *
 * \param [in] unit The selected chip unit number
 * \param [in] lpbmp The chip logical port bit map
 * \param [out] uplist The user port bit map array which may cover many chips
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_lpbmp_to_uplist(uint8 unit, pbmp_t lpbmp,
                                           uint8 *uplist) REENTRANT;

/*!
 * \brief Get SoC by unit.
 *
 * \param [in] unit Unit number.
 *
 * \return Pointer to switch device (e.g., soc_switch_bcm5340x).
 */
extern soc_switch_t *board_get_soc_by_unit(uint8 unit) REENTRANT;

/*!
 * \brief Get chip devID and revID by unit.
 *
 * \param [in] unit Unit number.
 * \param [out] dev The chip devID.
 * \param [out] rev The chip devID.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_chip_revision(uint8 unit, uint16 *dev, uint16 *rev);

/*!
 * \brief Get chip information by unit.
 *
 * \param [in] unit Unit number.
 *
 * \retval Pointer to chip information (e.g., bcm5357x_chip_info).
 */
extern chip_info_t *board_get_chipinfo_by_unit(uint8 unit) REENTRANT;

#ifndef CFG_PCM_SUPPORT_INCLUDED
/*!
 * \brief Get phy driver by user port.
 *
 * \param [in] uport User port.
 *
 * \retval Pointer to phy driver.
 */
extern phy_driver_t *board_get_phy_drv(uint16 uport) REENTRANT;
#endif
#endif /* _BOARDAPI_BASE_H_ */
