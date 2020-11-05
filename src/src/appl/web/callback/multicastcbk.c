/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "brdimpl/vlan.h"

#include "appl/ssp.h"
#include "utilcbk.h"
#include "appl/persistence.h"
#include "appl/igmpsnoop.h"

#include "../content/sspmacro_multicast.h"
#include "../content/sspmacro_pvlan.h"
#include "../content/multicast_htm.h"
#include "../content/errormsg_htm.h"


#define MULTICAST_DEBUG 0

#if MULTICAST_DEBUG
#define MULTICAST_DBG(x)    do { sal_printf("MULTICAST: "); \
                       sal_printf x; sal_printf("\n");\
                       } while(0);
#else
#define MULTICAST_DBG(x)
#endif




SSPLOOP_RETVAL
ssploop_multicast_tag_vlan(SSPTAG_PARAM *params, SSPLOOP_INDEX index, SSP_PSMH psmem)
{
#ifdef CFG_SWITCH_VLAN_INCLUDED
    if (index < board_vlan_count()) {
        return SSPLOOP_PROCEED;
    }
#endif
    return SSPLOOP_STOP;

}

#if defined(CFG_SWITCH_MCAST_INCLUDED)

static SSP_HANDLER_RETVAL
show_error(SSP_HANDLER_CONTEXT *cxt, SSP_PSMH psmem, int error) {

    webutil_show_error(
        cxt, psmem,
        "MULTICAST",
        "System error information!!",
        err_button_retry,
        err_action_back
        );

    /* We don't want to process it more */
    /* cxt->flags = 0; */
    return SSP_HANDLER_RET_MODIFIED;
}


void
sspvar_multicast_tag_ssi(SSPTAG_PARAM *params, SSPVAR_RET *ret, SSP_PSMH psmem)
{
    uint8 enable = FALSE;
#ifdef CFG_SWITCH_VLAN_INCLUDED
    vlan_type_t vlan_type;
    uint16 vid;
#endif
    ret->type = SSPVAR_RET_STRING;

    sal_strcpy(ssputil_shared_buffer, "");

    switch (params[0]) {
      case SSPMACRO_MULTICAST_IGMPSPMD:

          igmpsnoop_enable_get(&enable);

          if (enable) {
              sal_strcpy(ssputil_shared_buffer, "checked");
          }
          break;
      case SSPMACRO_MULTICAST_VLAN_ID_ENABLE:
#ifdef CFG_SWITCH_VLAN_INCLUDED
          if (board_vlan_type_get(&vlan_type) != SYS_OK) {
              sal_sprintf(ssputil_shared_buffer, "%d", 0);
          } else {
              sal_sprintf(ssputil_shared_buffer, "%d", 1);
          }
#else
          sal_sprintf(ssputil_shared_buffer, "%d", 0);
#endif
          break;
#ifdef CFG_SWITCH_VLAN_INCLUDED
      case SSPMACRO_MULTICAST_VLAN_ID_SELECT:
          igmpsnoop_enable_get(&enable);
          if (enable) {
              igmpsnoop_vid_get(&vid);
          } else {
              vid = igmpsnoop_get_vid_by_index(0);
          }

           sal_sprintf(ssputil_shared_buffer, "%d", vid);
      break;
      case SSPMACRO_MULTICAST_VLAN_RANGE:
           if (board_vlan_type_get(&vlan_type) != SYS_OK) {
               break;
           }
           if (vlan_type == VT_PORT_BASED) {
               sal_sprintf(ssputil_shared_buffer, " (1-%d)", board_uport_count());
           } else if (vlan_type == VT_DOT1Q) {
               sal_sprintf(ssputil_shared_buffer, " (1-4094)");
           }
      break;
#endif
      case SSPMACRO_MULTICAST_UNKNOWMT:

          board_block_unknown_mcast_get(&enable);
          if (enable) {
              sal_strcpy(ssputil_shared_buffer, "checked");
          }
          break;
      default:
          break;
    }



    ret->val_data.string = ssputil_shared_buffer;
}


SSP_HANDLER_RETVAL
ssphandler_multicast_cgi(SSP_HANDLER_CONTEXT *cxt, SSP_PSMH psmem)
{
    sys_error_t rv = SYS_OK;

    uint8 igmp = TRUE;

    uint8 unknowmlti = TRUE;

    MULTICAST_DBG(("__ssphandler_multicast_cgi\n"));

    if (cxt->type == SSP_HANDLER_QUERY_STRINGS) {

        MULTICAST_DBG(("__ssphandler_multicast_cgi:pairs[0]: %s", cxt->pairs[0].value));
        MULTICAST_DBG(("__ssphandler_multicast_cgi:pairs[1]: %s", cxt->pairs[1].value));
        MULTICAST_DBG(("__ssphandler_multicast_cgi:pairs[2]: %s", cxt->pairs[2].value));
        igmp = sal_atoi(cxt->pairs[0].value);
        rv = igmpsnoop_enable_set(igmp);
#ifdef CFG_SWITCH_VLAN_INCLUDED
        if (igmp) {
            rv = igmpsnoop_vid_set(sal_atoi(cxt->pairs[2].value));
            if (rv == SYS_ERR_NOT_FOUND) {
                webutil_show_error(
                    cxt, psmem,
                    "MULTICAST",
                    "IGMP VLAN ID setting fail. <br></br> The VLAN ID is not exist. <br></br>",
                    err_button_retry,
                    err_action_back
                );

                /* We don't want to process it more */
                /* cxt->flags = 0; */
                return SSP_HANDLER_RET_MODIFIED;

            } else if (rv != SYS_OK) {
                webutil_show_error(
                    cxt, psmem,
                    "MULTICAST",
                    "IGMP VLAN ID setting fail. <br></br> Please check VLAN setting. <br></br>",
                    err_button_retry,
                    err_action_back
                );

                /* We don't want to process it more */
                /* cxt->flags = 0; */
                return SSP_HANDLER_RET_MODIFIED;

            }
        }
#endif

#if CFG_PERSISTENCE_SUPPORT_ENABLED
        rv = persistence_save_current_settings("igmpsnoop");
#endif
        unknowmlti = sal_atoi(cxt->pairs[1].value);
        rv = board_block_unknown_mcast_set(unknowmlti);
#if CFG_PERSISTENCE_SUPPORT_ENABLED
        rv = persistence_save_current_settings("mcast");
#endif
    }

    if (rv < 0)
        return show_error(cxt, psmem, rv);
    else
        return SSP_HANDLER_RET_INTACT;
}
#endif /* defined(CFG_SWITCH_MCAST_INCLUDED) */


