/*
 * $Id: intr_entry.S,v 1.6 Broadcom SDK $
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * File:    intr_entry.S
 * Purpose: interrupt (IRQ) entry point.
 */
#include <config.h>

    .section ".text","ax"
    .global intr_entry
    .code 32

#ifdef CFG_INTR_INCLUDED
intr_entry:

    /* Init stack pointer for interrupt context */
    ldr     sp, intr_stack_ref

    /* Save cpu registers */
    push    {r0-r12,lr}

    /* Jump and link to C interrupt entry */
    ldr      lr, intr_return_ref
    ldr      pc, sys_intr_entry_ref

intr_return:

    /* Restore cpu register */
    pop     {r0-r12,lr}

    /* Restore PC (from R14_irq) and CPSR (from SPSR_irq) */
    subs    pc,lr,#4

intr_stack_ref:
    .word IRQStack

sys_intr_entry_ref:
    .word sys_intr_entry

intr_return_ref:
    .word intr_return

#endif
