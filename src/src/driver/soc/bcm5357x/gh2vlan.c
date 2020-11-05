/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.f
 */

#include "system.h"
#include "soc/port.h"
#include "brdimpl/vlan.h"

#ifdef CFG_SWITCH_VLAN_INCLUDED
extern vlan_info_t  vlan_info;
static vlan_type_t gh2_vlan_type = VT_COUNT;

/*
 *  Function : bcm5357x_pvlan_egress_set
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
bcm5357x_pvlan_egress_set(uint8 unit, uint8 lport, pbmp_t lpbmp)
{
    sys_error_t rv = SYS_OK;
    pbmp_t lpbmp_temp;
#ifdef CFG_SWITCH_LAG_INCLUDED
    TRUNK_BITMAPm_t trunk_bitmap;
    int lagid, lagid_lport;
    pbmp_t lag_pbmp[BOARD_MAX_NUM_OF_LAG];
#endif /* CFG_SWITCH_LAG_INCLUDED */
    EGR_MASKm_t egr_mask;
#ifdef CFG_SWITCH_LOOPDETECT_INCLUDED
    IFP_REDIRECTION_PROFILEm_t ifp_redirection_profile;
#endif

    PBMP_CLEAR(lpbmp_temp);
    PBMP_AND(lpbmp, BCM5357X_ALL_PORTS_MASK);

#ifdef CFG_SWITCH_LOOPDETECT_INCLUDED
#ifdef CFG_SWITCH_LAG_INCLUDED
    for (lagid = 0; lagid < BOARD_MAX_NUM_OF_LAG; lagid++) {
         READ_TRUNK_BITMAPm(0, lagid, trunk_bitmap);
         PBMP_CLEAR(lag_pbmp[lagid]);
         TRUNK_BITMAPm_TRUNK_BITMAPf_GET(trunk_bitmap, PBMP_PTR(lag_pbmp[lagid]));
    }
    /*  Revise the all_mask based on trunk port bitmap */
    for (lagid = 0; lagid < BOARD_MAX_NUM_OF_LAG; lagid++) {
        if (PBMP_NOT_NULL(lag_pbmp[lagid])) {
            PBMP_REMOVE(lpbmp, lag_pbmp[lagid]);
            if (!PBMP_MEMBER(lag_pbmp[lagid], lport)) {
                PBMP_ITER(lag_pbmp[lagid], lagid_lport) {
                     PBMP_PORT_ADD(lpbmp, lagid_lport);
                     break;
                }
            }
        }
    }
#endif /* CFG_SWITCH_LAG_INCLUDED */

    /* Update redirect pbmp to exclude src port. */        
    PBMP_PORT_REMOVE(lpbmp, lport); 

    /* In the same time, update redirect profile for the port */
    rv |= READ_IFP_REDIRECTION_PROFILEm(unit, lport, ifp_redirection_profile);
    IFP_REDIRECTION_PROFILEm_BITMAPf_GET(ifp_redirection_profile, SOC_PBMP(lpbmp_temp));
    PBMP_AND(lpbmp_temp, lpbmp);
    IFP_REDIRECTION_PROFILEm_BITMAPf_SET(ifp_redirection_profile, SOC_PBMP(lpbmp_temp));
    rv |= WRITE_IFP_REDIRECTION_PROFILEm(unit, lport, ifp_redirection_profile);
#endif /* CFG_SWITCH_LOOPDETECT_INCLUDED */

    PBMP_NEGATE(lpbmp, lpbmp);
    EGR_MASKm_CLR(egr_mask);
    EGR_MASKm_EGRESS_MASKf_SET(egr_mask, SOC_PBMP(lpbmp));
    rv |= WRITE_EGR_MASKm(unit, lport, egr_mask);

    return rv;
}

