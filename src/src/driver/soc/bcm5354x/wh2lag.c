/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <system.h>

/* Change the port name to the lport# */
#define SOC_PORT_NAME(unit, port)       (lport)

/* from sdk/include/sal/compiler.h */
#define COMPILER_REFERENCE(_a)    ((void)(_a))
#define COMPILER_ATTRIBUTE(_a)    __attribute__ (_a)
#ifndef FUNCTION_NAME
#define FUNCTION_NAME() (__FUNCTION__)
#endif

#include <shared/bsl.h>
#include <shared/bslenum.h>
#include <shared/bsltypes.h>

//#define BSL_LS_SOC_LAG_DEBUG

#if defined(LOG_VERBOSE) && defined(BSL_LS_SOC_LAG_DEBUG)
/* Outout the debug message with LOG_VERBOSE() */
#else
#undef LOG_VERBOSE
#define LOG_VERBOSE(ls_, stuff_)        
#endif



#ifdef CFG_SWITCH_LAG_INCLUDED
#if defined(CFG_SWITCH_RATE_INCLUDED) || defined(CFG_SWITCH_QOS_INCLUDED) || defined(CFG_SWITCH_LOOPDETECT_INCLUDED)
static void
_bcm5354x_lag_group_fp_set(uint8 unit, int start_index, uint8 lagid,
                     pbmp_t pbmp, pbmp_t old_pbmp, uint8 revise_redirect_pbmp, uint8 cpu_include)
{
    int i, j;


    
    FP_TCAMm_t fp_tcam;
    GLP_t  glp;

 
    /* The entry (pbmp) bit 0 is cpu port.
         * The policy[0] redirect entry, bit 5 is cpu port.
         */

    if (cpu_include == TRUE) {
        j = 0;
    } else {
        j = BCM5354X_LPORT_MIN;
    }
    for (i = start_index; j <= BCM5354X_LPORT_MAX; i++, j++) {
        if ((j > 0) && (j < BCM5354X_LPORT_MIN)) {
            continue;
        }
        READ_FP_TCAMm(unit, i, fp_tcam);
        /*  Revise the source tgid qualify if the port is trunk port */

        if (PBMP_MEMBER(pbmp, j)) {
            GLP_CLR(glp);
            GLP_TGIDf_SET(glp, lagid);
            GLP_Tf_SET(glp, 1);
        } else {
            GLP_CLR(glp);
            GLP_PORTf_SET(glp, j);
        }

        FP_TCAMm_F3_11_SGLPf_SET(fp_tcam, GLP_GET(glp));
        WRITE_FP_TCAMm(unit, i, fp_tcam);
    }

}

#endif /* CFG_SWITCH_RATE_INCLUDED || CFG_SWITCH_QOS_INCLUDED || CFG_SWITCH_LOOPDETECT_INCLUDED */

/*
 *  Function : bcm5354x_lag_group_set
 *  Purpose :
 *      Set lag group membership.
 *  Parameters :
 *  Return :
 *  Note :
 */
