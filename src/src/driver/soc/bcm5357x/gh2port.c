/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#include "soc/port.h"



#ifdef CFG_SWITCH_QOS_INCLUDED
static uint32 dscp_table[64] = {
    0x00000040, 0x00000041, 0x00000042, 0x00000043, 0x00000044, 0x00000045, 0x00000046, 0x00000047,
    0x00000048, 0x00000049, 0x0000004a, 0x0000004b, 0x0000004c, 0x0000004d, 0x0000004e, 0x0000004f,
    0x000000d0, 0x000000d1, 0x000000d2, 0x000000d3, 0x000000d4, 0x000000d5, 0x000000d6, 0x000000d7,
    0x000000d8, 0x000000d9, 0x000000da, 0x000000db, 0x000000dc, 0x000000dd, 0x000000de, 0x000000df,
    0x00000160, 0x00000161, 0x00000162, 0x00000163, 0x00000164, 0x00000165, 0x00000166, 0x00000167,
    0x00000168, 0x00000169, 0x0000016a, 0x0000016b, 0x0000016c, 0x0000016d, 0x0000016e, 0x0000016f,
    0x000001f0, 0x000001f1, 0x000001f2, 0x000001f3, 0x000001f4, 0x000001f5, 0x000001f6, 0x000001f7,
    0x000001f8, 0x000001f9, 0x000001fa, 0x000001fb, 0x000001fc, 0x000001fd, 0x000001fe, 0x000001ff
};
static uint32 dot1pmap[16] = {
    0x00000000, 0x00000001, 0x00000004, 0x00000005, 0x00000008, 0x00000009, 0x0000000c, 0x0000000d,
    0x00000010, 0x00000011, 0x00000014, 0x00000015, 0x00000018, 0x00000019, 0x0000001c, 0x0000001d
};

#endif /* CFG_SWITCH_QOS_INCLUDED */

#ifdef CFG_SWITCH_EEE_INCLUDED
static uint8 eee_state[BCM5357X_LPORT_MAX+1];
#endif /* CFG_SWITCH_EEE_INCLUDED */


#ifdef CFG_SWITCH_RATE_INCLUDED
/* Ingress rate limit: FP slice 1 */
void
bcm5357x_rate_init(void)
{
    uint8 lport;
    int idx = 0;

    FP_TCAMm_t fp_tcam;
    FP_GLOBAL_MASK_TCAMm_t fp_global_mask_tcam;
    FP_POLICY_TABLEm_t fp_policy_table;
    FP_PORT_FIELD_SELm_t fp_port_field_sel; 

    /* Ingress rate limit: FP slice 0 */
    for (lport = BCM5357X_LPORT_MIN; lport <= BCM5357X_LPORT_MAX; lport++) {
        READ_FP_PORT_FIELD_SELm(0, lport, fp_port_field_sel);
        FP_PORT_FIELD_SELm_SLICE0_F3f_SET(fp_port_field_sel, 11);
        WRITE_FP_PORT_FIELD_SELm(0, lport, fp_port_field_sel);

        FP_TCAMm_CLR(fp_tcam);
        FP_TCAMm_F3_11_SGLPf_SET(fp_tcam, lport);
        FP_TCAMm_F3_11_SGLP_MASKf_SET(fp_tcam, 0xFFFFFFFF);
        FP_TCAMm_VALIDf_SET(fp_tcam, 0);
        WRITE_FP_TCAMm(0, RATE_IGR_IDX + lport - BCM5357X_LPORT_MIN , fp_tcam);

        FP_GLOBAL_MASK_TCAMm_CLR(fp_global_mask_tcam);
        FP_GLOBAL_MASK_TCAMm_VALIDf_SET(fp_global_mask_tcam, 1);
        WRITE_FP_GLOBAL_MASK_TCAMm(0, RATE_IGR_IDX + lport - BCM5357X_LPORT_MIN, fp_global_mask_tcam);

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

        WRITE_FP_POLICY_TABLEm(0, RATE_IGR_IDX + (lport - BCM5357X_LPORT_MIN), fp_policy_table);          
	}

}
#endif /* CFG_SWITCH_RATE_INCLUDED */

