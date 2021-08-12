/*
 * $Id: flport.c,v 1.2 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#include "soc/port.h"
#include "boardapi/port.h"
#undef SOC_IF_ERROR_RETURN
#include <soc/error.h>

#ifdef CFG_SWITCH_QOS_INCLUDED
static uint32 dscp_table[64] = {
    0x00000000, 0x00000001, 0x00000002, 0x00000003, 0x00000004, 0x00000005, 0x00000006, 0x00000007,
    0x00000048, 0x00000049, 0x0000004a, 0x0000004b, 0x0000004c, 0x0000004d, 0x0000008e, 0x0000004f,
    0x00000090, 0x00000091, 0x00000092, 0x00000093, 0x00000094, 0x00000095, 0x00000096, 0x00000097,
    0x000000d8, 0x000000d9, 0x000000da, 0x000000db, 0x000000dc, 0x000000dd, 0x000000de, 0x000000df,
    0x00000120, 0x00000121, 0x00000122, 0x00000123, 0x00000124, 0x00000125, 0x00000126, 0x00000127,
    0x00000168, 0x00000169, 0x0000016a, 0x0000016b, 0x0000016c, 0x0000016d, 0x0000016e, 0x0000016f,
    0x000001b0, 0x000001b1, 0x000001b2, 0x000001b3, 0x000001b4, 0x000001b5, 0x000001b6, 0x000001b7,
    0x000001f8, 0x000001f9, 0x000001fa, 0x000001fb, 0x000001fc, 0x000001fd, 0x000001fe, 0x000001ff
};
static uint32 dot1pmap[16] = {
    0x00000000, 0x00000001, 0x00000004, 0x00000005, 0x00000008, 0x00000009, 0x0000000c, 0x0000000d,
    0x00000010, 0x00000011, 0x00000014, 0x00000015, 0x00000018, 0x00000019, 0x0000001c, 0x0000001d
};

#endif /* CFG_SWITCH_QOS_INCLUDED */

#ifdef CFG_SWITCH_EEE_INCLUDED
static uint8 eee_state[BCM5607X_LPORT_MAX+1];
#endif /* CFG_SWITCH_EEE_INCLUDED */


#ifdef CFG_SWITCH_RATE_INCLUDED
/* Ingress rate limit: FP slice 1 */
void
bcm5607x_rate_init(void)
{
    uint8 lport;
    int idx = 0;

    FP_TCAMm_t fp_tcam;
    FP_GLOBAL_MASK_TCAMm_t fp_global_mask_tcam;
    FP_POLICY_TABLEm_t fp_policy_table;
    FP_PORT_FIELD_SELm_t fp_port_field_sel;

    /* Ingress rate limit: FP slice 0 */
    for (lport = BCM5607X_LPORT_MIN; lport <= BCM5607X_LPORT_MAX; lport++) {
        READ_FP_PORT_FIELD_SELm(0, lport, fp_port_field_sel);
        FP_PORT_FIELD_SELm_SLICE0_F3f_SET(fp_port_field_sel, 11);
        WRITE_FP_PORT_FIELD_SELm(0, lport, fp_port_field_sel);

        FP_TCAMm_CLR(fp_tcam);
        FP_TCAMm_F3_11_SGLPf_SET(fp_tcam, lport);
        FP_TCAMm_F3_11_SGLP_MASKf_SET(fp_tcam, 0xFFFFFFFF);
        FP_TCAMm_VALIDf_SET(fp_tcam, 0);
        WRITE_FP_TCAMm(0, RATE_IGR_IDX + lport - BCM5607X_LPORT_MIN , fp_tcam);

        FP_GLOBAL_MASK_TCAMm_CLR(fp_global_mask_tcam);
        FP_GLOBAL_MASK_TCAMm_VALIDf_SET(fp_global_mask_tcam, 1);
        WRITE_FP_GLOBAL_MASK_TCAMm(0, RATE_IGR_IDX + lport - BCM5607X_LPORT_MIN, fp_global_mask_tcam);

        FP_POLICY_TABLEm_CLR(fp_policy_table);
        FP_POLICY_TABLEm_METER_SHARING_MODE_MODIFIERf_SET(fp_policy_table, 0x1);
        FP_POLICY_TABLEm_METER_PAIR_MODEf_SET(fp_policy_table, 0x1);
        FP_POLICY_TABLEm_R_DROPf_SET(fp_policy_table, 1);
        FP_POLICY_TABLEm_GREEN_TO_PIDf_SET(fp_policy_table, 1);

        if ( !(lport & 0x01) ) {
            FP_POLICY_TABLEm_METER_PAIR_INDEXf_SET(fp_policy_table, idx);
            FP_POLICY_TABLEm_METER_PAIR_MODE_MODIFIERf_SET(fp_policy_table, 0);
        } else {
            FP_POLICY_TABLEm_METER_PAIR_INDEXf_SET(fp_policy_table, idx);
            FP_POLICY_TABLEm_METER_PAIR_MODE_MODIFIERf_SET(fp_policy_table, 1);
            idx ++;
        }

        WRITE_FP_POLICY_TABLEm(0, RATE_IGR_IDX + (lport - BCM5607X_LPORT_MIN), fp_policy_table);
	}

}
#endif /* CFG_SWITCH_RATE_INCLUDED */

