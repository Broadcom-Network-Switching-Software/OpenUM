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
#ifndef CFG_HEAP_SIZE

#if CFG_IP_FRAGMENT_INCLUDED
#define CFG_HEAP_SIZE            240  /* heap size in kilobytes */  
/* Used for packet DMA */
#define CFG_DMA_HEAP_SIZE       8192 /* 8K */
#else
#define CFG_HEAP_SIZE            240  /* heap size in kilobytes */
/* Used for packet DMA */
#define CFG_DMA_HEAP_SIZE       8192 /* 8K */
#endif


#define CFG_STACK_SIZE          (16384-272) /* stack size (bytes, rounded up to K), 258 is reserved for exception */


#endif

#ifdef CFG_USE_UART1
#define CFG_UART_BASE          R_CHIPCOMMONG_UART1_UART_RBR_THR_DLL
#else
#define CFG_UART_BASE          R_CHIPCOMMONG_UART0_UART_RBR_THR_DLL
#endif
#define CFG_BSPI_BASE          R_QSPI_BSPI_REGISTERS_REVISION_ID
