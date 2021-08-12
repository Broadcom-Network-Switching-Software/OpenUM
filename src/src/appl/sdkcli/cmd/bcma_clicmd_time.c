/*! \file bcma_clicmd_time.c
 *
 * CLI 'time synce' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_INCLUDED
#if defined(CFG_SWITCH_SYNCE_INCLUDED) || defined(CFG_SWITCH_TIMESYNC_INCLUDED)

#include <appl/sdkcli/bcma_cli.h>
#include <appl/sdkcli/bcma_cli_parse.h>

#include <appl/sdkcli/bcma_clicmd_time.h>

#ifdef CFG_SWITCH_TIMESYNC_INCLUDED
#define MAX_TXINFO_FIFO_NUM   4
#undef _SOC_PHYCTRL_H_
#include <soc/phyctrl.h>
extern sys_error_t board_qvlan_add_cpu(uint16 vlanid, int tagged);
extern sys_error_t board_qvlan_remove_cpu(uint16 vlanid);
#endif

#ifdef CFG_SWITCH_SYNCE_INCLUDED

static bcma_cli_parse_enum_t freq_str[] = {
    { NULL, 0 },
    { "TSCF - 23.4375 MHz", bcmTimeSyncE23MHz },
    { "SDM - 25MHz", bcmTimeSyncE25MHz },
    { "General - 28.4 MHz", bcmTimeSyncE28MHz },
    { "General - 46.875 MHz", bcmTimeSyncE46MHz },
    { "General - 125MHz", bcmTimeSyncE125MHz },
    { "FGeneral - 515.625MHz", bcmTimeSyncE515MHz },
    { "SDM - 7.8125 MHz", bcmTimeSyncE7MHz },
    { "SDM - 12.5 MHz", bcmTimeSyncE12MHz },
    { "SDM - 31.25 MHz", bcmTimeSyncE31MHz },
    { "SDM - 37.5 MHz", bcmTimeSyncE37MHz },
    { "General - 156.25 MHz", bcmTimeSyncE156MHz },
    { NULL, 0 }
};

static bcma_cli_parse_enum_t en_str[] = {
    { "DISable", 0 },
    { "ENable", 1 },
    { NULL, 0 }
};

int
bcma_clicmd_synce(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    char *arg;
    uint8 unit = 0;
    int clk_control = 0;
    bcm_time_synce_clock_source_config_t clk_src_config;
    int set_val, get_val;
    int rv = 0;
    uint8 lport;

    COMPILER_REFERENCE(cli);

    if (BCMA_CLI_ARG_CNT(args) == 0) {
        return BCMA_CLI_CMD_USAGE;
    }

    /* First parameter only can be Frequency or Squelch */
    arg = BCMA_CLI_ARG_GET(args);
    if (bcma_cli_parse_cmp("Freq", arg,  ' ')) {
        /* freq(Frequency) */
        clk_control = 1;
        /* Pri(Primary) or Sec(Secondary) */
        if (BCMA_CLI_ARG_CNT(args) == 0) {
            return BCMA_CLI_CMD_USAGE;
        }
        arg = BCMA_CLI_ARG_GET(args);
        if (bcma_cli_parse_cmp("Primary", arg,  ' ')) {
            /* bcmTimeSynceClockSourcePrimary */
            clk_src_config.clk_src = 0;
        } else if (bcma_cli_parse_cmp("Secondary", arg,  ' ')) {
            /* bcmTimeSynceClockSourceSecondary */
            clk_src_config.clk_src = 1;
        } else {
            return BCMA_CLI_CMD_FAIL;
        }
        if (BCMA_CLI_ARG_CNT(args) == 0) {
            /* Get value*/
            rv = board_time_synce_clock_source_control_get(&clk_src_config,
                clk_control, &get_val);
            if (rv != 0) {
                return BCMA_CLI_CMD_FAIL;
            }
            if (get_val > COUNTOF(freq_str) - 1) {
                return BCMA_CLI_CMD_FAIL;
            } else if (freq_str[get_val].name == NULL) {
                return BCMA_CLI_CMD_FAIL;
            }

            /* bcmTimeSynceClockSourceControlFrequency */
            sal_printf("Frequency get_MHz = %d(%s)\n", get_val,
                freq_str[get_val].name);
            return BCMA_CLI_CMD_OK;
        }

        /* Set value */
        /* PORT(SrcTypePort) or PLL(SrcTypePLL), default=PORT */
        arg = BCMA_CLI_ARG_GET(args);
        if (bcma_cli_parse_cmp("PLL", arg,  ' ')) {
            /* bcmTimeSynceClockSourceSecondary */
            sal_printf("Firelight doesn't support PLL source type.\n");
            return BCMA_CLI_CMD_FAIL;
        } else if (bcma_cli_parse_cmp("PORT", arg,  ' ')) {
            int get_uport;

            /* bcmTimeSynceInputSourceTypePort and default */
            clk_src_config.input_src = bcmTimeSynceInputSourceTypePort;
            /* lport */
            /* enable or disable) */
            arg = BCMA_CLI_ARG_GET(args);
            if ((arg == NULL) || (bcma_cli_parse_int(arg, &get_uport) < 0)) {
                return BCMA_CLI_CMD_USAGE;
            } else {
                board_uport_to_lport(get_uport, &unit, &lport);
                clk_src_config.port = lport;
            }
            /* Frequency range=1~11 */
            arg = BCMA_CLI_ARG_GET(args);
            if (bcma_cli_parse_int(arg, &set_val) != -1 &&
                (set_val >= 1 && set_val <= 11)) {
            } else {
                return BCMA_CLI_CMD_FAIL;
            }
            board_time_synce_clock_source_control_set(&clk_src_config,
            clk_control, set_val);
        } else {
            return BCMA_CLI_CMD_FAIL;
        }
    } else if (bcma_cli_parse_cmp("Squelch", arg,  ' ')) {
        /* Squelch */
        clk_control = 0;
        /* Primary or Secondary */
        if (BCMA_CLI_ARG_CNT(args) == 0) {
            return BCMA_CLI_CMD_USAGE;
        }
        arg = BCMA_CLI_ARG_GET(args);
        if (bcma_cli_parse_cmp("Primary", arg,  ' ')) {
            /* bcmTimeSynceClockSourcePrimary */
            clk_src_config.clk_src = 0;
        } else if (bcma_cli_parse_cmp("Secondary", arg,  ' ')) {
            /* bcmTimeSynceClockSourceSecondary */
            clk_src_config.clk_src = 1;
        } else {
            return BCMA_CLI_CMD_FAIL;
        }
        if (BCMA_CLI_ARG_CNT(args) == 0) {
            /* Get enable/disable */
            board_time_synce_clock_source_control_get(&clk_src_config,
                clk_control, &get_val);
            /* bcmTimeSynceClockSourceControlSquelch,
               get TOP_L1_RCVD_CLK_CONTROLr__L1_RCVD_SW_OVWR_ENf  */
            sal_printf("Squelch get_enable = %s\n", en_str[get_val].name);
        } else {
            /* Set enable/disable */
            arg = BCMA_CLI_ARG_GET(args);
            if (bcma_cli_parse_cmp("ENable", arg,  ' ')) {
                set_val = 1;
            } else if (bcma_cli_parse_cmp("DISable", arg,  ' ')) {
                set_val = 0;
            } else {
                return BCMA_CLI_CMD_FAIL;
            }
            board_time_synce_clock_source_control_set(&clk_src_config,
                clk_control, set_val);
        }
    } else {
        return BCMA_CLI_CMD_FAIL;
    }

    return BCMA_CLI_CMD_OK;
}
#endif /* CFG_SWITCH_SYNCE_INCLUDED */

