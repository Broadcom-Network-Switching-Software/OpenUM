/*
 * $Id: config_umplus.h,v 1.2 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

/* Chip type */
#define CFG_XGS_CHIP                        (1)
/* #define CFG_ROBO_CHIP						(!CFG_XGS_CHIP) */


#ifdef __EMULATION__
#define CONFIG_EMULATION                    (1)
#else
#define CONFIG_EMULATION                    (0)
#endif

/* Enable UART1 output */
#define CFG_USE_UART1                       (1)

/* Enable console output */
#define CFG_CONSOLE_ENABLED                 (1)

/* Enable debug output */
#define CFG_DEBUGGING_ENABLED               (0)

/* Enable assertion */
#define CFG_ASSERTION_ENABLED               (0)

/* Enhanced power saving */
#define CFG_ENHANCED_POWER_SAVING           (0)

/* Enable packet RX/TX support */
#define CFG_RXTX_SUPPORT_ENABLED            (1)

/* Enable SAL LIB_support */
#define CFG_SAL_LIB_SUPPORT_ENABLED         (1)

/* CLI support */
#define CFG_CLI_ENABLED                     (1 && CFG_CONSOLE_ENABLED)

/* CLI prompt */
#define CFG_CLI_PROMPT                      "CMD> "

/* CLI system commands support */
#define CFG_CLI_SYSTEM_CMD_ENABLED          (1)

/* CLI switch commands support */
#define CFG_CLI_SWITCH_CMD_ENABLED          (1)

/* CLI power commands support */
#define CFG_CLI_POWER_CMD_ENABLED           (0)

/* CLI RX commands support */
#define CFG_CLI_RX_CMD_ENABLED              (1)

/* CLI TX commands support */
#define CFG_CLI_TX_CMD_ENABLED              (1)

/* CLI RX monitor */
#define CFG_CLI_RX_MON_PRIORITY             (1)

/* uIP main control RX priority */
#define CFG_UIP_RX_PRIORITY                 (10)

/* CLI TX packet configurations */
#define CFG_CLI_TX_MAX_PKTCFGS              (8)

/* UART baudrate */
#define CFG_UART_BAUDRATE                (115200)

/* Max background tasks */
#define CFG_MAX_BACKGROUND_TASKS            (8)

/* Enable timer callback mechanism */
#define CFG_TIMER_CALLBACK_SUPPORT          (1)

/* Max registered timer (callback) */
#define CFG_MAX_REGISTERED_TIMERS           (16)

/* Enable linkchange callback mechanism */
#define CFG_LINKCHANGE_CALLBACK_SUPPORT     (1)

/* Max registered link change callback */
#define CFG_MAX_REGISTERED_LINKCHANGE       (16)

/* Interval for checking link change (in us) */
#define CFG_LINKCHANGE_CHECK_INTERVAL       (600000UL)

/* Max registered RX callback functions */
#define CFG_MAX_REGISTERED_RX_CBKS          (16)

/* Whether RX uses interrupt for receiving notification */
#define CFG_RX_USE_INTERRUPT                (0)

/* Packet length (excluding CRC) */
#define MIN_PACKET_LENGTH                   (60)
#define MAX_PACKET_LENGTH                   (1514)

/* FP */
#define ENTRIES_PER_SLICE                   (128)

/* Critical region protection between CR5 and host processor. */
/* #define CFG_SOC_SEMAPHORE_INCLUDED */

/* chip type */
#define CFG_SWITCH_XGS_NEW_SBUS_FORMAT_INCLUDED

/* Stoarge related */
#define CFG_FLASH_SUPPORT_ENABLED           (1)
#define CFG_CLI_FLASH_CMD_ENABLED           (1)
#define CFG_FLASH_4BYTE_ADDR_ENABLED        (1)

/* A factory data management engine */
#define CFG_FACTORY_CONFIG_INCLUDED

/* A binary storage management engine, named serializer */
#define CFG_PERSISTENCE_SUPPORT_ENABLED     (1)

#define CFG_SWITCH_VLAN_INCLUDED

/* IP above layer related  setting*/
#define MAX_IP_TOTAL_LENGTH                 (1500)
#define CFG_NET_LINKCHANGE_NOTIFY_INCLUDED
#define CFG_NET_MAX_LINKCHANGE_HANDLER      (8)


/* To add a serial number into factory */
#define CFG_PRODUCT_REGISTRATION_INCLUDED

/* To enable login web pages and related mechanism*/
/* #define CFG_SYSTEM_PASSWORD_INCLUDED */