/*
 *  Function : bcm5357x_pvlan_egress_get
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
bcm5357x_pvlan_egress_get(uint8 unit, uint8 lport, pbmp_t *lpbmp)
{
    sys_error_t rv = SYS_OK;
    EGR_MASKm_t egr_mask;

    rv = READ_EGR_MASKm(unit, lport, egr_mask);
    PBMP_CLEAR((*lpbmp));
    EGR_MASKm_EGRESS_MASKf_GET(egr_mask, SOC_PBMP(*lpbmp));
    PBMP_NEGATE((*lpbmp), (*lpbmp));

    return rv;
}

sys_error_t 
bcm5357x_qvlan_port_set(uint8 unit, uint16  vlan_id, pbmp_t lpbmp, pbmp_t tag_lpbmp)
{
    sys_error_t rv = SYS_OK;
    pbmp_t untag_lpbmp;
    VLANm_t vlan;
    EGR_VLANm_t egr_vlan;

    /* check if no exists */
    rv = READ_VLANm(unit, vlan_id, vlan);
    if(VLANm_VALIDf_GET(vlan) == 0) {
        return SYS_ERR;
    }
    VLANm_PORT_BITMAPf_SET(vlan, SOC_PBMP(lpbmp));
    WRITE_VLANm(unit, vlan_id, vlan);

    PBMP_ASSIGN(untag_lpbmp, lpbmp);
    PBMP_REMOVE(untag_lpbmp, tag_lpbmp);

    rv |= READ_EGR_VLANm(unit, vlan_id, egr_vlan);
    EGR_VLANm_UT_BITMAPf_SET(egr_vlan, SOC_PBMP(untag_lpbmp));
    EGR_VLANm_PORT_BITMAPf_SET(egr_vlan, SOC_PBMP(lpbmp));
    rv |= WRITE_EGR_VLANm(unit, vlan_id, egr_vlan);

    return rv;
}

sys_error_t
bcm5357x_qvlan_port_get(uint8 unit, uint16  vlan_id, pbmp_t *lpbmp, pbmp_t *tag_lpbmp)
{
    sys_error_t rv = SYS_OK;
    VLANm_t vlan;
    EGR_VLANm_t egr_vlan;
    pbmp_t untag_lpbmp;

    /* check if no exists */
    rv = READ_VLANm(unit, vlan_id, vlan);
    if(VLANm_VALIDf_GET(vlan) == 0) {
        return SYS_ERR;
    }

    rv |= READ_EGR_VLANm(unit, vlan_id, egr_vlan);
    PBMP_CLEAR(untag_lpbmp);
    EGR_VLANm_UT_BITMAPf_GET(egr_vlan, SOC_PBMP(untag_lpbmp));
    PBMP_CLEAR(*lpbmp);
    EGR_VLANm_PORT_BITMAPf_GET(egr_vlan, SOC_PBMP(*lpbmp));
    PBMP_ASSIGN(*tag_lpbmp, *lpbmp);
    PBMP_REMOVE(*tag_lpbmp, untag_lpbmp);

    return rv;
}

/*
 *  Function : bcm5357x_vlan_create
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
bcm5357x_vlan_create(uint8 unit, vlan_type_t type, uint16  vlan_id)
{
    sys_error_t rv = SYS_OK;
    VLANm_t vlan;
    EGR_VLANm_t egr_vlan;
    pbmp_t pbmp;

    switch (type) {
        case VT_DOT1Q:
            /* Create vlan in VLAN_TAB
             * set VALID=1,PBMP=0, VLAN_PROFILE_PTR=0, STG=1
             */
            PBMP_CLEAR(pbmp);
            VLANm_CLR(vlan);
            VLANm_VALIDf_SET(vlan, 1);
            VLANm_VLAN_PROFILE_PTRf_SET(vlan, 0);
            VLANm_STGf_SET(vlan, 1);
            VLANm_PORT_BITMAPf_SET(vlan, SOC_PBMP(pbmp));
            WRITE_VLANm(unit, vlan_id, vlan);

            /* Create vlan in EGR_VLAN with empty pbmp */
            EGR_VLANm_CLR(egr_vlan);
            EGR_VLANm_STGf_SET(egr_vlan, 1);
            EGR_VLANm_VALIDf_SET(egr_vlan, 1);
            WRITE_EGR_VLANm(unit, vlan_id, egr_vlan);
            break;
        default:
              /* VT_PORT_BASED */
            break;
    }

    return rv;
}

/*
 *  Function : _bcm5357x_vlan_destroy
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
bcm5357x_vlan_destroy(uint8 unit, uint16  vlan_id)
{
    sys_error_t rv;
    VLANm_t vlan;
    EGR_VLANm_t egr_vlan;

    /* destroy vlan in VLAN_TAB */
    VLANm_CLR(vlan);
    rv = WRITE_VLANm(unit, vlan_id, vlan);

    /* destroy vlan in EGR_VLAN */
    EGR_VLANm_CLR(egr_vlan);
    rv |= WRITE_EGR_VLANm(unit, vlan_id, egr_vlan);

    return rv;
}

