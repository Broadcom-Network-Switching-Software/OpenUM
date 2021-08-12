/*! \file bcma_bcmpktcmd_rx.c
 *
 * BCMPKT RX commands in CLI.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#ifdef CFG_SDKCLI_INCLUDED
#include "cli_porting.h"
#include "bcma_bcmpktcmd_internal.h"

#if CFG_RXTX_SUPPORT_ENABLED

#include <appl/sdkcli/bcma_cli.h>
#include <appl/sdkcli/bcma_cli_parse.h>

#include <appl/sdkcli/bcma_clicmd_rx.h>

extern sys_error_t board_qvlan_add_cpu(uint16 vlanid, int tagged);
extern sys_error_t board_qvlan_remove_cpu(uint16 vlanid);

typedef struct px_watcher_s {
    int show_pkt_data;
    int show_meta_data;
    int show_rx_reason;
    int lb_pkt_data;
    int show_rx_rate;
    int terminate_packet;
} px_watcher_t;

typedef struct px_timesync_s {
    int port_tx;
    int port_1588;
    int two_step;
    int tx_info;
} px_timesync_t;

#define BCMDRD_CONFIG_MAX_UNITS         1
#define BCMA_BCMPKT_NETIF_ALL         (-1)

static watcher_data_t *wdata_list[BCMDRD_CONFIG_MAX_UNITS];

static void
watcher_cfg_api2px(uint32_t *flags, px_watcher_t *px_cfg)
{
    px_cfg->show_pkt_data =
        bcma_bcmpkt_flag_status_get(*flags, WATCHER_DEBUG_SHOW_PACKET_DATA);
    px_cfg->show_meta_data =
        bcma_bcmpkt_flag_status_get(*flags, WATCHER_DEBUG_SHOW_META_DATA);
    px_cfg->show_rx_reason =
        bcma_bcmpkt_flag_status_get(*flags, WATCHER_DEBUG_SHOW_RX_REASON);
    px_cfg->lb_pkt_data =
        bcma_bcmpkt_flag_status_get(*flags, WATCHER_DEBUG_LPBK_PACKET);
    px_cfg->show_rx_rate =
        bcma_bcmpkt_flag_status_get(*flags, WATCHER_DEBUG_SHOW_RX_RATE);
}

static void
watcher_cfg_px2api(const px_watcher_t *px_cfg, uint32_t *flags)
{
    bcma_bcmpkt_flag_set(flags, WATCHER_DEBUG_SHOW_PACKET_DATA,
                         px_cfg->show_pkt_data);
    bcma_bcmpkt_flag_set(flags, WATCHER_DEBUG_SHOW_META_DATA,
                         px_cfg->show_meta_data);
    bcma_bcmpkt_flag_set(flags, WATCHER_DEBUG_SHOW_RX_REASON,
                         px_cfg->show_rx_reason);
    bcma_bcmpkt_flag_set(flags, WATCHER_DEBUG_LPBK_PACKET,
                         px_cfg->lb_pkt_data);
    bcma_bcmpkt_flag_set(flags, WATCHER_DEBUG_SHOW_RX_RATE,
                         px_cfg->show_rx_rate);
}

static void
watcher_cfg_dump(shr_pb_t *pb,  watcher_data_t *wdata)
{
    shr_pb_printf(pb, "\nWatcher Debug Configuration:\n");
    shr_pb_printf(pb, "\tShowPacketData: %s\n",
                  (wdata->debug_mode & WATCHER_DEBUG_SHOW_PACKET_DATA) ? "True" : "False");
    shr_pb_printf(pb, "\tShowMetaData: %s\n",
                  (wdata->debug_mode & WATCHER_DEBUG_SHOW_META_DATA) ? "True" : "False");
}

static int
watcher_create(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    int unit = 0;
    int netif_id = BCMA_BCMPKT_NETIF_ALL;
    watcher_data_t *wdata = NULL, *pos;
    bcma_cli_parse_table_t pt;
    px_watcher_t px_cfg;
    int rv;
    shr_pb_t *pb;

    wdata = sal_alloc(sizeof(watcher_data_t), "bcmaCliRxWatcherCreate");
    if (wdata == NULL) {
        cli_out("Allocate wdata failed\n");
        return SYS_ERR;
    }
    sal_memset(wdata, 0, sizeof(*wdata));
    wdata->netif_id = netif_id;
    /* Show Packet raw data by default */
    wdata->debug_mode = WATCHER_DEBUG_SHOW_PACKET_DATA;

    /* Not terminate nay packet by default */
    wdata->term_netif = -1;
    wdata->term_vlan = -1;
    wdata->term_match_id = -1;

    bcma_cli_parse_table_init(cli, &pt);
    bcma_bcmpkt_parse_data_add();
    watcher_cfg_api2px(&wdata->debug_mode, &px_cfg);
    bcma_cli_parse_table_add(&pt, "ShowPacketData", "bool",
                             &px_cfg.show_pkt_data, NULL);
    bcma_cli_parse_table_add(&pt, "ShowMetaData", "bool",
                             &px_cfg.show_meta_data, NULL);
    bcma_cli_parse_table_add(&pt, "LoopbackData", "bool",
                             &px_cfg.lb_pkt_data, NULL);

    rv = bcma_cli_parse_table_do_args(&pt, args);
    bcma_cli_parse_table_done(&pt);
    if (rv < 0) {
        cli_out("%s: Invalid option: %s\n",
                BCMA_CLI_ARG_CMD(args), BCMA_CLI_ARG_CUR(args));
        sal_free(wdata);
        return BCMA_CLI_CMD_USAGE;
    }

    watcher_cfg_px2api(&px_cfg, &wdata->debug_mode);
    pb = shr_pb_create();
    watcher_cfg_dump(pb, wdata);
    um_console_print(shr_pb_str(pb));
    shr_pb_destroy(pb);

    for (pos = wdata_list[unit]; pos != NULL; pos = pos->next){
        if (pos->netif_id == netif_id) {
            cli_out("RX watcher already exists\n");
            sal_free(wdata);
            return BCMA_CLI_CMD_OK;
        }
    }

    rv = sys_rx_register(
                bcma_bcmpkt_watcher,
                CFG_CLI_RX_MON_PRIORITY,
                wdata,
                SYS_RX_REGISTER_FLAG_ACCEPT_PKT_ERRORS |
                SYS_RX_REGISTER_FLAG_ACCEPT_TRUNCATED_PKT
                );
    if (rv < 0) {
        cli_out("Create RX watcher failed (%d)\n", rv);
        sal_free(wdata);
        return BCMA_CLI_CMD_FAIL;
    }

    wdata->next = NULL;
    if (wdata_list[unit] == NULL) {
        wdata_list[unit] = wdata;
    }
    else {
        pos = wdata_list[unit];
        while (pos->next != NULL) {
            pos = pos->next;
        }
        pos->next = wdata;
    }
    cli_out("Create RX watcher succeeded\n");

    return BCMA_CLI_CMD_OK;
}