#ifdef CFG_SWITCH_QOS_INCLUDED
/* QoS: FP slice 2 */
void
bcm5607x_qos_init(void)
{
    int i, j;
    uint32 cos_map_setting[8] = { 0, 1, 2, 3, 4, 5, 6, 7} ;
    COS_MAPm_t cos_map;
    FP_TCAMm_t fp_tcam;
    FP_GLOBAL_MASK_TCAMm_t fp_global_mask_tcam;
    FP_POLICY_TABLEm_t fp_policy_table;
    FP_UDF_OFFSETm_t fp_udf_offset;
    DSCP_TBm_t dscp_tb;
    PORTm_t port;
    ING_PRI_CNG_MAPm_t ing_pri_cng_map;

    /* Using FP slice 2, entry (#define QOS_BASE_IDX                   (2 * ENTRIES_PER_SLICE)), to make 1p priority take
     * precedence over DSCP
     */
    for (i = 0; i < 8; i++) {
        FP_TCAMm_CLR(fp_tcam);
        FP_TCAMm_VALIDf_SET(fp_tcam, 3);
        /* VLAN TAG [TPID[16] PCP[3] CFI[1] VID[12]] */
        /* OuterValnPri */
        FP_TCAMm_F3_3_OUTER_VLAN_TAGf_SET(fp_tcam, (i << 13)); /* i is PCP */
        FP_TCAMm_F3_3_OUTER_VLAN_TAG_MASKf_SET(fp_tcam, (0x7 << 13));
        FP_TCAMm_F3_3_PACKET_FORMATf_SET(fp_tcam, 2); // out vlan tag
        FP_TCAMm_F3_3_PACKET_FORMAT_MASKf_SET(fp_tcam, 0x3); // out vlan tag mask
        WRITE_FP_TCAMm(0, DOT1P_BASE_IDX + i, fp_tcam);
        FP_GLOBAL_MASK_TCAMm_CLR(fp_global_mask_tcam);
        FP_GLOBAL_MASK_TCAMm_VALIDf_SET(fp_global_mask_tcam, 1);
        WRITE_FP_GLOBAL_MASK_TCAMm(0, DOT1P_BASE_IDX + i, fp_global_mask_tcam);

        FP_POLICY_TABLEm_CLR(fp_policy_table);
        FP_POLICY_TABLEm_R_COS_INT_PRIf_SET(fp_policy_table, i);
        FP_POLICY_TABLEm_Y_COS_INT_PRIf_SET(fp_policy_table, i);
        FP_POLICY_TABLEm_G_COS_INT_PRIf_SET(fp_policy_table, i);
        FP_POLICY_TABLEm_R_CHANGE_COS_OR_INT_PRIf_SET(fp_policy_table, 5);
        FP_POLICY_TABLEm_Y_CHANGE_COS_OR_INT_PRIf_SET(fp_policy_table, 5);
        FP_POLICY_TABLEm_G_CHANGE_COS_OR_INT_PRIf_SET(fp_policy_table, 5);
        FP_POLICY_TABLEm_METER_PAIR_MODE_MODIFIERf_SET(fp_policy_table, 1);
        FP_POLICY_TABLEm_METER_SHARING_MODE_MODIFIERf_SET(fp_policy_table, 1);
        FP_POLICY_TABLEm_GREEN_TO_PIDf_SET(fp_policy_table, 1);
        WRITE_FP_POLICY_TABLEm(0, DOT1P_BASE_IDX + i, fp_policy_table);
    }

    /* Offset for Data packets (qualifier MPLS_EXP) */
    FP_UDF_OFFSETm_CLR(fp_udf_offset);
    FP_UDF_OFFSETm_UDF1_OFFSET3f_SET(fp_udf_offset, 4);
    FP_UDF_OFFSETm_UDF1_OFFSET1f_SET(fp_udf_offset, 5);
    WRITE_FP_UDF_OFFSETm(0, 0, fp_udf_offset);
    WRITE_FP_UDF_OFFSETm(0, 1, fp_udf_offset);
    WRITE_FP_UDF_OFFSETm(0, 12, fp_udf_offset);

    /* MPLS_EXP with untag
     * Using FP slice 4, entry (#define MPLS_EXP_IDX
     * (4 * ENTRIES_PER_SLICE))
     */
    for (i = 0; i < 8; i++) {
        FP_TCAMm_CLR(fp_tcam);
        FP_TCAMm_VALIDf_SET(fp_tcam, 3);
        /* untag packet */
        /* ETHERTYPE: 0x8847, MPLS_EXP 3 bits */
        /* No macro for MPLS_EXP 3 bits, using raw data to set/get memories */
        /* OuterVlanPri */
        fp_tcam.fp_tcam[2]=((fp_tcam.fp_tcam[2] & ~((uint32)0x7 << 16)) | ((((uint32)i) & 0x7) << 16));
        fp_tcam.fp_tcam[9]=((fp_tcam.fp_tcam[9] & ~((uint32)0x7 << 28)) | ((((uint32)7) & 0x7) << 28));
        FP_TCAMm_F1_4_ETHERTYPEf_SET(fp_tcam, 0x8847);
        FP_TCAMm_F1_4_ETHERTYPE_MASKf_SET(fp_tcam, 0xffff);
        WRITE_FP_TCAMm(0, MPLS_EXP_IDX + i, fp_tcam);
        FP_GLOBAL_MASK_TCAMm_CLR(fp_global_mask_tcam);
        FP_GLOBAL_MASK_TCAMm_VALIDf_SET(fp_global_mask_tcam, 1);
        WRITE_FP_GLOBAL_MASK_TCAMm(0, MPLS_EXP_IDX + i, fp_global_mask_tcam);

        FP_POLICY_TABLEm_CLR(fp_policy_table);
        FP_POLICY_TABLEm_R_COS_INT_PRIf_SET(fp_policy_table, i);
        FP_POLICY_TABLEm_Y_COS_INT_PRIf_SET(fp_policy_table, i);
        FP_POLICY_TABLEm_G_COS_INT_PRIf_SET(fp_policy_table, i);
        FP_POLICY_TABLEm_R_CHANGE_COS_OR_INT_PRIf_SET(fp_policy_table, 5);
        FP_POLICY_TABLEm_Y_CHANGE_COS_OR_INT_PRIf_SET(fp_policy_table, 5);
        FP_POLICY_TABLEm_G_CHANGE_COS_OR_INT_PRIf_SET(fp_policy_table, 5);
        FP_POLICY_TABLEm_METER_PAIR_MODE_MODIFIERf_SET(fp_policy_table, 1);
        FP_POLICY_TABLEm_METER_SHARING_MODE_MODIFIERf_SET(fp_policy_table, 1);
        FP_POLICY_TABLEm_GREEN_TO_PIDf_SET(fp_policy_table, 1);
        WRITE_FP_POLICY_TABLEm(0, MPLS_EXP_IDX + i, fp_policy_table);
    }

    /* MPLS_EXP with untag, SVTAG
     * Using FP slice 6, entry (#define MPLS_EXP_IDX
     * (6 * ENTRIES_PER_SLICE))
     */
    for (i = 0; i < 8; i++) {
        FP_TCAMm_CLR(fp_tcam);
        /* untag packet */
        /* ETHERTYPE: 0x8847, MPLS_EXP 3 bits */
        /* No macro for MPLS_EXP 3 bits, using raw data to set/get memories */
        fp_tcam.fp_tcam[4]=((fp_tcam.fp_tcam[4] & ~((uint32)0x7 << 16)) | ((((uint32)i) & 0x7) << 16));
        field32_set(fp_tcam.fp_tcam, 380, 383, 0x7);
        FP_TCAMm_F1_4_ETHERTYPEf_SET(fp_tcam, 0x8847);
        FP_TCAMm_F1_4_ETHERTYPE_MASKf_SET(fp_tcam, 0xffff);
        FP_TCAMm_VALIDf_SET(fp_tcam, 3);
        FP_TCAMm_FIXED_CUSTOM_HEADER_PRESENTf_SET(fp_tcam, 1);
        FP_TCAMm_FIXED_CUSTOM_HEADER_PRESENT_MASKf_SET(fp_tcam, 1);
        WRITE_FP_TCAMm(0, MPLS_EXP_SVTAG_IDX + i, fp_tcam);
        FP_GLOBAL_MASK_TCAMm_CLR(fp_global_mask_tcam);
        FP_GLOBAL_MASK_TCAMm_VALIDf_SET(fp_global_mask_tcam, 1);
        WRITE_FP_GLOBAL_MASK_TCAMm(0, MPLS_EXP_SVTAG_IDX + i, fp_global_mask_tcam);

        FP_POLICY_TABLEm_CLR(fp_policy_table);
        FP_POLICY_TABLEm_R_COS_INT_PRIf_SET(fp_policy_table, i);
        FP_POLICY_TABLEm_Y_COS_INT_PRIf_SET(fp_policy_table, i);
        FP_POLICY_TABLEm_G_COS_INT_PRIf_SET(fp_policy_table, i);
        FP_POLICY_TABLEm_R_CHANGE_COS_OR_INT_PRIf_SET(fp_policy_table, 5);
        FP_POLICY_TABLEm_Y_CHANGE_COS_OR_INT_PRIf_SET(fp_policy_table, 5);
        FP_POLICY_TABLEm_G_CHANGE_COS_OR_INT_PRIf_SET(fp_policy_table, 5);
        FP_POLICY_TABLEm_METER_PAIR_MODE_MODIFIERf_SET(fp_policy_table, 1);
        FP_POLICY_TABLEm_METER_SHARING_MODE_MODIFIERf_SET(fp_policy_table, 1);
        FP_POLICY_TABLEm_GREEN_TO_PIDf_SET(fp_policy_table, 1);
        WRITE_FP_POLICY_TABLEm(0, MPLS_EXP_SVTAG_IDX + i, fp_policy_table);
    }

    /*
     * Assign 1p priority mapping:
     *  pri_1/2 ==> low    (COS0)
     *  pri_0/3 ==> normal (COS1)
     *  pri_4/5 ==> medium (COS2)
     *  pri_6/7 ==> high   (COS3)
     */
    for (i = 0; i < 8; i++) {
        COS_MAPm_CLR(cos_map);
        COS_MAPm_COSf_SET(cos_map, cos_map_setting[i]);
        WRITE_COS_MAPm(0,i,cos_map);
    }

    for (j = BCM5607X_LPORT_MIN; j <= BCM5607X_LPORT_MAX; j++) {
        /* Assign DSCP priority mapping */
        for (i = 0; i < 64; i++) {
            DSCP_TBm_CLR(dscp_tb);
            DSCP_TBm_SET(dscp_tb, dscp_table[i]);
            WRITE_DSCP_TBm(0,j * 64 + i, dscp_tb);
        }
        /* Assign dot1p mapping to port_tab */
        for (i = 0; i < 16; i++) {
            ING_PRI_CNG_MAPm_CLR(ing_pri_cng_map);
            ING_PRI_CNG_MAPm_SET(ing_pri_cng_map, dot1pmap[i]);
            WRITE_ING_PRI_CNG_MAPm(0, i + j*16, ing_pri_cng_map);
        }
    }

    /* DSCP mapping */
    bcm5607x_dscp_map_enable(1);

    /* Enable Trust_outer_dot1p/USE_INCOMING_DOT1P, mapping to ING_PRI_CNG_MAP. */
    for (i = BCM5607X_LPORT_MIN; i <= BCM5607X_LPORT_MAX; i++) {
        READ_PORTm(0,i,port);
        PORTm_TRUST_OUTER_DOT1Pf_SET(port, 1);
        
        PORTm_USE_INNER_PRIf_SET(port, 1);
        WRITE_PORTm(0,i,port);
 	}
}

