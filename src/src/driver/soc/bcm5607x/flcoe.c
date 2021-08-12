/*! \file flcoe.c
 *
 * bcm5607x device-specific COE driver.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include <system.h>
#include <utils/shr/shr_debug.h>

#ifdef CFG_COE_INCLUDED
/*******************************************************************************
 * Local definitions
 */
#define CUSTOM_HEADER_TABLE_SIZE    (128)

int coe_permitted_class_id = -1;
int coe_default_drop_init = 0;
int coe_l2mc_index = 0;
int coe_egr_header_encap_data_index = 0;
int coe_custom_header_match_index = 0;
uint32 coe_custom_header_match_table[128];
int coe_ifp_init = 0;
int coe_ifp_double_wide_mode = 1;
int coe_ifp_entries_per_slice = 0;
int coe_ifp_slice_number = 0;
int coe_ifp_rule_number = 0;
int coe_ifp_default_drop_rule_number = 0;
int coe_ifp_counter_mode = 0;
int coe_ifp_counter_index = 0;
int coe_ifp_counter_table_index = 0;
int ACT_CLASS_ID = 0x10;
int STB_CLASS_ID = 0x20;
coe_rule_table_t coe_rule_table[MAX_COE_RULES_NUMBER];

sys_error_t
fl_coe_active_port_class(int classid)
{
    SHR_FUNC_ENTER(SHR_NO_UNIT);

    if (COE_OUT_OF_RANGE(classid, 0 , 0xFF)) {
        sal_printf("Parameter out of range\n");
        SHR_ERR_EXIT(SYS_ERR_PARAMETER);
    }
    coe_permitted_class_id = classid;
    SHR_LOG_DEBUG("coe_permitted_class_id=%d\n", coe_permitted_class_id);
    SHR_IF_ERR_EXIT(SYS_OK);
exit:
    SHR_FUNC_EXIT();
}

/* Disable L2 learn on lport */
sys_error_t
fl_coe_disable_l2_learn(uint8 lport)
{
    PORTm_t port_entry;
    SHR_FUNC_ENTER(SHR_NO_UNIT);

    if (COE_OUT_OF_RANGE(lport, BCM5607X_LPORT_MIN , BCM5607X_LPORT_MAX)) {
        sal_printf("lport out of range\n");
        SHR_ERR_EXIT(SYS_ERR_PARAMETER);
    }

    PORTm_CLR(port_entry);
    SOC_IF_ERROR_RETURN(
        READ_PORTm(0, lport, port_entry));
    /* Disable L2 learn when COE enabled */
    PORTm_CML_FLAGS_NEWf_SET(port_entry, 0);
    SOC_IF_ERROR_RETURN(
            WRITE_PORTm(0, lport, port_entry));
    SHR_IF_ERR_EXIT(SYS_OK);
exit:
    SHR_FUNC_EXIT();
}

/* Disable TPID on lport */
sys_error_t
fl_coe_disable_tpid(uint8 lport)
{
    PORTm_t port_entry;
    SHR_FUNC_ENTER(SHR_NO_UNIT);

    if (COE_OUT_OF_RANGE(lport, BCM5607X_LPORT_MIN , BCM5607X_LPORT_MAX)) {
        SHR_LOG_ERROR("lport out of range\n");
        SHR_ERR_EXIT(SYS_ERR_PARAMETER);
    }

    PORTm_CLR(port_entry);
    SOC_IF_ERROR_RETURN(
        READ_PORTm(0, lport, port_entry));
    /* Disable TPID when COE enabled */
    PORTm_INNER_TPID_ENABLEf_SET(port_entry, 0);
    PORTm_OUTER_TPID_ENABLEf_SET(port_entry, 0);
    SOC_IF_ERROR_RETURN(
            WRITE_PORTm(0, lport, port_entry));
    SHR_IF_ERR_EXIT(SYS_OK);
exit:
    SHR_FUNC_EXIT();
}

/* Add L2MC entry */
sys_error_t
fl_coe_l2mc_add_pbmp(uint8 lport1, uint8 lport2, uint32 *index)
{
    pbmp_t pbmp;
    L2MCm_t l2mc;
    SHR_FUNC_ENTER(SHR_NO_UNIT);

    PBMP_CLEAR(pbmp);
    if (COE_OUT_OF_RANGE(lport1, BCM5607X_LPORT_MIN , BCM5607X_LPORT_MAX)) {
        sal_printf("lport1 out of range\n");
        SHR_ERR_EXIT(SYS_ERR_PARAMETER);
    }
    PBMP_PORT_ADD(pbmp, lport1);

    /* If lport2 is out of range, skipt it. */
    if (!COE_OUT_OF_RANGE(lport2, BCM5607X_LPORT_MIN , BCM5607X_LPORT_MAX)) {
        PBMP_PORT_ADD(pbmp, lport2);
    }
    SHR_LOG_TRACE("coe_l2mc_index=%d lport1=%d lport2=%d\n",
        coe_l2mc_index, lport1, lport2);

    /* Write L2MC table */
    L2MCm_CLR(l2mc);
    L2MCm_PORT_BITMAPf_SET(l2mc, SOC_PBMP(pbmp));
    L2MCm_VALIDf_SET(l2mc, 1);
    SOC_IF_ERROR_RETURN(WRITE_L2MCm(0, coe_l2mc_index, l2mc));
    *index = coe_l2mc_index;
    coe_l2mc_index++;
    SHR_IF_ERR_EXIT(SYS_OK);
exit:
    SHR_FUNC_EXIT();
}

/* Add EGR_HEADER_ENCAP_DATA entry */
sys_error_t
fl_coe_egr_header_encap_data_add(uint32 data, uint32 *index)
{
    EGR_HEADER_ENCAP_DATAm_t egr_header_encap_data;
    SHR_FUNC_ENTER(SHR_NO_UNIT);

    EGR_HEADER_ENCAP_DATAm_CLR(egr_header_encap_data);
    EGR_HEADER_ENCAP_DATAm_CUSTOM_PACKET_HEADERf_SET(
        egr_header_encap_data, data);
    SOC_IF_ERROR_RETURN(
        WRITE_EGR_HEADER_ENCAP_DATAm(
            0, coe_egr_header_encap_data_index, egr_header_encap_data));
    *index = coe_egr_header_encap_data_index;
    SHR_LOG_TRACE("coe_egr_header_encap_data_index=%d\n",
        coe_egr_header_encap_data_index);
    coe_egr_header_encap_data_index++;
    SHR_IF_ERR_EXIT(SYS_OK);
exit:
    SHR_FUNC_EXIT();
}

