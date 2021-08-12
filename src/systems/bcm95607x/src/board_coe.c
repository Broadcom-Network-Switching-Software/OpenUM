/*! \file board_coe.c
 *
 * BCM56070 COE Board APIs.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include <system.h>
#undef SOC_IF_ERROR_RETURN
#include <soc/error.h>

#ifdef CFG_COE_INCLUDED

#include <boardapi/coe.h>
#include <utils/shr/shr_debug.h>
#include <sal.h>

coe_mode_t coe_mode = COE_MODE_NOT_DEFINED;

sys_error_t
board_coe_active_port_class(int classid)
{
    SHR_FUNC_ENTER(SHR_NO_UNIT);

    SOC_IF_ERROR_RETURN(fl_coe_active_port_class(classid));

    SHR_IF_ERR_EXIT(SYS_OK);
exit:
    SHR_FUNC_EXIT();
}

/***********************************************************************************
 * APIs for COE chassis mode
 */
sys_error_t
board_coe_linecard_downstream_backplane_to_frontport_config(
    uint8 bp_port,
    uint8 front_port,
    int32 channel_id)
{
    uint8 unit;
    uint32 custom_header;
    uint32 custom_header_mask;
    uint32 custom_header_index;
    uint32 l2mc_index;
    uint16 bp_uport;
    BOOL pause_tx = false;
    BOOL pause_rx = false;
    uint32 coe_rule_number;
    uint32 fp_counter_table_index;
    uint16 uport;
    int entry_low, entry_high;
    SHR_FUNC_ENTER(SHR_NO_UNIT);

    if (coe_mode == COE_MODE_NOT_DEFINED) {
        coe_mode = COE_MODE_CHASSIS;
    }
    if (coe_mode == COE_MODE_PASSTHRU) {
        sal_printf("Cannot combine chassis and passthrough API.\n");
        SHR_ERR_EXIT(SYS_ERR);
    }
    if (COE_OUT_OF_RANGE(coe_permitted_class_id, 0 , 0xFF)) {
        sal_printf("Permitted class id has not been set.\n");
        SHR_ERR_EXIT(SYS_ERR);
    }

    bp_uport = bp_port;
    SHR_LOG_DEBUG("user port: bp_port=%d front_port=%d\n",
        bp_port, front_port);
    SHR_IF_ERR_EXIT(board_uport_to_lport(bp_port, &unit, &bp_port));
    SHR_IF_ERR_EXIT(board_uport_to_lport(front_port, &unit, &front_port));
    SHR_LOG_DEBUG("logical port: bp_port=%d front_port=%d\n",
        bp_port, front_port);

    /* Ingress port CPU Managed Learning (CML) is disabled */
    SHR_IF_ERR_EXIT(fl_coe_disable_l2_learn(bp_port));

    /* Disable VLAN check */
    SHR_IF_ERR_EXIT(bcm5607x_port_control_set(
        unit, bp_port, pcmPortControlDoNotCheckVlan, 1));
    SHR_IF_ERR_EXIT(fl_coe_disable_tpid(bp_port));

    /* Enable pause frame to go through IPIPE */
    SHR_IF_ERR_EXIT(bcm5607x_port_control_set(
        unit, bp_port, pcmPortControlPassControlFrames, 1));

    /* Disable backplane port flow control */
    SHR_IF_ERR_EXIT(board_port_pause_set(bp_uport, pause_tx, pause_rx));

    custom_header = (0x81000000 | channel_id);
    custom_header_mask = 0xFFFF0FFF;
    /* Enable custom header parser on ingress port */
    SHR_IF_ERR_EXIT(fl_coe_enable_custom_header_parser(bp_port, 1));
    SHR_IF_ERR_EXIT(fl_coe_custom_header_match_add(custom_header, &custom_header_index));

    /* Set custom header mask for parser */
    SHR_IF_ERR_EXIT(fl_coe_custom_header_mask_set(custom_header_mask));

    /* Decap the Custom Header */
    SHR_IF_ERR_EXIT(fl_coe_egr_custom_header_enable_set(front_port, 0x0));

    /* Add L2MC entry */
    SHR_IF_ERR_EXIT(fl_coe_l2mc_add_pbmp(front_port, 0, &l2mc_index));

    SHR_IF_ERR_EXIT(fl_coe_linecard_downstream_ifp_qualify(
        custom_header,
        custom_header_mask,
        bp_port,
        l2mc_index));

    /* Reverse Lport to Uport and record it. */
    SHR_IF_ERR_EXIT(board_lport_to_uport(unit, bp_port, &uport));
    bp_port = uport;
    SHR_IF_ERR_EXIT(board_lport_to_uport(unit, front_port, &uport));
    front_port = uport;

    coe_rule_number = (coe_ifp_rule_number - 1);
    fp_counter_table_index = coe_ifp_counter_table_index - 1;
    coe_ifp_slice_number = coe_rule_number / (ENTRIES_PER_SLICE / 2);
    entry_low = (coe_ifp_slice_number * 2 * 64) + (coe_ifp_rule_number % 64);
    entry_high = (coe_ifp_slice_number * 2 * 64) + (coe_ifp_rule_number % 64)
        + 64;
    coe_rule_table[coe_rule_number].valid = 1;
    coe_rule_table[coe_rule_number].dir = CH_DOWNSTREAM;
    coe_rule_table[coe_rule_number].bp_port1 = bp_port;
    coe_rule_table[coe_rule_number].bp_port2 = 0;
    coe_rule_table[coe_rule_number].front_port = front_port;
    coe_rule_table[coe_rule_number].channel_id = channel_id;
    coe_rule_table[coe_rule_number].custom_header = custom_header;
    coe_rule_table[coe_rule_number].custom_header_mask = custom_header_mask;
    coe_rule_table[coe_rule_number].fp_entry_low = entry_low;
    coe_rule_table[coe_rule_number].fp_entry_high = entry_high;
    coe_rule_table[coe_rule_number].fp_counter_table_index =
        fp_counter_table_index;

    SHR_IF_ERR_EXIT(SYS_OK);
exit:
    SHR_FUNC_EXIT();
}