/* Port Control Manager support */
#define CFG_PCM_SUPPORT_INCLUDED

/* Port Control Manager support */
#define CFG_FACTORY_CONFIG_INCLUDED


/*
  *  CFG_NVRAM_SUPPORT_INCLUDED
  *      undefined : exclude the NVRAM defined advance command line engine
  *      defined: include the NVRAM storage
  */

/* #define CFG_NVRAM_SUPPORT_INCLUDED */




/****************************************************************************
 *
 * Features defined below can be selectively add or removed before building the image
 */

 /*
  *  CFG_CHIP_SYMBOLS_INCLUDED
  *      undefined : exclude symbol table for  all registers and memories
  *      defined: include symbol table for  all registers and memories
  */
#define CFG_CHIP_SYMBOLS_INCLUDED

 /*
  *  CFG_CHIP_SYMBOLS_FIELD_INCLUDED
  *      undefined : exclude fields information from symbol table
  *      defined: include fields information in symbol table
  */
#define CFG_CHIP_SYMBOLS_FIELD_INCLUDED

/*
  *  CFG_SDKCLI_INCLUDED
  *      undefined : exclude SDK-like CLI shell
  *      defined: include SDK-like CLI shell
  */
#define CFG_SDKCLI_INCLUDED

/*
  *  CFG_SDKCLI_GPIO_INCLUDED
  *      undefined : exclude SDK-like "gpio" command
  *      defined: include SDK-like "gpio" command
  */
#define CFG_SDKCLI_GPIO_INCLUDED

/*
  *  CFG_SDKCLI_PORT_INCLUDED
  *      undefined : exclude SDK-like "port" command
  *      defined: include SDK-like "port" command
  */
#define CFG_SDKCLI_PORT_INCLUDED

/*
  *  CFG_SDKCLI_PHY_INCLUDED
  *      undefined : exclude SDK-like "phy" command
  *      defined: include SDK-like "phy" command
  */
#define CFG_SDKCLI_PHY_INCLUDED

/*
  *  CFG_SDKCLI_COE_INCLUDED
  *      undefined : exclude SDK-like "coe" command
  *      defined: include SDK-like "coe" command
  */
#define CFG_SDKCLI_COE_INCLUDED

/*
  *  CFG_SDKCLI_LED_INCLUDED
  *      undefined : exclude SDK-like "led" command
  *      defined: include SDK-like "led" command
  */
#define CFG_SDKCLI_LED_INCLUDED

/*
  *  CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
  *      undefined : exclude VENDOR CONFIG related code
  *      defined: will include the NVRAM ,some XCMD utils and VENDOR CONFIG code
  */

#define CFG_VENDOR_CONFIG_SUPPORT_INCLUDED


/*
  *  CFG_XCOMMAND_INCLUDED
  *      undefined : exclude the XML defined advance command line engine
  *      defined: include the XML defined advance command line engine
  */
/* #define CFG_XCOMMAND_INCLUDED */

/*
  *  CFG_SWITCH_L2_ADDR_INCLUDED
  *      undefined : exclude L2 address APIs
  *      defined: include L2 address APIs
  */
/*#define CFG_SWITCH_L2_ADDR_INCLUDED */

/*
  *  CFG_SWITCH_DOS_INCLUDED
  *      undefined : exclude DOS control feature
  *      defined: include DOS control feature
  */
/* #define CFG_SWITCH_DOS_INCLUDED */

/*
  *  CFG_SWITCH_LAG_INCLUDED
  *      undefined : exclude LAG feature
  *      defined: include LAG feature
  */
/* #define CFG_SWITCH_LAG_INCLUDED */

/*
  *  CFG_UIP_STACK_ENABLED
  *      defined as 0 : disable UIP stack
  *      defined as 1 : enable UIP stack
  */
#define CFG_UIP_STACK_ENABLED               (0 && CFG_RXTX_SUPPORT_ENABLED)

/*
  *  CFG_UIP_IPV6_ENABLED
  *      defined as 0 : disable IPv6
  *      defined as 1 : enable IPv6
  */
#define CFG_UIP_IPV6_ENABLED                (1 && CFG_UIP_STACK_ENABLED)

#ifdef CFG_UIP_STACK_ENABLED
/**
  *  CFG_DHCPC_INCLUDED
  *      undefined : DHCP client is disabled
  *      defined: DHCP client is enabled
  */
/* #define CFG_DHCPC_INCLUDED */
#endif /* CFG_UIP_STACK_ENABLED */

