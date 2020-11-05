/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"

#ifdef CFG_ZEROCONF_MDNS_INCLUDED

#include "soc/bcm5346x.h"
#include "appl/mdns.h"

#define MDNS_LISTEN_PORT (5353)

/* Function:
 *   bcm5346x_mdns_enable_set
 * Description:
 *   Enable/Disable copying MDNS packets to CPU
 * Parameters:
 *   unit :
 *   mdns_enable :
 * Returns:
 *   None
 */
sys_error_t
bcm5346x_mdns_enable_set(uint8 unit, BOOL mdns_enable)
{
    uint32 xy_entry[FP_TCAM_T_SIZE];
    uint32 dm_entry[FP_TCAM_T_SIZE];

	/* TCAM  : Copy to CPU */
	uint32 tcam_entry_to_cpu[FP_TCAM_T_SIZE]=
			{ 0x00000003, 0x00000000, 0xa4000000, 0x00000053,
				0x00000000, 0x00000000, 0x00000000, 0x00000000,
				0x00000000, 0x00000000, 0x003fffc0, 0x00000000, 
				0x00000000, 0x00000000, 0x00000000
			};
	/* Action  : Copy to CPU */
    uint32 policy_entry_to_cpu[FP_POLICY_T_SIZE] = 
    		{	0x00000000, 0x00000000, 0x00000000, 0x00000000,
    			0x00070088, 0x00400010, 0x00080000, 0x00000000,
    			0x00000000};

    uint32 global_tcam_mask_entry[FP_GLOBAL_TCAM_MASK_T_SIZE] = { 0x1, 0x20000000, 0, 0 };

    if (unit >= BOARD_NUM_OF_UNITS){
        return SYS_ERR_PARAMETER;
    }

    if (mdns_enable) {
        /* Program L4 Dst port for MDNS at entry MDNX_IDX
         * FP_TCAM : L4_DST starts from bit 90
         */
        bcm5346x_dm_to_xy(tcam_entry_to_cpu, xy_entry, FP_TCAM_T_SIZE, (32*FP_TCAM_T_SIZE));
        bcm5346x_mem_set(0, M_FP_TCAM(MDNS_TO_CPU_IDX), xy_entry, FP_TCAM_T_SIZE);
        bcm5346x_mem_set(0, M_FP_POLICY_TABLE(MDNS_TO_CPU_IDX), policy_entry_to_cpu, FP_POLICY_T_SIZE);
        bcm5346x_mem_set(0, M_FP_GLOBAL_MASK_TCAM(MDNS_TO_CPU_IDX),
                                                global_tcam_mask_entry, FP_GLOBAL_TCAM_MASK_T_SIZE);

    } else {
        bcm5346x_mem_get(0, M_FP_TCAM(MDNS_TO_CPU_IDX), xy_entry, FP_TCAM_T_SIZE);
        bcm5346x_xy_to_dm(xy_entry, dm_entry, FP_TCAM_T_SIZE, (32*FP_TCAM_T_SIZE));
        dm_entry[0] &= 0xfffffffc;
        bcm5346x_dm_to_xy(dm_entry, xy_entry, FP_TCAM_T_SIZE, (32*FP_TCAM_T_SIZE));
        bcm5346x_mem_set(0, M_FP_TCAM(MDNS_TO_CPU_IDX), xy_entry, FP_TCAM_T_SIZE);
        bcm5346x_mem_set(0, M_FP_GLOBAL_MASK_TCAM(MDNS_TO_CPU_IDX),
                                                global_tcam_mask_entry, FP_GLOBAL_TCAM_MASK_T_SIZE);
    }

    return SYS_OK;
}
/* Function:
 *   bcm533xx_mdns_enable_get
 * Description:
 *   Retrieve status of copying MDNS packets to CPU
 * Parameters:
 *   unit :
 *   mdns_enable :
 * Returns:
 *   None
 */
sys_error_t
bcm5346x_mdns_enable_get(uint8 unit, BOOL *mdns_enable)
{
    uint32 tcam_entry[FP_TCAM_T_SIZE], dm_entry[FP_TCAM_T_SIZE];

    if (unit >= BOARD_NUM_OF_UNITS){
        return SYS_ERR_PARAMETER;
    }

    bcm5346x_mem_get(0, M_FP_TCAM(MDNS_TO_CPU_IDX), dm_entry, FP_TCAM_T_SIZE);
    bcm5346x_xy_to_dm(dm_entry, tcam_entry, FP_TCAM_T_SIZE, (32*FP_TCAM_T_SIZE));

    /* Check MDNS port */
    if (0x0 == tcam_entry[0]) {
        *mdns_enable = FALSE;
    } else {
        *mdns_enable = TRUE;
    }

    return SYS_OK;
}
#endif /* CFG_ZEROCONF_MDNS_INCLUDED */