/* Add CUSTOM_HEADER_MASK entry */
sys_error_t
fl_coe_custom_header_mask_set(uint32 data)
{
    CUSTOM_HEADER_MASKr_t custom_header_mask;
    SHR_FUNC_ENTER(SHR_NO_UNIT);

    CUSTOM_HEADER_MASKr_CLR(custom_header_mask);
    CUSTOM_HEADER_MASKr_MASKf_SET(custom_header_mask, data);
    SOC_IF_ERROR_RETURN(
        WRITE_CUSTOM_HEADER_MASKr(0, custom_header_mask));
    SHR_LOG_TRACE("custom_header_mask=0x%08x\n", data);
    SHR_IF_ERR_EXIT(SYS_OK);
exit:
    SHR_FUNC_EXIT();
}

/* Add CUSTOM_HEADER_MATCH entry */
sys_error_t
fl_coe_custom_header_match_add(uint32 data, uint32 *index)
{
    CUSTOM_HEADER_MATCHm_t custom_header_match;
    int i;
    SHR_FUNC_ENTER(SHR_NO_UNIT);

    for (i = 0; i < 128; i++) {
        if (coe_custom_header_match_table[i] == data) {
            *index = i;
            /* Skip if existed */
            goto exit;
        }
    }
    CUSTOM_HEADER_MATCHm_CLR(custom_header_match);
    CUSTOM_HEADER_MATCHm_VALIDf_SET(
        custom_header_match, 1);
    CUSTOM_HEADER_MATCHm_CUSTOM_HEADERf_SET(
        custom_header_match, data);
    SOC_IF_ERROR_RETURN(
        WRITE_CUSTOM_HEADER_MATCHm(
            0, coe_custom_header_match_index, custom_header_match));
    coe_custom_header_match_table[coe_custom_header_match_index] = data;
    *index = coe_custom_header_match_index;
    SHR_LOG_TRACE("coe_custom_header_match_index=%d data=%08x\n",
        coe_custom_header_match_index, data);
    coe_custom_header_match_index++;

    SHR_IF_ERR_EXIT(SYS_OK);
exit:
    SHR_FUNC_EXIT();
}

/* Enable CPH parser on lport
* port_tab.port.CUSTOM_HEADER_ENABLE = 1
*/
sys_error_t
fl_coe_enable_custom_header_parser(uint8 lport, int enable)
{
    PORTm_t port_entry;
    SHR_FUNC_ENTER(SHR_NO_UNIT);

    if (COE_OUT_OF_RANGE(lport, BCM5607X_LPORT_MIN , BCM5607X_LPORT_MAX)) {
        sal_printf("lport out of range\n");
        SHR_ERR_EXIT(SYS_ERR_PARAMETER);
    }

    PORTm_CLR(port_entry);
    SOC_IF_ERROR_RETURN(
        READ_PORTm(0, lport, port_entry));
    /* Enable CPH parser on lport */
    PORTm_CUSTOM_HEADER_ENABLEf_SET(port_entry, (enable & 0x1));
    SOC_IF_ERROR_RETURN(
            WRITE_PORTm(0, lport, port_entry));
    SHR_IF_ERR_EXIT(SYS_OK);
exit:
    SHR_FUNC_EXIT();
}
/* Set EGR_PORT_64.CUSTOM_HEADER_ENABLE */
sys_error_t
fl_coe_egr_custom_header_enable_set(uint8 lport, int enable)
{
    EGR_PORT_64r_t egr_port_64;
    SHR_FUNC_ENTER(SHR_NO_UNIT);

    if (COE_OUT_OF_RANGE(lport, BCM5607X_LPORT_MIN , BCM5607X_LPORT_MAX)) {
        sal_printf("lport out of range\n");
        SHR_ERR_EXIT(SYS_ERR_PARAMETER);
    }

    SOC_IF_ERROR_RETURN(
        READ_EGR_PORT_64r(0, lport, egr_port_64));
    EGR_PORT_64r_CUSTOM_HEADER_ENABLEf_SET(egr_port_64, (enable ? 1 : 0));
    SOC_IF_ERROR_RETURN(
        WRITE_EGR_PORT_64r(0, lport, egr_port_64));
    SHR_LOG_TRACE("Set egr_port_64 lport=%d CUSTOM_HEADER_ENABLE=%d\n", lport, enable);

    SHR_IF_ERR_EXIT(SYS_OK);
exit:
    SHR_FUNC_EXIT();
}