/*
  *  CFG_SWITCH_LOOPDETECT_INCLUDED
  *      undefined : exclude loop-detect feature
  *      defined: include loop-detect feature
  */
/* #define CFG_SWITCH_LOOPDETECT_INCLUDED */

/*
  *  CFG_SWITCH_MIRROR_INCLUDED
  *      undefined : exclude mirror feature
  *      defined: include mirror feature
  */
/* #define CFG_SWITCH_MIRROR_INCLUDED */

/*
  *  CFG_SWITCH_QOS_INCLUDED
  *      undefined : exclude QoS feature
  *      defined: include QoS feature
  */
#define CFG_SWITCH_QOS_INCLUDED

/*
  *  CFG_SWITCH_RATE_INCLUDED
  *      undefined : exclude rate feature
  *      defined: include rate feature
  */
/* #define CFG_SWITCH_RATE_INCLUDED */

/*
  *  CFG_SWITCH_SNAKETEST_INCLUDED
  *      undefined : exclude snaketest feature
  *      defined: include snaketest feature
  */
#define CFG_SWITCH_SNAKETEST_INCLUDED

/*
  *  CFG_SWITCH_STAT_INCLUDED
  *      undefined : exclude statistic feature
  *      defined: include statistic feature
  */
#define CFG_SWITCH_STAT_INCLUDED

/*
  *  CFG_SWITCH_MCAST_INCLUDED
  *      undefined : exclude MCAST feature
  *      defined: include MCAST feature
  */
/* #define CFG_SWITCH_MCAST_INCLUDED */

/*
  *  CFG_SWITCH_EEE_INCLUDED
  *      undefined : exclude EEE (Ethernet Energy Effieciency) feature
  *      defined: include EEE (Ethernet Energy Effieciency) feature
  */
/* #define CFG_SWITCH_EEE_INCLUDED */

/*
  *  CFG_SWITCH_PVLAN_INCLUDED
  *      undefined : exclude port-based VLAN feature
  *      defined: include port-based VLAN feature
  */
/* #define CFG_SWITCH_PVLAN_INCLUDED */
/*
  *  CFG_SWITCH_SYNCE_INCLUDED
  *      undefined : exclude time-synce feature
  *      defined: include time-synce feature
  */
#define CFG_SWITCH_SYNCE_INCLUDED

/*
  *  CFG_SWITCH_TIMESYNC_INCLUDED
  *      undefined : exclude timesync feature
  *      defined: include timesync feature
  */
#define CFG_SWITCH_TIMESYNC_INCLUDED

/*
  *  CFG_HW_CABLE_DIAG_INCLUDED
  *      undefined : exclude cable dialognostic feature
  *      defined: include cable dialognostic feature
  */
/* #define CFG_HW_CABLE_DIAG_INCLUDED */

/*
  *  CFG_ETHERNET_WIRESPEED_INCLUDED
  *      undefined : exclude ethernet wirespeed feature
  *      defined: include ethernet wirespeed feature
  */
/* #define CFG_ETHERNET_WIRESPEED_INCLUDED */

#ifdef CFG_ETHERNET_WIRESPEED_INCLUDED
/*
 * Ethernet@wirespeed retry disable.
 *   defined as 0 1'b1 : downgrade after 1 failed link attemp
 *   defined as 0 1'b0 : use WIRESPEED_RETRY_LIMIT
 */
#define CFG_ETHERNET_WIRESPEED_RETRY_DIS       (0)
/*
 * Ethernet@wirespeed retry limit.
 * It is the number of auto-negotiation attemps to link-up prior to speed downgrade.
 * The ethernet@wirespeed mode must be enabled for retry limit.
 * The retry limit can be chosen from 2-8.
 */
#define CFG_ETHERNET_WIRESPEED_RETRY_LIMIT     (5)
#endif /* CFG_ETHERNET_WIRESPEED_INCLUDED */

/*
  *  CFG_DUAL_IMAGE_INCLUDED
  *      undefined : exclude dual image feature
  *      defined: include dual image feature
  */
#define CFG_DUAL_IMAGE_INCLUDED

/*
  *  CFG_RESET_BUTTON_INCLUDED
  *      undefined : exclude reset button feature
  *      defined: include reset button feature
  */
/* #define CFG_RESET_BUTTON_INCLUDED */

/*
  *  CFG_DEBUGGING_INCLUDED
  *      undefined : exclude debugging code
  *      defined: include debugging code
  */