sys_error_t
bcm5354x_lag_group_set(uint8 unit, uint8 lagid, pbmp_t lpbmp)
{
    uint8 i, j, count = 0;
    uint8 trunk_port[BOARD_MAX_PORT_PER_LAG];

    TRUNK_BITMAPm_t old_bitmap;
    SOURCE_TRUNK_MAPm_t source_trunk_map;
    TRUNK_GROUPm_t trunk_group;
    NONUCAST_TRUNK_BLOCK_MASKm_t nonucast_trunk_block_mask;
    pbmp_t old_lpbmp, pbmp;

    for (i = BCM5354X_LPORT_MIN; i <= BCM5354X_LPORT_MAX && count < BOARD_MAX_PORT_PER_LAG; i++) {
        if(PBMP_MEMBER(lpbmp, i)) {
            count ++;
            trunk_port[count-1] = i;
        }
    }

    READ_TRUNK_BITMAPm(0, lagid, old_bitmap);

    PBMP_CLEAR(old_lpbmp);
    PBMP_WORD_SET(old_lpbmp, 0, TRUNK_BITMAPm_TRUNK_BITMAPf_GET(old_bitmap));
   
    if (PBMP_NEQ(lpbmp, old_lpbmp)) {

        /* Need to update source port qualifier in FP TCAM entry  */
#ifdef CFG_SWITCH_RATE_INCLUDED
        /*
         * Slice 1 Entry 0~23 (one entry for each port):
         * Rate ingress
         */
        _bcm5354x_lag_group_fp_set(unit, RATE_IGR_IDX, lagid,
                                   lpbmp, old_lpbmp, FALSE, FALSE);
#endif /* CFG_SWITCH_RATE_INCLUDED */

#ifdef CFG_SWITCH_QOS_INCLUDED
        /*
         * Slice 2 Entry 0~23 (one entry for each port):
         * Port based QoS
         */
        _bcm5354x_lag_group_fp_set(unit, QOS_BASE_IDX, lagid,
                                   lpbmp, old_lpbmp, FALSE, FALSE);
#endif /* CFG_SWITCH_QOS_INCLUDED */

#ifdef CFG_SWITCH_LOOPDETECT_INCLUDED
        /*
         * Slice 3 Entry BCM5354X_PORT_MIN~BCM5354X_PORT_MAX (one entry for each port):
         * Loop detect counter
         */
        _bcm5354x_lag_group_fp_set(unit, LOOP_COUNT_IDX, lagid,
                                   lpbmp, old_lpbmp, FALSE, FALSE);
        /*
         * Slice 3, #define LOOP_REDIRECT_IDX              ( 3 * ENTRIES_PER_SLICE + 35)
         * (one entry for each port, including CPU):
         * Update both source port qualifier 
         */
        _bcm5354x_lag_group_fp_set(unit, LOOP_REDIRECT_IDX, lagid,
                                    lpbmp, old_lpbmp, TRUE, TRUE);
#endif /* CFG_SWITCH_LOOPDETECT_INCLUDED */

    }

  /* Set TRUNK Bitmap, TRUNK Group, Source TRUNK Map and NonUcast TRUNK Block Mask Table */
    for (i = BCM5354X_LPORT_MIN; i <= BCM5354X_LPORT_MAX; i++) {
        if (PBMP_MEMBER(old_lpbmp, i)) {
            SOURCE_TRUNK_MAPm_CLR(source_trunk_map);
            WRITE_SOURCE_TRUNK_MAPm(unit, i, source_trunk_map);
        }

        if(PBMP_MEMBER(lpbmp, i)) {
            SOURCE_TRUNK_MAPm_CLR(source_trunk_map);
            SOURCE_TRUNK_MAPm_PORT_TYPEf_SET(source_trunk_map, 1);
            SOURCE_TRUNK_MAPm_TGIDf_SET(source_trunk_map, lagid);
            WRITE_SOURCE_TRUNK_MAPm(unit, i, source_trunk_map);
        }
    }
    TRUNK_GROUPm_CLR(trunk_group);

    for (i = 0; i < BOARD_MAX_PORT_PER_LAG; i++) {
        if (count == 0) {
            j = 0;
        } else {
            j = trunk_port[i%count];
        }
        switch (i) {
            case 0:
                TRUNK_GROUPm_PORT0f_SET(trunk_group, j);
                break;
            case 1:
                TRUNK_GROUPm_PORT1f_SET(trunk_group, j);
                break;
            case 2:
                TRUNK_GROUPm_PORT2f_SET(trunk_group, j);
                break;
            case 3:
                TRUNK_GROUPm_PORT3f_SET(trunk_group, j);
                break;
            case 4:
                TRUNK_GROUPm_PORT4f_SET(trunk_group, j);
                break;
            case 5:
                TRUNK_GROUPm_PORT5f_SET(trunk_group, j);
                break;
            case 6:
                TRUNK_GROUPm_PORT6f_SET(trunk_group, j);
                break;
            case 7:
                TRUNK_GROUPm_PORT7f_SET(trunk_group, j);
                break;
        }
    }

    /* Set RTAG to 0x3 (SA+DA) */
    TRUNK_BITMAPm_TRUNK_BITMAPf_SET(old_bitmap, SOC_PBMP(lpbmp));
    WRITE_TRUNK_BITMAPm(unit, lagid, old_bitmap);


    TRUNK_GROUPm_RTAGf_SET(trunk_group, 0x3);
    WRITE_TRUNK_GROUPm(unit, lagid, trunk_group);


    for (i = 0; i <= NONUCAST_TRUNK_BLOCK_MASKm_MAX; i++) {

        READ_NONUCAST_TRUNK_BLOCK_MASKm(unit, i, nonucast_trunk_block_mask);

        PBMP_CLEAR(pbmp);

        PBMP_WORD_SET(pbmp, 0, NONUCAST_TRUNK_BLOCK_MASKm_BLOCK_MASKf_GET(nonucast_trunk_block_mask));
        
        PBMP_REMOVE(pbmp, old_lpbmp);
        PBMP_OR(pbmp, lpbmp);

        if (count != 0) {
            PBMP_PORT_REMOVE(pbmp, trunk_port[i%count]);
        }

        NONUCAST_TRUNK_BLOCK_MASKm_BLOCK_MASKf_SET(nonucast_trunk_block_mask, SOC_PBMP(pbmp));

        WRITE_NONUCAST_TRUNK_BLOCK_MASKm(unit, i, nonucast_trunk_block_mask);
    }

    return SYS_OK;
}

/*
 *  Function : bcm5354x_lag_group_get
 *  Purpose :
 *      Get lag group membership.
 *  Parameters :
 *  Return :
 *  Note :
 */
void
bcm5354x_lag_group_get(uint8 unit, uint8 lagid, pbmp_t *pbmp) {

    TRUNK_BITMAPm_t trunk_bitmap;

    READ_TRUNK_BITMAPm(unit, lagid, trunk_bitmap);
    PBMP_CLEAR(*pbmp);
    PBMP_WORD_SET(*pbmp, 0, TRUNK_BITMAPm_TRUNK_BITMAPf_GET(trunk_bitmap));

}
#endif /* CFG_SWITCH_LAG_INCLUDED */