void
bcm5607x_dscp_map_enable(BOOL enable)
{
    int i;
    PORTm_t port;

    for (i = BCM5607X_LPORT_MIN; i <= BCM5607X_LPORT_MAX; i++) {
        READ_PORTm(0, i, port);
        if (enable) {
            /* Enable TRUST_DSCP_V6 and TRUST_DSCP_V4 */
            PORTm_TRUST_DSCP_V6f_SET(port, 1);
            PORTm_TRUST_DSCP_V4f_SET(port, 1);
        } else {
            /* Disable TRUST_DSCP_V6 and TRUST_DSCP_V4 */
            PORTm_TRUST_DSCP_V6f_SET(port, 0);
            PORTm_TRUST_DSCP_V4f_SET(port, 0);

        }
        WRITE_PORTm(0, i, port);
    }
}
#endif /* CFG_SWITCH_QOS_INCLUDED */

#ifdef CFG_SWITCH_EEE_INCLUDED
void
bcm5607x_eee_init(void)
{
    uint8 lport;
    int mode;

    SOC_LPORT_ITER(lport){
		   pcm_port_eee_enable_set(0, lport, TRUE, &mode);
           eee_state[lport] = mode;
    }

}

/*
 *  Function : bcm5607x_port_eee_enable_set
 *
 *  Purpose :
 *      Set the EEE enable control of the specific port.
 *
 *  Parameters :
 *
 *  Return :
 *      SYS_OK : success
 *
 *  Note :
 */