/*
 *  Function : bcm5357x_vlan_type_set
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
bcm5357x_vlan_type_set(uint8 unit, vlan_type_t type)
{
    int i;
    VLAN_CTRLr_t vlan_ctrl;
    VLANm_t vlan;
    EGR_VLANm_t egr_vlan;
    PORTm_t port;
    vlan_list_t *this_vlan;
    sys_error_t rv = SYS_OK;

    if ((type == VT_NONE) || (type == VT_PORT_BASED)) {
        /* Don't have to do this again if it was port based vlan */
        if ((gh2_vlan_type == VT_NONE) || (gh2_vlan_type == VT_PORT_BASED)) { 
             gh2_vlan_type = type;
             return SYS_OK;
        }

        /* Enable USE_LEARN_VID and set LEARN_VID as '1' */
        READ_VLAN_CTRLr(unit, vlan_ctrl);
        VLAN_CTRLr_LEARN_VIDf_SET(vlan_ctrl, 1);
        VLAN_CTRLr_USE_LEARN_VIDf_SET(vlan_ctrl, 1);
        WRITE_VLAN_CTRLr(unit,vlan_ctrl); 

        /* clear EN_IFILTER in PORT_TAB */ 
        for (i = BCM5357X_LPORT_MIN; i <= BCM5357X_LPORT_MAX; i++) {
            READ_PORTm(unit, i, port);
            PORTm_EN_IFILTERf_SET(port, 0);
            WRITE_PORTm(unit, i, port); 
        }

        /* STG=1, VALID=1, PBMP=all except CPU, VLAN_PROFILE_PTR=0 */
        VLANm_CLR(vlan);
        VLANm_PORT_BITMAPf_SET(vlan, SOC_PBMP(BCM5357X_ALL_PORTS_MASK));
        VLANm_STGf_SET(vlan, 1);
        VLANm_VALIDf_SET(vlan, 1);
        VLANm_VLAN_PROFILE_PTRf_SET(vlan, 0);
        /* create 2-4094 vlans */
        for (i = 2; i <= 4094; i++) {
             WRITE_VLANm(unit, i, vlan);
        }

        /* All ports(exclude cpu) in vlan 2-4094 are tagged, STG=1 */
        EGR_VLANm_CLR(egr_vlan);
        EGR_VLANm_PORT_BITMAPf_SET(egr_vlan, SOC_PBMP(BCM5357X_ALL_PORTS_MASK));
        EGR_VLANm_STGf_SET(egr_vlan, 1);
        EGR_VLANm_VALIDf_SET(egr_vlan, 1);
        for (i = 2; i <= 4094; i++) {
            /* create vlan in EGR_VLAN */
            WRITE_EGR_VLANm(unit, i, egr_vlan);
        }

        if (type == VT_PORT_BASED) {
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
                VLANm_CLR(vlan);
                VLANm_PORT_BITMAPf_SET(vlan, SOC_PBMP(BCM5357X_ALL_PORTS_MASK));
                VLANm_STGf_SET(vlan, 1);
                VLANm_VALIDf_SET(vlan, 1);
                VLANm_VLAN_PROFILE_PTRf_SET(vlan, 0);
                WRITE_VLANm(unit, 1, vlan);

                /* Setup EGR_VLAN, all ports untagged(exclude cpu) */
                EGR_VLANm_CLR(egr_vlan);
                EGR_VLANm_PORT_BITMAPf_SET(egr_vlan, SOC_PBMP(BCM5357X_ALL_PORTS_MASK));
                EGR_VLANm_UT_PORT_BITMAPf_SET(egr_vlan, SOC_PBMP(BCM5357X_ALL_PORTS_MASK));
                EGR_VLANm_STGf_SET(egr_vlan, 1);
                EGR_VLANm_VALIDf_SET(egr_vlan, 1);
                WRITE_EGR_VLANm(unit, 1, egr_vlan);

                /* Reset PVID to default vlan */
                for (i = BCM5357X_LPORT_MIN; i <= BCM5357X_LPORT_MAX; i++) {
                    rv |= READ_PORTm(unit, i, port);
                    PORTm_PORT_VIDf_SET(port, this_vlan->vlan_id);
                    rv |= WRITE_PORTm(unit, i, port);
                }
        
            }
        }
    } else if (type == VT_DOT1Q) {
        /* Don't have to do this again if it was 802.1Q vlan */
        if (gh2_vlan_type == VT_DOT1Q) { 
             gh2_vlan_type = type;
             return SYS_OK;
        }

        /* Disable USE_LEARN_VID and set LEARN_VID as '0' */
        READ_VLAN_CTRLr(unit, vlan_ctrl);
        VLAN_CTRLr_LEARN_VIDf_SET(vlan_ctrl, 0);
        VLAN_CTRLr_USE_LEARN_VIDf_SET(vlan_ctrl, 0);
        WRITE_VLAN_CTRLr(unit,vlan_ctrl); 

        /* Enable EN_IFILTER to check VLAN membership */ 
        for (i = BCM5357X_LPORT_MIN; i <= BCM5357X_LPORT_MAX; i++) {
            READ_PORTm(unit, i, port);
            PORTm_EN_IFILTERf_SET(port, 1);
            WRITE_PORTm(unit, i, port); 
        }

        /* Clear VLAN and EGR_VLAN tables */
        VLANm_CLR(vlan);
        EGR_VLANm_CLR(egr_vlan);
        for (i = 1; i <= 4094; i++) {
            if (i == VLAN_DEFAULT) {
                continue;
            }

            /* destroy vlan in VLAN_TAB */
            rv |= WRITE_VLANm(unit,i,vlan);
            rv |= WRITE_EGR_VLANm(unit,i,egr_vlan);
        }
        /* end of egress_mask change */
    }

    gh2_vlan_type = type;
    return rv;
}

