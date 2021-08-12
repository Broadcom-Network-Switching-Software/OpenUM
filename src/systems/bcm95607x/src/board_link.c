/*! \file board_link.c
 *
 * Board APIs for linkscan.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include <system.h>
#include <boardapi/link.h>
#include <lm_drv_internal.h>

static board_hw_linkchange_intr_func_f linkchange_handler = NULL;

static void
board_hw_linkchange_intr_entry(uint32 unit)
{
    pbmp_t link_lpbmp;
    uint8 uplist[MAX_UPLIST_WIDTH];

    lm_hw_link_get(unit, &link_lpbmp);

    board_lpbmp_to_uplist(0, link_lpbmp, uplist);

    if (linkchange_handler) {
        linkchange_handler(uplist);
    }
}

sys_error_t
board_hw_linkchange_intr_func_set(board_hw_linkchange_intr_func_f f)
{
    linkchange_handler = f;

    if (f) {
        return lm_hw_intr_cb_set(0, board_hw_linkchange_intr_entry);
    } else {
        return lm_hw_intr_cb_set(0, NULL);
    }
}

sys_error_t
board_hw_linkchange_config(uint8* uplist)
{
    pbmp_t lpbmp;

    /* Convert uplist to logical pbmp. */
    board_uplist_to_lpbmp(uplist, 0, &lpbmp);

    /* Config hardware linkscan. */
    return lm_hw_config(0, lpbmp);
}
