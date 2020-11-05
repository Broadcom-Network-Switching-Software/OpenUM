/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "appl/ssp.h"
#include "appl/httpd.h"
#include "utils/net.h"
#include "appl/persistence.h"
#include "boardapi/port.h"
#include "../src/appl/web/content/sspmacro_cable.h"


SSP_HANDLER_RETVAL
ssphandler_cable_cgi(SSP_HANDLER_CONTEXT *cxt, SSP_PSMH psmem);

typedef enum {
  CD_STATE_IDLE = 0,
  CD_STATE_FIRST_REPONSE, 
  CD_STATE_START, 
  CD_STATE_WAIT_LNKUP_1SEC, 
  CD_STATE_WAIT_LNKUP_2SEC, 
  CD_STATE_WAIT_LNKUP_3SEC, 
  CD_STATE_WAIT_LNKUP_4SEC, 
} CD_STATE;

typedef struct cable_cookie {
    uint16 uport;
    int done;
    CD_STATE phase;
    port_cable_diag_t cd;
}cable_cookie_t;

/* When calling board_port_cable_diag(), PHY may link down and up. */
/* Add this variable to prevent from the cycle :
   board_port_cable_diag() -> link down/up -> (TCP) packet re-transmit -> board_port_cable_diag() .... */
static cable_cookie_t cd;


void
sspvar_cable_tag_ssi(SSPTAG_PARAM *params, SSPVAR_RET *ret, SSP_PSMH psmem)
{
    int cur_uport, user;
    switch (params[0]) {
      case SSPMACRO_CABLE_PHASE:
           ret->type =   SSPVAR_RET_STRING;          
           sal_sprintf(ssputil_shared_buffer, "%d", cd.phase);
           ret->val_data.string = ssputil_shared_buffer;
      break;
      case SSPMACRO_CABLE_PORTSEL:
          user = SAL_ZUPORT_TO_UPORT(params[1]);
          ret->type = SSPVAR_RET_STRING;
          cur_uport = cd.uport;
          if (cur_uport == user) {
              sal_strcpy(ssputil_shared_buffer, "selected");
          } else {
              sal_strcpy(ssputil_shared_buffer, "");
          }
          ret->val_data.string = ssputil_shared_buffer;
          break;

      case SSPMACRO_CABLE_CUR_PORT:
          cur_uport = cd.uport;
          sal_sprintf(ssputil_shared_buffer, "%02u", cur_uport);
          ret->type = SSPVAR_RET_STRING;
          if (cur_uport == 0)
              ret->val_data.string = "--";
          else
              ret->val_data.string = ssputil_shared_buffer;
          break;

      case SSPMACRO_CABLE_STATUS:
          if (cd.cd.state == PORT_CABLE_STATE_OK) {
                 sal_sprintf(ssputil_shared_buffer, "OK");
          } else if (cd.cd.state == PORT_CABLE_STATE_NO_CABLE) {
                 sal_sprintf(ssputil_shared_buffer, "No Cable");
          } else if (cd.cd.state == PORT_CABLE_STATE_OPEN) {
                 sal_sprintf(ssputil_shared_buffer, "Open Cable (length = %d meters)", cd.cd.length);
          } else if (cd.cd.state == PORT_CABLE_STATE_SHORT) {
                 sal_sprintf(ssputil_shared_buffer, "Short");
          } else if (cd.cd.state == PORT_CABLE_STATE_CROSSTALK) {
                 sal_sprintf(ssputil_shared_buffer, "Cross Talk");
          }
          ret->type = SSPVAR_RET_STRING;
          ret->val_data.string = ssputil_shared_buffer;
          break;
    }
}

void
sspvar_cable_tag_show(SSPTAG_PARAM *params, SSPVAR_RET *ret, SSP_PSMH psmem)
{
    int i = 0, cnt = 0;
    BOOL support = FALSE;
    uint16 uport;

    ret->type = SSPVAR_RET_INTEGER;

    i = params[0];
    
    SAL_UPORT_ITER(uport) {

        board_get_cable_diag_support_by_port(uport, &support);

        if (support) {
            if (cnt == i) {
                ret->val_data.integer = SAL_UPORT_TO_NZUPORT(uport); 

                return;
            }
            cnt++;
        }
    }

}

SSPLOOP_RETVAL
ssploop_cable_tag_ports(SSPTAG_PARAM *params, SSPLOOP_INDEX index, SSP_PSMH psmem) REENTRANT
{
    BOOL support = FALSE;
    int32 cnt = 0;
    uint16 uport;

    /* show ports in single column */
    SAL_UPORT_ITER(uport) {

        board_get_cable_diag_support_by_port(uport, &support);
        if (!support) {
            continue;
        }
        cnt++;
    }
    if (index < cnt){
        return SSPLOOP_PROCEED;
    }

    return SSPLOOP_STOP;
}


SSPLOOP_RETVAL
ssploop_cable_tag_cd_done(SSPTAG_PARAM *params, SSPLOOP_INDEX index, SSP_PSMH psmem)
{

    if (cd.done == 1 && index == 0) {
            return SSPLOOP_PROCEED;
    } else {
        return SSPLOOP_STOP;
    }
}
#ifdef HTTPD_TIMER_SUPPORT
STATICCBK void
cable_dialog_timer(SSP_PSMH psmem) REENTRANT
{

    switch (cd.phase) {
        case CD_STATE_FIRST_REPONSE:
        break; 
        case CD_STATE_START:
            /* turn off net linkchange handler to keep IP is invariant after cable dialog*/ 
            net_enable_linkchange(FALSE);
            board_port_cable_diag(SAL_NZUPORT_TO_UPORT(cd.uport), &cd.cd);
        break;
        case CD_STATE_WAIT_LNKUP_1SEC:     
        case CD_STATE_WAIT_LNKUP_2SEC:
        case CD_STATE_WAIT_LNKUP_3SEC:
        case CD_STATE_WAIT_LNKUP_4SEC:            
        break;
        default:
            /* turn on net linkchange handler after port link up to keep IP is invariant after cable dialog */ 
            net_enable_linkchange(TRUE);
            cd.phase = CD_STATE_IDLE;
            cd.done = 1;
            httpd_delete_timers_by_callback(cable_dialog_timer);
            return;
    }                
            
    cd.phase++;
}
#endif /* HTTPD_TIMER_SUPPORT */


SSP_HANDLER_RETVAL
ssphandler_cable_cgi(SSP_HANDLER_CONTEXT *cxt, SSP_PSMH psmem)
{

    if (cxt->type == SSP_HANDLER_QUERY_STRINGS) {
        if (sal_atoi(cxt->pairs[0].value) != 0) {
            if (cd.phase == CD_STATE_IDLE) /* there is no cable dialog in progress */ 
            {

              /* trigger cable dialog for another port */
              cd.uport = SAL_NZUPORT_TO_UPORT(sal_atoi(cxt->pairs[0].value));
              cd.phase++;
              cd.done = 0;
              httpd_create_timer(1, cable_dialog_timer, (void *) psmem);
            } 
        }
    }

    return SSP_HANDLER_RET_INTACT;
}