static int
watcher_destroy(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    int unit = 0;
    int netif_id = BCMA_BCMPKT_NETIF_ALL;

    bcma_bcmpktcmd_watcher_destroy(unit, netif_id);

    return BCMA_CLI_CMD_OK;
}

static int
watcher_output(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    bcma_cli_parse_table_t pt;
    int file_en = 0; /* No file logging by default */
    int console_en = 1; /* Enable console display by default */
    int rv = SYS_OK;

    bcma_cli_parse_table_init(cli, &pt);
    bcma_bcmpkt_parse_data_add();
    bcma_cli_parse_table_add(&pt, "ConsoleEnable", "bool",
                             &console_en, NULL);

    rv = bcma_cli_parse_table_do_args(&pt, args);
    bcma_cli_parse_table_done(&pt);
    if (rv < 0) {
        cli_out("%s: Invalid option: %s\n",
                BCMA_CLI_ARG_CMD(args), BCMA_CLI_ARG_CUR(args));
        return BCMA_CLI_CMD_USAGE;
    }

    rv = bcma_bcmpkt_rx_watch_output_enable(file_en, console_en);
    if (rv < 0) {
        return BCMA_CLI_CMD_FAIL;
    }

    return BCMA_CLI_CMD_OK;
}

static int
watcher_cmd(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    char* cmd;

    if (!(cmd = BCMA_CLI_ARG_GET(args))) {
        return BCMA_CLI_CMD_USAGE;
    }

    if (bcma_cli_parse_cmp("Create", cmd, ' ')) {
        return watcher_create(cli, args);
    }
    if (bcma_cli_parse_cmp("Destroy", cmd, ' ')) {
        return watcher_destroy(cli, args);
    }
    if (bcma_cli_parse_cmp("Output", cmd, ' ')) {
        return watcher_output(cli, args);
    }

    cli_out("%sUnknown RX Watcher command: %s\n", BCMA_CLI_CONFIG_ERROR_STR, cmd);
    return BCMA_CLI_CMD_FAIL;
}