sys_error_t
board_coe_linecard_upstream_frontport_to_backplane_config(
    uint8 act_bp_port,
    uint8 stb_bp_port,
    uint8 front_port,
    int32 channel_id)
{
    uint8 unit;
    uint32 encap_id;
    uint32 custom_header;
    uint32 l2mc_index;
    uint32 coe_rule_number;
    uint32 fp_counter_table_index;
    uint16 uport;
    int entry_low, entry_high;
    SHR_FUNC_ENTER(SHR_NO_UNIT);

    if (coe_mode == COE_MODE_NOT_DEFINED) {
        coe_mode = COE_MODE_CHASSIS;
    }
    if (coe_mode == COE_MODE_PASSTHRU) {
        sal_printf("Cannot combine chassis and passthrough API.\n");
        SHR_ERR_EXIT(SYS_ERR);
    }
    SHR_LOG_DEBUG("user port: front_port=%d act_bp_port=%d stb_bp_port=%d\n",
        front_port, act_bp_port, stb_bp_port);
    SHR_IF_ERR_EXIT(board_uport_to_lport(act_bp_port, &unit, &act_bp_port));
    if (stb_bp_port != 0) {
        SHR_IF_ERR_EXIT(board_uport_to_lport(stb_bp_port, &unit, &stb_bp_port));
    }
    SHR_IF_ERR_EXIT(board_uport_to_lport(front_port, &unit, &front_port));
    SHR_LOG_DEBUG("logical port: front_port=%d act_bp_port=%d stb_bp_port=%d\n",
        front_port, act_bp_port, stb_bp_port);

    /* Ingress port CPU Managed Learning (CML) is disabled */
    SHR_IF_ERR_EXIT(fl_coe_disable_l2_learn(front_port));

    /* Disable VLAN check */
    SHR_IF_ERR_EXIT(bcm5607x_port_control_set(
        unit, front_port, pcmPortControlDoNotCheckVlan, 1));
    SHR_IF_ERR_EXIT(fl_coe_disable_tpid(front_port));

    /* Enable pause frame to go through IPIPE */
    SHR_IF_ERR_EXIT(bcm5607x_port_control_set(
        unit, front_port, pcmPortControlPassControlFrames, 1));

    custom_header = (0x81000000 | channel_id);
    /* Add custom header encap entry */
    SHR_IF_ERR_EXIT(fl_coe_egr_header_encap_data_add(custom_header, &encap_id));
    SHR_LOG_DEBUG("custom_header=%08x encap_id=%d\n", custom_header, encap_id);

    /* Enable custom header encap */
    SHR_IF_ERR_EXIT(fl_coe_egr_custom_header_enable_set(act_bp_port, 0x1));
    if (stb_bp_port != 0) {
        SHR_IF_ERR_EXIT(fl_coe_egr_custom_header_enable_set(stb_bp_port, 0x1));
    }

    /* Add L2MC entry */
    SHR_IF_ERR_EXIT(fl_coe_l2mc_add_pbmp(act_bp_port, stb_bp_port, &l2mc_index));

    /* IFP qualify source port, and rediret to backplane ports with encapped */
    fl_coe_linecard_upstream_ifp_qualify(
        front_port,
        encap_id,
        l2mc_index
    );

    /* Reverse Lport to Uport and record it. */
    SHR_IF_ERR_EXIT(board_lport_to_uport(unit, act_bp_port, &uport));
    act_bp_port = uport;
    if (stb_bp_port != 0) {
        SHR_IF_ERR_EXIT(board_lport_to_uport(unit, stb_bp_port, &uport));
        stb_bp_port = uport;
    }
    SHR_IF_ERR_EXIT(board_lport_to_uport(unit, front_port, &uport));
    front_port = uport;

    coe_rule_number = (coe_ifp_rule_number - 1);
    fp_counter_table_index = coe_ifp_counter_table_index - 1;
    coe_ifp_slice_number = coe_rule_number / (ENTRIES_PER_SLICE / 2);
    entry_low = (coe_ifp_slice_number * 2 * 64) + (coe_ifp_rule_number % 64);
    entry_high = (coe_ifp_slice_number * 2 * 64) + (coe_ifp_rule_number % 64)
        + 64;
    coe_rule_table[coe_rule_number].valid = 1;
    coe_rule_table[coe_rule_number].dir = CH_UPSTREAM;
    coe_rule_table[coe_rule_number].bp_port1 = act_bp_port;
    coe_rule_table[coe_rule_number].bp_port2 = stb_bp_port;
    coe_rule_table[coe_rule_number].front_port = front_port;
    coe_rule_table[coe_rule_number].channel_id = channel_id;
    coe_rule_table[coe_rule_number].custom_header = custom_header;
    coe_rule_table[coe_rule_number].fp_entry_low = entry_low;
    coe_rule_table[coe_rule_number].fp_entry_high = entry_high;
    coe_rule_table[coe_rule_number].fp_counter_table_index =
        fp_counter_table_index;

    SHR_IF_ERR_EXIT(SYS_OK);
exit:
    SHR_FUNC_EXIT();
}

