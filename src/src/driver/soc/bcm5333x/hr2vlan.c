/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#include "soc/bcm5333x.h"
#include "soc/port.h"
#include "brdimpl/vlan.h"

#ifdef CFG_SWITCH_VLAN_INCLUDED

extern vlan_info_t  vlan_info;

static vlan_type_t hr2_vlan_type = VT_COUNT;
/*
 *  Function : bcm5333x_pvlan_egress_set
 *
 *  Purpose :
 *      Set egr_mask in EGR_VLAN.
 *
 *  Parameters :
 *
 *  Return :
 *
 *  Note :
 */

sys_error_t
bcm5333x_pvlan_egress_set(uint8 unit, uint8 lport, pbmp_t lpbmp)
{
    sys_error_t rv = SYS_OK;
    uint32 entry, all_mask;
#ifdef CFG_SWITCH_LOOPDETECT_INCLUDED
    uint32 redirection_entry[2];
#endif
    all_mask = BCM5333x_ALL_PORTS_MASK;

    entry = lpbmp;

#ifdef CFG_SWITCH_LOOPDETECT_INCLUDED
    /* In the same time, update redirect profile for the port */
    bcm5333x_mem_get(0, M_IFP_REDIRECTION_PROFILE(lport), redirection_entry, 2);
    redirection_entry[0] &= (entry & all_mask);
    bcm5333x_mem_set(0, M_IFP_REDIRECTION_PROFILE(lport), redirection_entry, 2);
#endif /* CFG_SWITCH_LOOPDETECT_INCLUDED */

    entry = ~(entry & all_mask);
    rv = bcm5333x_mem_set(0, M_EGR_MASK(lport), &entry, 1);

    return rv;
}


/*
 *  Function : bcm5333x_pvlan_egress_get
 *
 *  Purpose :
 *      Get egr_mask in EGR_VLAN.
 *
 *  Parameters :
 *
 *  Return :
 *
 *  Note :
 */
sys_error_t
bcm5333x_pvlan_egress_get(uint8 unit, uint8 lport, pbmp_t *lpbmp)
{
    sys_error_t rv = SYS_OK;
    uint32 entry;

    rv = bcm5333x_mem_get(0, M_EGR_MASK(lport), &entry, 1);
    *lpbmp = (~entry);

    return rv;
}

sys_error_t
bcm5333x_qvlan_port_set(uint8 unit, uint16  vlan_id, pbmp_t lpbmp, pbmp_t tag_lpbmp)
{
    sys_error_t rv = SYS_OK;

    uint32 entry[4];
    pbmp_t untag_lpbmp;

    /* check if no exists */
    rv = bcm5333x_mem_get(0, M_VLAN(vlan_id), entry, 4);
    if((entry[1] & 0x40) != 0x40) {
        return SYS_ERR;
    }

    entry[0] = lpbmp | (entry[0] & 0xc0000000);
    rv = bcm5333x_mem_set(0, M_VLAN(vlan_id), entry, 4);

    untag_lpbmp = lpbmp & (~tag_lpbmp);
    rv = bcm5333x_mem_get(0, M_EGR_VLAN(vlan_id), entry, 3);
    entry[0] = ((lpbmp & 0x3) << 30) | untag_lpbmp;
    entry[1] &= ~(0xFFFFFFF);
    entry[1] |= lpbmp >> 2;
    rv = bcm5333x_mem_set(0, M_EGR_VLAN(vlan_id), entry, 3);

    return rv;
}

 sys_error_t
bcm5333x_qvlan_port_get(uint8 unit, uint16  vlan_id, pbmp_t *lpbmp, pbmp_t *tag_lpbmp)
{
    sys_error_t rv = SYS_OK;

    uint32 entry[4];

    /* Check VALID[Bit 38] filed for existence */
    rv = bcm5333x_mem_get(0, M_VLAN(vlan_id), entry, 4);
    if((entry[1] & 0x40) != 0x40) {
        return SYS_ERR;
    }

    rv = bcm5333x_mem_get(0, M_EGR_VLAN(vlan_id), entry, 3);
    *lpbmp = (entry[0] >> 30) | ((entry[1] & 0xFFFFFFF) << 2);
    *tag_lpbmp = (~entry[0] & 0x3fffffff) & (*lpbmp);

    return rv;
}

