/*
 * $Id: cache.S,v 1.1 Broadcom SDK $
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 *
 * File:    cache.S
 * Purpose: cache support
 */

#include "thumb.h"
#include "board.h"
#include "arm.h"

        .syntax unified
        .code   32

//
// Issue a membar
//
        .global membar
membar:
        mov     r0, #0
        mcr     p15, 0, r0, c7, c10, 4
        mcr     p15, 0, r0, c7, c10, 5
        bx      lr

//
// Routine to flush dcache
//   r0 - start addr
//   r1 - length
        .global dcache_flush
dcache_flush:
        adds    r1,r0,r1        // Compute the end addr
        and     r0,r0,CACHELINE_MASK
1:
        mcr     p15,0,r0,c7,c11,1
        add     r0,r0,CACHELINE_SIZE
        cmp     r0,r1
        blt     1b
        mcr     p15,0,r0,c7,c10,5       // mem bar
        bx      lr

//
// Routine to invalidate dcache
//   r0 - start addr
//   r1 - length
        .global dcache_invalidate
dcache_invalidate:
        adds    r1,r0,r1        // Compute the end addr
        and     r0,r0,CACHELINE_MASK
1:
        mcr     p15,0,r0,c7,c6,1
        add     r0,r0,CACHELINE_SIZE
        cmp     r0,r1
        blt     1b
        mcr     p15,0,r0,c7,c10,5       // mem bar
        bx      lr

//
// Routine to flush and invalidate dcache
//   r0 - start addr
//   r1 - length
        .global dcache_flush_and_inv
dcache_flush_and_inv:
        adds    r1,r0,r1        // Compute the end addr
        and     r0,r0,CACHELINE_MASK
1:
        mcr     p15,0,r0,c7,c14,1
        add     r0,r0,CACHELINE_SIZE
        cmp     r0,r1
        blt     1b
        mcr     p15,0,r0,c7,c10,5       // mem bar
        bx      lr
#if 0
//
// Routine to flush and invalidate the entire dcache
//
        .global dcache_flush_and_inv_all
dcache_flush_and_inv_all:
	ldr	r0,=system_baseaddr
	ldr	r1,=system_endaddr
1:
        mcr     p15,0,r0,c7,c14,1
        add     r0,r0,CACHELINE_SIZE
        cmp     r0,r1
        blt     1b
        mcr     p15,0,r0,c7,c10,5       // mem bar
        bx      lr

//
// Routine to flush the entire dcache
//
        .global dcache_flush_all
dcache_flush_all:
	ldr	r0,=system_baseaddr
	ldr	r1,=system_endaddr
1:
        mcr     p15,0,r0,c7,c10,1
        add     r0,r0,CACHELINE_SIZE
        cmp     r0,r1
        blt     1b
        mcr     p15,0,r0,c7,c10,5       // mem bar
        bx      lr

//
// Routine to invalidate the entire dcache
//
        .global dcache_inv_all
dcache_inv_all:
	ldr	r0,=system_baseaddr
	ldr	r1,=system_endaddr
1:
        mcr     p15,0,r0,c7,c6,1
        add     r0,r0,CACHELINE_SIZE
        cmp     r0,r1
        blt     1b
        mcr     p15,0,r0,c7,c10,5       // mem bar
        bx      lr
#endif

//
// Routine to invalidate icache
//   r0 - start addr
//   r1 - length
        .global icache_invalidate
icache_invalidate:
        adds    r1,r0,r1        // Compute the end addr
1:
        mcr     p15,0,r0,c7,c5,1
        add     r0,r0,CACHELINE_SIZE
        cmp     r0,r1
        blt     1b
        mcr     p15,0,r0,c7,c10,5       // mem bar
#ifdef CORTEX_R4
        mcr     p15,0,r0,c7,c5,6        // Invalidate branch predictor array
        mcr     p15,0,r0,c7,c5,4        // Flush prefetch buffer
        mcr     p15,0,r0,c7,c10,5       // mem bar
#endif
        bx      lr
