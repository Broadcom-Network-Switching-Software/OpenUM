/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
 
#ifdef __C51__
#ifdef CODE_USERCLASS
#pragma userclass (code = appinit)
#endif /* CODE_USERCLASS */
#endif /* __C51__ */

#include "system.h"
#if (CFG_RXTX_SUPPORT_ENABLED && defined(CFG_LLDP_INCLUDED))
#include "appl/lldp/lldp.h"
#endif /* CFG_RXTX_SUPPORT_ENABLED && defined(CFG_LLDP_INCLUDED) */

#if defined(CFG_SWITCH_MCAST_INCLUDED)
#include "appl/igmpsnoop.h"
#endif /* defined(CFG_SWITCH_MCAST_INCLUDED) */

#if (CFG_RXTX_SUPPORT_ENABLED && CFG_UIP_STACK_ENABLED)
#include "uip_task.h"
#endif /* CFG_RXTX_SUPPORT_ENABLED && CFG_UIP_STACK_ENABLED */


extern void APIFUNC(appl_init)(void) REENTRANT;
extern void cli_init(void) REENTRANT;
extern void ui_system_init(void) REENTRANT;
extern void ui_flash_init(void) REENTRANT;
extern void ui_switch_init(void) REENTRANT;
extern void ui_rx_init(void) REENTRANT;
extern void ui_tx_init(void) REENTRANT;
extern void ui_lldp_init(void);
extern void ui_igmpsnoop_init(void) REENTRANT;
extern void ui_qt_init(void) REENTRANT;
extern void ui_pcm_init(void) REENTRANT; 
void
APIFUNC(appl_init)(void) REENTRANT
{
#if CFG_CLI_ENABLED

    cli_init();

#if CFG_CLI_SYSTEM_CMD_ENABLED
    ui_system_init();
#endif /* CFG_CLI_SYSTEM_CMD_ENABLED */

#if CFG_CLI_FLASH_CMD_ENABLED
    ui_flash_init();
#endif /* CFG_CLI_FLASH_CMD_ENABLED */

#if CFG_CLI_SWITCH_CMD_ENABLED
    ui_switch_init();
#endif /* CFG_CLI_SWITCH_CMD_ENABLED */

#if (CFG_RXTX_SUPPORT_ENABLED && CFG_CLI_RX_CMD_ENABLED)
    ui_rx_init();
#endif /* CFG_RXTX_SUPPORT_ENABLED && CFG_CLI_RX_CMD_ENABLED */

#if (CFG_RXTX_SUPPORT_ENABLED && CFG_CLI_TX_CMD_ENABLED)
    ui_tx_init();
#endif /* CFG_RXTX_SUPPORT_ENABLED && CFG_CLI_TX_CMD_ENABLED */

#endif /* CFG_CLI_ENABLED */

#if (CFG_RXTX_SUPPORT_ENABLED && defined(CFG_LLDP_INCLUDED))
    lldp_init();
#if (CFG_CLI_ENABLED)
    ui_lldp_init();
#endif
#endif /* CFG_RXTX_SUPPORT_ENABLED && defined(CFG_LLDP_INCLUDED) */

#ifdef CFG_PCM_SUPPORT_INCLUDED
    ui_pcm_init();
#endif
#if (CFG_RXTX_SUPPORT_ENABLED && defined(CFG_SWITCH_MCAST_INCLUDED))
    igmpsnoop_database_init();

    #if CFG_CLI_ENABLED
        ui_igmpsnoop_init();
    #endif

#endif 


#if (CFG_RXTX_SUPPORT_ENABLED && CFG_UIP_STACK_ENABLED)
    uip_task_init();
#endif /* CFG_RXTX_SUPPORT_ENABLED && CFG_UIP_STACK_ENABLED */

#ifdef __EMULATION__
    ui_qt_init();
#endif

}