/* IFP init */
sys_error_t
fl_coe_ifp_init(void)
{
    FP_SLICE_MAPm_t         fp_slice_map;
    FP_SLICE_KEY_CONTROLm_t fp_slice_key_control;
    FP_PORT_FIELD_SELm_t    fp_port_field_sel;
    PORTm_t                 port_table;
    FP_SLICE_ENABLEr_t      fp_slice_enable;
    int i;
    SHR_FUNC_ENTER(SHR_NO_UNIT);

    if (!coe_ifp_init) {
        sal_memset(coe_rule_table, 0, sizeof(coe_rule_table));

        coe_ifp_entries_per_slice = ENTRIES_PER_SLICE / 2;  //64 entries
        coe_ifp_default_drop_rule_number = coe_ifp_entries_per_slice - 1;
        sal_memset(coe_custom_header_match_table, 0,
            sizeof(coe_custom_header_match_table));

        /* Initialize FP_SLICE_MAP */
        FP_SLICE_MAPm_CLR(fp_slice_map);
        FP_SLICE_MAPm_VIRTUAL_SLICE_0_PHYSICAL_SLICE_NUMBER_ENTRY_0f_SET(fp_slice_map, 0);
        FP_SLICE_MAPm_VIRTUAL_SLICE_0_VIRTUAL_SLICE_GROUP_ENTRY_0f_SET(fp_slice_map, 0);
        FP_SLICE_MAPm_VIRTUAL_SLICE_1_PHYSICAL_SLICE_NUMBER_ENTRY_0f_SET(fp_slice_map, 1);
        FP_SLICE_MAPm_VIRTUAL_SLICE_1_VIRTUAL_SLICE_GROUP_ENTRY_0f_SET(fp_slice_map, 1);
        FP_SLICE_MAPm_VIRTUAL_SLICE_2_PHYSICAL_SLICE_NUMBER_ENTRY_0f_SET(fp_slice_map, 2);
        FP_SLICE_MAPm_VIRTUAL_SLICE_2_VIRTUAL_SLICE_GROUP_ENTRY_0f_SET(fp_slice_map, 2);
        FP_SLICE_MAPm_VIRTUAL_SLICE_3_PHYSICAL_SLICE_NUMBER_ENTRY_0f_SET(fp_slice_map, 3);
        FP_SLICE_MAPm_VIRTUAL_SLICE_3_VIRTUAL_SLICE_GROUP_ENTRY_0f_SET(fp_slice_map, 3);
        FP_SLICE_MAPm_VIRTUAL_SLICE_4_PHYSICAL_SLICE_NUMBER_ENTRY_0f_SET(fp_slice_map, 4);
        FP_SLICE_MAPm_VIRTUAL_SLICE_4_VIRTUAL_SLICE_GROUP_ENTRY_0f_SET(fp_slice_map, 4);
        FP_SLICE_MAPm_VIRTUAL_SLICE_5_PHYSICAL_SLICE_NUMBER_ENTRY_0f_SET(fp_slice_map, 5);
        FP_SLICE_MAPm_VIRTUAL_SLICE_5_VIRTUAL_SLICE_GROUP_ENTRY_0f_SET(fp_slice_map, 5);
        FP_SLICE_MAPm_VIRTUAL_SLICE_6_PHYSICAL_SLICE_NUMBER_ENTRY_0f_SET(fp_slice_map, 6);
        FP_SLICE_MAPm_VIRTUAL_SLICE_6_VIRTUAL_SLICE_GROUP_ENTRY_0f_SET(fp_slice_map, 6);
        FP_SLICE_MAPm_VIRTUAL_SLICE_7_PHYSICAL_SLICE_NUMBER_ENTRY_0f_SET(fp_slice_map, 7);
        FP_SLICE_MAPm_VIRTUAL_SLICE_7_VIRTUAL_SLICE_GROUP_ENTRY_0f_SET(fp_slice_map, 7);
        SOC_IF_ERROR_RETURN(WRITE_FP_SLICE_MAPm(0, fp_slice_map));

        /* Initialize FP_SLICE_KEY_CONTROL */
        FP_SLICE_KEY_CONTROLm_CLR(fp_slice_key_control);
        FP_SLICE_KEY_CONTROLm_SLICE0_AUX_TAG_2_SELf_SET(
            fp_slice_key_control, 0x3);
        SOC_IF_ERROR_RETURN(
            WRITE_FP_SLICE_KEY_CONTROLm(0, fp_slice_key_control));

        /* Initialize FP_PORT_FIELD_SEL */
        /* Slice 0~3 is used as double wide */
        FP_PORT_FIELD_SELm_CLR(fp_port_field_sel);
        FP_PORT_FIELD_SELm_SLICE0_F3f_SET(fp_port_field_sel, 0xc);
        FP_PORT_FIELD_SELm_SLICE0_F1f_SET(fp_port_field_sel, 0xb);
        FP_PORT_FIELD_SELm_SLICE1_F3f_SET(fp_port_field_sel, 0xc);
        FP_PORT_FIELD_SELm_SLICE1_F1f_SET(fp_port_field_sel, 0xb);
        FP_PORT_FIELD_SELm_SLICE2_F3f_SET(fp_port_field_sel, 0xc);
        FP_PORT_FIELD_SELm_SLICE2_F1f_SET(fp_port_field_sel, 0xb);
        FP_PORT_FIELD_SELm_SLICE3_F3f_SET(fp_port_field_sel, 0xc);
        FP_PORT_FIELD_SELm_SLICE3_F1f_SET(fp_port_field_sel, 0xb);
        FP_PORT_FIELD_SELm_SLICE0_DOUBLE_WIDE_MODEf_SET(fp_port_field_sel, 0x1);
        FP_PORT_FIELD_SELm_SLICE1_DOUBLE_WIDE_MODEf_SET(fp_port_field_sel, 0x1);
        FP_PORT_FIELD_SELm_SLICE2_DOUBLE_WIDE_MODEf_SET(fp_port_field_sel, 0x1);
        FP_PORT_FIELD_SELm_SLICE3_DOUBLE_WIDE_MODEf_SET(fp_port_field_sel, 0x1);
        for (i = 0; i <= BCM5607X_LPORT_MAX; i++) {
            SOC_IF_ERROR_RETURN(
                WRITE_FP_PORT_FIELD_SELm(0, i, fp_port_field_sel));

            /* Initialize PORT_TAB.FILTER_ENABLE */
            SOC_IF_ERROR_RETURN(READ_PORTm(0, i, port_table));
            PORTm_FILTER_ENABLEf_SET(port_table, 0x1);
            SOC_IF_ERROR_RETURN(WRITE_PORTm(0, i, port_table));
        }

        /* FP_SLICE_ENABLE is enabled by default */
        FP_SLICE_ENABLEr_FP_SLICE_ENABLE_ALLf_SET(fp_slice_enable, 0xFF);
        FP_SLICE_ENABLEr_FP_LOOKUP_ENABLE_ALLf_SET(fp_slice_enable, 0xFF);
        SOC_IF_ERROR_RETURN(WRITE_FP_SLICE_ENABLEr(0, fp_slice_enable));

        coe_ifp_init = 1;
    }

    SHR_IF_ERR_EXIT(SYS_OK);
exit:
    SHR_FUNC_EXIT();
}

