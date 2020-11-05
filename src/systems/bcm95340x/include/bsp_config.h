/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#define CFG_INIT_L1     1   /* initialize the L1 cache */
#define CFG_INIT_L2     0   /* there is no L2 cache */

#define CFG_INIT_DRAM       0   /* initialize DRAM controller */
#define CFG_DRAM_SIZE       1   /* size of DRAM if you don't initialize */

#ifndef CFG_SERIAL_BAUD_RATE
#define CFG_SERIAL_BAUD_RATE    CFG_UART_BAUDRATE   /* normal console speed */
#endif

#define CFG_MULTI_CPUS      0   /* no multi-cpu support */

#define CFG_SERIAL_LEDS     0   /* never set this (UART can hang) */
#define CFG_HEAP_SIZE             32  /* heap size in kilobytes */
/* Used for packet DMA */
#define CFG_DMA_HEAP_SIZE       8192 /* 8K */

#define CFG_STACK_SIZE          8192 /* stack size (bytes, rounded up to K) */

#define DMU_CRU_RESET          0x1800f200
#define PTIM_WATCHDOG_COUNTER  0x19020624
#ifdef CFG_UART1
#define CFG_UART_BASE          0x18021000
#else
#define CFG_UART_BASE          0x18020000
#endif
#define CFG_BSPI_BASE          0x18047000
