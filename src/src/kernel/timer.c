/*
 * $Id: timer.c,v 1.10 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifdef __C51__
#ifdef CODE_USERCLASS
#pragma userclass (code = krntime)
#endif /* CODE_USERCLASS */
#endif /* __C51__ */

#include "system.h"

#if CFG_TIMER_CALLBACK_SUPPORT

struct timer_t {
    tick_t      interval;   /* In ticks */
    tick_t      start;      /* In ticks */
    TIMER_FUNC  cbk;
    void *      arg;
    BOOL        inprogress; /* To avoid reentrance */
};

STATIC struct timer_t timer[CFG_MAX_REGISTERED_TIMERS];
STATIC tick_t previous_tick;

/* Forwards */
APISTATIC void sys_timer_task(void *param) REENTRANT;

extern void sys_timer_init(void) REENTRANT;

APISTATIC void
APIFUNC(sys_timer_task)(void *param) REENTRANT
{
    /* Check and run timers only once per tick */
    if (previous_tick != sal_get_ticks()) {

        uint8 i;
        TIMER_FUNC cbk;
        tick_t curr;
        struct timer_t *pt = &timer[0];

        UNREFERENCED_PARAMETER(param);

        for(i=0; i<CFG_MAX_REGISTERED_TIMERS; i++, pt++) {
            cbk = pt->cbk;
            if (cbk == NULL) {
                break;
            }
            if (!pt->inprogress) {
                curr = sal_get_ticks();

                if (pt->interval == 0 ||
                    curr - pt->start >= pt->interval) {

                    pt->inprogress = TRUE;
                    (*cbk)(pt->arg);

                    /* Double check in case it's removed during execution */
                    if (pt->cbk == cbk) {
                        pt->inprogress = FALSE;
                        pt->start = curr;
                    }
                }
            }
        }

        previous_tick = sal_get_ticks();
    }
}

void
APIFUNC(sys_timer_init)(void) REENTRANT
{
    uint8 i;
    for(i=0; i<CFG_MAX_REGISTERED_TIMERS; i++) {
        timer[i].cbk = NULL;
    }

    /* Get current ticks */
    previous_tick = sal_get_ticks();

    /* Register background process for handling timer callback */
    task_add(sys_timer_task, NULL);
}

BOOL
APIFUNC(timer_add)(TIMER_FUNC func, void *arg, uint32 usec) REENTRANT
{
    uint8 i;
    tick_t curr = sal_get_ticks();

    SAL_DEBUGF(("timer_add: %p\n", func));
    if (func == NULL) {
        return FALSE;
    }

    /* Check if it has been registered */
    for(i=0; i<CFG_MAX_REGISTERED_TIMERS; i++) {
        if (timer[i].cbk != NULL) {

            if (func == timer[i].cbk) {

                /* Already registered */
                timer[i].interval = usec / sal_get_us_per_tick();
                timer[i].start = curr;
                timer[i].arg = arg;

                return TRUE;
            }
        }
    }

    /* Found an empty slot */
    for(i=0; i<CFG_MAX_REGISTERED_TIMERS; i++) {
        if (timer[i].cbk == NULL) {

            timer[i].cbk = func;
            timer[i].interval = usec / sal_get_us_per_tick();
            timer[i].start = curr;
            timer[i].arg = arg;
            timer[i].inprogress = FALSE;

            return TRUE;
        }
    }

    return FALSE;
}
void
APIFUNC(timer_remove)(TIMER_FUNC func) REENTRANT
{
    uint8 i;
    SAL_DEBUGF(("timer_remove: %p\n", func));
    for(i=0; i<CFG_MAX_REGISTERED_TIMERS; i++) {
        if (timer[i].cbk == func) {
            break;
        }
    }

    if (i == CFG_MAX_REGISTERED_TIMERS) {
        return;
    }

    for (; i < CFG_MAX_REGISTERED_TIMERS - 1; i++) {
        timer[i].cbk = timer[i + 1].cbk;
        timer[i].interval = timer[i + 1].interval;
        timer[i].start = timer[i + 1].start;
        timer[i].arg = timer[i + 1].arg;
        timer[i].inprogress = timer[i + 1].inprogress;
    }
    timer[i].cbk = NULL;
}

#endif /* CFG_TIMER_CALLBACK_SUPPORT */