#ifdef CFG_SWITCH_TIMESYNC_INCLUDED

/*!
 * \brief TimeSynctest command structure.
 */
typedef struct clicmd_timesync_cmd_s {

    /*! Command name. */
    const char *name;

    /*! Command function. */
    int (*func)(bcma_cli_t *cli, bcma_cli_args_t *arg, int port);

} clicmd_timesync_cmd_t;

static const char *str_indent_lv1 = "|-- ";
static const char *str_indent_lv2 = "    |-- ";

/* TimeSync mode */
static bcma_cli_parse_enum_t tc_mode[] = {
    {"NONE", 0},
    {"1step", BCM_PORT_TIMESYNC_ONE_STEP_TIMESTAMP},
    {"2step", BCM_PORT_TIMESYNC_TWO_STEP_TIMESTAMP},
    {NULL, 0}
};

static const char *
cli_enum_str_get(bcma_cli_parse_enum_t *map, int val)
{
    int idx = 0;

    while (map[idx].name) {
        if ((val & (BCM_PORT_TIMESYNC_ONE_STEP_TIMESTAMP |
                    BCM_PORT_TIMESYNC_TWO_STEP_TIMESTAMP)) == map[idx].val) {
            return map[idx].name;
        }
        idx++;
    }

    return "unknown";
}

