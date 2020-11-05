/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARD_H_
#define _BOARD_H_

/* Number of units */
#define BOARD_NUM_OF_UNITS              (1)


/* Maximal port number for pbmp_t structure */
#define SOC_MAX_NUM_PORTS               (42)

/* Maximal number of ports (may be different than actual port count) */
#ifdef CFG_WEB_LARGE_PORT_COUNT
#define  BOARD_MAX_NUM_OF_PORTS         (64)
#else
#define  BOARD_MAX_NUM_OF_PORTS         (28)
#endif
/* Maximal number of ports for led display (used in top right web page) */
#define  BOARD_MAX_NUM_OF_PORTS_FOR_LED_DISPLAY  (32)

/* Maximal number of qvlan groups (may be different than actual vlan group) */
#define  BOARD_MAX_NUM_OF_QVLANS        (128)

/* Maximal number of port based vlan groups : 
 *  - groups for every ports and uplink port(egress on all ports)
 */
#define  BOARD_MAX_NUM_OF_PVLANS         (BOARD_MAX_NUM_OF_PORTS + 1)

#define  BOARD_MAX_NUM_OF_LAG           (4)
#define  BOARD_MAX_PORT_PER_LAG         (8)

/* Max required width (in bytes) for logical port list */
#define MAX_UPLIST_WIDTH  ((BOARD_MAX_NUM_OF_PORTS + 7) / 8)

/* Memory pool (for malloc) */

/* System AXI clock in Hz (400MHz) */
#define BOARD_DEFAULT_AXI_CLOCK     (400000000)

#if CONFIG_EMULATION

/* UART clock in ChipCommonA in Hz (refer to c_clk100 emulation clock) */
#define BOARD_CCA_WH2_UART_CLOCK           (76894)

/* UART clock in ChipCommonA in Hz (refer to c_clk50 emulation clock) */
#define BOARD_CCA_WF2_UART_CLOCK           (76894)

/* UART clock in ChipCommonA in Hz (refer to c_clk31.25 emulation clock) */
#define BOARD_CCA_FH2_UART_CLOCK          (76894)

/* CPU clock in Hz (emulation clock) */
#define BOARD_CPU_CLOCK               (114872)

#else
/* APB clock in ChipCommonA in Hz (100MHz, AXI clock/4) */
#define BOARD_CCA_WH2_UART_CLOCK           (100000000)

/* APB clock in ChipCommonA in Hz (50MHz, AXI clock/4) */
#define BOARD_CCA_WF2_UART_CLOCK           (50000000)

/* APB clock in ChipCommonA in Hz (25MHz, AXI clock/4) */
#define BOARD_CCA_FH2_UART_CLOCK           (25000000)


/* CPU clock in Hz (100MHz) */
#define BOARD_CPU_CLOCK               (wh2_sw_info.cpu_clock)

#endif

/* Max size of RX packet buffer */
#define DEFAULT_RX_BUFFER_SIZE          (1600)

/* Max number of packet buffers */
#define DEFAULT_RX_BUFFER_COUNT         (1)

/* Loader address */
#define BOARD_LOADER_ADDR              (0xF0000000)

/* Frimware address to program */
#define BOARD_FIRMWARE_ADDR            (0xF0100000)

/* Frimware address to program */
#define BOARD_SECONDARY_FIRMWARE_ADDR  (0xF0280000)

/* Shared internal SRAM memory address for loader and firmware */
#define BOARD_BOOKKEEPING_ADDR         (0x02000000)

/* Factory address */
#define CFG_FACTORY_CONFIG_BASE        (0xF00FD000)
#define CFG_FACTORY_CONFIG_OFFSET      (0x0)

/* Persistence address */
#define CFG_FLASH_START_ADDRESS        (0xF0000000)

/* Config address */
#define CFG_CONFIG_BASE                 (0xF00FC000)
#define CFG_CONFIG_OFFSET               (0x000)
#define CFG_CONFIG_SIZE                 (0x1000 - CFG_CONFIG_OFFSET)

#define MEDIUM_FLASH_START_ADDRESS     (0xF00FE000)

#define MEDIUM_FLASH_SIZE              (0x2000)

/* Shortcut TX/RX for boot loader */
#define BOOT_SOC_INCLUDE_FILE          "soc/bcm5354x.h"
#define BOOT_FUNC_TX                    bcm5354x_tx
#define BOOT_FUNC_RX_SET_HANDLER        bcm5354x_rx_set_handler
#define BOOT_FUNC_RX_FILL_BUFFER        bcm5354x_rx_fill_buffer

/* The MPU supports zero, 12, or 16 regions */
#define MPU_NUM_REGIONS                 (12)

#ifndef __ASSEMBLER__
extern void board_early_init(void);
extern sys_error_t board_init(void);
extern void board_late_init(void); 
extern uint8  board_linkscan_disable;
extern uint8  board_linkdown_message;
extern uint8  board_upload_vc;
#endif

/* default 1Q-VLAN disabled(PVLAN mode) */
#ifdef CFG_SWITCH_VLAN_INCLUDED
/* SWITCH_VLAN_FEATURE_IS_READY :
 *  - This symbol will be removed after VLAN Board/SOC API implementation is finished.
 */
#define SWITCH_VLAN_FEATURE_IS_READY    0
#if SWITCH_VLAN_FEATURE_IS_READY
#define DEFAULT_QVLAN_DISABLED
#endif  /* SWITCH_VLAN_FEATURE_IS_READY */
#endif  /* CFG_SWITCH_VLAN_INCLUDED */


#endif /* _BOARD_H_ */
