/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_ZEROCONF_MDNS_INCLUDED
#include "appl/mdns.h"

#define MDNS_LISTEN_PORT (5353)

/* Function:
 *   bcm5357x_mdns_enable_set
 * Description:
 *   Enable/Disable copying MDNS packets to CPU
 * Parameters:
 *   unit :
 *   mdns_enable :
 * Returns:
 *   None
 */
sys_error_t
bcm5357x_mdns_enable_set(uint8 unit, BOOL mdns_enable)
{
    FP_TCAMm_t fp_tcam;
    FP_POLICY_TABLEm_t fp_policy_table;
    FP_GLOBAL_MASK_TCAMm_t fp_global_mask_tcam;    
    pbmp_t lpbmp;

    if (unit >= BOARD_NUM_OF_UNITS){
        return SYS_ERR_PARAMETER;
    }

    if (mdns_enable) {
        /* Program L4 Dst port for MDNS at entry MDNX_IDX
        * FP_TCAM : L4_DST starts from bit 90
        */       

        FP_TCAMm_CLR(fp_tcam);
        FP_TCAMm_F2_0_L4_DSTf_SET(fp_tcam, MDNS_LISTEN_PORT);
        FP_TCAMm_F2_0_L4_DST_MASKf_SET(fp_tcam, 0x0000FFFF);
        FP_TCAMm_VALIDf_SET(fp_tcam, 3);
        WRITE_FP_TCAMm(unit, MDNS_TO_CPU_IDX, fp_tcam);

        FP_POLICY_TABLEm_CLR(fp_policy_table);
        FP_POLICY_TABLEm_Y_COPY_TO_CPUf_SET(fp_policy_table, 1);
        FP_POLICY_TABLEm_R_COPY_TO_CPUf_SET(fp_policy_table, 1);
        FP_POLICY_TABLEm_G_COPY_TO_CPUf_SET(fp_policy_table, 1);
        FP_POLICY_TABLEm_COUNTER_MODEf_SET(fp_policy_table, 1);
        FP_POLICY_TABLEm_METER_SHARING_MODE_MODIFIERf_SET(fp_policy_table, 1);
        FP_POLICY_TABLEm_METER_PAIR_MODE_MODIFIERf_SET(fp_policy_table, 1);
        FP_POLICY_TABLEm_GREEN_TO_PIDf_SET(fp_policy_table, 1);
        WRITE_FP_POLICY_TABLEm(unit, MDNS_TO_CPU_IDX, fp_policy_table);

        FP_GLOBAL_MASK_TCAMm_CLR(fp_global_mask_tcam);
        
        PBMP_CLEAR(lpbmp);
        FP_GLOBAL_MASK_TCAMm_IPBMf_SET(fp_global_mask_tcam, SOC_PBMP(lpbmp));        
        PBMP_PORT_ADD(lpbmp, 0 /* CPU port mask */);
        FP_GLOBAL_MASK_TCAMm_IPBM_MASKf_SET(fp_global_mask_tcam, SOC_PBMP(lpbmp));        

        FP_GLOBAL_MASK_TCAMm_VALIDf_SET(fp_global_mask_tcam, 1);
        WRITE_FP_GLOBAL_MASK_TCAMm(unit, MDNS_TO_CPU_IDX, fp_global_mask_tcam);

    } else {
        READ_FP_TCAMm(0, MDNS_TO_CPU_IDX, fp_tcam);
        FP_TCAMm_VALIDf_SET(fp_tcam, 0);
        WRITE_FP_TCAMm(0, MDNS_TO_CPU_IDX, fp_tcam);

    }
    return SYS_OK;
}
/* Function:
 *   bcm5357x_mdns_enable_get
 * Description:
 *   Retrieve status of copying MDNS packets to CPU
 * Parameters:
 *   unit :
 *   mdns_enable :
 * Returns:
 *   None
 */
sys_error_t
bcm5357x_mdns_enable_get(uint8 unit, BOOL *mdns_enable)
{
    FP_TCAMm_t fp_tcam;

    if (unit >= BOARD_NUM_OF_UNITS){
        return SYS_ERR_PARAMETER;
    }
    
    READ_FP_TCAMm(unit, MDNS_TO_CPU_IDX, fp_tcam);    
    /* Check MDNS port */
    if (FP_TCAMm_VALIDf_GET(fp_tcam) == 0x0) {
        *mdns_enable = FALSE;
    } else {
        *mdns_enable = TRUE;
    }

    return SYS_OK;
}
#endif /* CFG_ZEROCONF_MDNS_INCLUDED */