sys_error_t
bcm5607x_port_eee_enable_set(uint8 unit, uint8 lport, uint8 enable, uint8 save)
{

    UMAC_EEE_CTRLr_t umac_eee_ctrl;
    XLMAC_EEE_CTRLr_t xlmac_eee_ctrl;
    CLMAC_EEE_CTRLr_t clmac_eee_ctrl;

    if (IS_GX_PORT(lport)) {

         READ_UMAC_EEE_CTRLr(unit, lport, umac_eee_ctrl);

         /* EEE_EN is bit 3 of register UMAC_EEE_CTRL */
         if(enable == TRUE) {
            UMAC_EEE_CTRLr_EEE_ENf_SET(umac_eee_ctrl, 1);
         } else {
            UMAC_EEE_CTRLr_EEE_ENf_SET(umac_eee_ctrl, 0);
         }
         WRITE_UMAC_EEE_CTRLr(unit, lport, umac_eee_ctrl);

    } else if (IS_XL_PORT(lport)) {

         READ_XLMAC_EEE_CTRLr(unit, lport, xlmac_eee_ctrl);

         /* EEE_EN is bit 3 of register UMAC_EEE_CTRL */
        if(enable == TRUE) {
           XLMAC_EEE_CTRLr_EEE_ENf_SET(xlmac_eee_ctrl, 1);
        } else {
           XLMAC_EEE_CTRLr_EEE_ENf_SET(xlmac_eee_ctrl, 0);
        }
        WRITE_XLMAC_EEE_CTRLr(unit, lport, xlmac_eee_ctrl);
    } else if (IS_CL_PORT(lport)) {

       READ_CLMAC_EEE_CTRLr(unit, lport, clmac_eee_ctrl);

       /* EEE_EN is bit 3 of register UMAC_EEE_CTRL */
      if(enable == TRUE) {
         CLMAC_EEE_CTRLr_EEE_ENf_SET(clmac_eee_ctrl, 1);
    } else {
       CLMAC_EEE_CTRLr_EEE_ENf_SET(clmac_eee_ctrl, 0);
    }
    WRITE_CLMAC_EEE_CTRLr(unit, lport, clmac_eee_ctrl);

    }



    if(save == TRUE) {
        eee_state[lport] = enable;
    }

    return SYS_OK;
}