/*
* IFP qualify source port, and then redirect to backplane ports with encapped
*/
sys_error_t
fl_coe_linecard_upstream_ifp_qualify(
    int front_port,
    int encap_id,
    int l2mc_index)
{
    FP_TCAMm_t              fp_tcam_low, fp_tcam_high;
	FP_GLOBAL_MASK_TCAMm_t  fp_global_mask_tcam;
    FP_POLICY_TABLEm_t      fp_policy_table;
    GLP_t glp;
    int entry_low, entry_high;
    SHR_FUNC_ENTER(SHR_NO_UNIT);
    SHR_LOG_DEBUG("front_port=%d\n", front_port);

    if (coe_ifp_rule_number == coe_ifp_default_drop_rule_number) {
        coe_ifp_rule_number++;
    }
    coe_ifp_slice_number = coe_ifp_rule_number / (ENTRIES_PER_SLICE / 2);
    entry_low = (coe_ifp_slice_number * 2 * 64) + (coe_ifp_rule_number % 64);
    entry_high = (coe_ifp_slice_number * 2 * 64) + (coe_ifp_rule_number % 64) + 64;

    /* Configure FP_TCAM */
    /*--------------------------------------------------------------------*/
    /* Configure entry low */
    FP_TCAMm_CLR(fp_tcam_low);
    FP_TCAMm_VALIDf_SET(fp_tcam_low, 0x3);

    GLP_CLR(glp);
    GLP_PORTf_SET(glp, front_port);
    FP_TCAMm_F1_11_SGLPf_SET(fp_tcam_low, GLP_GET(glp));
    FP_TCAMm_F1_11_SGLP_MASKf_SET(fp_tcam_low, 0xFFFF);
    SOC_IF_ERROR_RETURN(WRITE_FP_TCAMm(0, entry_low, fp_tcam_low));

    /* Configure entry high */
    FP_TCAMm_CLR(fp_tcam_high);
    FP_TCAMm_VALIDf_SET(fp_tcam_high, 0x3);
    SOC_IF_ERROR_RETURN(WRITE_FP_TCAMm(0, entry_high, fp_tcam_high));
    /*--------------------------------------------------------------------*/

    /* Configure FP_POLICY_TABLE */
    /*--------------------------------------------------------------------*/
    /* Configure entry low */
    FP_POLICY_TABLEm_CLR(fp_policy_table);
    FP_POLICY_TABLEm_G_PACKET_REDIRECTIONf_SET(fp_policy_table, 0x3);
    FP_POLICY_TABLEm_REDIRECTION_TYPEf_SET(fp_policy_table, 0x2);
    FP_POLICY_TABLEm_REDIRECTION_PROFILE_INDEXf_SET(fp_policy_table, l2mc_index);
    FP_POLICY_TABLEm_HEADER_ENCAP_INDEXf_SET(fp_policy_table, encap_id);
    FP_POLICY_TABLEm_ASSIGN_HEADER_ENCAP_FIELDSf_SET(fp_policy_table, 0x1);
    FP_POLICY_TABLEm_METER_SHARING_MODE_MODIFIERf_SET(fp_policy_table, 0x1);
    FP_POLICY_TABLEm_METER_PAIR_MODEf_SET(fp_policy_table, 0x1);
    FP_POLICY_TABLEm_GREEN_TO_PIDf_SET(fp_policy_table, 0x1);

    coe_ifp_counter_mode = (coe_ifp_counter_table_index & 0x1) + 1;
    coe_ifp_counter_index = (coe_ifp_counter_table_index / 2)  ;
    FP_POLICY_TABLEm_COUNTER_MODEf_SET(fp_policy_table, coe_ifp_counter_mode);
    FP_POLICY_TABLEm_COUNTER_INDEXf_SET(fp_policy_table, coe_ifp_counter_index);
    SOC_IF_ERROR_RETURN(WRITE_FP_POLICY_TABLEm(0, entry_low, fp_policy_table));
    SHR_LOG_DEBUG("FP_COUNTER_TABLE index=%d\n", coe_ifp_counter_table_index);
    /* increase counter index count after WRITE_FP_POLICY_TABLEm */
    coe_ifp_counter_table_index++;

    /* Configure entry high */
    FP_POLICY_TABLEm_CLR(fp_policy_table);
    FP_POLICY_TABLEm_METER_SHARING_MODE_MODIFIERf_SET(fp_policy_table, 0x1);
    FP_POLICY_TABLEm_METER_PAIR_MODEf_SET(fp_policy_table, 0x1);
    FP_POLICY_TABLEm_GREEN_TO_PIDf_SET(fp_policy_table, 0x1);
    SOC_IF_ERROR_RETURN(WRITE_FP_POLICY_TABLEm(0, entry_high, fp_policy_table));
    /*--------------------------------------------------------------------*/

    /* Configure FP_GLOBAL_MASK_TCAM */
    /*--------------------------------------------------------------------*/
    /* Configure entry low */
    FP_GLOBAL_MASK_TCAMm_CLR(fp_global_mask_tcam);
    FP_GLOBAL_MASK_TCAMm_VALIDf_SET(fp_global_mask_tcam, 0x1);
    WRITE_FP_GLOBAL_MASK_TCAMm(0, entry_low, fp_global_mask_tcam);

    /* Configure entry high */
    FP_GLOBAL_MASK_TCAMm_CLR(fp_global_mask_tcam);
    FP_GLOBAL_MASK_TCAMm_VALIDf_SET(fp_global_mask_tcam, 0x1);
    WRITE_FP_GLOBAL_MASK_TCAMm(0, entry_high, fp_global_mask_tcam);
    /*--------------------------------------------------------------------*/

    coe_ifp_rule_number++;
    SHR_IF_ERR_EXIT(SYS_OK);
exit:
    SHR_FUNC_EXIT();
}