/*
 *  Function : bcm5333x_vlan_create
 *
 *  Purpose :
 *      Create vlan.
 *
 *  Parameters :
 *
 *  Return :
 *
 *  Note :
 */
sys_error_t
bcm5333x_vlan_create(uint8 unit, vlan_type_t type, uint16  vlan_id)
{
    sys_error_t rv = SYS_OK;
    uint32 entry[4];

    switch (type) {
        case VT_DOT1Q:
            /* Create vlan in VLAN_TAB
             * set VALID=1,PBMP=0, VLAN_PROFILE_PTR=0, STG=1
             */
            entry[0] = 0x0;
            entry[1] = 0xc0;
            entry[2] = 0x0;
            entry[3] = 0x0;
            rv = bcm5333x_mem_set(0, M_VLAN(vlan_id), entry, 4);

            /* Create vlan in EGR_VLAN with empty pbmp */
            entry[0] = 0x0;
            entry[1] = (0x1 << 28);
            entry[2] = 0x10;
            rv = bcm5333x_mem_set(0, M_EGR_VLAN(vlan_id), entry, 3);
            break;
        default:
              /* VT_PORT_BASED */
            break;
    }

    return rv;
}


/*
 *  Function : _bcm5333x_vlan_destroy
 *
 *  Purpose :
 *      Destroy vlan.
 *
 *  Parameters :
 *
 *  Return :
 *
 *  Note :
 */
sys_error_t
bcm5333x_vlan_destroy(uint8 unit, uint16  vlan_id)
{

    sys_error_t rv;
    uint32 entry[4] = { 0x0, 0x0, 0x0, 0x0 };

    /* destroy vlan in VLAN_TAB */
    rv = bcm5333x_mem_set(0, M_VLAN(vlan_id), entry, 4);

    /* destroy vlan in EGR_VLAN */
    rv = bcm5333x_mem_set(0, M_EGR_VLAN(vlan_id), entry, 3);

    return rv;
}


/*
 *  Function : bcm5333x_vlan_type_set
 *
 *  Purpose :
 *      Set current vlan type.
 *
 *  Parameters :
 *
 *  Return :
 *
 *  Note :
 */