/*
 *  Function : bcm5607x_port_eee_enable_get
 *
 *  Purpose :
 *      Get the current EEE enable status of the specific port.
 *
 *  Parameters :
 *
 *  Return :
 *      SYS_OK : success
 *
 *  Note :
 */
void
bcm5607x_port_eee_enable_get(uint8 unit, uint8 lport, uint8 *enable)
{
    *enable  = eee_state[lport];
}
#endif /* CFG_SWITCH_EEE_INCLUDED */




/*
 * Function    :
 *     bcm5607x_port_class_set
 * Purpose :
 *     Set the ports class ID. Ports with the
 *     same class ID can be treated as a group in
 *     field processing and VLAN translation.
 * Parameters  :
 *     unit - StrataSwitch Unit #.
 *     lport - StrataSwitch port #.
 *     pclass - Classification type
 *     pclass_id - New class ID of the port.
* Returns:
 *      SOC_E_NONE
 *      SOC_E_XXX
 */
sys_error_t
bcm5607x_port_class_set(int unit, int lport,
                       port_class_t pclass, uint32 pclass_id)
{
    sys_error_t rv = SYS_OK;
    EGR_PORT_64r_t egr_port;
    SOURCE_TRUNK_MAPm_t source_trunk_map;
    SOURCE_TRUNK_MAP_MODBASEm_t source_trunk_map_modbase;
    PORTm_t port_tab;
    int base_index, index;
    int modid = 0;

    switch (pclass) {
    case PortClassFieldLookup:
        /* Get table index. */
        READ_SOURCE_TRUNK_MAP_MODBASEm(unit, modid, source_trunk_map_modbase);
        base_index = SOURCE_TRUNK_MAP_MODBASEm_BASEf_GET(source_trunk_map_modbase);
        if (base_index == 0) {
            base_index = SOC_INFO(unit).port_addr_max + 1;

            SOURCE_TRUNK_MAP_MODBASEm_BASEf_SET(source_trunk_map_modbase, base_index);
            WRITE_SOURCE_TRUNK_MAP_MODBASEm(unit, modid, source_trunk_map_modbase);
        }
        index = base_index + lport;

        READ_PORTm(unit, index, port_tab);
        PORTm_VFP_PORT_GROUP_IDf_SET(port_tab, pclass_id);
        WRITE_PORTm(unit, index, port_tab);

        READ_SOURCE_TRUNK_MAPm(unit, index, source_trunk_map);
        SOURCE_TRUNK_MAPm_VFP_PORT_GROUP_IDf_SET(source_trunk_map, pclass_id);
        WRITE_SOURCE_TRUNK_MAPm(unit, index, source_trunk_map);
        break;
    case PortClassFieldIngress:
        /* Get table index. */
        READ_SOURCE_TRUNK_MAP_MODBASEm(unit, modid, source_trunk_map_modbase);
        base_index = SOURCE_TRUNK_MAP_MODBASEm_BASEf_GET(source_trunk_map_modbase);
        if (base_index == 0) {
            base_index = SOC_INFO(unit).port_addr_max + 1;

            SOURCE_TRUNK_MAP_MODBASEm_BASEf_SET(source_trunk_map_modbase, base_index);
            WRITE_SOURCE_TRUNK_MAP_MODBASEm(unit, modid, source_trunk_map_modbase);
        }
        index = base_index + lport;

        READ_SOURCE_TRUNK_MAPm(unit, index, source_trunk_map);
        SOURCE_TRUNK_MAPm_CLASS_IDf_SET(source_trunk_map, pclass_id);
        WRITE_SOURCE_TRUNK_MAPm(unit, index, source_trunk_map);
        break;
    case PortClassFieldEgress:
        READ_EGR_PORT_64r(unit, lport, egr_port);
        EGR_PORT_64r_EGR_PORT_GROUP_IDf_SET(egr_port, pclass_id);
        WRITE_EGR_PORT_64r(unit, lport, egr_port);
        break;
    case PortClassVlanTranslateEgress:
        READ_EGR_PORT_64r(unit, lport, egr_port);
        EGR_PORT_64r_VT_PORT_GROUP_IDf_SET(egr_port, pclass_id);
        WRITE_EGR_PORT_64r(unit, lport, egr_port);
        break;
    default:
        rv = SOC_E_PARAM;
    }

    return rv;
}

