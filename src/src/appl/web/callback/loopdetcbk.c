/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "appl/ssp.h"
#include "appl/persistence.h"

void
sspvar_loopdet_tag_checked(SSPTAG_PARAM *params, SSPVAR_RET *ret, SSP_PSMH psmem)
{
#ifdef CFG_SWITCH_LOOPDETECT_INCLUDED
    if (board_loop_detect_status_get() != 0)
    {
        ret->type = SSPVAR_RET_STRING;
        ret->val_data.string = "checked";
    } else
#endif /* CFG_SWITCH_LOOPDETECT_INCLUDED */
    {
        ret->type = SSPVAR_RET_NULL;
    }
}

SSP_HANDLER_RETVAL
ssphandler_loopdet_cgi(SSP_HANDLER_CONTEXT *cxt, SSP_PSMH psmem)
{
#ifdef CFG_SWITCH_LOOPDETECT_INCLUDED
    if (cxt->type != SSP_HANDLER_QUERY_STRINGS) {
        return SSP_HANDLER_RET_INTACT;
    }

    if (cxt->count == 0)
    {
          board_loop_detect_enable(0);
    } else
    {
        board_loop_detect_enable(1);
    }
#if CFG_PERSISTENCE_SUPPORT_ENABLED
    persistence_save_current_settings("loopdetect");
#endif /* CFG_PERSISTENCE_SUPPORT_ENABLED */
#endif /* CFG_SWITCH_LOOPDETECT_INCLUDED */
    return SSP_HANDLER_RET_INTACT;
}

