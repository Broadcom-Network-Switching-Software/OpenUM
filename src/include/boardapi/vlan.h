/*! \file vlan.h
 *
 * VLAN board APIs.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_VLAN_H_
#define _BOARDAPI_VLAN_H_

#ifdef CFG_SWITCH_VLAN_INCLUDED
/*
 * Select vlan types
 */

/*!
 * \brief Set the VLAN type.
 *
 * \param [in] type
 *    \li VT_PORT_BASED
 *    \li VT_DOT1Q
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_vlan_type_set(vlan_type_t type);

/*!
 * \brief Get the VLAN type.
 *
 * \param [out] type
 *    \li VT_PORT_BASED
 *    \li VT_DOT1Q
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_vlan_type_get(vlan_type_t *type);

/*
 * VLAN creation/deletion
 */

/*!
 * \brief Create a new VLAN by the VLAN ID.
 *
 * \param [in] vlan_id VLAN ID number.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_vlan_create(uint16 vlan_id) REENTRANT;

/*!
 * \brief Destroy the selected VLAN.
 *
 * \param [in] vlan_id VLAN ID number.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_vlan_destroy(uint16 vlan_id) REENTRANT;

/*
 * Port-base VLAN port set/get
 */

/*!
 * \brief Set the port-based VLAN members for a given VLAN ID.
 *
 * \param [in] vlan_id VLAN ID number.
 * \param [in] uplist VLAN members port list number.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_pvlan_port_set(uint16 vlan_id, uint8 *uplist);

/*!
 * \brief Get the port-based VLAN members for a given VLAN ID.
 *
 * \param [in] vlan_id VLAN ID number.
 * \param [out] uplist VLAN members port list number.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_pvlan_port_get(uint16 vlan_id, uint8 *uplist);

/*
 * Port-base VLAN egress ports get
 */

/*!
 * \brief Get the egress port list for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] uplist Egress mask of the port list.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_pvlan_egress_get(uint16 uport, uint8 *uplist);

/*
 * Port default VLAN set/get
 */

/*!
 * \brief Set the PVID for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] vlan_id PVID number.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_untagged_vlan_set(uint16 uport, uint16 vlan_id) REENTRANT;

/*!
 * \brief Get the PVID for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] vlan_id PVID number.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_untagged_vlan_get(uint16 uport, uint16 *vlan_id) REENTRANT;

/*
 * 802.1Q VLAN port set/get
 */

/*!
 * \brief Set IEEE 802.1Q VLAN members and tag members by the VLAN ID.
 *
 * \param [in] vlan_id QVLAN ID number.
 * \param [in] uplist (IN) VLAN members port list number.
 * \param [in] tag_uplist VLAN tagged members port list number.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_qvlan_port_set(uint16 vlan_id, uint8 *uplist,
                                      uint8 *tag_uplist);

/*!
 * \brief Get the IEEE 802.1Q VLAN members and tag members by the VLAN ID.
 *
 * \param [in] vlan_id QVLAN ID number.
 * \param [out] uplist VLAN members port list number.
 * \param [out] tag_uplist VLAN tagged members port list number.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_qvlan_port_get(uint16 vlan_id, uint8 *uplist,
                                      uint8 *tag_uplist);

/*
 * retrieve total vlan numbers
 */

/*!
 * \brief Get the count of IEEE 802.1Q VLAN entry.
 *
 * \return VLAN count.
 */
extern uint16 board_vlan_count(void) REENTRANT;

/*
 * retrieve vlan members with index
 */

/*!
 * \brief Get the IEEE 802.1Q VLAN ID, members, and tag members by index
 *
 * \param [in] index Index number.
 * \param [out] vlan_id QVLAN ID number.
 * \param [out] uplist VLAN members port list number.
 * \param [out] tag_uplist VLAN tagged members port list number.
 * \param [in] get_uplist Return the user port list or not.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_qvlan_get_by_index(uint16  index, uint16 *vlan_id, uint8 *uplist, uint8 *tag_uplist, BOOL get_uplist) REENTRANT;

#endif  /* CFG_SWITCH_VLAN_INCLUDED */

#endif /* _BOARDAPI_VLAN_H_ */
