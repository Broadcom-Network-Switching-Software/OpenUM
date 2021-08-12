/*! \file coe.h
 *
 * COE board APIs.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef COE_H
#define COE_H

#include <types.h>


/*!
 * \brief Set the permitted port class ID
 *
 * \param [in] classid Permitted port class ID
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_coe_active_port_class(int classid);

/*!
 * \brief Forward traffic from backplane port to front port based on channel ID.
 *        Create a default FP rule to drop mismatched packets when being called for the first time.
 *
 * \param [in] bp_port The backplane user port
 * \param [in] front_port The front panel user port
 * \param [in] channel_id The front port channel ID
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_coe_linecard_downstream_backplane_to_frontport_config(
    uint8 bp_port,
    uint8 front_port,
    int32 channel_id);

/*!
 * \brief Redirect traffic from front panel port to one or two backplane ports and encap custom header
 *
 * \param [in] act_bp_port The active backplane user port
 * \param [in] stb_bp_port The standby backplane user port; 0 if no standby port
 * \param [in] front_port The front panel user port
 * \param [in] channel_id The front port channel ID
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_coe_linecard_upstream_frontport_to_backplane_config(
    uint8 act_bp_port,
    uint8 stb_bp_port,
    uint8 front_port,
    int32 channel_id);

/*!
 * \brief Forward traffic from backplane port to front port
 *
 * \param [in] act_bp_port The active backplane user port
 * \param [in] stb_bp_port The standby backplane user port; 0 if no standby port
 * \param [in] front_port The front panel user port
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_coe_uplink_backplane_to_frontport_config(
    uint8 act_bp_port,
    uint8 stb_bp_port,
    uint8 front_port);

/*!
 * \brief Redirect traffic from front panel port to backplane ports
 *
 * \param [in] act_bp_port The active backplane user port
 * \param [in] stb_bp_port The standby backplane user port; 0 if no standby port
 * \param [in] front_port The front panel user port
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_coe_uplink_frontport_to_backplane_config(
    uint8 act_bp_port,
    uint8 stb_bp_port,
    uint8 front_port);

/*!
 * \brief Get COE pass-through mode configuration
 *
 * \param [in] rule_idx Index for the FP rule.
 * \param [out] coe_fp_qualify COE pass-through FP entry.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_coe_uplink_get(int rule_idx, coe_rule_table_t *coe_fp_qualify);


#endif /* COE_H */
