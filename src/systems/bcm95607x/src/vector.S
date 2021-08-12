/*
 * $Id: vector.S, $
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 *
 * File:    vector.S
 * Purpose: exception handling
 */
        .syntax unified

#include "thumb.h"
#include "board.h"
#include "arm.h"
#include "bsp_config.h"
#include "config.h"

#ifndef __BOOTLOADER__
#include "exchandler.h"

#define REF_GEN(sym) \
        .global sym ## _ref ;\
        .global sym ;\
sym ## _ref: ;\
        .word sym

#define EXCEPTION_MODE \
        .code   32

        EXCEPTION_MODE
        .section ".ATCM","ax"
        .global reset_v

reset_v:
        b       reset_reentry
        b       undef_instr
        b       sw_rupt
        b       prefetch_abort
        b       data_abort
        b       reset_v
        b       irq
        b       fiq

/* Reset reentry */
        EXCEPTION_MODE
reset_reentry:
        mov     sp, #0
        b       exception

/* Undefined instruction */
        EXCEPTION_MODE
        .global  undef_instr
undef_instr:
        mov     sp, #1
        b       exception

/* Software interrupt */
        EXCEPTION_MODE
        .global  sw_rupt
sw_rupt:
        mov     sp,#2
        b       exception

/* Instruction prefetch abort */
        EXCEPTION_MODE
        .global  prefetch_abort
prefetch_abort:
        mov     sp,#3
        b       exception

/* Data abort */
        EXCEPTION_MODE
        .global  data_abort
data_abort:
        mov     sp,#4
        b       exception

/* Unified exception handling */
        EXCEPTION_MODE
        .global  exception_r0
exception_r0:
        .word  0

exception:
        cpsID   if                              /* disable interrupt */

        str     r0, exception_r0                /* store exception r0 temporarily */

        /* store info where host can get */
        ldr     r0, =CFG_EXCEPTION_DUMP_BASE
        add     r0, #4                          /* reserve space for exception header */
        str     sp, [r0], #4                    /* exception type */
        add     r0, #4                          /* reserve space for r0 to store later */
        stmia   r0!, {r1-r12}                   /* store r1-r12 */

        add     r0, #8                          /* reserve space for sp and lr to store later */

        str     lr, [r0], #4                    /* banked lr contains PC when exception occurred */

        mrs     r1, spsr                        /* CPSR when exception occurred */
        str     r1, [r0], #4

        mrc     p15, 0, r2, c5, c0, 0           /* Data Fault Status Register */
        str     r2, [r0], #4

        mrc     p15, 0, r2, c6, c0, 0           /* Data Fault Address Register */
        str     r2, [r0], #4

        mrc     p15, 0, r2, c5, c0, 1           /* Instruction Fault Status Register */
        str     r2, [r0], #4

        mrc     p15, 0, r2, c6, c0, 2           /* Instruction Fault Address Register */
        str     r2, [r0], #4

        mrc     p15, 0, r2, c5, c1, 0           /* Auxiliary Data Fault Status Registerv */
        str     r2, [r0], #4

        mrc     p15, 0, r2, c5, c1, 1           /* Auxiliary Instruction Fault Status Registerv*/
        str     r2, [r0], #4

        mrs     r2, cpsr                        /* remember current mode, so we can re-enter */

        /* enter mode that was in place at exception time so that we can store SP_mode and LR_mode */

        /* r1 has spsr */
        orr     r1, r1, #0xc0                    /* disable interrupts in spsr */
        msr     cpsr_c, r1                       /* spsr + no_interrupt -> cpsr */

        ldr     r0, =CFG_EXCEPTION_DUMP_BASE
        ldr     r1, =EXC_HEADER                  /* exception dump header = EXC. */
        str     r1, [r0]
        ldr     r1, exception_r0                 /* retrieve exception r0 */
        str     r1, [r0, #8]                     /* store exception r0 */
        str     r13, [r0, #60]                   /* r13/sp from faulted mode */
        str     r14, [r0, #64]                   /* r14/lr from faulted mode */

        /* restore to exception mode */
        msr     cpsr_c, r2

        ldr     sp, exception_stack_ref
        /* r0 already set to CFG_EXCEPTION_DUMP_BASE */
        blx     exc_handler                      /* Never returns */

        REF_GEN(exception_stack)

/* IRQ */
        EXCEPTION_MODE
       .global  irq
irq:
#ifdef CFG_INTR_INCLUDED
        b       intr_entry
#else
        mov     sp,#6
        b       exception
#endif


/* FIQ */
        EXCEPTION_MODE
       .global  fiq
fiq:
        b fiq
#endif /* __BOOTLOADER__ */