/*
* IFP qualify custom header, and then redirect to front ports
*/
sys_error_t
fl_coe_linecard_downstream_ifp_qualify(
    uint32 custom_header,
    uint32 custom_header_mask,
    int bp_port,
    int l2mc_index)
{
    FP_TCAMm_t              fp_tcam_low, fp_tcam_high;
	FP_GLOBAL_MASK_TCAMm_t  fp_global_mask_tcam;
    FP_POLICY_TABLEm_t      fp_policy_table;
    GLP_t glp;
    int entry_low, entry_high;
    SHR_FUNC_ENTER(SHR_NO_UNIT);
    SHR_LOG_DEBUG("bp_port=%d\n", bp_port);

    if (coe_ifp_rule_number == coe_ifp_default_drop_rule_number) {
        coe_ifp_rule_number++;
    }
    coe_ifp_slice_number = coe_ifp_rule_number / (ENTRIES_PER_SLICE / 2);
    entry_low = (coe_ifp_slice_number * 2 * 64) + (coe_ifp_rule_number % 64);
    entry_high = (coe_ifp_slice_number * 2 * 64) + (coe_ifp_rule_number % 64) + 64;

    /* Configure FP_TCAM */
    /*--------------------------------------------------------------------*/
    /* Configure entry low */
    FP_TCAMm_CLR(fp_tcam_low);
    FP_TCAMm_VALIDf_SET(fp_tcam_low, 0x3);

    GLP_CLR(glp);
    GLP_PORTf_SET(glp, bp_port);
    FP_TCAMm_F1_11_SGLPf_SET(fp_tcam_low, GLP_GET(glp));
    FP_TCAMm_F1_11_SGLP_MASKf_SET(fp_tcam_low, 0xFFFF);
    FP_TCAMm_FIXED_CUSTOM_HEADER_PRESENTf_SET(fp_tcam_low, 0x1);
    FP_TCAMm_FIXED_CUSTOM_HEADER_PRESENT_MASKf_SET(fp_tcam_low, 0x1);
    FP_TCAMm_F3_12_AUX_TAG_2f_SET(fp_tcam_low, custom_header);
    FP_TCAMm_F3_12_AUX_TAG_2_MASKf_SET(fp_tcam_low, custom_header_mask);
    SOC_IF_ERROR_RETURN(WRITE_FP_TCAMm(0, entry_low, fp_tcam_low));

    /* Configure entry high */
    FP_TCAMm_CLR(fp_tcam_high);
    FP_TCAMm_VALIDf_SET(fp_tcam_high, 0x3);
    FP_TCAMm_DW_F2_0_INTERFACE_CLASSIDf_SET(fp_tcam_high, coe_permitted_class_id);
    FP_TCAMm_DW_F2_0_INTERFACE_CLASSID_MASKf_SET(fp_tcam_high, 0xFF);
    SOC_IF_ERROR_RETURN(WRITE_FP_TCAMm(0, entry_high, fp_tcam_high));
    /*--------------------------------------------------------------------*/

    /* Configure FP_POLICY_TABLE */
    /*--------------------------------------------------------------------*/
    /* Configure entry low */
    FP_POLICY_TABLEm_CLR(fp_policy_table);
    FP_POLICY_TABLEm_G_PACKET_REDIRECTIONf_SET(fp_policy_table, 0x3);
    FP_POLICY_TABLEm_REDIRECTION_TYPEf_SET(fp_policy_table, 0x2);
    FP_POLICY_TABLEm_REDIRECTION_PROFILE_INDEXf_SET(fp_policy_table, l2mc_index);
    FP_POLICY_TABLEm_METER_SHARING_MODE_MODIFIERf_SET(fp_policy_table, 0x1);
    FP_POLICY_TABLEm_METER_PAIR_MODEf_SET(fp_policy_table, 0x1);
    FP_POLICY_TABLEm_GREEN_TO_PIDf_SET(fp_policy_table, 0x1);

    coe_ifp_counter_mode = (coe_ifp_counter_table_index & 0x1) + 1;
    coe_ifp_counter_index = (coe_ifp_counter_table_index / 2)  ;
    FP_POLICY_TABLEm_COUNTER_MODEf_SET(fp_policy_table, coe_ifp_counter_mode);
    FP_POLICY_TABLEm_COUNTER_INDEXf_SET(fp_policy_table, coe_ifp_counter_index);
    SOC_IF_ERROR_RETURN(WRITE_FP_POLICY_TABLEm(0, entry_low, fp_policy_table));
    SHR_LOG_DEBUG("FP_COUNTER_TABLE index=%d\n", coe_ifp_counter_table_index);
    /* increase counter index count after WRITE_FP_POLICY_TABLEm */
    coe_ifp_counter_table_index++;

    /* Configure entry high */
    FP_POLICY_TABLEm_CLR(fp_policy_table);
    FP_POLICY_TABLEm_METER_SHARING_MODE_MODIFIERf_SET(fp_policy_table, 0x1);
    FP_POLICY_TABLEm_METER_PAIR_MODEf_SET(fp_policy_table, 0x1);
    FP_POLICY_TABLEm_GREEN_TO_PIDf_SET(fp_policy_table, 0x1);
    SOC_IF_ERROR_RETURN(WRITE_FP_POLICY_TABLEm(0, entry_high, fp_policy_table));
    /*--------------------------------------------------------------------*/

    /*--------------------------------------------------------------------*/
    /* Configure entry low */
    FP_GLOBAL_MASK_TCAMm_CLR(fp_global_mask_tcam);
    FP_GLOBAL_MASK_TCAMm_VALIDf_SET(fp_global_mask_tcam, 0x1);
    WRITE_FP_GLOBAL_MASK_TCAMm(0, entry_low, fp_global_mask_tcam);

    /* Configure entry high */
    FP_GLOBAL_MASK_TCAMm_CLR(fp_global_mask_tcam);
    FP_GLOBAL_MASK_TCAMm_VALIDf_SET(fp_global_mask_tcam, 0x1);
    WRITE_FP_GLOBAL_MASK_TCAMm(0, entry_high, fp_global_mask_tcam);
    /*--------------------------------------------------------------------*/
    coe_ifp_rule_number++;

    if (!coe_default_drop_init) {
        SHR_IF_ERR_EXIT(fl_coe_linecard_downstream_ifp_default_drop());
        coe_default_drop_init = 1;
    }

    SHR_IF_ERR_EXIT(SYS_OK);
exit:
    SHR_FUNC_EXIT();
}

