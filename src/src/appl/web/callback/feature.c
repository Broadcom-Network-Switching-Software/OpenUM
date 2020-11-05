/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "appl/ssp.h"
#include "../content/sspmacro_feature.h"


#if (CFG_UIP_IPV6_ENABLED)
#define IPV6_SUPPORT
#endif  /* CFG_UIP_IPV6_ENABLED */

#if (CFG_PERSISTENCE_SUPPORT_ENABLED)
#define PERSISTENCE_SUPPORT
#endif  /* CFG_PERSISTENCE_SUPPORT_ENABLED */

SSPLOOP_RETVAL 
ssploop_feature_tag_enable(SSPTAG_PARAM *params, SSPLOOP_INDEX index, SSP_PSMH psmem) REENTRANT
{
    UNREFERENCED_PARAMETER(psmem);

    if (index) { 
        /* already shown this feature */
        return SSPLOOP_STOP;
    }

    /*
     * If feature available, set proceed=1.
     * Else remain proceed = 0.
     */
    switch (params[0]) {
        case SSPMACRO_FEATURE_MCAST:
#ifndef CFG_SWITCH_MCAST_INCLUDED
        return SSPLOOP_STOP;
#endif
        break;
        case SSPMACRO_FEATURE_STAT:
#ifndef CFG_SWITCH_STAT_INCLUDED
        return SSPLOOP_STOP; 
#endif
        break;
        
        case SSPMACRO_FEATURE_BONJOUR:
#ifndef CFG_ZEROCONF_MDNS_INCLUDED
            return SSPLOOP_STOP;
#endif  /*  BONJOUR_SUPPORT */
            break;
        case SSPMACRO_FEATURE_PERSISTENCE:
#ifndef PERSISTENCE_SUPPORT
            return SSPLOOP_STOP;
#endif  /* PERSISTENCE_SUPPORT */
            break;

        case SSPMACRO_FEATURE_IPV6:
#ifndef IPV6_SUPPORT
            return SSPLOOP_STOP;
#endif  /* IPV6_SUPPORT */
            break;

        case SSPMACRO_FEATURE_PVLAN:
#if !defined(CFG_SWITCH_PVLAN_INCLUDED)
            return SSPLOOP_STOP;
#endif  /* defined(CFG_SWITCH_PVLAN_INCLUDED) */
            break;

        case SSPMACRO_FEATURE_TRUNK:
#ifndef CFG_SWITCH_LAG_INCLUDED
            return SSPLOOP_STOP;
#endif /* CFG_SWITCH_LAG_INCLUDED */
            break;

        case SSPMACRO_FEATURE_CABLE:
#ifndef CFG_HW_CABLE_DIAG_INCLUDED
            return SSPLOOP_STOP;
#endif /* CFG_HW_CABLE_DIAG_INCLUDED */
            if (board_cable_diag_port_count() == 0) {
                return SSPLOOP_STOP;
            }
            break;

        case SSPMACRO_FEATURE_LEDSHIFT:
            if (board_uport_count() <= BOARD_MAX_NUM_OF_PORTS_FOR_LED_DISPLAY) {
                return SSPLOOP_STOP;
            }
            break;

        case SSPMACRO_FEATURE_MIRROR:
#ifndef CFG_SWITCH_MIRROR_INCLUDED
            return SSPLOOP_STOP;
#endif /* CFG_SWITCH_MIRROR_INCLUDED */
            break;
        
        case SSPMACRO_FEATURE_LOOPDET:
#ifndef CFG_SWITCH_LOOPDETECT_INCLUDED
            return SSPLOOP_STOP;
#endif /* CFG_SWITCH_LOOPDETECT_INCLUDED */
            break;

        case SSPMACRO_FEATURE_RATE:
#ifndef CFG_SWITCH_RATE_INCLUDED
            return SSPLOOP_STOP;
#endif /* CFG_SWITCH_RATE_INCLUDED */
            break;

        case SSPMACRO_FEATURE_QOS:
#ifndef CFG_SWITCH_QOS_INCLUDED
            return SSPLOOP_STOP;
#endif /* CFG_SWITCH_QOS_INCLUDED */
            break;

        case SSPMACRO_FEATURE_UPLOADVC:
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
            if (board_upload_vc == 0) {
                return SSPLOOP_STOP;
            }
#else
            return SSPLOOP_STOP;
#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */
            break;

        default:
            return SSPLOOP_STOP;
    }

    return SSPLOOP_PROCEED;
}