/***********************************************************************************
 * APIs for COE passthrough mode
 */
/*!
 * \brief Get COE pass-through mode configuration
 *
 * \param [in] rule_idx Index for the FP rule.
 * \param [out] coe_fp_qualify COE pass-through FP entry.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_coe_uplink_get(int rule_idx, coe_rule_table_t *coe_fp_qualify)
{
    int idx;
    SHR_FUNC_ENTER(SHR_NO_UNIT);

    if (coe_mode == COE_MODE_NOT_DEFINED) {
        coe_mode = COE_MODE_PASSTHRU;
    }
    if (coe_mode == COE_MODE_CHASSIS) {
        sal_printf("Cannot combine chassis and passthrough API.\n");
        SHR_ERR_EXIT(SYS_ERR);
    }

    if (!coe_fp_qualify) {
        SHR_ERR_EXIT(SYS_ERR);
    }

    if (rule_idx >= MAX_COE_RULES_NUMBER) {
        SHR_ERR_EXIT(SYS_ERR);
    }

    /* Save the configuration */
    for (idx = 0; idx <= MAX_COE_RULES_NUMBER; idx++) {
        if (coe_rule_table[idx].valid) {
            if (rule_idx == 0) {
                sal_memcpy(coe_fp_qualify, &coe_rule_table[idx], sizeof(coe_rule_table_t));
                return SYS_OK;
            } else {
                rule_idx--;
            }
        }
    }

    SHR_ERR_EXIT(SYS_ERR);