static int
timesync_tc_show(int port)
{
    int rv, array_count;
    bcm_port_timesync_config_t ts_config;

    rv = board_port_timesync_config_get(port, 1, &ts_config, &array_count);
    if ((rv != SYS_OK) || (array_count != 1)) {
        sal_printf("%sboard_port_timesync_config_get() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    sal_printf("%sTC Config:\n", str_indent_lv1);
    sal_printf("%sMode: %s\n", str_indent_lv2,
               cli_enum_str_get(tc_mode, ts_config.flags));

    sal_printf("\n");

    return BCMA_CLI_CMD_OK;
}

static void
phy_timesync_enable_set(int port, int en)
{
    sys_error_t rv = SYS_OK;
    int enable;
    /*
     * For single-lane port, select ModeNone.
     * For multi-lane port, select ModeEarliestLane or ModeLatestlane.
     */
    uint64 am_norm_mode = bcmPortPhyTimesyncCompensationModeNone;
    /* Suggested offset to be added to timestamp. */
    uint64 ts_offset = 0x1C;

    if (en) {
        /* Should be called after link is up first time. */
        rv = board_port_control_phy_timesync_set(port,
                    bcmPortControlPhyTimesyncTimestampOffset, ts_offset);
        if (rv != SYS_OK) {
            sal_printf("%sboard_port_control_phy_timesync_set() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
            return;
        }

        rv = board_port_control_phy_timesync_set(port,
                        bcmPortControlPhyTimesyncTimestampAdjust, am_norm_mode);
        if (rv != SYS_OK) {
            sal_printf("%sboard_port_control_phy_timesync_set() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
            return;
        }

        rv = board_port_phy_timesync_enable_set(port, 1);
        if (rv != SYS_OK) {
            sal_printf("%sboard_port_phy_timesync_enable_set() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
            return;
        }
        enable = 0;
        rv = board_port_phy_timesync_enable_get(port, &enable);
        if ((rv != SYS_OK) || (enable != 1)) {
           sal_printf("%sboard_port_phy_timesync_enable_get() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
            return;
        }
    } else {
        rv = board_port_phy_timesync_enable_set(port, 0);
        if (rv != SYS_OK) {
            sal_printf("%sboard_port_phy_timesync_enable_set() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
            return;
        }
        enable = 1;
        rv = board_port_phy_timesync_enable_get(port, &enable);
        if ((rv != SYS_OK) || (enable != 0)) {
            sal_printf("%sboard_port_phy_timesync_enable_get() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
            return;
        }
    }
}

static int
timesync_cmd_tc(bcma_cli_t *cli, bcma_cli_args_t *args, int port)
{
    int rv, mode = -1, old_mode, array_count, pcs_ts = 0;
    uint8 unit, lport;
    bcma_cli_parse_table_t pt;
    bcm_port_timesync_config_t ts_config;

   if (BCMA_CLI_ARG_CNT(args) == 0) {
        sal_printf("Port %d:\n", port);
        return timesync_tc_show(port);
    }

    bcma_cli_parse_table_init(cli, &pt);
    bcma_cli_parse_table_add(&pt, "Mode", "enum", &mode, tc_mode);
    rv = bcma_cli_parse_table_do_args(&pt, args);
    bcma_cli_parse_table_done(&pt);
    if (rv < 0) {
        sal_printf("%s: Invalid option: %s\n",
                BCMA_CLI_ARG_CMD(args), BCMA_CLI_ARG_CUR(args));
        return BCMA_CLI_CMD_FAIL;
    }

    if (mode == -1) {
        return BCMA_CLI_CMD_USAGE;
    }

    sal_memset(&ts_config, 0 , sizeof(bcm_port_timesync_config_t));
    rv = board_port_timesync_config_get(port, 1, &ts_config, &array_count);
    if ((rv != SYS_OK) || (array_count != 1)) {
        sal_printf("%sboard_port_timesync_config_get() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    old_mode = (ts_config.flags & (BCM_PORT_TIMESYNC_ONE_STEP_TIMESTAMP |
                                    BCM_PORT_TIMESYNC_TWO_STEP_TIMESTAMP));

    if (mode == old_mode) {
        return BCMA_CLI_CMD_OK;
    }

    board_uport_to_lport(port, &unit, &lport);
    if (sal_strstr(soc_phy_name_get(unit, lport), "TSCF16_GEN3")) {
        pcs_ts = 1;
    }

    if (old_mode) {
        board_port_timesync_config_set(port, 0, NULL);
        if (pcs_ts) {
            phy_timesync_enable_set(port, 0);
        }
        if (!mode) {
            return BCMA_CLI_CMD_OK;
        }
    }

    sal_memset(&ts_config, 0 , sizeof(bcm_port_timesync_config_t));
    /* Enabling timesync for the port */
    ts_config.flags |= BCM_PORT_TIMESYNC_DEFAULT;
    if (mode & BCM_PORT_TIMESYNC_TWO_STEP_TIMESTAMP) {
        ts_config.flags |= BCM_PORT_TIMESYNC_TWO_STEP_TIMESTAMP;
    } else if (mode & BCM_PORT_TIMESYNC_ONE_STEP_TIMESTAMP) {
        ts_config.flags |= BCM_PORT_TIMESYNC_ONE_STEP_TIMESTAMP;
    }

    rv = board_port_timesync_config_set(port, 1, &ts_config);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_timesync_config_set() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    if (pcs_ts) {
        phy_timesync_enable_set(port, 1);
    }

    return BCMA_CLI_CMD_OK;
}

static int
timesync_txinfo_show(int port)
{
    int i, rv;
    bcm_port_timesync_tx_info_t tx_info;

    sal_printf("%sTX info:\n", str_indent_lv1);

    for (i = 0; i < MAX_TXINFO_FIFO_NUM; i++) {
        rv = board_port_timesync_tx_info_get(port, &tx_info);
        if (rv == SYS_OK) {
            sal_printf("%sSequence id: %d\n", str_indent_lv2, tx_info.sequence_id);
            sal_printf("%sTimestamp ns: %llu\n", str_indent_lv2, tx_info.timestamp);
            sal_printf("%sTimestamp sub_ns: %llu\n", str_indent_lv2, tx_info.sub_ns);
        }
    }

    sal_printf("\n");

    return BCMA_CLI_CMD_OK;
}

static int
timesync_cmd_txinfo(bcma_cli_t *cli, bcma_cli_args_t *args, int port)
{

    if (BCMA_CLI_ARG_CNT(args) != 0) {
        return BCMA_CLI_CMD_USAGE;
    }

    sal_printf("Port %d:\n", port);

    return timesync_txinfo_show(port);
}

int
bcma_clicmd_timesync_test(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    int rv;
    static int pi = 0, pe = 0;
    uint8 uplist[MAX_UPLIST_WIDTH];
    uint8 tag_uplist[MAX_UPLIST_WIDTH];
    bcma_cli_parse_table_t pt;
    const char *arg;

    if (BCMA_CLI_ARG_CNT(args) == 0) {
        if (!SAL_UPORT_IS_NOT_VALID(pi) && !SAL_UPORT_IS_NOT_VALID(pe)) {
            sal_printf("Testing pi: %d and pe: %d\n", pi, pe);
        } else {
            sal_printf("Not configured\n");
        }
        return BCMA_CLI_CMD_OK;
    }

    if (BCMA_CLI_ARG_CNT(args) == 1) {
        arg = BCMA_CLI_ARG_GET(args);
        if (arg && sal_strcmp("clear", arg) == 0) {
            if (!SAL_UPORT_IS_NOT_VALID(pi) && !SAL_UPORT_IS_NOT_VALID(pe)) {
                board_port_loopback_set(pi, PORT_LOOPBACK_NONE);
                board_port_loopback_set(pe, PORT_LOOPBACK_NONE);
                board_untagged_vlan_set(pi, 1);
                board_untagged_vlan_set(pe, 1);
                board_vlan_destroy(2);
                board_vlan_destroy(3);
                board_qvlan_remove_cpu(3);
                pi = pe = 0;
            }
            return BCMA_CLI_CMD_OK;
        }
        return BCMA_CLI_CMD_USAGE;
    }

    bcma_cli_parse_table_init(cli, &pt);
    bcma_cli_parse_table_add(&pt, "PortIngress", "int",
                             &pi, NULL);
    bcma_cli_parse_table_add(&pt, "PortEgress", "int",
                             &pe, NULL);
    rv = bcma_cli_parse_table_do_args(&pt, args);
    bcma_cli_parse_table_done(&pt);
    if (rv < 0) {
        sal_printf("%s: Invalid option: %s\n",
                BCMA_CLI_ARG_CMD(args), BCMA_CLI_ARG_CUR(args));
        return BCMA_CLI_CMD_USAGE;
    }

    if (SAL_UPORT_IS_NOT_VALID(pi) || SAL_UPORT_IS_NOT_VALID(pe)) {
        sal_printf("%sInvalid port number pi=%d, pe=%d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, pi, pe);
        return BCMA_CLI_CMD_USAGE;
    }

    /* Setup VLAN and PVID for testing with CPU tx and rx. */
    board_port_loopback_set(pi, PORT_LOOPBACK_PHY);
    board_port_loopback_set(pe, PORT_LOOPBACK_PHY);
    board_vlan_create(2);
    uplist_clear(tag_uplist);
    uplist_clear(uplist);
    uplist_port_add(uplist, pi);
    uplist_port_add(uplist, pe);
    board_qvlan_port_set(2, uplist, tag_uplist);

    board_untagged_vlan_set(pi, 2);

    board_vlan_create(3);
    uplist_clear(tag_uplist);
    uplist_clear(uplist);
    uplist_port_add(uplist, pe);
    uplist_port_add(tag_uplist, pe);
    board_qvlan_port_set(3, uplist, tag_uplist);
    /* Add CPU to VLAN 3 */
    board_qvlan_add_cpu(3, 0);

    board_untagged_vlan_set(pe, 3);

    return BCMA_CLI_CMD_OK;
}

/* Sub-command list */
static clicmd_timesync_cmd_t timesync_cmds[] = {
    { "TC", timesync_cmd_tc },
    { "TxInfo", timesync_cmd_txinfo },
};

int
bcma_clicmd_timesync(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    int idx, uport;
    const char *arg;

    if (!cli) {
        return BCMA_CLI_CMD_FAIL;
    }

    /* Require a port number at least. */
    if (BCMA_CLI_ARG_CNT(args) < 1) {
        return BCMA_CLI_CMD_USAGE;
    }

    /* Get port number. */
    arg = BCMA_CLI_ARG_GET(args);
    if (bcma_cli_parse_int(arg, &uport) < 0) {
        sal_printf("%sInvalid port number %s.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, arg);
        return BCMA_CLI_CMD_BAD_ARG;
    }
    if (SAL_UPORT_IS_NOT_VALID(uport)) {
        sal_printf("%sInvalid port number %s.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, arg);
        return BCMA_CLI_CMD_BAD_ARG;
    }

    /* Get subcommand. */
    arg = BCMA_CLI_ARG_GET(args);

    if (arg) {
        for (idx = 0; idx < COUNTOF(timesync_cmds); idx++) {
            if (bcma_cli_parse_cmp(timesync_cmds[idx].name, arg, '\0')) {
                return (*timesync_cmds[idx].func)(cli, args, uport);
            }
        }
    } else {
        sal_printf("Port %d:\n", uport);
        timesync_tc_show(uport);
        timesync_txinfo_show(uport);
        return BCMA_CLI_CMD_OK;
    }

    return BCMA_CLI_CMD_USAGE;
}
#endif /* CFG_SWITCH_TIMESYNC_INCLUDED */

#endif /* CFG_SWITCH_SYNCE_INCLUDED || CFG_SWITCH_TIMESYNC_INCLUDED */
#endif /* CFG_SDKCLI_INCLUDED */