/*
* IFP default drop
*/
sys_error_t
fl_coe_linecard_downstream_ifp_default_drop(void)
{
    FP_TCAMm_t              fp_tcam_low, fp_tcam_high;
	FP_GLOBAL_MASK_TCAMm_t  fp_global_mask_tcam;
    FP_POLICY_TABLEm_t      fp_policy_table;
    int entry_low, entry_high;
    SHR_FUNC_ENTER(SHR_NO_UNIT);

    entry_low = (ENTRIES_PER_SLICE / 2) - 1;
    entry_high = ENTRIES_PER_SLICE - 1;
    SHR_LOG_DEBUG("entry_low=%d entry_high=%d\n", entry_low, entry_high);

    /* Configure FP_TCAM */
    /*--------------------------------------------------------------------*/
    /* Configure entry low */
    FP_TCAMm_CLR(fp_tcam_low);
    FP_TCAMm_VALIDf_SET(fp_tcam_low, 0x3);
    SOC_IF_ERROR_RETURN(WRITE_FP_TCAMm(0, entry_low, fp_tcam_low));

    /* Configure entry high */
    FP_TCAMm_CLR(fp_tcam_high);
    FP_TCAMm_VALIDf_SET(fp_tcam_high, 0x3);
    SOC_IF_ERROR_RETURN(WRITE_FP_TCAMm(0, entry_high, fp_tcam_high));
    /*--------------------------------------------------------------------*/

    /* Configure FP_POLICY_TABLE */
    /*--------------------------------------------------------------------*/
    /* Configure entry low */
    FP_POLICY_TABLEm_CLR(fp_policy_table);
    FP_POLICY_TABLEm_METER_SHARING_MODE_MODIFIERf_SET(fp_policy_table, 0x1);
    FP_POLICY_TABLEm_METER_PAIR_MODEf_SET(fp_policy_table, 0x1);
    FP_POLICY_TABLEm_GREEN_TO_PIDf_SET(fp_policy_table, 0x1);
    FP_POLICY_TABLEm_Y_DROPf_SET(fp_policy_table, 0x1);
    FP_POLICY_TABLEm_R_DROPf_SET(fp_policy_table, 0x1);
    FP_POLICY_TABLEm_G_DROPf_SET(fp_policy_table, 0x1);
    coe_ifp_counter_mode = (entry_low & 0x1) + 1;
    coe_ifp_counter_index = (entry_low / 2)  ;
    FP_POLICY_TABLEm_COUNTER_MODEf_SET(fp_policy_table, coe_ifp_counter_mode);
    FP_POLICY_TABLEm_COUNTER_INDEXf_SET(fp_policy_table, coe_ifp_counter_index);
    SOC_IF_ERROR_RETURN(WRITE_FP_POLICY_TABLEm(0, entry_low, fp_policy_table));
    SHR_LOG_DEBUG("FP_COUNTER_TABLE index=%d\n", entry_low);

    /* Configure entry high */
    FP_POLICY_TABLEm_CLR(fp_policy_table);
    FP_POLICY_TABLEm_METER_SHARING_MODE_MODIFIERf_SET(fp_policy_table, 0x1);
    FP_POLICY_TABLEm_METER_PAIR_MODEf_SET(fp_policy_table, 0x1);
    FP_POLICY_TABLEm_GREEN_TO_PIDf_SET(fp_policy_table, 0x1);
    SOC_IF_ERROR_RETURN(WRITE_FP_POLICY_TABLEm(0, entry_high, fp_policy_table));
    /*--------------------------------------------------------------------*/

    /*--------------------------------------------------------------------*/
    /* Configure entry low */
    FP_GLOBAL_MASK_TCAMm_CLR(fp_global_mask_tcam);
    FP_GLOBAL_MASK_TCAMm_VALIDf_SET(fp_global_mask_tcam, 0x1);
    SOC_IF_ERROR_RETURN(
        WRITE_FP_GLOBAL_MASK_TCAMm(0, entry_low, fp_global_mask_tcam));

    /* Configure entry high */
    FP_GLOBAL_MASK_TCAMm_CLR(fp_global_mask_tcam);
    FP_GLOBAL_MASK_TCAMm_VALIDf_SET(fp_global_mask_tcam, 0x1);
    SOC_IF_ERROR_RETURN(
        WRITE_FP_GLOBAL_MASK_TCAMm(0, entry_high, fp_global_mask_tcam));
    /*--------------------------------------------------------------------*/

    coe_rule_table[coe_ifp_default_drop_rule_number].valid = 1;
    coe_rule_table[coe_ifp_default_drop_rule_number].default_drop = 1;
    coe_rule_table[coe_ifp_default_drop_rule_number].fp_entry_low = entry_low;
    coe_rule_table[coe_ifp_default_drop_rule_number].fp_entry_high = entry_high;
    coe_rule_table[coe_ifp_default_drop_rule_number].fp_counter_table_index =
        entry_low;
    SHR_IF_ERR_EXIT(SYS_OK);
exit:
    SHR_FUNC_EXIT();
}

