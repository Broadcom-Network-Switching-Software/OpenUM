/*
 * $Id: sal_timer.c,v 1.10 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"

#define TIMER0_TICK         1UL  /* unit:ms */

/******************************************************************************
 *
 *   COMPARE configuration
 */
#if (CONFIG_HURRICANE2_EMULATION==1) || (CONFIG_EMULATION==1)
/* Clock is too slow in emulation */
#define TICKS_PER_US    (1)
#else
#define TICKS_PER_US    ((BOARD_CPU_CLOCK)/1000000)
#endif

volatile STATIC uint32 sys_ticks;
volatile STATIC uint32 last_sys_ticks;
volatile STATIC uint32 sys_seconds;

extern uint32 _getticks(void);

#if !CFG_TIMER_USE_INTERRUPT
STATIC uint32 timer_oldcount; /* For keeping track of ticks */
STATIC uint32 timer_remticks;
STATIC uint32 clockspertick;
STATIC uint32 clocksperus;
#endif /* !CFG_TIMER_USE_INTERRUPT */

tick_t
sal_get_cpu_ticks(void) REENTRANT
{
    uint32 (*funcptr)(void) = (uint32 (*)(void))_getticks;
    return (*funcptr)();
}

tick_t
sal_get_ticks(void) REENTRANT
{
    return (tick_t)sys_ticks;
}

uint32
APIFUNC(sal_get_seconds)(void) REENTRANT
{
    uint32 delta;

    delta = sys_ticks - last_sys_ticks;
    last_sys_ticks = sys_ticks - (delta % 1000UL);
    sys_seconds += (delta / 1000UL);

    return sys_seconds;
}

/* sal_get_us_per_tick
 * return number of micro seconds per sys_ticks instead of CPU Timer ticks
 */
uint32
sal_get_us_per_tick(void)
{
    return TIMER0_TICK * 1000UL;
}

#if CFG_TIMER_USE_INTERRUPT
STATICFN void arm_counter_compapre_isr(int ip)
{
    sys_ticks++;
}
#else
extern void sal_timer_task(void *param);
void
sal_timer_task(void *param)
{
    uint32 count, delta;
    uint32 (*funcptr)(void) = (uint32 (*)(void))_getticks;

    count = (*funcptr)();
    delta = count - timer_oldcount;

    timer_remticks += delta;
    
    if (timer_remticks > (clockspertick << 4)) {
        sys_ticks += (timer_remticks / clockspertick);
        timer_remticks %= clockspertick;
        }
    else {
        while (timer_remticks > clockspertick) {
            timer_remticks -= clockspertick;
            sys_ticks++;
        }
    }
    timer_oldcount = count;
}
#endif /* CFG_TIMER_USE_INTERRUPT */

void
sal_timer_init(uint32 clk_hz, BOOL init)
{
#if CFG_TIMER_USE_INTERRUPT
    uint32 val;

    /* Init global variable system tick */
    if (init) {
        sys_ticks = 0;
        last_sys_ticks = 0;
        sys_seconds = 0;
    }

    val = TIMER0_TICK*clk_hz/2000;

    /*
     * Set up the exception vectors
     */
    um_setup_exceptions();

    um_irq_init();

    um_setup_counter_compare_irq(val, mips_counter_compapre_isr);
#else
    uint32 (*funcptr)(void) = (uint32 (*)(void))_getticks;

    if (init) {
        sys_ticks = 0;
        last_sys_ticks = 0;
        sys_seconds = 0;
        timer_remticks = 0;
        timer_oldcount = (*funcptr)();
    }
    /* Unit in 1ms. */
    clockspertick = clk_hz/1000;
    /* Unit in 1us. */
    clocksperus = clockspertick/1000;
#endif /* CFG_TIMER_USE_INTERRUPT */
}


void
sal_usleep(uint32 usec)
{
#if CFG_LONG_US_DEALY_ENABLED
    uint64 ticks, delta;
    tick_t curr, count;
#else
    tick_t curr, ticks;
#endif
    uint32 (*funcptr)(void) = (uint32 (*)(void))_getticks;

#if CFG_TIMER_USE_INTERRUPT  
    if (usec < TIMER0_TICK * 1000UL) {
        ticks = usec*TICKS_PER_US;
        curr = (*funcptr)();
        while((*funcptr)() - curr < ticks);
    } else {
        curr = sys_ticks;
        ticks = SAL_USEC_TO_TICKS(usec);
        while((sal_get_ticks() - curr) < ticks);
    }
#else

#if CFG_LONG_US_DEALY_ENABLED
    curr = (*funcptr)();
    delta = 0;
    ticks = (uint64)usec * TICKS_PER_US;
    while (delta < ticks) {
        count = (*funcptr)();
        delta += (count - curr);
        curr = count;
    }
#else
    ticks = usec*TICKS_PER_US;
    curr = (*funcptr)();
    while((*funcptr)() - curr < ticks);
#endif

#endif
}
#ifndef __BOOTLOADER__
void
sal_sleep(tick_t ticks)
{
    tick_t curr;
    if (ticks == 0) {
        return;
    }
    curr = sal_get_ticks();
    while(!SAL_TIME_EXPIRED(curr, ticks)) {
        POLL();
    }
}
#endif /* !__BOOTLOADER__ */

#if defined(CFG_PCM_SUPPORT_INCLUDED) || defined(CFG_RXTX_SUPPORT_ENABLED)
/*
 * Function:
 *      sal_time_usecs
 * Purpose:
 *      Returns the relative time in microseconds modulo 2^32.
 * Returns:
 *      Time in microseconds modulo 2^32
 * Notes:
 *      The precision is limited to the Unix clock period.
 *      Time is monotonic if supported by the O/S.
 */

uint32 
sal_time_usecs(void)
{
#if CFG_LONG_US_DEALY_ENABLED
    uint64 val64, remticks, us;

    sal_timer_task(NULL);
    us = 0;
    remticks = timer_remticks;
    while (remticks > clocksperus) {
        remticks -= clocksperus;
        us++;
    }
    val64 = (((uint64)sys_ticks * 1000 + us) & 0xFFFFFFFF);
    return (uint32)val64;
#else
    uint32 (*funcptr)(void) = (uint32 (*)(void))_getticks;
    uint32 count;

    count = (*funcptr)();
    count = count / TICKS_PER_US;

    return count;
#endif
}
#endif
