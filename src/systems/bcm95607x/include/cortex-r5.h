/*
 * $Id: cortex-r5.h,v 1.1 Broadcom SDK $
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * File:    cortex-r5.h
 */

#ifndef CORTEX_R5_H
#define CORTEX_R5_H

#ifdef TOOLCHAIN_gnu
#define LOW_POWER_MODE asm volatile ("WFI")
#define LOW_POWER_ASM wfi
#endif

#ifdef TOOLCHAIN_arm
#define LOW_POWER_MODE  mos_low_power_mode()
#define LOW_POWER_ASM wfi
#endif

#define THUMB2
#define CYCLE_COUNTER

#ifdef BIG_ENDIAN
#define SET_ENDIAN      SETEND BE
#else
#define SET_ENDIAN      SETEND LE
#endif

#define CORTEX_R4
#define CORTEX_R5
#define DCACHE
#define ICACHE
#define ASM_ICACHE_INVALIDATE    mcr p15,0,r0,c7,c5,0
#define ASM_DCACHE_INVALIDATE    mcr p15,0,r0,c15,c5,0
#define ASM_BRANCH_PREDICT_CLEAR mcr p15,0,r0,c7,c5,6

#endif
