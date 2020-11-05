 /*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"


#if defined(CFG_SWITCH_PVLAN_INCLUDED)
#include "appl/ssp.h"
#include "utilcbk.h"
#include "appl/persistence.h"
#include "appl/igmpsnoop.h"

#include "../content/sspmacro_pvlan.h"
#include "../content/sspmacro_ports.h"
#include "../content/pvlanview_htm.h"
#include "../content/addpvlan_htm.h"
#include "../content/errormsg_htm.h"
#include "../content/vlanlist_htm.h"

#define PVLAN_DEBUG 0

#if PVLAN_DEBUG
#define PVLAN_DBG(x)   do { sal_printf("PVLANCBK: "); \
                       sal_printf x; sal_printf("\n");\
                       } while(0);
#else
#define PVLAN_DBG(x)
#endif



#define MAX_PORT_NUM board_uport_count()

#define NON_MEMBER_ICON     '0'
#define UNTAG_ICON          '1'
#define TAG_ICON            '2'


#define MAX_PVLAN_NUM  8
#define MAX_PVLAN_DESCR_LENGTH  8

static int nums_pewrow = (MAX_PORTS_PER_ROW / 2);

#ifdef PVID
extern int setportcbk_PVID_set(int idx, int vid);
extern int setportcbk_PVID_get(int idx);
#endif

/* Forward declaration */
SSP_HANDLER_RETVAL
ssphandler_addpvlan_cgi(SSP_HANDLER_CONTEXT *cxt, SSP_PSMH psmem);
SSP_HANDLER_RETVAL
ssphandler_delpvlan_cgi(SSP_HANDLER_CONTEXT *cxt, SSP_PSMH psmem);
SSP_HANDLER_RETVAL
ssphandler_showaddpvlan_cgi(SSP_HANDLER_CONTEXT *cxt, SSP_PSMH psmem);


static SSP_HANDLER_RETVAL
show_error(SSP_HANDLER_CONTEXT *cxt, SSP_PSMH psmem, int error) {

    webutil_show_error(
        cxt, psmem,
        "PVLAN",
        "system error information!!",
        err_button_retry,
        err_action_back
        );

    /* We don't want to process it more */
    /* cxt->flags = 0; */
    return SSP_HANDLER_RET_MODIFIED;
}

static uint8
pvlancbk_check_vlan_exist(uint16  vlan_id, uint8 *uplist)
{
    sal_memset(&uplist[0], 0, sizeof(uint8) * MAX_UPLIST_WIDTH);
    if (board_pvlan_port_get(vlan_id, uplist) == SYS_OK) {
        return TRUE;
    } else {
        return FALSE;
    }
}

uint16 pvlancbk_available_vlan_id_get(void)
{
    uint8 uplist[MAX_UPLIST_WIDTH];
    uint8 b_vlan = FALSE;
    uint16 vid = 1, count = 0, vid_count = 0;

    count = board_vlan_count(); /*vlan id base from 1~ 259 maxmum*/

    sal_memset(&uplist[0], 0, sizeof(uint8) * MAX_UPLIST_WIDTH);

    while(vid_count < count ){
        b_vlan = pvlancbk_check_vlan_exist(vid, &uplist[0]);

        if(b_vlan == FALSE){
            return vid;
        }else{
            vid_count++;
        }
         vid++;
    }
    return vid;
}

SSPLOOP_RETVAL
ssploop_pvlan_tag_ports_rows(SSPTAG_PARAM *params, SSPLOOP_INDEX index, SSP_PSMH psmem)
{
    int d_val;
    PVLAN_DBG(("ssploop_pvlan_tag_ports_rows : index = %d  nums_pewrow:%d\n", index, nums_pewrow));
    d_val = board_uport_count() / nums_pewrow;
    if ((board_uport_count() % nums_pewrow) != 0) {
        d_val++;
    }
    if (board_uport_count() <= ( d_val* nums_pewrow)) {
        if (index < d_val) {
            return SSPLOOP_PROCEED;
        }
    }

    return SSPLOOP_STOP;
}

SSPLOOP_RETVAL
ssploop_pvlan_tag_ports_per_row(SSPTAG_PARAM *params, SSPLOOP_INDEX index, SSP_PSMH psmem)
{
    int port;

    port = params[0] * nums_pewrow + index + 1;
    if ((index < nums_pewrow) && (port <= board_uport_count())) {
        return SSPLOOP_PROCEED;
    }
 
    return SSPLOOP_STOP;
}

