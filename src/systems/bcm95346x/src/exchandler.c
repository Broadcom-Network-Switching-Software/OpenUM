/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *  
 * This is the "C" part of the exception handler and the
 * associated setup routines.  We call these routines from
 * the assembly-language exception handler.
 */

#include "system.h"

/*@api
 * um_exception
 *
 * @brief
 * Exception handling
 *
 * @param=type - H/W exception type
 * @param=lr - link register at time of rupt
 *
 * @returns never
 *
 * @desc
 * Exception code:
 *  0 - reset reentry
 *  1 - undef instr
 *  2 - sw exception (SWI)
 *  3 - prefetch abort
 *  4 - data abort
 */
void mos_exception(uint32 type, uint32 lr)
{
    sal_printf("um_exception: type 0x%08x lr 0x%08x\n", type, lr);
    sal_usleep(1000000); // wait 1 sec for realy print out to console 
    board_reset(0);
}

/* Dummy entries for linker */
void mos_fiq_handler(void) { }
#if 0
void mos_irq_handler(void) { }
void mos_rupt_yield(void) { }
void mos_lock_push_irq(void) { }
void mos_lock_pop_irq(void) { }
#endif