/*
 *  Function : bcm5357x_vlan_reset
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
bcm5357x_vlan_reset(uint8 unit)
{
    sys_error_t rv = SYS_OK;
    int lport;
    VLANm_t vlan;
    EGR_VLANm_t egr_vlan;
    VLAN_STGm_t vlan_stg;
    EGR_VLAN_STGm_t egr_vlan_stg;
    EGR_MASKm_t egr_mask;
    PORTm_t port;
#ifdef CFG_SWITCH_LOOPDETECT_INCLUDED
    IFP_REDIRECTION_PROFILEm_t ifp_redirection_profile;
#endif
#if defined(CFG_SWITCH_LAG_INCLUDED) || defined(CFG_SWITCH_LOOPDETECT_INCLUDED)
    pbmp_t all_mask;
#endif /* CFG_SWITCH_LAG_INCLUDED || CFG_SWITCH_LOOPDETECT_INCLUDED */
#ifdef CFG_SWITCH_LAG_INCLUDED
    TRUNK_BITMAPm_t trunk_bitmap;
    int lagid, lagid_lport;
    pbmp_t lag_pbmp[BOARD_MAX_NUM_OF_LAG];
#endif /* CFG_SWITCH_LAG_INCLUDED */

#if defined(CFG_SWITCH_LAG_INCLUDED) || defined(CFG_SWITCH_LOOPDETECT_INCLUDED)
    /* Setup default vlan */
    PBMP_ASSIGN(all_mask, BCM5357X_ALL_PORTS_MASK);
#endif

#ifdef CFG_SWITCH_LAG_INCLUDED
	for (lagid = 0; lagid < BOARD_MAX_NUM_OF_LAG; lagid++) {
		 READ_TRUNK_BITMAPm(0, lagid, trunk_bitmap);
		 PBMP_CLEAR(lag_pbmp[lagid]);
		 TRUNK_BITMAPm_TRUNK_BITMAPf_GET(trunk_bitmap, PBMP_PTR(lag_pbmp[lagid])); 
	}