/*
 * Function    :
 *     bcm_port_class_get
 * Purpose :
 *     Get the ports class ID. Ports with the
 *     same class ID can be treated as a group in
 *     field processing and VLAN translation.
 * Parameters  :
 *     unit - StrataSwitch Unit #.
 *     lport - StrataSwitch port #.
 *     pclass - Classification type
 *     pclass_id - (OUT) New class ID of the port.
 * Returns:
 *     SOC_E_NONE
 *     SOC_E_XXX
 */
sys_error_t
bcm5607x_port_class_get(int unit, int lport,
                            port_class_t pclass, uint32 *pclass_id)
{
    sys_error_t rv = SYS_OK;
    EGR_PORT_64r_t egr_port;
    SOURCE_TRUNK_MAPm_t source_trunk_map;
    SOURCE_TRUNK_MAP_MODBASEm_t source_trunk_map_modbase;
    PORTm_t port_tab;
    int base_index, index;
    int modid = 0;

    if (NULL == pclass_id) {
        return SYS_ERR_PARAMETER;
    }

    switch (pclass) {
    case PortClassFieldLookup:
        /* Get table index. */
        READ_SOURCE_TRUNK_MAP_MODBASEm(unit, modid, source_trunk_map_modbase);
        base_index = SOURCE_TRUNK_MAP_MODBASEm_BASEf_GET(source_trunk_map_modbase);
        index = base_index + lport;

        READ_PORTm(unit, index, port_tab);
        *pclass_id = PORTm_VFP_PORT_GROUP_IDf_GET(port_tab);
        break;
    case PortClassFieldIngress:
        /* Get table index. */
        READ_SOURCE_TRUNK_MAP_MODBASEm(unit, modid, source_trunk_map_modbase);
        base_index = SOURCE_TRUNK_MAP_MODBASEm_BASEf_GET(source_trunk_map_modbase);
        index = base_index + lport;

        READ_SOURCE_TRUNK_MAPm(unit, index, source_trunk_map);
        *pclass_id = SOURCE_TRUNK_MAPm_CLASS_IDf_GET(source_trunk_map);
        break;
    case PortClassFieldEgress:
        READ_EGR_PORT_64r(unit, lport, egr_port);
        *pclass_id = EGR_PORT_64r_EGR_PORT_GROUP_IDf_GET(egr_port);
        break;
    case PortClassVlanTranslateEgress:
        READ_EGR_PORT_64r(unit, lport, egr_port);
        *pclass_id = EGR_PORT_64r_VT_PORT_GROUP_IDf_GET(egr_port);
        break;
    default:
        rv = SOC_E_PARAM;
    }

    return rv;
}


