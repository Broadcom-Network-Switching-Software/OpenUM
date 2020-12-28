/*
 * $Id: sal_init.c,v 1.6 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
 
#include "system.h"
#include "bsp_config.h"
#if defined(CFG_PCM_SUPPORT_INCLUDED)
#include <pcm/pcm_int.h>   /* for bsl_debug_init */
#endif

extern long mem_heapstart, mem_dmaheapstart;
extern long mem_heap2start;

extern void sal_console_init(int reset);
extern void sal_alloc_init(void *addr, uint32 len);
extern void sal_dma_alloc_init(void *addr, uint32 len);

void
APIFUNC(sal_init)(void) REENTRANT
{
#if CFG_CONSOLE_ENABLED
    sal_console_init(0);
#endif /* CFG_CONSOLE_ENABLED */

    sal_alloc_init((void *)mem_heapstart, ((CFG_HEAP_SIZE)*1024));
    sal_printf("Allocated %dK Heap.\n", CFG_HEAP_SIZE);
#ifdef CFG_HEAP2_SIZE
	sal_alloc_init((void *)mem_heap2start, ((CFG_HEAP2_SIZE)*1024));
#endif
#if defined(CFG_PCM_SUPPORT_INCLUDED)
	bsl_debug_init();	
#endif
    sal_dma_alloc_init((void *)mem_dmaheapstart, CFG_DMA_HEAP_SIZE);
}