#endif /* CFG_SWITCH_LAG_INCLUDED */

    /* STG=1, VALID=1, PBMP=all except CPU, VLAN_PROFILE_PTR=0 */
    VLANm_CLR(vlan);
    VLANm_PORT_BITMAPf_SET(vlan, SOC_PBMP(BCM5357X_ALL_PORTS_MASK));
    VLANm_STGf_SET(vlan, 1);
    VLANm_VALIDf_SET(vlan, 1);
    VLANm_VLAN_PROFILE_PTRf_SET(vlan, 0);
    WRITE_VLANm(unit, VLAN_DEFAULT, vlan);

    /* Setup EGR_VLAN, all ports untagged(exclude cpu) */
    EGR_VLANm_CLR(egr_vlan);
    EGR_VLANm_UT_BITMAPf_SET(egr_vlan, SOC_PBMP(BCM5357X_ALL_PORTS_MASK));
    EGR_VLANm_PORT_BITMAPf_SET(egr_vlan, SOC_PBMP(BCM5357X_ALL_PORTS_MASK));
    EGR_VLANm_STGf_SET(egr_vlan, 1);
    EGR_VLANm_VALIDf_SET(egr_vlan, 1);
    WRITE_EGR_VLANm(unit, VLAN_DEFAULT, egr_vlan);

    /* Set VLAN_STG and EGR_VLAN_STG */
    VLAN_STGm_CLR(vlan_stg);
    VLAN_STGm_SET(vlan_stg, 0 , 0xfffffff0);
    VLAN_STGm_SET(vlan_stg, 1 , 0xffffffff);
    VLAN_STGm_SET(vlan_stg, 2 , 0xffffffff);
    VLAN_STGm_SET(vlan_stg, 3 , 0xffffffff);
    VLAN_STGm_SET(vlan_stg, 4 , 0x0000000f);

    EGR_VLAN_STGm_CLR(egr_vlan_stg);
    EGR_VLAN_STGm_SET(egr_vlan_stg, 0 , 0xfffffff0);
    EGR_VLAN_STGm_SET(egr_vlan_stg, 1 , 0xffffffff);
    EGR_VLAN_STGm_SET(egr_vlan_stg, 2 , 0xffffffff);
    EGR_VLAN_STGm_SET(egr_vlan_stg, 3 , 0xffffffff);
    EGR_VLAN_STGm_SET(egr_vlan_stg, 4 , 0x0000000f);

    WRITE_VLAN_STGm(unit, 1, vlan_stg);
    WRITE_EGR_VLAN_STGm(unit, 1, egr_vlan_stg);
    
    /* Clear egr_mask for reconstruct */
    EGR_MASKm_CLR(egr_mask);
    for (lport = BCM5357X_LPORT_MIN; lport <= BCM5357X_LPORT_MAX; lport++) {
        rv |= WRITE_EGR_MASKm(unit, lport, egr_mask);
    }

    /* Recover egress_mask change in pvlan_port_set */
    for (lport = BCM5357X_LPORT_MIN; lport <= BCM5357X_LPORT_MAX; lport++) {
#ifdef CFG_SWITCH_LAG_INCLUDED 
        /*  Revise the all_mask based on trunk port bitmap */
        for (lagid = 0; lagid < BOARD_MAX_NUM_OF_LAG; lagid++) {
            if (PBMP_NOT_NULL(lag_pbmp[lagid])) {
                PBMP_REMOVE(all_mask, lag_pbmp[lagid]);
                if (!PBMP_MEMBER(lag_pbmp[lagid], lport)) {
                    PBMP_ITER(lag_pbmp[lagid], lagid_lport) {
                         PBMP_PORT_ADD(all_mask, lagid_lport);
                         break;
                    }
                }
            }
        }
#endif /* CFG_SWITCH_LAG_INCLUDED */

#ifdef CFG_SWITCH_LOOPDETECT_INCLUDED
        /* Update redirect pbmp to exclude src port. */        
        PBMP_PORT_REMOVE(all_mask, lport); 
        READ_IFP_REDIRECTION_PROFILEm(unit, lport, ifp_redirection_profile);
        IFP_REDIRECTION_PROFILEm_BITMAPf_SET(ifp_redirection_profile, SOC_PBMP(all_mask));
        WRITE_IFP_REDIRECTION_PROFILEm(unit, lport, ifp_redirection_profile);
        PBMP_PORT_ADD(all_mask, lport); 
#endif /* CFG_SWITCH_LOOPDETECT_INCLUDED */
    }

    /* Reset PVID to default vlan */
    for (lport = BCM5357X_LPORT_MIN; lport <= BCM5357X_LPORT_MAX; lport++) {
        rv |= READ_PORTm(unit, lport, port);
        PORTm_PORT_VIDf_SET(port, VLAN_DEFAULT);
        rv |= WRITE_PORTm(unit, lport, port);
    }

    return rv;
}
#endif /* CFG_SWITCH_VLAN_INCLUDED */