/* #define CFG_DEBUGGING_INCLUDED */

/*
  *  CFG_SHR_DEBUG_INCLUDED
  *      Valid as CFG_DEBUGGING_INCLUDED is definded.
  *      undefined : exclude SHR debug macro
  *      defined: include SHR debug macro
  */
/* #define CFG_SHR_DEBUG_INCLUDED */

/*
  *  CFG_SHR_DEBUG_LEVEL
  *      Valid as CFG_SHR_DEBUG_INCLUDED is defined.
  *      0 : Error messages only.
  *      1 : Debug + error messages.
  *      2 : Trace + debug + error messages.
  */
/* #define CFG_SHR_DEBUG_LEVEL  0 */

/*
  *  CFG_LED_MICROCODE_INCLUDED
  *      undefined : exclude serial LED feature
  *      defined: includee serial LED feature
  */
#define CFG_LED_MICROCODE_INCLUDED

/*
 *  CFG_INTR_INCLUDED
 *      undefined : exclude interrupt system.
 *      defined: include interrupt system.
 */
#define CFG_INTR_INCLUDED

/*
 *  CFG_POLLED_INTR: Enable polled irq and disable true irq.
 */
#define CFG_POLLED_INTR   (0)

/*
 * CFG_INTR_CHECK_NO_FLASH_CODE:
 *     Check there's no flash code being executed in interrupt context
 *     as CFG_POLLED_INTR=0.
 *
 *     defined: BSPI mode will be disabled in interrupt context.
 *     If any function on flash is invoked, an exception will be triggered.
 *
 *     undefined: Turn off checking.
 */
/* #define CFG_INTR_CHECK_NO_FLASH_CODE */

/*
 * CFG_INTR_CACHE_INVALID_INCLUDED:
 *     Perform cache invalid as interrupt happen for test interrupt latency purose.
 *
 *     defined: Cache invalid as interupt happen.
 *     undefined: Won't do cache invalid.
 */
/* #define CFG_INTR_CACHE_INVALID_INCLUDED */

/*
 *  CFG_WDT_INCLUDED
 *      undefined : exclude watchdog timer support.
 *      defined: include watchdog timer support.
 */
#define CFG_WDT_INCLUDED

/* Watchdog timeout in us */
#define CFG_WDT_TIMEOUT    (5000000UL)

/*
 *  CFG_MCS_INCLUDED
 *      undefined : exclude MCS system.
 *      defined: include MCS system.
 */
#define CFG_MCS_INCLUDED

/*
 *  CFG_BROADSYNC_INCLUDED
 *      undefined : exclude broadsync function.
 *      defined: include broadsync function.
 */
/* #define CFG_BROADSYNC_INCLUDED */

/*
 *  CFG_SPI_MGMT_INCLUDED
 *      undefined : exclude SPI management sample code.
 *      defined: include SPI management sample code.
 */
/* #define CFG_SPI_MGMT_INCLUDED */

/*
 *  CFG_COE_INCLUDED
 *      undefined : exclude COE function
 *      defined: include COE function
 */
#define CFG_COE_INCLUDED
#ifdef CFG_COE_INCLUDED
#ifdef CFG_SWITCH_MCAST_INCLUDED
#error Need to rework the L2MC resource
#endif
#endif

/*
 *  CFG_COE_SCENARIO_INCLUDED
 *      undefined : exclude default init for PTN9XX project on test purpose
 *      defined: include default init for PTN9XX project on test purpose
 */
#define CFG_COE_SCENARIO_INCLUDED

/*
 *  CFG_TEMP_MONITOR_INCLUDED
 *      undefined : exclude monitored temperature query.
 *      defined: include monitored temperature query.
 */
#define CFG_TEMP_MONITOR_INCLUDED

/*
 *  CFG_LONG_US_DEALY_ENABLED
 *      undefined : exclude long micro second delay.
 *      defined: include long micro second delay.
 */
#define CFG_LONG_US_DEALY_ENABLED                 (1)

/*
 *  CFG_M0SSQ_INCLUDED
 *      undefined : exclude M0SSQ driver.
 *      defined: include M0SSQ driver.
 */
#define CFG_M0SSQ_INCLUDED

/*
 *  CFG_HARDWARE_LINKSCAN_INCLUDED
 *      undefined : exclude hardware linkscan driver.
 *      defined: include hardware linkscan driver.
 */
#define CFG_HARDWARE_LINKSCAN_INCLUDED

#endif /* _CONFIG_H_ */
