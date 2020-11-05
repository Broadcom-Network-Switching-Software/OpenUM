/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "appl/ssp.h"
#include "utilcbk.h"
#include "appl/persistence.h"

#include "../content/sspmacro_mirror.h"
#include "../content/sspmacro_ports.h"
#include "../content/sspmacro_pvlan.h"
#include "../content/mirror_htm.h"
#include "../content/errormsg_htm.h"

#define MIRROR_DEBUG 0

#if MIRROR_DEBUG
#define MIRROR_DBG(x)    do { sal_printf("MIRROR: "); \
                       sal_printf x; sal_printf("\n");\
                       } while(0);
#else
#define MIRROR_DBG(x)
#endif

#ifdef CFG_SWITCH_MIRROR_INCLUDED
#define MAX_PORT_NUM  board_uport_count()
#define NUM_PORTS_PER_SYSTEM MAX_PORT_NUM

static SSP_HANDLER_RETVAL
show_error(SSP_HANDLER_CONTEXT *cxt, SSP_PSMH psmem, char *error_msg) {

    webutil_show_error(
        cxt, psmem,
        "MIRROR",
        error_msg,
        err_button_retry,
        err_action_back
        );

    /* We don't want to process it more */
    /* cxt->flags = 0; */
    return SSP_HANDLER_RET_MODIFIED;
}
#endif /* CFG_SWITCH_MIRROR_INCLUDED */


void
sspvar_mirror_tag_ssi(SSPTAG_PARAM *params, SSPVAR_RET *ret, SSP_PSMH psmem)
{
#ifdef CFG_SWITCH_MIRROR_INCLUDED
    uint8 enable = FALSE;
    uint16 tmp_uport, uport; /*uport: it stared from 0 to 23 number of port*/
    char *pbuf = NULL;

    ret->type = SSPVAR_RET_STRING;

    uport = SAL_ZUPORT_TO_UPORT(params[1]);

    switch (params[0]) {
      case SSPMACRO_MIRROR_VALUES:
          pbuf = ssputil_shared_buffer;
           /*deafault value*/

          SAL_UPORT_ITER(tmp_uport) {
              board_mirror_port_get(tmp_uport, &enable);
              *pbuf = enable + 48;
              pbuf++;

              *pbuf  = ',';
              pbuf++;
          }
          *(--pbuf)='\0';

          MIRROR_DBG(("1SSPMACRO_MIRROR_VALUES: pbuf=%s", ssputil_shared_buffer));
          ret->val_data.string = ssputil_shared_buffer;
          break;


      case SSPMACRO_MIRROR_XPBM_EN: /*multi mirror ports*/
          board_mirror_port_get(uport, &enable);

          if ((TRUE == enable) && (!SAL_UPORT_IS_NOT_VALID(uport))){
              sal_strcpy(ssputil_shared_buffer, "checked");
          }else {
              sal_strcpy(ssputil_shared_buffer, "");
          }
          ret->val_data.string = ssputil_shared_buffer;

          break;


      case SSPMACRO_MIRROR_MTP_EN:
          if (board_mirror_to_get(&tmp_uport) != SYS_OK) {
              MIRROR_DBG(("bcm_mirror_ingress_get failed\n"));
          } else {
              if(tmp_uport == uport){
                  enable = TRUE;
              }
          }
          if (TRUE == enable) {
              sal_strcpy(ssputil_shared_buffer, "checked");
          } else {
              sal_strcpy(ssputil_shared_buffer, "");
          }
          ret->val_data.string = ssputil_shared_buffer;
          break;

      case SSPMACRO_MIRROR_MODES:
          SAL_UPORT_ITER(tmp_uport) {
              board_mirror_port_get((uint16)tmp_uport, &enable);
              if (TRUE == enable) {
                  MIRROR_DBG(("SSPMACRO_MIRROR_MTP_EN uport:%d\n", tmp_uport));
                  break;
              }
          }

          if (TRUE == enable) {
              sal_strcpy(ssputil_shared_buffer, "checked");
          } else {
              sal_strcpy(ssputil_shared_buffer, "");
          }
          ret->val_data.string = ssputil_shared_buffer;
          break;
    }
#endif /* CFG_SWITCH_MIRROR_INCLUDED */
}