sys_error_t
bcm5333x_vlan_type_set(uint8 unit, vlan_type_t type)
{
    int i;
    uint32 entry[4], port_entry[8];
    uint32 val;
    vlan_list_t     *this_vlan;

    sys_error_t rv = SYS_OK;

    if ((type == VT_NONE) || (type == VT_PORT_BASED))
    {

        /* Don't have to do this again if it was port based vlan */
        if ((hr2_vlan_type == VT_NONE) || (hr2_vlan_type == VT_PORT_BASED)) {
             hr2_vlan_type = type;
             return SYS_OK;
        }

        /* Enable USE_LEARN_VID and set LEARN_VID as '1' */
        bcm5333x_reg_get(0, R_VLAN_CTRL, &val);
        val = (val & 0xffffe000) | 0x00001001;
        bcm5333x_reg_set(0, R_VLAN_CTRL, val);

        /* clear EN_IFILTER in PORT_TAB */
        for (i = BCM5333X_LPORT_MIN; i <= BCM5333X_LPORT_MAX; i++) {
            bcm5333x_mem_get(0, M_PORT(i), port_entry, 8);
            port_entry[0] &= 0xffffffdf;
            bcm5333x_mem_set(0, M_PORT(i), port_entry, 8);
        }

        /* STG=1, VALID=1, PBMP=all except CPU, VLAN_PROFILE_PTR=0 */
        entry[0] = BCM5333x_ALL_PORTS_MASK;
        entry[1] = 0xc0;
        entry[2] = 0x0;
        entry[3] = 0x0;

        /* create 2-4094 vlans */
        for (i = 2; i <= 4094; i++) {
            bcm5333x_mem_set(0, M_VLAN(i), entry, 4);
        }

        /* All ports(exclude cpu) in vlan 2-4094 are tagged, STG=1 */
        entry[0] = 0x0;
        entry[1] = 0x1fffffff;
        entry[2] = 0x10;

        for (i = 2; i <= 4094; i++) {
            /* create vlan in EGR_VLAN */
            bcm5333x_mem_set(0, M_EGR_VLAN(i), entry, 3);
        }
        
        if (type == VT_PORT_BASED)
        {
            if (VLAN_DEFAULT != 1){
                this_vlan = vlan_info.head;
                if (this_vlan == NULL) {
                    sal_printf("%s..:%s..:this_vlan == NULL. Should not happen after brdimpl_vlan_reset()\n", __FILE__, __func__);
                    return SYS_ERR_OUT_OF_RESOURCE;
                }
                this_vlan->vlan_id = 1;
                            
                /* Re-overwrite the default VLAN to 1 for VT_PORT_BASED */
                /* Setup default vlan */
                /* STG=1, VALID=1, PBMP=all except CPU, VLAN_PROFILE_PTR=0 */
                entry[0] = BCM5333x_ALL_PORTS_MASK;
                entry[1] = 0xc0;
                entry[2] = 0x0;
                entry[3] = 0x0;
                bcm5333x_mem_set(0, M_VLAN(1), entry, 4);
            
                /* Setup EGR_VLAN, all ports untagged(exclude cpu) */
                entry[0] = BCM5333x_ALL_PORTS_MASK;
                entry[1] = 0x1fffffff;
                entry[2] = 0x10;
                bcm5333x_mem_set(0, M_EGR_VLAN(1), entry, 3);
                
                /* Reset PVID to default vlan */
                for (i = BCM5333X_LPORT_MIN; i <= BCM5333X_LPORT_MAX; i++) {
                    bcm5333x_mem_get(0, M_PORT(i), port_entry, 8);
                    port_entry[0] = (port_entry[0] & 0x00ffffff) | (this_vlan->vlan_id << 24);
            		port_entry[1] = (port_entry[1] & 0xfffffff0) | ((this_vlan->vlan_id & 0xf00) >> 8);   
                    
                    rv = bcm5333x_mem_set(0, M_PORT(i), port_entry, 8);
                }
    
            }
        }

    } else if (type == VT_DOT1Q)
    {

        /* Don't have to do this again if it was 802.1Q vlan */
        if (hr2_vlan_type == VT_DOT1Q) {
             hr2_vlan_type = type;
             return SYS_OK;
        }

        /* Disable USE_LEARN_VID and set LEARN_VID as '0' */
        bcm5333x_reg_get(0, R_VLAN_CTRL, &val);
        val = val & 0xffffe000;
        bcm5333x_reg_set(0, R_VLAN_CTRL, val);

        /* Enable EN_IFILTER to check VLAN membership */
        for (i = BCM5333X_LPORT_MIN; i <= BCM5333X_LPORT_MAX; i++) {
            bcm5333x_mem_get(0, M_PORT(i), port_entry, 8);
            port_entry[0] |= 0x00000020;
            bcm5333x_mem_set(0, M_PORT(i), port_entry, 8);
        }

        /* Clear VLAN and EGR_VLAN tables */
        entry[0] = entry[1] = entry[2] = entry[3] = 0;
        for (i = 1; i <= 4094; i++) {
            if (i==VLAN_DEFAULT)
                continue;
                
            /* destroy vlan in VLAN_TAB */
            rv = bcm5333x_mem_set(0, M_VLAN(i), entry, 4);
            rv = bcm5333x_mem_set(0, M_EGR_VLAN(i), entry, 3);
        }
        /* end of egress_mask change */
    }

    hr2_vlan_type = type;
    return rv;
}

/*
 *  Function : bcm5333x_vlan_reset
 *
 *  Purpose :
 *      Clear all vlan related tables..
 *
 *  Parameters :
 *
 *  Return :
 *
 *  Note :
 */
