/*
 * $Id: background.c,v 1.6 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifdef __C51__
#ifdef CODE_USERCLASS
#pragma userclass (code = krnbg)
#endif /* CODE_USERCLASS */
#endif /* __C51__ */

#include "system.h"

enum {
  TASK_FLAG_SUSPEND = 0x01
} TASK_FLAG_T;

/*  *********************************************************************
    *  Globals
 */

STATIC BACKGROUND_TASK_FUNC bg_tasklist[CFG_MAX_BACKGROUND_TASKS];
STATIC void *bg_args[CFG_MAX_BACKGROUND_TASKS];
STATIC uint8 bg_task_flag[CFG_MAX_BACKGROUND_TASKS];



/*  *********************************************************************
    *  background_init()
    *
    *  Initialize the background task list
    *
    *  Input parameters:
    *      nothing
    *
    *  Return value:
    *      nothing
 */

void
APIFUNC(background_init)(void) REENTRANT
{
#if CFG_SAL_LIB_SUPPORT_ENABLED
    sal_memset(bg_tasklist, 0, sizeof(bg_tasklist));
#else /* !CFG_SAL_LIB_SUPPORT_ENABLED */
    int i;
    for (i=0; i < CFG_MAX_BACKGROUND_TASKS; i++) {
        bg_tasklist[i] = NULL;
        bg_task_flag[i] = 0;
    }
#endif /* CFG_SAL_LIB_SUPPORT_ENABLED */
}


/*  *********************************************************************
    *  task_add(func,arg)
    *
    *  Add a function to be called periodically in the background
    *  polling loop.
    *
    *  Input parameters:
    *      func - function pointer
    *      arg - arg to pass to function
    *
    *  Return value:
    *      nothing
 */

void
APIFUNC(task_add)(BACKGROUND_TASK_FUNC func, void *arg) REENTRANT
{
    int idx;

    SAL_ASSERT(func != NULL);
    SAL_DEBUGF(("task_add: %p\n", func));
    for (idx = 0; idx < CFG_MAX_BACKGROUND_TASKS; idx++) {
        if (bg_tasklist[idx] == NULL) {
            bg_tasklist[idx] = func;
            bg_args[idx] = arg;
            return;
        }
    }

    /* Too many background tasks */
    sal_assert("Too many tasks!", __FILE__, __LINE__);
}

/*  *********************************************************************
    *  task_suspend(func)
    *
    *  Put task into suspend state
    *
    *
    *      func - function pointer
    *
    *  Return value:
    *      nothing
 */

void
APIFUNC(task_suspend)(BACKGROUND_TASK_FUNC func) REENTRANT
{
    int idx;

    SAL_ASSERT(func != NULL);
    SAL_DEBUGF(("task_suspend: %p\n", func));
    for (idx = 0; idx < CFG_MAX_BACKGROUND_TASKS; idx++) {
        if (bg_tasklist[idx] == func) {
            bg_task_flag[idx] |= TASK_FLAG_SUSPEND;
            return;
        }
    }

}

/*  *********************************************************************
    *  task_resume(func)
    *
    *  To resume task
    *
    *  Input parameters:
    *      func - function pointer
    *
    *  Return value:
    *      nothing
 */

void
APIFUNC(task_resume)(BACKGROUND_TASK_FUNC func) REENTRANT
{
    int idx;

    SAL_ASSERT(func != NULL);
    SAL_DEBUGF(("task_resume: %p\n", func));
    for (idx = 0; idx < CFG_MAX_BACKGROUND_TASKS; idx++) {
        if (bg_tasklist[idx] == func) {
            bg_task_flag[idx] &= (~TASK_FLAG_SUSPEND);
            return;
        }
    }

}


/*  *********************************************************************
    *  task_remove(func)
    *
    *  Remove a function from the background polling loop
    *
    *  Input parameters:
    *      func - function pointer
    *
    *  Return value:
    *      nothing
 */

#ifndef __BOOTLOADER__
void
APIFUNC(task_remove)(BACKGROUND_TASK_FUNC func) REENTRANT
{
    int idx;

    SAL_ASSERT(func);
    SAL_DEBUGF(("task_remove: %p\n", func));
    for (idx = 0; idx < CFG_MAX_BACKGROUND_TASKS; idx++) {
        if (bg_tasklist[idx] == func) {
            break;
        }
    }

    if (idx == CFG_MAX_BACKGROUND_TASKS) {
        return;
    }

    for (; idx < CFG_MAX_BACKGROUND_TASKS - 1; idx++) {
        bg_tasklist[idx] = bg_tasklist[idx + 1];
        bg_args[idx] = bg_args[idx + 1];
    }

    bg_tasklist[idx] = NULL;
}
#endif /* !__BOOTLOADER__ */

/*  *********************************************************************
    *  background()
    *
    *  The main loop and other places that wait for stuff call
    *  this routine to make sure the background handlers get their
    *  time.
    *
    *  Input parameters:
    *      nothing
    *
    *  Return value:
    *      nothing
 */

void
APIFUNC(background)(void) REENTRANT
{
    int idx;
    BACKGROUND_TASK_FUNC func;

    for (idx = 0; idx < CFG_MAX_BACKGROUND_TASKS; idx++) {
        func = bg_tasklist[idx];
        if (func == NULL) {
            break;
        }
        if (bg_task_flag[idx] & TASK_FLAG_SUSPEND) {
            continue;
        }

        /* Do polling irq. */
        POLLED_IRQ();

        (*func)(bg_args[idx]);
    }

    /* Do polling irq before enter CLI. */
    POLLED_IRQ();

}
