/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Macros to deal with physical, virtual, and uncached addresses.
 * for MIPS, these map to the appropriate KSEG0/KSEG1 macros
 */

//#include "sbmips.h"

/*  *********************************************************************
    *  Address space coercion macros
 */

#define PHYS_TO_K0(pa)  ((pa))
#define PHYS_TO_K1(pa)  ((pa))
#define K0_TO_PHYS(va)  ((va))
#define K1_TO_PHYS(va)  ((va))
#define K0_TO_K1(va)    ((va))
#define K1_TO_K0(va)    ((va))

#define PHYSADDR(x)  (x)
#define KERNADDR(x)  (x)
#define UNCADDR(x)   (x)