/* IFP qualify source port and l2mc */
sys_error_t
fl_coe_uplink_ifp_qualify_source_port_and_l2mc(
            int source_port, int l2mc_idx, int active_class_id)
{
    FP_TCAMm_t fp_tcam;
    FP_GLOBAL_MASK_TCAMm_t fp_global_mask_tcam;
    FP_POLICY_TABLEm_t fp_policy_table;
    GLP_t glp;
    int idx1, idx2;

    SHR_FUNC_ENTER(SHR_NO_UNIT);

    SHR_LOG_DEBUG("source_port=%d\n", source_port);
    if (COE_OUT_OF_RANGE(source_port, BCM5607X_LPORT_MIN , BCM5607X_LPORT_MAX)) {
        sal_printf("source_port out of range\n");
        SHR_ERR_EXIT(SYS_ERR_PARAMETER);
    }

    if (coe_ifp_rule_number == coe_ifp_default_drop_rule_number) {
        coe_ifp_rule_number++;
    }
    coe_ifp_slice_number = coe_ifp_rule_number / (ENTRIES_PER_SLICE / 2);
    idx1 = (coe_ifp_slice_number * 2 * 64) + (coe_ifp_rule_number % 64);
    idx2 = (coe_ifp_slice_number * 2 * 64) + (coe_ifp_rule_number % 64) + 64;

    /* Configure FP_TCAM */
    FP_TCAMm_CLR(fp_tcam);
    FP_TCAMm_VALIDf_SET(fp_tcam, 0x3);

    GLP_CLR(glp);
    GLP_PORTf_SET(glp, source_port);
    FP_TCAMm_F1_11_SGLPf_SET(fp_tcam, GLP_GET(glp));
    FP_TCAMm_F1_11_SGLP_MASKf_SET(fp_tcam, 0xFFFF);
    SOC_IF_ERROR_RETURN(WRITE_FP_TCAMm(0, idx1, fp_tcam));

    FP_TCAMm_CLR(fp_tcam);
    FP_TCAMm_VALIDf_SET(fp_tcam, 0x3);
    if (active_class_id >= 0) {
        FP_TCAMm_DW_F2_0_INTERFACE_CLASSIDf_SET(fp_tcam, active_class_id);
        FP_TCAMm_DW_F2_0_INTERFACE_CLASSID_MASKf_SET(fp_tcam, 0xFF);
    }
    SOC_IF_ERROR_RETURN(WRITE_FP_TCAMm(0, idx2, fp_tcam));

    /* Configure FP_POLICY_TABLE */
    FP_POLICY_TABLEm_CLR(fp_policy_table);
    FP_POLICY_TABLEm_G_PACKET_REDIRECTIONf_SET(fp_policy_table, 0x3);
    FP_POLICY_TABLEm_REDIRECTION_TYPEf_SET(fp_policy_table, 0x2);
    FP_POLICY_TABLEm_REDIRECTION_PROFILE_INDEXf_SET(fp_policy_table, l2mc_idx);
    FP_POLICY_TABLEm_METER_SHARING_MODE_MODIFIERf_SET(fp_policy_table, 0x1);
    FP_POLICY_TABLEm_METER_PAIR_MODE_MODIFIERf_SET(fp_policy_table, 0x1);
    FP_POLICY_TABLEm_GREEN_TO_PIDf_SET(fp_policy_table, 0x1);

    coe_ifp_counter_mode = (coe_ifp_counter_table_index & 0x1) + 1;
    coe_ifp_counter_index = (coe_ifp_counter_table_index / 2)  ;
    FP_POLICY_TABLEm_COUNTER_MODEf_SET(fp_policy_table, coe_ifp_counter_mode);
    FP_POLICY_TABLEm_COUNTER_INDEXf_SET(fp_policy_table, coe_ifp_counter_index);
    SOC_IF_ERROR_RETURN(WRITE_FP_POLICY_TABLEm(0, idx1, fp_policy_table));
    SHR_LOG_DEBUG("FP_COUNTER_TABLE index=%d\n", coe_ifp_counter_table_index);
    coe_ifp_counter_table_index++;

    /* Configure entry high */
    FP_POLICY_TABLEm_CLR(fp_policy_table);
    FP_POLICY_TABLEm_METER_SHARING_MODE_MODIFIERf_SET(fp_policy_table, 0x1);
    FP_POLICY_TABLEm_METER_PAIR_MODEf_SET(fp_policy_table, 0x1);
    FP_POLICY_TABLEm_GREEN_TO_PIDf_SET(fp_policy_table, 0x1);
    SOC_IF_ERROR_RETURN(WRITE_FP_POLICY_TABLEm(0, idx2, fp_policy_table));

    /* Configure FP_GLOBAL_MASK_TCAM */
    FP_GLOBAL_MASK_TCAMm_CLR(fp_global_mask_tcam);
    FP_GLOBAL_MASK_TCAMm_VALIDf_SET(fp_global_mask_tcam, 0x1);
    WRITE_FP_GLOBAL_MASK_TCAMm(0, idx1, fp_global_mask_tcam);

    FP_GLOBAL_MASK_TCAMm_CLR(fp_global_mask_tcam);
    FP_GLOBAL_MASK_TCAMm_VALIDf_SET(fp_global_mask_tcam, 0x1);
    WRITE_FP_GLOBAL_MASK_TCAMm(0, idx2, fp_global_mask_tcam);

    coe_ifp_rule_number++;

    /* Configure the default drop */
    if (!coe_default_drop_init) {
        SHR_IF_ERR_EXIT(fl_coe_linecard_downstream_ifp_default_drop());
        coe_default_drop_init = 1;
    }

    SHR_IF_ERR_EXIT(SYS_OK);
exit:
    SHR_FUNC_EXIT();
}