exit:
    SHR_FUNC_EXIT();
}

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
board_coe_uplink_frontport_to_backplane_config(
    uint8 act_bp_port,
    uint8 stb_bp_port,
    uint8 front_port)
{
    uint8 unit;
    uint32 l2mc_index;
    uint8 bport1 = act_bp_port, bport2 = stb_bp_port, fport = front_port;
    uint32 idx;

    SHR_FUNC_ENTER(SHR_NO_UNIT);

    if (coe_mode == COE_MODE_NOT_DEFINED) {
        coe_mode = COE_MODE_PASSTHRU;
    }
    if (coe_mode == COE_MODE_CHASSIS) {
        sal_printf("Cannot combine chassis and passthrough API.\n");
        SHR_ERR_EXIT(SYS_ERR);
    }

    SHR_IF_ERR_EXIT(board_uport_to_lport(act_bp_port, &unit, &act_bp_port));
    SHR_IF_ERR_EXIT(board_uport_to_lport(front_port, &unit, &front_port));

    /* Add L2MC entry */
    if (stb_bp_port) {
        SHR_IF_ERR_EXIT(
            board_uport_to_lport(stb_bp_port, &unit, &stb_bp_port));
        SHR_IF_ERR_EXIT(
            fl_coe_l2mc_add_pbmp(act_bp_port, stb_bp_port, &l2mc_index));
    } else {
        SHR_IF_ERR_EXIT(
            fl_coe_l2mc_add_pbmp(act_bp_port, -1, &l2mc_index));
    }

    /* Ingress port CPU Managed Learning (CML) is disabled */
    SHR_IF_ERR_EXIT(fl_coe_disable_l2_learn(front_port));

    /* Disable VLAN check */
    SHR_IF_ERR_EXIT(bcm5607x_port_control_set(
            unit, front_port, pcmPortControlDoNotCheckVlan, 1));

    /* Change pause state */
    SHR_IF_ERR_EXIT(pcm_port_pause_set(
            unit, front_port, false, false));

    /* Enable pause frame to go through IPIPE */
    SHR_IF_ERR_EXIT(bcm5607x_port_control_set(
            unit, front_port, pcmPortControlPassControlFrames, 1));

    SHR_IF_ERR_EXIT(fl_coe_uplink_ifp_qualify_source_port_and_l2mc(
            front_port, l2mc_index, -1));

    /* Save the configuration */
    for (idx = 0; idx <= MAX_COE_RULES_NUMBER; idx++) {
        if (!coe_rule_table[idx].valid) {
            coe_rule_table[idx].valid = 1;
            coe_rule_table[idx].dir = CH_DOWNSTREAM;
            coe_rule_table[idx].bp_port1 = bport1;
            coe_rule_table[idx].bp_port2 = bport2;
            coe_rule_table[idx].front_port = fport;
            coe_rule_table[idx].active_class_id = -1;
            idx = MAX_COE_RULES_NUMBER;
        }
    }

    SHR_IF_ERR_EXIT(SYS_OK);
exit:
    SHR_FUNC_EXIT();
}

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
board_coe_uplink_backplane_to_frontport_config(
    uint8 act_bp_port,
    uint8 stb_bp_port,
    uint8 front_port)
{
    uint8 unit;
    uint32 l2mc_index;
    uint32 idx;
    uint8 bport1 = act_bp_port, bport2 = stb_bp_port, fport = front_port;
    int active_class_id = coe_permitted_class_id;  /* Defined in flcoe.c */

    SHR_FUNC_ENTER(SHR_NO_UNIT);

    if (coe_mode == COE_MODE_NOT_DEFINED) {
        coe_mode = COE_MODE_PASSTHRU;
    }
    if (coe_mode == COE_MODE_CHASSIS) {
        sal_printf("Cannot combine chassis and passthrough API.\n");
        SHR_ERR_EXIT(SYS_ERR);
    }
    if (COE_OUT_OF_RANGE(coe_permitted_class_id, 0 , 0xFF)) {
        sal_printf("Permitted class id has not been set.\n");
        SHR_ERR_EXIT(SYS_ERR);
    }

    SHR_IF_ERR_EXIT(board_uport_to_lport(act_bp_port, &unit, &act_bp_port));
    SHR_IF_ERR_EXIT(board_uport_to_lport(front_port, &unit, &front_port));

    /* Add L2MC entry */
    SHR_IF_ERR_EXIT(
        fl_coe_l2mc_add_pbmp(front_port, -1, &l2mc_index));

    /* Ingress port CPU Managed Learning (CML) is disabled */
    SHR_IF_ERR_EXIT(fl_coe_disable_l2_learn(act_bp_port));

    /* Disable VLAN check */
    SHR_IF_ERR_EXIT(bcm5607x_port_control_set(
            unit, act_bp_port, pcmPortControlDoNotCheckVlan, 1));

    /* Change pause state */
    SHR_IF_ERR_EXIT(pcm_port_pause_set(
            unit, act_bp_port, false, false));

    /* Enable pause frame to go through IPIPE */
    SHR_IF_ERR_EXIT(bcm5607x_port_control_set(
            unit, act_bp_port, pcmPortControlPassControlFrames, 1));

    SHR_IF_ERR_EXIT(fl_coe_uplink_ifp_qualify_source_port_and_l2mc(
            act_bp_port, l2mc_index, active_class_id));

    if(stb_bp_port) {
        SHR_IF_ERR_EXIT(board_uport_to_lport(
                stb_bp_port, &unit, &stb_bp_port));

        /* Ingress port CPU Managed Learning (CML) is disabled */
        SHR_IF_ERR_EXIT(fl_coe_disable_l2_learn(stb_bp_port));

        /* Disable VLAN check */
        SHR_IF_ERR_EXIT(bcm5607x_port_control_set(
                unit, stb_bp_port, pcmPortControlDoNotCheckVlan, 1));

        /* Change pause state */
        SHR_IF_ERR_EXIT(pcm_port_pause_set(
                unit, stb_bp_port, false, false));

        /* Enable pause frame to go through IPIPE */
        SHR_IF_ERR_EXIT(bcm5607x_port_control_set(
                unit, stb_bp_port, pcmPortControlPassControlFrames, 1));

        SHR_IF_ERR_EXIT(fl_coe_uplink_ifp_qualify_source_port_and_l2mc(
                stb_bp_port, l2mc_index, active_class_id));
    }

    /* Save the configuration */
    for (idx = 0; idx <= MAX_COE_RULES_NUMBER; idx++) {
        if (!coe_rule_table[idx].valid) {
            coe_rule_table[idx].valid = 1;
            coe_rule_table[idx].dir = CH_UPSTREAM;
            coe_rule_table[idx].bp_port1 = bport1;
            coe_rule_table[idx].bp_port2 = bport2;
            coe_rule_table[idx].front_port = fport;
            coe_rule_table[idx].active_class_id = active_class_id;
            idx = MAX_COE_RULES_NUMBER;
        }
    }

    SHR_IF_ERR_EXIT(SYS_OK);
exit:
    SHR_FUNC_EXIT();
}


#endif /* CFG_COE_INCLUDED */