#ifdef CFG_SWITCH_QOS_INCLUDED
/* QoS: FP slice 2 */
void
bcm5357x_qos_init(void)
{
    int i, j;
    uint32 cos_map_setting[8] = { 1, 0, 0, 1, 2, 2, 3, 3} ;
    COS_MAPm_t cos_map;
    FP_TCAMm_t fp_tcam;
    FP_GLOBAL_MASK_TCAMm_t fp_global_mask_tcam;
    FP_POLICY_TABLEm_t fp_policy_table;
    DSCP_TBm_t dscp_tb;
    PORTm_t port;
    ING_PRI_CNG_MAPm_t ing_pri_cng_map;

    /* Using FP slice 2, entry (#define QOS_BASE_IDX                   (2 * ENTRIES_PER_SLICE)), to make 1p priority take
     * precedence over DSCP
     */
    for (i = 0; i < 8; i++) {
        FP_TCAMm_CLR(fp_tcam);
        /* VLAN TAG [TPID[16] PCP[3] CFI[1] VID[12]] */
        FP_TCAMm_F3_3_PACKET_FORMATf_SET(fp_tcam, 2); // out vlan tag 
        FP_TCAMm_F3_3_PACKET_FORMAT_MASKf_SET(fp_tcam, 0x3); // out vlan tag mask
        FP_TCAMm_F3_3_OUTER_VLAN_TAGf_SET(fp_tcam, (i << 13)); /* i is PCP */
        FP_TCAMm_F3_3_OUTER_VLAN_TAG_MASKf_SET(fp_tcam, (0x7 << 13));
        FP_TCAMm_VALIDf_SET(fp_tcam, 3);
        WRITE_FP_TCAMm(0,DOT1P_BASE_IDX + i, fp_tcam);
        
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

    /*
     * Having 1p priority take precedence over DSCP:
     * Disable port base QoS via setting invalid bit in FP_TCAM for entry
     * 0_23 for port based QoS
     */
    for (i = BCM5357X_LPORT_MIN; i <= BCM5357X_LPORT_MAX; i++) {
        READ_FP_TCAMm(0, QOS_BASE_IDX + (i - BCM5357X_LPORT_MIN), fp_tcam);
        FP_TCAMm_VALIDf_SET(fp_tcam, 0);
        WRITE_FP_TCAMm(0,QOS_BASE_IDX + (i - BCM5357X_LPORT_MIN), fp_tcam); 
        
        FP_GLOBAL_MASK_TCAMm_CLR(fp_global_mask_tcam);
        FP_GLOBAL_MASK_TCAMm_VALIDf_SET(fp_global_mask_tcam, 1);
        WRITE_FP_GLOBAL_MASK_TCAMm(0, QOS_BASE_IDX + (i - BCM5357X_LPORT_MIN), fp_global_mask_tcam);
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

    for (j = BCM5357X_LPORT_MIN; j <= BCM5357X_LPORT_MAX; j++) {
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

    /* Enable Trust_outer_dot1p/USE_INCOMING_DOT1P, mapping to ING_PRI_CNG_MAP. */
    for (i = BCM5357X_LPORT_MIN; i <= BCM5357X_LPORT_MAX; i++) {
        READ_PORTm(0,i,port);
        PORTm_TRUST_OUTER_DOT1Pf_SET(port, 1);
        
        PORTm_USE_INNER_PRIf_SET(port, 1);
        WRITE_PORTm(0,i,port);
 	}
}

void
bcm5357x_dscp_map_enable(BOOL enable)
{
    int i;
    PORTm_t port;

    for (i = BCM5357X_LPORT_MIN; i <= BCM5357X_LPORT_MAX; i++) {
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
bcm5357x_eee_init(void)
{
    uint8 lport;
    int mode;

    SOC_LPORT_ITER(lport){
		   pcm_port_eee_enable_set(0, lport, TRUE, &mode);
           eee_state[lport] = mode;
    }

}

/*
 *  Function : bcm5357x_port_eee_enable_set
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
bcm5357x_port_eee_enable_set(uint8 unit, uint8 lport, uint8 enable, uint8 save)
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
 *  Function : bcm5357x_port_eee_enable_get
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
bcm5357x_port_eee_enable_get(uint8 unit, uint8 lport, uint8 *enable)
{
    *enable  = eee_state[lport];
}
#endif /* CFG_SWITCH_EEE_INCLUDED */