/* COE default init */
sys_error_t
fl_coe_config_init(void){
#ifdef CFG_COE_SCENARIO_INCLUDED
    int i;
    int bp_port1, bp_port2, total_frontport;
    int frontport_set1, frontport_set2;
    uint32 coe_scenario = 0;
#endif
    SHR_FUNC_ENTER(SHR_NO_UNIT);

#ifdef CFG_COE_SCENARIO_INCLUDED
    if (sal_config_uint32_get(
            SAL_CONFIG_COE_SCENARIO, &coe_scenario)
            != SYS_OK) {
        SHR_LOG_DEBUG("COE Chassis Default read failed.\n");
    } else {
        if (coe_scenario != 0) {
            SHR_LOG_DEBUG("%s: coe_scenario=%d\n", __func__, coe_scenario);
        }
    }

    if (coe_scenario == 3) {
        /* config.um.956070K_op03-4x25g_2x100g
        *
        * CLI.0> ps
        * Port  Enable  Link  Speed   AN   Pause  Frame  LoopBack       INTerFace
        * 1     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  XFI
        * 2     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  XFI
        * 3     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  XFI
        * 4     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  XFI
        * 5     Yes     Up    100000  No   None   12284  LOOPBACK_NONE  KR4
        * 6     Yes     Up    100000  No   None   12284  LOOPBACK_NONE  KR4
        * CLI.0>
        */
        bp_port1 = 5;
        bp_port2 = 6;
        total_frontport = 4;
        for (i = 0; i < total_frontport; i++) {
            SHR_LOG_DEBUG("i=%d\n", i);
            SHR_IF_ERR_EXIT(
                board_coe_linecard_upstream_frontport_to_backplane_config(
                    bp_port1 /* redirect active port */,
                    bp_port2 /* redirect standby port */,
                    (1 + i) /* source port: uport */,
                    (2 + i) /* channel ID to encap */));
        }

        SHR_IF_ERR_EXIT(board_port_class_set(bp_port1, PortClassFieldIngress,
            ACT_CLASS_ID));
        SHR_IF_ERR_EXIT(board_port_class_set(bp_port2, PortClassFieldIngress,
            STB_CLASS_ID));
        SHR_IF_ERR_EXIT(board_coe_active_port_class(ACT_CLASS_ID));
        for (i = 0; i < total_frontport; i++) {
            SHR_IF_ERR_EXIT(
                board_coe_linecard_downstream_backplane_to_frontport_config(
                    bp_port1 /* active port */,
                    (1 + i) /* uport */,
                    (2 + i) /* channel ID */));
            SHR_IF_ERR_EXIT(
                board_coe_linecard_downstream_backplane_to_frontport_config(
                    bp_port2 /* standby port */,
                    (1 + i) /* uport */,
                    (2 + i) /* channel ID */));
        }
    } else if (coe_scenario == 4) {
        /* config.um.956070K_op05-10x10g_2x100g
        *
        * CLI.0> ps
        * Port  Enable  Link  Speed   AN   Pause  Frame  LoopBack       INTerFace
        * 1     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  SFI
        * 2     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  SFI
        * 3     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  SFI
        * 4     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  SFI
        * 5     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  SFI
        * 6     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  SFI
        * 7     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  SFI
        * 8     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  SFI
        * 9     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  SFI
        * 10    Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  SFI
        * 11    Yes     Up    100000  No   None   12284  LOOPBACK_NONE  KR4
        * 12    Yes     Up    100000  No   None   12284  LOOPBACK_NONE  KR4
        * CLI.0>
        */
        bp_port1 = 11;
        bp_port2 = 12;
        total_frontport = 10;
        for (i = 0; i < total_frontport; i++) {
            SHR_LOG_DEBUG("i=%d\n", i);
            SHR_IF_ERR_EXIT(
                board_coe_linecard_upstream_frontport_to_backplane_config(
                    bp_port1 /* redirect active port */,
                    bp_port2 /* redirect standby port */,
                    (1 + i) /* source port: uport */,
                    (2 + i) /* channel ID to encap */));
        }

        SHR_IF_ERR_EXIT(board_port_class_set(bp_port1, PortClassFieldIngress,
            ACT_CLASS_ID));
        SHR_IF_ERR_EXIT(board_port_class_set(bp_port2, PortClassFieldIngress,
            STB_CLASS_ID));
        SHR_IF_ERR_EXIT(board_coe_active_port_class(ACT_CLASS_ID));
        for (i = 0; i < total_frontport; i++) {
            SHR_IF_ERR_EXIT(
                board_coe_linecard_downstream_backplane_to_frontport_config(
                    bp_port1 /* active port */,
                    (1 + i) /* uport */,
                    (2 + i) /* channel ID */));
            SHR_IF_ERR_EXIT(
                board_coe_linecard_downstream_backplane_to_frontport_config(
                    bp_port2 /* standby port */,
                    (1 + i) /* uport */,
                    (2 + i) /* channel ID */));
        }
    } else if ((coe_scenario == 5) ||
                (coe_scenario == 6)) {
        /* config.um.956070K_op05-12x10g_2x100g
        *  config.um.956070K_op05-12x10g_1x100g_1x50g
        *  (port status for config.um.956070K_op05-12x10g_2x100g as below)
        *
        * CLI.0> ps
        * Port  Enable  Link  Speed   AN   Pause  Frame  LoopBack       INTerFace
        * 1     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  XFI
        * 2     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  XFI
        * 3     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  XFI
        * 4     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  XFI
        * 5     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  XFI
        * 6     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  XFI
        * 7     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  XFI
        * 8     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  XFI
        * 9     Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  XFI
        * 10    Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  XFI
        * 11    Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  XFI
        * 12    Yes     Up    10000   No   TX RX  12284  LOOPBACK_NONE  XFI
        * 13    Yes     Up    100000  No   None   12284  LOOPBACK_NONE  KR4
        * 14    Yes     Up    100000  No   None   12284  LOOPBACK_NONE  KR4
        * CLI.0>
        */
        bp_port1 = 13;
        bp_port2 = 14;
        frontport_set1 = 8;
        frontport_set2 = 4;

        /* No failover in this option, both backplane ports are active. */
        SHR_IF_ERR_EXIT(board_port_class_set(bp_port1, PortClassFieldIngress,
            ACT_CLASS_ID));
        SHR_IF_ERR_EXIT(board_port_class_set(bp_port2, PortClassFieldIngress,
            ACT_CLASS_ID));
        SHR_IF_ERR_EXIT(board_coe_active_port_class(ACT_CLASS_ID));

        for (i = 0; i < frontport_set1; i++) {
            SHR_LOG_DEBUG("i=%d\n", i);
            SHR_IF_ERR_EXIT(
                board_coe_linecard_upstream_frontport_to_backplane_config(
                    bp_port1 /* redirect backplane port */,
                    0 /* do NOT redirect standby port */,
                    (1 + i) /* source port: uport */,
                    (2 + i) /* channel ID to encap */));
        }
        for (i = frontport_set1; i < (frontport_set1 + frontport_set2); i++) {
            SHR_LOG_DEBUG("i=%d\n", i);
            SHR_IF_ERR_EXIT(
                board_coe_linecard_upstream_frontport_to_backplane_config(
                    bp_port2 /* redirect backplane port */,
                    0 /* do NOT redirect standby port */,
                    (1 + i) /* source port: uport */,
                    (2 + i) /* channel ID to encap */));
        }

        for (i = 0; i < frontport_set1; i++) {
            SHR_IF_ERR_EXIT(
                board_coe_linecard_downstream_backplane_to_frontport_config(
                    bp_port1 /* backplane port */,
                    (1 + i) /* uport */,
                    (2 + i) /* channel ID */));
        }
        for (i = frontport_set1; i < (frontport_set1 + frontport_set2); i++) {
            SHR_IF_ERR_EXIT(
                board_coe_linecard_downstream_backplane_to_frontport_config(
                    bp_port2 /* backplane port */,
                    (1 + i) /* uport */,
                    (2 + i) /* channel ID */));
        }
    }
#endif

    SHR_IF_ERR_EXIT(SYS_OK);
exit:
    SHR_FUNC_EXIT();
}

#endif /* CFG_COE_INCLUDED */