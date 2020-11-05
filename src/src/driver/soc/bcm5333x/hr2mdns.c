/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"

#ifdef CFG_ZEROCONF_MDNS_INCLUDED

#include "soc/bcm5333x.h"
#include "appl/mdns.h"

#define MDNS_LISTEN_PORT (5353)

/* Function:
 *   bcm5333x_mdns_enable_set
 * Description:
 *   Enable/Disable copying MDNS packets to CPU
 * Parameters:
 *   unit :
 *   mdns_enable :
 * Returns:
 *   None
 */
sys_error_t
bcm5333x_mdns_enable_set(uint8 unit, BOOL mdns_enable)
{
    uint32 tcam_entry[15], xy_entry[15];

    /* Action  : Copy to CPU */
    uint32 policy_entry[8] = {0x00000000, 0x02000000, 0x00000000, 0x00000110,
                              0x80004000, 0x04000000, 0x00000000, 0x00000000};
    uint32 global_tcam_mask_entry[3] = { 0x1, 0xffffff80, 0x7ff };

    if (unit >= BOARD_NUM_OF_UNITS){
        return SYS_ERR_PARAMETER;
    }

    sal_memset(tcam_entry, 0, sizeof(tcam_entry));

    if (mdns_enable) {
        /* Program L4 Dst port for MDNS at entry 127
         * FP_TCAM : L4_DST starts from bit 88
         */
        tcam_entry[0] = 0x3;
        tcam_entry[2] = ((MDNS_LISTEN_PORT << 24) & 0xFF000000);
        tcam_entry[3] = ((MDNS_LISTEN_PORT >> 8) & 0x000000FF);
        /* MASK */
        tcam_entry[10] = 0x0003fffc;

        bcm5333x_dm_to_xy(tcam_entry, xy_entry, 15, 234);
        bcm5333x_mem_set(0, M_FP_TCAM(1), xy_entry, 15);
        bcm5333x_mem_set(0, M_FP_POLICY_TABLE(1), policy_entry, 8);
        bcm5333x_mem_set(0, M_FP_GLOBAL_MASK_TCAM(1),
                                                global_tcam_mask_entry, 3);
    } else {
        bcm5333x_dm_to_xy(tcam_entry, xy_entry, 15, 234);
        bcm5333x_mem_set(0, M_FP_TCAM(1), xy_entry, 15);
        bcm5333x_mem_set(0, M_FP_GLOBAL_MASK_TCAM(1),
                                                global_tcam_mask_entry, 3);
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
bcm5333x_mdns_enable_get(uint8 unit, BOOL *mdns_enable)
{
    uint32 tcam_entry[15], dm_entry[15];

    if (unit >= BOARD_NUM_OF_UNITS){
        return SYS_ERR_PARAMETER;
    }

    bcm5333x_mem_get(0, M_FP_TCAM(1), dm_entry, 15);
    bcm5333x_xy_to_dm(dm_entry, tcam_entry, 15, 234);

    /* Check MDNS port */
    if (0x0 == tcam_entry[0]) {
        *mdns_enable = FALSE;
    } else {
        *mdns_enable = TRUE;
    }

    return SYS_OK;
}
#endif /* CFG_ZEROCONF_MDNS_INCLUDED */

