/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

 #ifdef __C51__
#ifdef CODE_USERCLASS
#pragma userclass (code = krnlink)
#endif /* CODE_USERCLASS */
#endif /* __C51__ */
 
#include "system.h"

#if CFG_LINKCHANGE_CALLBACK_SUPPORT

extern void sys_linkchange_init(void) REENTRANT;

STATIC SYS_LINKCHANGE_FUNC linkchange_handlers[CFG_MAX_REGISTERED_LINKCHANGE];
STATIC void *linkchange_args[CFG_MAX_REGISTERED_LINKCHANGE];
STATIC BOOL linkchange_status[BOARD_MAX_NUM_OF_PORTS + 1];

/* Forwards */
APISTATIC void sys_linkchange_timer(void *arg) REENTRANT;

BOOL 
APIFUNC(sys_register_linkchange)(SYS_LINKCHANGE_FUNC func, void *arg) REENTRANT
{
    uint8 i;
    
    SAL_ASSERT(func != NULL);
    SAL_DEBUGF(("sys_register_linkchange: %p\n", func));
    
    for(i=0; i<CFG_MAX_REGISTERED_LINKCHANGE; i++) {
        if (linkchange_handlers[i] == NULL) {
            linkchange_handlers[i] = func;
            linkchange_args[i] = arg;
            return TRUE;
        }
    }
    
    return FALSE;
}

void 
APIFUNC(sys_unregister_linkchange)(SYS_LINKCHANGE_FUNC func) REENTRANT
{
    uint8 i;
    
    SAL_ASSERT(func != NULL);
    SAL_DEBUGF(("sys_unregister_linkchange: %p\n", func));
    
    for(i=0; i<CFG_MAX_REGISTERED_LINKCHANGE; i++) {
        if (linkchange_handlers[i] == func) {
            linkchange_handlers[i] = NULL;
            linkchange_args[i] = NULL;
            return;
        }
    }
}

APISTATIC void 
APIFUNC(sys_linkchange_timer)(void *arg) REENTRANT
{
    uint16 uport;
    uint8 i;
    BOOL link;
    
    UNREFERENCED_PARAMETER(arg);
    
    SAL_UPORT_ITER(uport) {
        if (board_get_port_link_status(uport, &link) != SYS_OK) {
            SAL_ASSERT(FALSE);
            continue;
        }
        if (linkchange_status[uport] != link) {
            linkchange_status[uport] = link;
            for(i=0; i<CFG_MAX_REGISTERED_LINKCHANGE; i++) {
                SYS_LINKCHANGE_FUNC func = linkchange_handlers[i];
                if (func != NULL) {
                    (*func)(uport, link, linkchange_args[i]);
                }
            }
        } 
    }
}

void 
APIFUNC(sys_linkchange_init)(void) REENTRANT
{
    uint16 i;

    for(i=0; i<CFG_MAX_REGISTERED_LINKCHANGE; i++) {
        linkchange_handlers[i] = NULL;
    }
    
    SAL_UPORT_ITER(i) {
        linkchange_status[i] = FALSE;
    }
    
    timer_add(sys_linkchange_timer, NULL, CFG_LINKCHANGE_CHECK_INTERVAL);
}

#endif /* CFG_LINKCHANGE_CALLBACK_SUPPORT */