void sspvar_pvlan_tag_set(SSPTAG_PARAM *params, SSPVAR_RET *ret,
                           SSP_PSMH psmem)
{
    uint8 uplist[MAX_UPLIST_WIDTH];
    uint16 uport = 0;
    int *p, cur_vid, port = 0;
    int d_val;

    p = (int *)ssputil_psmem_get(psmem, ssphandler_showaddpvlan_cgi);
    if (p == NULL)
        cur_vid = 1; /* should NEVER happen */
    else
        cur_vid = *p;

    ret->type = SSPVAR_RET_STRING;
    ret->val_data.string = ssputil_shared_buffer;

    nums_pewrow = (board_uport_count()/2) + (board_uport_count()%2);

    if (nums_pewrow > (MAX_PORTS_PER_ROW/2)) {
        nums_pewrow = (MAX_PORTS_PER_ROW/2);
    }
    switch (params[0]) {
        case SSPMACRO_PVLAN_CUR_VID:
            sprintf(ssputil_shared_buffer, "%02d", cur_vid);
            break;

        case SSPMACRO_PVLAN_CUR_VID_NUM:
            ret->type = SSPVAR_RET_INTEGER;
            ret->val_data.integer = cur_vid;
            break;

        case SSPMACRO_PVLAN_PORT_ID:
            d_val = board_uport_count() / nums_pewrow;
            if ((board_uport_count() % nums_pewrow) != 0) {
                d_val++;
            }
            port = params[1]*nums_pewrow+params[2]+1;
            sprintf(ssputil_shared_buffer, "%02d", port);
            break;

        case SSPMACRO_PVLAN_MEMBER:
            sal_memset(&uplist[0], 0, sizeof(uint8) * MAX_UPLIST_WIDTH);

            if (pvlancbk_check_vlan_exist(cur_vid, &uplist[0])) {
                port = (params[1]*nums_pewrow+params[2])+1;
                uport = port - 1;
                PVLAN_DBG(("port:%d  board_vlan_count:%d\n", port, board_vlan_count()));
                PVLAN_DBG(("port:%d  uplist[%d] = %x %x\n", port, uport/8, uplist[uport/8], (1 << ((uport) % 8))));                
                if((uint8)(uplist[(uport) / 8]  & (1 << ((uport) % 8)) ) ==  (1 << ((uport) % 8))){
                    sprintf(ssputil_shared_buffer, "checked");
                    PVLAN_DBG(("%s\n",ssputil_shared_buffer));                
                    return;
                }
            }
            sprintf(ssputil_shared_buffer, "%s", "");
            PVLAN_DBG(("null %s\n",ssputil_shared_buffer));                
            break;
    }
}

void sspvar_pvlan_tag_info(SSPTAG_PARAM *params, SSPVAR_RET *ret,
                           SSP_PSMH psmem)
{
    int   i;
    uint16 vid_idx = 0, vlan_id;

    uint8 uplist[MAX_UPLIST_WIDTH], tag_uplist[MAX_UPLIST_WIDTH];
    char  buf[MAX_PVLAN_DESCR_LENGTH];

    sal_memset(ssputil_shared_buffer, 0, sizeof(ssputil_shared_buffer));

    vid_idx = params[1];

    for (i = 0; i < board_vlan_count(); i++) {
        board_qvlan_get_by_index(i, &vlan_id, uplist, tag_uplist, FALSE);
        if (i == vid_idx) {
            break;
        }
    }

    switch (params[0]) {
        case SSPMACRO_PVLAN_VID:
            ret->type = SSPVAR_RET_INTEGER;
            ret->val_data.integer = vlan_id;
            break;

        case SSPMACRO_PVLAN_MEMBER:
            ret->type = SSPVAR_RET_STRING;
            ret->val_data.string = ssputil_shared_buffer;

            PVLAN_DBG(("__SSPMACRO_PVLAN_MEMBER params[1] %d: vid:%d\n", params[1], vid));
            sal_memset(&uplist[0], 0, sizeof(uint8) * MAX_UPLIST_WIDTH);

            if (board_pvlan_port_get(vlan_id, &uplist[0]) == SYS_OK) {
                SAL_UPORT_ITER(i) {
                    if(uplist_port_matched(uplist, i) == SYS_OK) {
                         /* member */
                        sprintf(buf, "%02d ", SAL_NZUPORT_TO_UPORT(i));
                        sal_strcat(ssputil_shared_buffer, buf);
                    }
                }
            } else {
                sprintf(ssputil_shared_buffer,  "&nbsp;");
            }
            break;

        default:
            break;
    }
    return;
}