sys_error_t
bcm5333x_vlan_reset(uint8 unit)
{
    sys_error_t rv = SYS_OK;
    int i;
    uint32 entry[8];
#if defined(CFG_SWITCH_LAG_INCLUDED) || defined(CFG_SWITCH_LOOPDETECT_INCLUDED)
    uint32 all_mask = BCM5333x_ALL_PORTS_MASK;
#endif /* CFG_SWITCH_LAG_INCLUDED || CFG_SWITCH_LOOPDETECT_INCLUDED */    
#ifdef CFG_SWITCH_LAG_INCLUDED
    int j, k;
    uint32 lag_lpbmp[BOARD_MAX_NUM_OF_LAG];
#endif /* CFG_SWITCH_LAG_INCLUDED */

	/* Setup default vlan */
    /* STG=1, VALID=1, PBMP=all except CPU, VLAN_PROFILE_PTR=0 */
    entry[0] = BCM5333x_ALL_PORTS_MASK;
    entry[1] = 0xc0;
    entry[2] = 0x0;
    entry[3] = 0x0;
    bcm5333x_mem_set(0, M_VLAN(VLAN_DEFAULT), entry, 4);

    /* Setup EGR_VLAN, all ports untagged(exclude cpu) */
    entry[0] = BCM5333x_ALL_PORTS_MASK;
    entry[1] = 0x1fffffff;
    entry[2] = 0x10;
    bcm5333x_mem_set(0, M_EGR_VLAN(VLAN_DEFAULT), entry, 3);

    /* Set VLAN_STG and EGR_VLAN_STG */
    entry[0] = 0xfffffff0;
    entry[1] = 0x0fffffff;
    bcm5333x_mem_set(0, M_VLAN_STG(1), entry, 2);
    bcm5333x_mem_set(0, M_EGR_VLAN_STG(1), entry, 2);

    /* Clear egr_mask for reconstruct */
    entry[0] = 0x0;
    for (i = BCM5333X_PORT_MIN; i <= BCM5333X_PORT_MAX; i++) {
        rv = bcm5333x_mem_set(0, M_EGR_MASK(i), &entry[0], 1);
    }

    /* Recover egress_mask change in pvlan_port_set */
    for (i = BCM5333X_PORT_MIN; i <= BCM5333X_PORT_MAX; i++) {
#ifdef CFG_SWITCH_LAG_INCLUDED
        /*  Revise the all_mask based on trunk port bitmap */
        all_mask = BCM5333x_ALL_PORTS_MASK;
        for (j = 0; j < BOARD_MAX_NUM_OF_LAG; j++) {
            if (lag_lpbmp[j] != 0) {
                all_mask &= ~(lag_lpbmp[j]);
                if (!(lag_lpbmp[j] & (1 << i))) {
                    for (k = BCM5333X_PORT_MIN; k <= BCM5333X_PORT_MAX; k++) {
                        if (lag_lpbmp[j] & (1 << k)) {
                            all_mask |= (1 << k);
                            break;
                        }
                    }
                }
            }
        }
#endif /* CFG_SWITCH_LAG_INCLUDED */

#ifdef CFG_SWITCH_LOOPDETECT_INCLUDED
        bcm5333x_mem_get(0, M_IFP_REDIRECTION_PROFILE(i), entry, 2);
        entry[0] = all_mask;
        bcm5333x_mem_set(0, M_IFP_REDIRECTION_PROFILE(i), entry, 2);
#endif /* CFG_SWITCH_LOOPDETECT_INCLUDED */
    }

    /* Reset PVID to default vlan */
    for (i = BCM5333X_LPORT_MIN; i <= BCM5333X_LPORT_MAX; i++) {
        bcm5333x_mem_get(0, M_PORT(i), entry, 8);
        entry[0] = (entry[0] & 0x00ffffff) | (VLAN_DEFAULT << 24);
		entry[1] = (entry[1] & 0xfffffff0) | ((VLAN_DEFAULT & 0xf00) >> 8);   
        rv = bcm5333x_mem_set(0, M_PORT(i), entry, 8);
    }

    return rv;
}
#endif /* CFG_SWITCH_VLAN_INCLUDED */
