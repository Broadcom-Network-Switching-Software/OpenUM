/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * This file contains data declared by the init module.  It also
 * contains externs for that data so we can keep the types straight.
 */

#if defined(__ASSEMBLER__)
#define DECLARE_INITVAR(x) \
       .globl x ; \
x:     _LONG_   0
#else
#define DECLARE_INITVAR(x) \
       extern long x;
#endif

DECLARE_INITVAR(mem_textreloc)
DECLARE_INITVAR(mem_textbase)
DECLARE_INITVAR(mem_textsize)
DECLARE_INITVAR(mem_totalsize)
DECLARE_INITVAR(mem_topofmem)
DECLARE_INITVAR(mem_heapstart)
DECLARE_INITVAR(mem_bottomofmem)
DECLARE_INITVAR(mem_datareloc)
DECLARE_INITVAR(mem_dmaheapstart)
DECLARE_INITVAR(cpu_prid)