SSP_HANDLER_RETVAL
ssphandler_addpvlan_cgi(SSP_HANDLER_CONTEXT *cxt, SSP_PSMH psmem)
{
    int rv = SYS_OK;
    int i, vid;
    char *members;
    uint8 uport[MAX_UPLIST_WIDTH];
    uint8 *uplist;
    char error_msg[256];
#ifdef CFG_SWITCH_LAG_INCLUDED
    uint16 k;
    uint8 tmp_uplist[MAX_UPLIST_WIDTH], lag_uplist[MAX_UPLIST_WIDTH];
    uint8 enable;
    uint8 lag_group_num;
#endif
    if (cxt->type == SSP_HANDLER_QUERY_STRINGS) {

        vid = sal_atoi(cxt->pairs[0].value);

        if(vid > board_uport_count()){
            sal_sprintf(error_msg,"Vlan Group maximum numbers is %d",
                    board_uport_count());
            webutil_show_error(
                    cxt, psmem,
                    "Added Port-Based Vlan",
                    error_msg,
                    err_button_retry,
                    err_action_back);
            return SSP_HANDLER_RET_MODIFIED;
        }

        members = (char *)cxt->pairs[2].value;

        uplist = &uport[0];
        sal_memset(uplist, 0 , sizeof(uint8) * MAX_UPLIST_WIDTH);

        SAL_UPORT_ITER(i) {
            if (members[SAL_UPORT_TO_ZUPORT(i)] == (char)'1') {
                uplist_port_add(uplist, i);
                PVLAN_DBG(("Added vid#%d 's Port member: %d",vid, i));
            }
        }
        PVLAN_DBG(("__ssphandler_addpvlan_cgi uport %x\n", uplist));
#ifdef CFG_SWITCH_LAG_INCLUDED
        board_lag_group_max_num(&lag_group_num);

        for (k=1; k <= lag_group_num; k++) {
             board_lag_group_get(k,&enable,lag_uplist);
             uplist_manipulate(tmp_uplist, lag_uplist, UPLIST_OP_COPY);
             uplist_manipulate(tmp_uplist, uplist, UPLIST_OP_AND);
             if ((uplist_manipulate(tmp_uplist, lag_uplist, UPLIST_OP_EQU) != SYS_OK) &&
                 (uplist_is_empty(tmp_uplist) != SYS_OK)) {
                    webutil_show_error(
                    cxt, psmem,
                    "VLAN",
                    "VLAN and Trunk page setting conflict\nPorts of a trunk group should not be partially assigned to any PVLAN.",
                    err_button_retry,
                    err_action_back);
                    /* We don't want to process it more */
                    /* cxt->flags = 0; */
                    return SSP_HANDLER_RET_MODIFIED;

             }
             
        }
#endif
        board_vlan_type_set(VT_PORT_BASED);
        board_vlan_create(vid);
        board_pvlan_port_set((uint16) vid,  uplist);
        PVLAN_DBG(("wss_vlanmgr_pvlan_set = %d", rv));
#if CFG_PERSISTENCE_SUPPORT_ENABLED
        persistence_save_current_settings("vlan");
#endif
    }

    if (rv != SYS_OK) {
        return show_error(cxt, psmem, rv);
    } else {
        cxt->page = sspfile_pvlanview_htm;
        return SSP_HANDLER_RET_MODIFIED;
    }
}

SSP_HANDLER_RETVAL
ssphandler_delpvlan_cgi(SSP_HANDLER_CONTEXT *cxt, SSP_PSMH psmem)
{
    sys_error_t rv = SYS_OK;
    int vid;
    char err_msg[64];
#ifdef CFG_SWITCH_MCAST_INCLUDED
    uint16 val16;
    uint8 val8;
#endif
    vid = sal_atoi(cxt->pairs[0].value);
    if (cxt->type == SSP_HANDLER_QUERY_STRINGS) {
        PVLAN_DBG(("Remove vid %d", vid));
#ifdef CFG_SWITCH_MCAST_INCLUDED
           igmpsnoop_enable_get(&val8);
           if (val8) {
               igmpsnoop_vid_get(&val16);
               if(val16 == vid) {
                    sal_sprintf(err_msg, "Please change IGMP snoop VLAN setting before removing this VLAN");

                    webutil_show_error(
                            cxt, psmem,
                            "Remove Vlan",
                            err_msg,
                            err_button_retry,
                            err_action_back);
                    return SSP_HANDLER_RET_MODIFIED;
               }
           }
#endif
        if (board_vlan_count() <= 1) {

            sal_sprintf(err_msg, "There should be at least one VLAN in setting");

            webutil_show_error(
                        cxt, psmem,
                        "Remove Vlan",
                        err_msg,
                        err_button_retry,
                        err_action_back);
            return SSP_HANDLER_RET_MODIFIED;


        }
        rv = board_vlan_destroy((uint16)vid);
#if CFG_PERSISTENCE_SUPPORT_ENABLED
        persistence_save_current_settings("vlan");
#endif
    }

    if (rv != SYS_OK) {
        return show_error(cxt, psmem, rv);
    } else {
        cxt->page = sspfile_pvlanview_htm;
        return SSP_HANDLER_RET_MODIFIED;
    }

}

SSP_HANDLER_RETVAL
ssphandler_showaddpvlan_cgi(SSP_HANDLER_CONTEXT *cxt, SSP_PSMH psmem)
{
    int *p = NULL;
    if (cxt->type == SSP_HANDLER_QUERY_STRINGS) {

        p = (int *)ssputil_psmem_alloc(psmem,
                                       ssphandler_showaddpvlan_cgi,
                                       sizeof(int));

        if (cxt->count == 0) {
            /* Add VLAN clicked */
            /*To get available vlan_group_id */
            *p = pvlancbk_available_vlan_id_get();
        } else {
            /* vid clicked to change members */
            *p = sal_atoi(cxt->pairs[0].value);
        }

        PVLAN_DBG(("showaddpvlan_cgi vid = %d", *p));
        cxt->page = sspfile_addpvlan_htm;
        cxt->flags = 0;
    }
    return SSP_HANDLER_RET_MODIFIED;
}

#endif /* defined(CFG_SWITCH_PVLAN_INCLUDED) */