/*
 * Function:
 *      bcm5607x_port_frame_max_set
 * Description:
 *      Set the maximum receive frame size for the port
 * Parameters:
 *      unit - Device number
 *      lport - Port number
 *      size - Maximum frame size in bytes
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      Depending on chip or port type the actual maximum receive frame size
 *      might be slightly higher.
 *
 *      It looks like this operation is performed the same way on all the chips
 *      and the only depends on the port type.
 */
sys_error_t
bcm5607x_port_frame_max_set(int unit, int lport, int size)
{
    int rv;
    int max_size = PORT_JUMBO_MAXSZ;
    uint32 mtu_size = size;
    EGR_MTUr_t egr_mtu;

    if (SOC_INFO(unit).max_mtu) {
        max_size = SOC_INFO(unit).max_mtu;
    }

    if (size < 0 || size > max_size) {
        return SYS_ERR_PARAMETER;
    }

    rv = MAC_FRAME_MAX_SET(PORT(unit, lport).p_mac, unit, lport, size);
    if (SOC_FAILURE(rv)) {
        return SYS_ERR;
    }

    if (IS_XE_PORT(lport) || IS_GE_PORT(lport) || IS_CE_PORT(lport)) {
        mtu_size += 4;
    }

    READ_EGR_MTUr(unit, lport, egr_mtu);
    EGR_MTUr_MTU_SIZEf_SET(egr_mtu, mtu_size);
    EGR_MTUr_MTU_ENABLEf_SET(egr_mtu, 1);
    WRITE_EGR_MTUr(unit, lport, egr_mtu);

    return SYS_OK;
}

/*
 * Function:
 *      bcm5607x_port_frame_max_get
 * Description:
 *      Get the maximum receive frame size for the port
 * Parameters:
 *      unit - Device number
 *      in_port - Port number
 *      size - Maximum frame size in bytes
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      Depending on chip or port type the actual maximum receive frame size
 *      might be slightly higher.
 *
 *      For GE ports that use 2 separate MACs (one for GE and another one for
 *      10/100 modes) the function returns the maximum rx frame size set for
 *      the current mode.
 */
