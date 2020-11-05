
#ifndef _UIP_TIMERS_H_
#define _UIP_TIMERS_H_

#include "system.h"

/*
 * Units of current system time/ticks
 */
typedef tick_t clock_time_t;

/*
 * Ticks per second
 */
#define CLOCK_SECOND        SAL_USEC_TO_TICKS(1000000UL)
#define clock_time()        sal_get_ticks()

/*
 * Timer in ticks
 */
struct timer {
    clock_time_t start;
    clock_time_t interval;
};
#define timer_set(ptimer, interv)       do {                        \
    (ptimer)->start = sal_get_ticks();                              \
    (ptimer)->interval = (interv);                                  \
} while(0)
#define timer_reset(ptimer)             do {                        \
    (ptimer)->start = sal_get_ticks();                              \
} while(0)
#define timer_expired(ptimer)                                       \
    SAL_TIME_EXPIRED((ptimer)->start, (ptimer)->interval)
#define timer_remaining(ptimer)                                     \
    ((ptimer)->start + (ptimer)->interval - sal_get_ticks())
    
/*
 * Timer in seconds
 */
struct stimer {
    clock_time_t start;
    clock_time_t interval;
};
#define stimer_set(ptimer, interv)      do {                        \
    (ptimer)->start = sal_get_ticks();                              \
    (ptimer)->interval = (interv) * CLOCK_SECOND;                   \
} while(0)
#define stimer_reset(ptimer)            do {                        \
    (ptimer)->start = sal_get_ticks();                              \
} while(0)
#define stimer_expired(ptimer)                                      \
    SAL_TIME_EXPIRED((ptimer)->start, (ptimer)->interval)
#define stimer_remaining(ptimer)                                    \
    (((ptimer)->start + (ptimer)->interval - sal_get_ticks() - 1)   \
        / CLOCK_SECOND + 1)

/*
 * Event timers (some hard mappings required) 
 */
#define etimer                          timer
#define etimer_set                      timer_set
#define etimer_reset                    timer_reset
#define etimer_expired(ptimer)                                      \
    ((ptimer)->interval != 0 &&                                     \
     SAL_TIME_EXPIRED((ptimer)->start, (ptimer)->interval))
#define etimer_stop(ptimer)             do  {                       \
    (ptimer)->interval = 0;                                         \
} while(0)

#endif /* _UIP_TIMERS_H_ */
