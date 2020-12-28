/*
 * $Id: mcast.h,v 1.5 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_MCAST_H_
#define _BOARDAPI_MCAST_H_

/*!
 * \brief Add an entry in the multicast table.
 *
 * \param [in] mac_addr MAC address.
 * \param [in] vlan_id VLAN ID number.
 * \param [in] uplist Port list.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_mcast_addr_add(uint8 *mac_addr,
                                        uint16 vlan_id, uint8 *uplist);

/*!
 * \brief Remove an entry from the multicast table.
 *
 * \param [in] mac_addr MAC address.
 * \param [in] vlan_id VALN ID number.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_mcast_addr_remove(uint8 *mac_addr,
                                           uint16 vlan_id);

/*!
 * \brief Add port for a given entry in multicast table.
 *
 * \param [in] mac_addr MAC address.
 * \param [in] vlan_id VLAN ID number.
 * \param [in] uport Port number.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_mcast_port_add(uint8 *mac_addr, uint16 vlan_id,
                                        uint16 uport);

/*!
 * \brief Remove port for a given entry from multicast table.
 *
 * \param [in] mac_addr MAC address.
 * \param [in] vlan_id VLAN ID number.
 * \param [in] uport Port number.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_mcast_port_remove(uint8 *mac_addr, uint16 vlan_id,
                                           uint16 uport);

/*!
 * \brief Get the port list for a given entry in multicast table.
 *
 * \param [in] mac_addr MAC address.
 * \param [in] vlan_id VLAN ID number.
 * \param [out] uplist Port list.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_mcast_port_get(uint8 *mac_addr, uint16 vlan_id,
                                        uint8 *uplist);

/*!
 * \brief Set the IGMP snooping state.
 *
 * \param [in] enable
 *    \li TRUE = Enable IGMP snooping.
 *    \li FALSE = Disable IGMP snooping.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_igmp_snoop_enable_set(uint8 enable) REENTRANT;

/*!
 * \brief Get the IGMP snooping state.
 *
 * \param [out] enable
 *    \li TRUE = IGMP snooping is enabled.
 *    \li FALSE = IGMP snooping is disabled.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_igmp_snoop_enable_get(uint8 *enable) REENTRANT;

/*!
 * \brief Set the state of block unknown multicast packet.
 *
 * \param [in] enable
 *    \li TRUE = Enable block unknown multicast.
 *    \li FALSE = Disable block unknown multicast.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_block_unknown_mcast_set(uint8 enable) REENTRANT;

/*!
 * \brief Get the state of block unknown multicast packet.
 *
 * \param [out] enable
 *    \li TRUE = Block unknown multicast is enabled.
 *    \li FALSE = Block unknown multicast is disabled.
 */
extern sys_error_t board_block_unknown_mcast_get(uint8 *enable) REENTRANT;

#endif /* _BOARDAPI_MCAST_H_ */