static int
rx_vlan_add_cpu(bcma_cli_args_t *args, int remove)
{
#ifdef CFG_CMICX_SUPPORT
    char *cmd;
    int unit = 0, vlan_id = 0;
    watcher_data_t *pos = wdata_list[unit];

    if (remove) {
        if (!pos) {
            return BCMA_CLI_CMD_OK;
        }
        vlan_id = pos->vlan_id;
    } else {
        if (!(cmd = BCMA_CLI_ARG_GET(args))) {
            if (pos) {
                sal_printf("%s: vlan = %d.\n", __func__, pos->vlan_id);
            }
            return BCMA_CLI_CMD_OK;
        }

        vlan_id = sal_strtol(cmd, &cmd, 0);
        if (*cmd != 0 || vlan_id <= 0) {
            return BCMA_CLI_CMD_FAIL;
        }
    }

    if (vlan_id > 0) {
        if (remove) {
            board_qvlan_remove_cpu(vlan_id);
        } else {
            board_qvlan_add_cpu(vlan_id, 1);
        }
        sal_printf("%s: %s cpu port for vlan %d.\n",
                __func__, (remove == 1)? "remove":"add", vlan_id);
    }


    if (pos) {
        if (remove) {
            pos->vlan_id = 0;
        } else {
            pos->vlan_id = vlan_id;
        }
    }
#endif /* CFG_CMICX_SUPPORT */

    return BCMA_CLI_CMD_OK;
}

int
bcma_bcmpktcmd_rx(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    char* cmd;

    if (!(cmd = BCMA_CLI_ARG_GET(args))) {
        return BCMA_CLI_CMD_USAGE;
    }

    if (bcma_cli_parse_cmp("Watcher", cmd,  ' ')) {
        return watcher_cmd(cli, args);
    }

    if (bcma_cli_parse_cmp("AddCpu2Vlan", cmd,  ' ')) {
        return rx_vlan_add_cpu(args, 0);
    }

    cli_out("%sUnknown RX command: %s\n", BCMA_CLI_CONFIG_ERROR_STR, cmd);

    return BCMA_CLI_CMD_FAIL;
}

void
bcma_bcmpktcmd_watcher_destroy(int unit, int netif_id)
{
    int rv;
    watcher_data_t *pos, *pre;

    for (pre = wdata_list[unit], pos = pre; pos != NULL;) {
        if (netif_id == -1) { /* Delete all watchers. */
            pre = pos;
            rv = sys_rx_unregister(bcma_bcmpkt_watcher);
            if (rv < 0) {
                cli_out("Destroy watcher failed\n");
            }

            rx_vlan_add_cpu(NULL, 1);
            pos = pos->next;
            sal_free(pre);
        } else { /* Not found, go to next. */
            pre = pos;
            pos = pos->next;
        }
    }

    if (netif_id == -1 && wdata_list[unit] != NULL) { /* Delete all watchers. */
        wdata_list[unit] = NULL;
    }

    rv = bcma_bcmpkt_rx_watch_output_enable(0, 1);
    if (rv < 0) {
        cli_out("Failed to disable RX watcher log file(%d).\n", rv);
    }
}
#endif
#endif
