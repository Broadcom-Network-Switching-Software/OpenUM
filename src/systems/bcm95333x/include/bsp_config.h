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
#define CFG_HEAP_SIZE           24     /* heap size in kilobytes */

#define CFG_STACK_SIZE          16384 /* stack size (bytes, rounded up to K) */


/************* Robo/NTSW specific features *******/
#define CFG_PTABLE          0
#define CFG_TCP_NEW         0
#define CFG_FULLDIAG        0
#define CFG_NETWORK 0   /* define to include network support */
#define CFG_FATFS       0
#define CFG_TCP     0
#define CFG_HTTPFS  0
#define CFG_URLS    0

#define CCA_WATCHDOG_COUNTER   0x18000080
#define DMU_CRU_RESET          0x1803f200

#define CFG_UART0_BASE         0x18000300
#define CFG_UART1_BASE         0x18000400

#ifdef CFG_UART1
#define CFG_UART_BASE          CFG_UART1_BASE
#else
#define CFG_UART_BASE          CFG_UART0_BASE
#endif
#define CFG_BSPI_BASE          0x18027000
