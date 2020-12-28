/*! \file exchandler.c
 *
 * Exception handler C routine
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"

#ifndef __BOOTLOADER__

#include "exchandler.h"

#define CFG_EXC_DEBUG

/*!
 * Exception handler - print out exception info if CFG_EXC_DEBUG defined.
 */
void exc_handler(uint32 exc_dump_base)
{
#ifdef CFG_EXC_DEBUG
    exc_core_dump_t *base;
    int i;

    base = (exc_core_dump_t *)exc_dump_base;
    if (base->exc_header != EXC_HEADER) {
        sal_printf("um_exception: header 0x%08x does NOT match 0x%08x\n", base->exc_header, EXC_HEADER);
        while (1) { };
    }

    sal_printf("um_exception: type = 0x%08x, pc = 0x%08x\n", base->exc_type, base->r_pc);

    switch (base->exc_type) {
        case EXC_RESET:
            sal_printf("Reset exception\n");
            break;
        case EXC_UNDEF_INSTR:
            sal_printf("Undefined instruction exception\n");
            break;
        case EXC_SW_INTR:
            sal_printf("Software interrupt exception\n");
            break;
        case EXC_PREFETCH_ABORT:
            sal_printf("Prefetch abort exception\n");
            break;
        case EXC_DATA_ABORT:
            sal_printf("Data abort exception\n");
            break;
        case EXC_UNHANDLED_IRQ:
            sal_printf("Unhandled IRQ\n");
            break;
        default:
            sal_printf("Unsupported exception\n");
            break;
    };

    for (i=0 ; i <= 12 ; i++)
        sal_printf("um_exception: register[%d] = 0x%08x \n", i, base->regs[i]);

    sal_printf("um_exception: register_sp = 0x%08x \n", base->r_sp);
    sal_printf("um_exception: register_lr = 0x%08x \n", base->r_lr);
    sal_printf("um_exception: register_pc = 0x%08x \n", base->r_pc);
    sal_printf("um_exception: CPSR = 0x%08x \n", base->cpsr);
    sal_printf("um_exception: DFSR = 0x%08x \n", base->dfsr);
    sal_printf("um_exception: DFAR = 0x%08x \n", base->dfar);
    sal_printf("um_exception: IFSR = 0x%08x \n", base->ifsr);
    sal_printf("um_exception: IFAR = 0x%08x \n", base->ifar);
    sal_printf("um_exception: ADFSR = 0x%08x \n", base->adfsr);
    sal_printf("um_exception: AIFSR = 0x%08x \n", base->aifsr);
#endif /* CFG_EXC_DEBUG */

    while (1) { };
}

#else

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
 *  6 - unhanlded IRQ
 */

void mos_exception(uint32 type, uint32 lr)
{
    sal_printf("um_exception: type 0x%08x lr 0x%08x\n", type, lr);
    while (1) {
        sal_usleep(1000000);
    }
}

#endif /* __BOOTLOADER__ */

/* Dummy entries for linker */
void mos_fiq_handler(void) { }
#if 0
void mos_irq_handler(void) { }
void mos_rupt_yield(void) { }
void mos_lock_push_irq(void) { }
void mos_lock_pop_irq(void) { }
#endif
