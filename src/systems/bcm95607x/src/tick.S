/*
 * $Id: tick.S,v 1.1 Broadcom SDK $
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * File:    tick.S
 * Purpose: ARM r5 cycle counter support and _getticks
 *          implementation.
 */

#include "thumb.h"
#include "board.h"
#include "arm.h"

    .syntax unified
    .code   32
    .section ".text","ax"
    .global enable_arm_cyclecount
enable_arm_cyclecount:
    mrc p15,0,r1,c9,c12,0
    ldr r2,=1
    orr r1,r1,r2
    mcr p15,0,r1,c9,c12,0   /* bit[0] in PMCR: enable all counters */
    ldr r1,=0x80000000
    mcr p15,0,r1,c9,c12,1   /* C,bit[31]: enable PMCCNTR */
    bx  lr

    .global disable_arm_cyclecount
disable_arm_cyclecount:
    ldr r1,=0x80000000
    mcr p15,0,r1,c9,c12,2   /* Count enable clear register: PMCNTENCLR */
    bx  lr

    .global _getticks
_getticks:
    mrc p15,0,r0,c9,c13,0   /* Cycle count depends on PMCR.D bit[3] */
    bx  lr

    .global set_arm_cyclecount
set_arm_cyclecount:
    mcr p15,0,r0,c9,c13,0
    bx  lr
