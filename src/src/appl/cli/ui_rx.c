/*
 * $Id: ui_rx.c,v 1.8 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
 
#ifdef __C51__
#ifdef CODE_USERCLASS
#pragma userclass (code = uirx)
#endif /* CODE_USERCLASS */
#endif /* __C51__ */

#include "system.h"
#include "appl/cli.h"
#include "utils/ui.h"
#include "utils/ports.h"

#if (CFG_UM_BCMSIM==1) || (CONFIG_EMULATION==1)
#if defined(__LINUX__)
extern void exit(int status);
#endif
#endif


#if (CFG_CLI_ENABLED && CFG_RXTX_SUPPORT_ENABLED && CFG_CLI_RX_CMD_ENABLED)

/* RX monitoring and forwarding */
STATIC BOOL     rx_registered;
STATIC uint8    rx_monitor_state;
STATIC uint8    rx_forward_enabled;
STATIC uint8    rx_forward_uplist[MAX_UPLIST_WIDTH];
STATIC uint8    rx_forward_untag_uplist[MAX_UPLIST_WIDTH];
STATIC uint32   rx_count;

/* Forwards */
void APIFUNC(ui_rx_init)(void) REENTRANT;
APISTATIC void cli_cmd_switch_rx_mon(CLI_CMD_OP op) REENTRANT;
APISTATIC sys_rx_t cli_switch_rx_handler(sys_pkt_t *pkt, void *cookie) REENTRANT;

APISTATIC sys_rx_t
APIFUNC(cli_switch_rx_handler)(sys_pkt_t *pkt, void *cookie) REENTRANT
{
    UNREFERENCED_PARAMETER(cookie);
    
    SAL_ASSERT(pkt != NULL);
    if (pkt == NULL) {
        return SYS_RX_HANDLED;
    }
    
    rx_count++;
    if (rx_monitor_state > 0) {
        BOOL error = FALSE;
        sal_printf("\nRX(%lu): length=%u port=%u", 
                    rx_count, pkt->pkt_len, pkt->rx_src_uport);
        if (pkt->flags) {
            if (pkt->flags & SYS_RX_FLAG_COS) {
                sal_printf(" cos=%u", pkt->cos);
            }
            if (pkt->flags & SYS_RX_FLAG_TIMESTAMP) {
                sal_printf(" Timestamp=%lu", pkt->rx_timestamp);
            }
            if (pkt->flags & SYS_RX_FLAG_TRUNCATED) {
                sal_printf(" TRUNCATED");
            }
            if (pkt->flags & SYS_RX_FLAG_ERROR_CRC) {
                error = TRUE;
                sal_printf(" !CRC-ERROR!");
            }
            if (pkt->flags & SYS_RX_FLAG_ERROR_OTHER) {
                error = TRUE;
                sal_printf(" !ERROR!");
            }
        }
        if (rx_monitor_state == 2 && !error) {
            ui_dump_memory(pkt->pkt_data, pkt->pkt_len);
        } else {
            sal_putchar('\n');
        }
    } 
    
    if (rx_forward_enabled) {
        sys_error_t r;
        
        sal_memcpy(pkt->tx_uplist, rx_forward_uplist, MAX_UPLIST_WIDTH);
        sal_memcpy(pkt->tx_untag_uplist, rx_forward_untag_uplist, MAX_UPLIST_WIDTH);
        pkt->flags = 0;
        r = sys_tx(pkt, NULL);
        if (r != SYS_OK) {
            sal_printf("TX error: %d\n", (int16)r);
        }
        
        return SYS_RX_HANDLED;
    }
    
    return SYS_RX_NOT_HANDLED;    
}

APISTATIC void
APIFUNC(cli_cmd_switch_rx_mon)(CLI_CMD_OP op) REENTRANT
{
    uint8 uplist[MAX_UPLIST_WIDTH];
    
    if (op == CLI_CMD_OP_HELP) {
        sal_printf("Command to enable/disable RX monitor.\n"
                   "When it's enabled, notification of packet receiving will\n"
                   "be shown.\n");
    } else if (op == CLI_CMD_OP_DESC) {
        sal_printf("RX monitor");
    } else {
        char c;
        sal_printf("  0 - Disable RX monitor\n"
                   "  1 - Show one line summary\n"
                   "  2 - Show packet content\n"
                   "  3 - Forward received packets (silently)\n"
                   "  4 - Forward and show one line summary\n"
                   "  5 - Forward and show packet content\n"
                   "Enter your choice: ");
        c = sal_getchar();
        sal_putchar('\n');
        if (c == '0' || c == '1' || c == '2') {
            rx_monitor_state = (uint8)(c - '0');
        } else if (c == '3' || c == '4' || c == '5') {
            sal_printf("Port bitmap to forward:\n");
            uplist_clear(uplist);
            if (ui_get_bytes(uplist, MAX_UPLIST_WIDTH, "Port list", FALSE) == UI_RET_OK) {
                if (uplist_is_empty(uplist) == SYS_OK) {
                    rx_forward_enabled = FALSE;
                    sal_printf("No ports to forward!\n");
                } else {
                    sal_memcpy(rx_forward_uplist, uplist, MAX_UPLIST_WIDTH);
                    if (ui_yes_or_no("Untag?", 1)) {
                        sal_memcpy(rx_forward_untag_uplist, 
                                    rx_forward_uplist, MAX_UPLIST_WIDTH);
                    } else {
                        uplist_clear(rx_forward_untag_uplist);
                    }
                    rx_forward_enabled = TRUE;
                    rx_monitor_state = (uint8)(c - '3');
                }
            } else {
                sal_printf("Cancelled.\n");
            }
#if (CFG_UM_BCMSIM==1) || (CONFIG_EMULATION==1)
#if defined(__LINUX__)
        } else if (c == '9') {
            sal_printf("exit(1)..\n");
            exit(1);
#endif
#endif
        } else {
            sal_printf("Invalid choice.\n");
        }
        
        if (rx_registered == FALSE && rx_monitor_state > 0) {
            /* Lazy registration to enable RX engine on demand */
            sys_rx_register(
                cli_switch_rx_handler, 
                CFG_CLI_RX_MON_PRIORITY, 
                NULL,
                SYS_RX_REGISTER_FLAG_ACCEPT_PKT_ERRORS | 
                SYS_RX_REGISTER_FLAG_ACCEPT_TRUNCATED_PKT
                );
            rx_registered = TRUE;
        }
    }
}

void
APIFUNC(ui_rx_init)(void) REENTRANT
{
    /* RX monitor */
    rx_registered = FALSE;
    rx_count = 0;
    rx_monitor_state = 0;
    rx_forward_enabled = FALSE;
    uplist_clear(rx_forward_uplist);
    uplist_clear(rx_forward_untag_uplist);
    cli_add_cmd('R', cli_cmd_switch_rx_mon);
}

#endif