sys_error_t
bcm5607x_port_frame_max_get(int unit, int lport, int *size)
{
    int rv;

    rv = MAC_FRAME_MAX_GET(PORT(unit, lport).p_mac, unit, lport, size);
    return (rv) ? SYS_ERR : SYS_OK;
}


/*
 * Function:
 *      bcm5607x_port_control_get
 * Description:
 *      Get the status of specified port feature.
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      type - Enum  value of the feature
 *      value - (OUT) Current value of the port feature
 * Return Value:
 *      SYS_OK
 *      SYS_ERR
 */
sys_error_t
bcm5607x_port_control_get(int unit, int lport, int type, int *value)
{
    int rv = SOC_E_NONE;

    switch (type) {
        case pcmPortControlDoNotCheckVlan:
        {
            int modid = 0;
            int index, base_index;
            SOURCE_TRUNK_MAPm_t source_trunk_map;
            SOURCE_TRUNK_MAP_MODBASEm_t source_trunk_map_modbase;

            /* Get table index. */
            READ_SOURCE_TRUNK_MAP_MODBASEm(unit, modid, source_trunk_map_modbase);
            base_index = SOURCE_TRUNK_MAP_MODBASEm_BASEf_GET(source_trunk_map_modbase);
            index = base_index + lport;

            READ_SOURCE_TRUNK_MAPm(unit, index, source_trunk_map);
            *value = SOURCE_TRUNK_MAPm_DISABLE_VLAN_CHECKSf_GET(source_trunk_map);

            break;
        }

        case pcmPortControlPassControlFrames:
            if (IS_XE_PORT(lport) || IS_CE_PORT(lport) || IS_GE_PORT(lport)) {
                rv = MAC_CONTROL_GET(PORT(unit, lport).p_mac, unit, lport,
                                        SOC_MAC_PASS_CONTROL_FRAME, value);
            }
            break;

        default:
            break;
     }

    return (rv) ? SYS_ERR : SYS_OK;
}

/*
 * Function:
 *      bcm5607x_port_control_set
 * Description:
 *      Enable/Disable specified port feature.
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      type - Enum value of the feature
 *      value - value to be set
 * Return Value:
 *      SYS_OK
 *      SYS_ERR
 */
sys_error_t
bcm5607x_port_control_set(int unit, int lport, int type, int value)
{
    int rv = SOC_E_NONE;

    switch (type) {
        case pcmPortControlDoNotCheckVlan:
        {
            int modid = 0;
            int index, base_index;
            SOURCE_TRUNK_MAPm_t source_trunk_map;
            SOURCE_TRUNK_MAP_MODBASEm_t source_trunk_map_modbase;

            /* Get table index. */
            READ_SOURCE_TRUNK_MAP_MODBASEm(unit, modid, source_trunk_map_modbase);
            base_index = SOURCE_TRUNK_MAP_MODBASEm_BASEf_GET(source_trunk_map_modbase);
            if (base_index == 0) {
                base_index = SOC_INFO(unit).port_addr_max + 1;

                SOURCE_TRUNK_MAP_MODBASEm_BASEf_SET(source_trunk_map_modbase, base_index);
                WRITE_SOURCE_TRUNK_MAP_MODBASEm(unit, modid, source_trunk_map_modbase);
            }
            index = base_index + lport;

            READ_SOURCE_TRUNK_MAPm(unit, index, source_trunk_map);
            SOURCE_TRUNK_MAPm_DISABLE_VLAN_CHECKSf_SET(source_trunk_map, value);
            WRITE_SOURCE_TRUNK_MAPm(unit, index, source_trunk_map);

            break;
        }

        case pcmPortControlPassControlFrames:
        {
            PORTm_t port_tab;
            int val;

            value = (value) ? 1 : 0;

            READ_PORTm(unit, lport, port_tab);
            val = PORTm_PASS_CONTROL_FRAMESf_GET(port_tab);
            if (val != value) {
                PORTm_PASS_CONTROL_FRAMESf_SET(port_tab, value);
            }
            WRITE_PORTm(unit, lport, port_tab);

            if (IS_XE_PORT(lport) || IS_CE_PORT(lport) || IS_GE_PORT(lport)) {
                rv = MAC_CONTROL_SET(PORT(unit, lport).p_mac, unit, lport,
                                        SOC_MAC_PASS_CONTROL_FRAME, value);
            }

            break;
        }
    }

    return (rv) ? SYS_ERR : SYS_OK;
}



