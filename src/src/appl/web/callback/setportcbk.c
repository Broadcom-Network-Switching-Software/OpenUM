/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#include "utils/ports.h"
#include "appl/ssp.h"
#include "../content/sspmacro_ports.h"
#include "../content/sspmacro_vlan.h"

#define PORT_DEBUG 1

#if PORT_DEBUG
#define PORT_DBG(x)    do { sal_printf("PORT-CBK: "); sal_printf x; sal_printf("\n"); } while(0);
#else
#define PORT_DBG(x)
#endif

SSP_HANDLER_RETVAL ssphandler_setport_cgi(SSP_HANDLER_CONTEXT *cxt, SSP_PSMH psmem) REENTRANT;

#define HALF_10 1
#define FULL_10 2
#define HALF_100 3
#define FULL_100 4
#define FULL_1000 5

void
sspvar_ports_tag_config(SSPTAG_PARAM *params, SSPVAR_RET *ret, SSP_PSMH psmem) REENTRANT
{
    int *p;
    uint16  uport;
    port_mode_t mode;
    int speed, duplex;
    uint16 pvid = 1;

    p = (int *)ssputil_psmem_get(psmem, ssphandler_setport_cgi);

    uport = SAL_NZUPORT_TO_UPORT(*p);

    if (params[0] == SSPMACRO_PORTS_TYPE) {
        ret->type = SSPVAR_RET_INTEGER;
        /* force at static value, need new board API for this */
        ret->val_data.integer = 1;   /* GE port */
    } else if (params[0] == SSPMACRO_PORTS_NUM) {
        if(SAL_UPORT_TO_NZUPORT(uport) < 10) {
            sal_sprintf(ssputil_shared_buffer, "0%d", SAL_UPORT_TO_NZUPORT(uport));
        } else {
            sal_sprintf(ssputil_shared_buffer, "%d", SAL_UPORT_TO_NZUPORT(uport));
        }

        ret->type = SSPVAR_RET_STRING;
        ret->val_data.string = ssputil_shared_buffer;
    } else if (params[0] == SSPMACRO_PORTS_ADMIN) {
        ret->type = SSPVAR_RET_INTEGER;
        /* force at static value, need new board API for this */
        ret->val_data.integer = 1;   /* enabled */
    } else if (params[0] == SSPMACRO_PORTS_AUTONEGO) {
        ret->type = SSPVAR_RET_INTEGER;
        /* force at static value, need new board API for this */
        ret->val_data.integer = 1;  /* AN */
    } else if (params[0] == SSPMACRO_PORTS_SPEED) {
        mode = PM_LINKDOWN;
        board_port_mode_get(uport, &mode);
        if(mode != PM_LINKDOWN) {
            speed = (mode <= 3) ? 10 :
                    (mode <= 5) ? 100 : 1000;
            duplex = (mode == 2 || mode == 4) ? 0 : 1; /* 0:half; 1:full */
        } else {
             speed = 10;
            duplex = 0; /* half */
        }
        ret->type = SSPVAR_RET_INTEGER;
        if (speed == 10) {
            if (duplex) {
                ret->val_data.integer = FULL_10;
            } else {
                ret->val_data.integer = HALF_10;
            }
        } else if (speed == 100) {
            if (duplex) {
                ret->val_data.integer = FULL_100;
            } else {
                ret->val_data.integer = HALF_100;
            }
        } else if (speed == 1000) {
            ret->val_data.integer = FULL_1000;
        } else {
            ret->val_data.integer = HALF_10;
        }
    } else if (params[0] == SSPMACRO_PORTS_PVID) {
#if defined(CFG_SWITCH_VLAN_INCLUDED)
        board_untagged_vlan_get(uport, &pvid);
#endif
        ret->type = SSPVAR_RET_INTEGER;
        ret->val_data.integer = pvid;
    }

    return;
}

SSP_HANDLER_RETVAL
ssphandler_portset_cgi(SSP_HANDLER_CONTEXT *cxt, SSP_PSMH psmem) REENTRANT
{
    uint16 uport;
    uint16 pvid;

    UNREFERENCED_PARAMETER(psmem);


    if (cxt->type == SSP_HANDLER_QUERY_STRINGS) {
        uport = SAL_NZUPORT_TO_UPORT(sal_atoi(cxt->pairs[0].value));

        /* currently there is only PVID is configurable through board API */
        if (cxt->count == 4) {
            pvid = sal_atoi(cxt->pairs[3].value);
#if defined(CFG_SWITCH_VLAN_INCLUDED)
            board_untagged_vlan_set(uport, pvid);
#endif
            PORT_DBG(("port#%d pvid:%d\n", uport, pvid));
        }
            PORT_DBG(("port#%d pvid:%d\n", uport, sal_atoi(cxt->pairs[3].value)));

    }

    return SSP_HANDLER_RET_INTACT;
}