SSP_HANDLER_RETVAL
ssphandler_mirror_cgi(SSP_HANDLER_CONTEXT *cxt, SSP_PSMH psmem)
{
#ifdef CFG_SWITCH_MIRROR_INCLUDED
    sys_error_t rv = SYS_OK;
    uint8 mode = TRUE;
    uint16 mirror_to_uport;
    uint32 i = 0;
    uint8 mirror_uplist[MAX_UPLIST_WIDTH];
    uint8 mirror_to_uplist[MAX_UPLIST_WIDTH]; 
    uint8 temp_uplist[MAX_UPLIST_WIDTH];
    
    uint32_t mirror_upbmp0;  /* port 1 ~ port 32 */
    uint32_t mirror_upbmp1;  /* port 33 ~ port 64 */
    uint32_t mirror_upbmp2;  /* port 65 ~ port 96 */
    uint32_t mirror_upbmp3;  /* port 97 ~ port 128 */
#ifdef CFG_SWITCH_LAG_INCLUDED
    uint8 enable = FALSE;
    uint8 trunk_mirror_uplist[MAX_UPLIST_WIDTH];
    uint8 trunk_mirror_to_uplist[MAX_UPLIST_WIDTH];
#endif /* CFG_SWITCH_LAG_INCLUDED */
    MIRROR_DBG(("ssphandler_mirror_cgi\n"));
    if (cxt->type == SSP_HANDLER_QUERY_STRINGS) {

        MIRROR_DBG(("__ssphandler_mirror_cgi:pairs[0]: %s", cxt->pairs[0].value));
        MIRROR_DBG(("__ssphandler_mirror_cgi:pairs[1]: %s", cxt->pairs[1].value));
        MIRROR_DBG(("__ssphandler_mirror_cgi:paris[2]: %s", cxt->pairs[2].value));
        MIRROR_DBG(("__ssphandler_mirror_cgi:paris[3]: %s", cxt->pairs[3].value));
        MIRROR_DBG(("__ssphandler_mirror_cgi:paris[4]: %s", cxt->pairs[4].value));
        MIRROR_DBG(("__ssphandler_mirror_cgi:paris[5]: %s", cxt->pairs[5].value));

        uplist_clear(mirror_uplist);
        uplist_clear(mirror_to_uplist);
        uplist_clear(temp_uplist);
        mirror_to_uport = SAL_NZUPORT_TO_UPORT(sal_atoi(cxt->pairs[4].value));
        uplist_port_add(mirror_to_uplist, mirror_to_uport);       
        mode = sal_atoi(cxt->pairs[5].value);
        if (mode == TRUE) {
         mirror_upbmp0 = sal_strtoul(cxt->pairs[0].value, NULL, 10);
         mirror_upbmp1 = sal_strtoul(cxt->pairs[1].value, NULL, 10);
         mirror_upbmp2 = sal_strtoul(cxt->pairs[2].value, NULL, 10);
         mirror_upbmp3 = sal_strtoul(cxt->pairs[3].value, NULL, 10);
         for (i=0 ; i< MAX_UPLIST_WIDTH; i++) {
              if (i < 4) {
                  mirror_uplist[i] = (mirror_upbmp0 >> (i * 8)) & 0xFF;
              } else if ((i >= 4) && (i < 8)) {
                  mirror_uplist[i] = (mirror_upbmp1 >> ((i - 4) * 8)) & 0xFF;
              } else if ((i >= 8) && (i < 12)) {
                  mirror_uplist[i] = (mirror_upbmp2 >> ((i - 8) * 8)) & 0xFF;
              } else if ((i >= 12) && (i < 16)) {
                  mirror_uplist[i] = (mirror_upbmp3 >> ((i - 12) * 8)) & 0xFF;
              }
         }
#ifdef CFG_SWITCH_LAG_INCLUDED
            uplist_clear(trunk_mirror_to_uplist);
            uplist_clear(trunk_mirror_uplist);

            /*check if mirror-to port belongs to a port trunk*/
            for (i=0; i < BOARD_MAX_NUM_OF_LAG ; i++) {
                    rv = board_lag_group_get((uint8)i + 1, &enable, &temp_uplist[0]);                  
                    if (rv == SYS_OK && (enable == TRUE)) {
                        uplist_manipulate(temp_uplist, mirror_to_uplist, UPLIST_OP_AND);                        
                        if(uplist_manipulate(temp_uplist, mirror_to_uplist, UPLIST_OP_EQU) == SYS_OK) {
                            /* add all trunk member ports into mirror-to-port bit map */
                            board_lag_group_get((uint8)i + 1, &enable, &trunk_mirror_to_uplist[0]);                  
                            break; // trunk check finished
                        }
                    }
            }

            /*check if any mirror port belongs to a port trunk */
            for (i=0; i < BOARD_MAX_NUM_OF_LAG ; i++) {
                    rv = board_lag_group_get((uint8)i + 1, &enable, temp_uplist);
                    if (rv == SYS_OK && (enable == TRUE)) {
                        uplist_manipulate(temp_uplist, mirror_uplist, UPLIST_OP_AND);       
                        if(uplist_is_empty(temp_uplist) != SYS_OK) {
                           board_lag_group_get((uint8)i + 1, &enable, temp_uplist);
                           uplist_manipulate(trunk_mirror_uplist, temp_uplist, UPLIST_OP_OR);                           
                        }
                    }
            }


            /* check if trunk mirror port  != trunk mirror-to port */
            uplist_manipulate(temp_uplist, trunk_mirror_uplist, UPLIST_OP_COPY); 
            uplist_manipulate(temp_uplist, trunk_mirror_to_uplist, UPLIST_OP_AND);
            if (uplist_is_empty(temp_uplist) != SYS_OK) {
                    return show_error(cxt, psmem, "Please check Trunk page setting. <BR></BR>The mirror ports and mirror-to port can not belong to the same trunk group.");
            }


            /* check if trunk mirror port  != trunk mirror-to port */
            uplist_manipulate(temp_uplist, mirror_uplist, UPLIST_OP_COPY); 
            uplist_manipulate(temp_uplist, trunk_mirror_to_uplist, UPLIST_OP_AND);
            if (uplist_is_empty(temp_uplist) != SYS_OK) {
                return show_error(cxt, psmem, "Please check Trunk page setting. <BR></BR>The mirror ports and mirror-to port can not belong to the same trunk group.");
            }

            /* check if trunk mirror port  != trunk mirror-to port */
            uplist_manipulate(temp_uplist, trunk_mirror_to_uplist, UPLIST_OP_COPY); 
            uplist_manipulate(temp_uplist, mirror_uplist, UPLIST_OP_AND);
            if (uplist_is_empty(temp_uplist) != SYS_OK) {
                return show_error(cxt, psmem, "Please check Trunk page setting. <BR></BR>The mirror ports and mirror-to port can not belong to the same trunk group.");
            }
#endif /* CFG_SWITCH_LAG_INCLUDED */
            /* check if mirror port  != mirror-to port */
            uplist_manipulate(temp_uplist, mirror_uplist, UPLIST_OP_COPY); 
            uplist_manipulate(temp_uplist, mirror_to_uplist, UPLIST_OP_AND);
            if (uplist_is_empty(temp_uplist) != SYS_OK) {
                return show_error(cxt, psmem, "One of mirror ports is the same with mirror-to port\n");
            }
            SAL_UPORT_ITER(i) {
                if (uplist_port_matched(mirror_uplist, i) == SYS_OK) {
                    rv = board_mirror_port_set(i, TRUE);
                    MIRROR_DBG(("__ssphandler_mirror_cgi:mirror port: %d", i));
                } else {
                    rv = board_mirror_port_set(i, FALSE);
                }
            }

            rv = board_mirror_to_set(mirror_to_uport);

        }else{
            SAL_UPORT_ITER(i) {
                rv = board_mirror_port_set(i, mode);
            }
        }
#if CFG_PERSISTENCE_SUPPORT_ENABLED
        if(SYS_OK == rv){
            persistence_save_current_settings("mirror");
        }
#endif /* CFG_PERSISTENCE_SUPPORT_ENABLED */
    }

    if (rv != SYS_OK) {
        return show_error(cxt, psmem, "error occuring as apply setting");
    } else {
        return SSP_HANDLER_RET_INTACT;
    }
#else
    return SSP_HANDLER_RET_INTACT;
#endif /* CFG_SWITCH_MIRROR_INCLUDED */
}

