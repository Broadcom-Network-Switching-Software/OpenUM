/*! \file bcma_clicmd_porttest.c
 *
 * CLI 'porttest' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"

#if defined(CFG_SDKCLI_INCLUDED) && defined(CFG_SDKCLI_PHY_INCLUDED)

#include <appl/sdkcli/bcma_cli.h>
#include <appl/sdkcli/bcma_cli_parse.h>
#include <appl/sdkcli/bcma_clicmd_porttest.h>

/*!
 * \brief Porttest command structure.
 */
typedef struct clicmd_porttest_cmd_s {

    /*! Command name. */
    const char *name;

    /*! Command function. */
    int (*func)(bcma_cli_t *cli, bcma_cli_args_t *arg, int port);

} clicmd_porttest_cmd_t;

static const char *str_indent_lv1 = "|-- ";
static const char *str_indent_lv2 = "    |-- ";

static bcma_cli_parse_enum_t cli_fec_map[] = {
    { "None", BOARD_PHY_FEC_NONE },
    { "CL74", BOARD_PHY_FEC_CL74, },
    { "CL91", BOARD_PHY_FEC_CL91 },
    { "CL108", BOARD_PHY_FEC_CL108 },
    { NULL, 0 }
};

static bcma_cli_parse_enum_t cli_prbs_map[] = {
    { "Poly7", BOARD_PHY_PRBS_POLY_7 },
    { "Poly9", BOARD_PHY_PRBS_POLY_9, },
    { "Poly11", BOARD_PHY_PRBS_POLY_11 },
    { "Poly15", BOARD_PHY_PRBS_POLY_15 },
    { "Poly23", BOARD_PHY_PRBS_POLY_23 },
    { "Poly31", BOARD_PHY_PRBS_POLY_31 },
    { "Poly58", BOARD_PHY_PRBS_POLY_58 },
    { "Poly49", BOARD_PHY_PRBS_POLY_49 },
    { NULL, 0 }
};

static const char *
cli_enum_str_get(bcma_cli_parse_enum_t *map, int val)
{
    int idx = 0;

    while (map[idx].name) {
        if (val == map[idx].val) {
            return map[idx].name;
        }
        idx++;
    }

    return "unknown";
}

static int
porttest_fault_show(int port)
{
    int rv;
    board_port_fault_ctrl_t ctrl;
    board_port_fault_st_t st;

    rv = board_port_fault_ctrl_get(port, &ctrl);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_fault_ctrl_get() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    rv = board_port_fault_status_get(port, &st);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_fault_status_get() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    sal_printf("Port %d:\n", port);
    sal_printf("%sConfig:\n", str_indent_lv1);
    sal_printf("%sLocal fault: %d\n", str_indent_lv2, ctrl.local_fault);
    sal_printf("%sRemote fault: %d\n", str_indent_lv2, ctrl.remote_fault);
    sal_printf("%sForced remote fault: %d\n",
               str_indent_lv2, ctrl.forced_remote_fault);
    sal_printf("%sStatus:\n", str_indent_lv1);
    sal_printf("%sLocal fault: %d\n", str_indent_lv2, st.local_fault);
    sal_printf("%sRemote fault: %d\n", str_indent_lv2, st.remote_fault);

    return BCMA_CLI_CMD_OK;
}

static int
porttest_fec_show(int port)
{
    int rv;
    board_phy_fec_mode_t mode;
    board_phy_fec_st_t st;

    sal_memset(&st, 0, sizeof(st));

    rv = board_port_phy_fec_mode_get(port, &mode);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_phy_fec_mode_get() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    /* Get FEC counters when FEC is enabled */
    if (mode != BOARD_PHY_FEC_NONE) {
        rv = board_port_phy_fec_status_get(port, &st);
        if (rv != SYS_OK) {
            sal_printf("%sboard_port_phy_fec_status_get() failed: %d.\n",
                       BCMA_CLI_CONFIG_ERROR_STR, rv);
            return BCMA_CLI_CMD_FAIL;
        }
    }

    sal_printf("Port %d:\n", port);
    sal_printf("%sConfig:\n", str_indent_lv1);
    sal_printf("%sMode: %s\n", str_indent_lv2,
               cli_enum_str_get(cli_fec_map, mode));
    sal_printf("%sStatus:\n", str_indent_lv1);
    sal_printf("%sCorrected blocks: %d\n",
               str_indent_lv2, st.corrected_blocks);
    sal_printf("%sUncorrected blocks: %d\n",
               str_indent_lv2, st.uncorrected_blocks);
    sal_printf("%sCorrected codewords: %d\n",
               str_indent_lv2, st.corrected_cws);
    sal_printf("%sUncorrected codewords: %d\n",
               str_indent_lv2, st.uncorrected_cws);

    return BCMA_CLI_CMD_OK;
}

static int
porttest_prbs_show(int port)
{
    int rv, flags, en_tx, en_rx;
    board_phy_prbs_ctrl_t ctrl_tx, ctrl_rx;
    board_phy_prbs_st_t st;

    flags = BOARD_PHY_PRBS_CTRL_F_TX;
    rv = board_port_phy_prbs_enable_get(port, flags, &en_tx);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_phy_prbs_enable_get() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }
    rv = board_port_phy_prbs_ctrl_get(port, flags, &ctrl_tx);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_phy_prbs_ctrl_get() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    flags = BOARD_PHY_PRBS_CTRL_F_RX;
    rv = board_port_phy_prbs_enable_get(port, flags, &en_rx);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_phy_prbs_enable_get() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }
    rv = board_port_phy_prbs_ctrl_get(port, flags, &ctrl_rx);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_phy_prbs_ctrl_get() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    rv = board_port_phy_prbs_status_get(port, &st);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_phy_prbs_status_get() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    sal_printf("Port %d:\n", port);
    sal_printf("%sTx config:\n", str_indent_lv1);
    sal_printf("%sEnable: %d\n", str_indent_lv2, en_tx);
    sal_printf("%sPolynomial: %s\n", str_indent_lv2,
               cli_enum_str_get(cli_prbs_map, ctrl_tx.poly));
    sal_printf("%sInvert: %d\n", str_indent_lv2, ctrl_tx.invert);
    sal_printf("%sRx config:\n", str_indent_lv1);
    sal_printf("%sEnable: %d\n", str_indent_lv2, en_rx);
    sal_printf("%sPolynomial: %s\n", str_indent_lv2,
               cli_enum_str_get(cli_prbs_map, ctrl_rx.poly));
    sal_printf("%sInvert: %d\n", str_indent_lv2, ctrl_rx.invert);
    sal_printf("%sStatus:\n", str_indent_lv1);
    sal_printf("%sLock: %d\n", str_indent_lv2, st.prbs_lock);
    sal_printf("%sLock loss: %d\n", str_indent_lv2, st.prbs_lock_loss);
    sal_printf("%sError count: %d\n", str_indent_lv2, st.error_count);

    return BCMA_CLI_CMD_OK;
}

static int
porttest_pwr_show(int port)
{
    int rv, pwr;

    rv = board_port_phy_power_get(port, &pwr);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_phy_power_get() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    sal_printf("Port %d:\n", port);
    sal_printf("%sPower on: %d\n", str_indent_lv1, pwr);

    return BCMA_CLI_CMD_OK;
}

static int
porttest_txfir_show(int port)
{
    int rv;
    board_phy_tx_ctrl_t ctrl;

    rv = board_port_phy_tx_ctrl_get(port, &ctrl);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_phy_tx_ctrl_set() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    sal_printf("Port %d:\n", port);
    sal_printf("%sConfig:\n", str_indent_lv1);
    sal_printf("%sPre: %d\n", str_indent_lv2, ctrl.pre);
    sal_printf("%sMain: %d\n", str_indent_lv2, ctrl.main);
    sal_printf("%sPost: %d\n", str_indent_lv2, ctrl.post);
    sal_printf("%sPost2: %d\n", str_indent_lv2, ctrl.post2);
    sal_printf("%sPost3: %d\n", str_indent_lv2, ctrl.post3);

    return BCMA_CLI_CMD_OK;
}

static int
porttest_cmd_fault(bcma_cli_t *cli, bcma_cli_args_t *args, int port)
{
    int rv;
    board_port_fault_ctrl_t ctrl;
    bcma_cli_parse_table_t pt;

    if (BCMA_CLI_ARG_CNT(args) == 0) {
        return porttest_fault_show(port);
    }

    rv = board_port_fault_ctrl_get(port, &ctrl);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_fault_ctrl_get() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    bcma_cli_parse_table_init(cli, &pt);
    bcma_cli_parse_table_add(&pt, "LocalFault", "bool",
                             &ctrl.local_fault, NULL);
    bcma_cli_parse_table_add(&pt, "RemoteFault", "bool",
                             &ctrl.remote_fault, NULL);
    bcma_cli_parse_table_add(&pt, "ForcedRemoteFault", "bool",
                             &ctrl.forced_remote_fault, NULL);

    rv = bcma_cli_parse_table_do_args(&pt, args);
    bcma_cli_parse_table_done(&pt);
    if (rv < 0) {
        sal_printf("%s: Invalid option: %s\n",
                BCMA_CLI_ARG_CMD(args), BCMA_CLI_ARG_CUR(args));
        return BCMA_CLI_CMD_FAIL;
    }

    rv = board_port_fault_ctrl_set(port, &ctrl);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_fault_ctrl_set() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    return BCMA_CLI_CMD_OK;
}

static int
porttest_cmd_fec(bcma_cli_t *cli, bcma_cli_args_t *args, int port)
{
    int rv;
    board_phy_fec_mode_t mode = BOARD_PHY_FEC_NONE;
    bcma_cli_parse_table_t pt;

    if (BCMA_CLI_ARG_CNT(args) == 0) {
        return porttest_fec_show(port);
    }

    bcma_cli_parse_table_init(cli, &pt);
    bcma_cli_parse_table_add(&pt, "Mode", "enum", &mode, cli_fec_map);

    rv = bcma_cli_parse_table_do_args(&pt, args);
    bcma_cli_parse_table_done(&pt);
    if (rv < 0) {
        sal_printf("%s: Invalid option: %s\n",
                BCMA_CLI_ARG_CMD(args), BCMA_CLI_ARG_CUR(args));
        return BCMA_CLI_CMD_FAIL;
    }

    rv = board_port_phy_fec_mode_set(port, mode);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_phy_fec_mode_set() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    return BCMA_CLI_CMD_OK;
}

static int
porttest_cmd_prbs(bcma_cli_t *cli, bcma_cli_args_t *args, int port)
{
    const char *arg;
    int rv, flags, en;
    board_phy_prbs_ctrl_t ctrl;
    bcma_cli_parse_table_t pt;

    if (BCMA_CLI_ARG_CNT(args) == 0) {
        return porttest_prbs_show(port);
    }

    arg = BCMA_CLI_ARG_CUR(args);
    if (sal_strcmp("tx", arg) == 0) {
        flags = BOARD_PHY_PRBS_CTRL_F_TX;
        BCMA_CLI_ARG_NEXT(args);
    } else if (sal_strcmp("rx", arg) == 0) {
        flags = BOARD_PHY_PRBS_CTRL_F_RX;
        BCMA_CLI_ARG_NEXT(args);
    } else {
        flags = BOARD_PHY_PRBS_CTRL_F_TX | BOARD_PHY_PRBS_CTRL_F_RX;
    }

    rv = board_port_phy_prbs_enable_get(port, flags, &en);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_phy_prbs_enable_get() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }
    rv = board_port_phy_prbs_ctrl_get(port, flags, &ctrl);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_phy_prbs_ctrl_get() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    bcma_cli_parse_table_init(cli, &pt);
    bcma_cli_parse_table_add(&pt, "ENable", "bool", &en, NULL);
    bcma_cli_parse_table_add(&pt, "Polynomial", "enum", &ctrl.poly,
                             cli_prbs_map);
    bcma_cli_parse_table_add(&pt, "Invert", "bool", &ctrl.invert, NULL);

    rv = bcma_cli_parse_table_do_args(&pt, args);
    bcma_cli_parse_table_done(&pt);
    if (rv < 0) {
        sal_printf("%s: Invalid option: %s\n",
                BCMA_CLI_ARG_CMD(args), BCMA_CLI_ARG_CUR(args));
        return BCMA_CLI_CMD_FAIL;
    }

    rv = board_port_phy_prbs_enable_set(port, flags, en);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_phy_prbs_enable_set() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }
    rv = board_port_phy_prbs_ctrl_set(port, flags, &ctrl);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_phy_prbs_enable_set() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    return BCMA_CLI_CMD_OK;
}

static int
porttest_cmd_pwr(bcma_cli_t *cli, bcma_cli_args_t *args, int port)
{
    int rv, pwr = 0;
    bcma_cli_parse_table_t pt;

    if (BCMA_CLI_ARG_CNT(args) == 0) {
        return porttest_pwr_show(port);
    }

    bcma_cli_parse_table_init(cli, &pt);
    bcma_cli_parse_table_add(&pt, "ENable", "bool", &pwr, NULL);

    rv = bcma_cli_parse_table_do_args(&pt, args);
    bcma_cli_parse_table_done(&pt);
    if (rv < 0) {
        sal_printf("%s: Invalid option: %s\n",
                BCMA_CLI_ARG_CMD(args), BCMA_CLI_ARG_CUR(args));
        return BCMA_CLI_CMD_FAIL;
    }

    rv = board_port_phy_power_set(port, pwr);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_phy_power_set() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    return BCMA_CLI_CMD_OK;
}

static int
porttest_cmd_txfir(bcma_cli_t *cli, bcma_cli_args_t *args, int port)
{
    int rv;
    board_phy_tx_ctrl_t ctrl;
    bcma_cli_parse_table_t pt;

    if (BCMA_CLI_ARG_CNT(args) == 0) {
        return porttest_txfir_show(port);
    }

    rv = board_port_phy_tx_ctrl_get(port, &ctrl);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_phy_tx_ctrl_set() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    bcma_cli_parse_table_init(cli, &pt);
    bcma_cli_parse_table_add(&pt, "pre", "int", &ctrl.pre, NULL);
    bcma_cli_parse_table_add(&pt, "main", "int", &ctrl.main, NULL);
    bcma_cli_parse_table_add(&pt, "post", "int", &ctrl.post, NULL);
    bcma_cli_parse_table_add(&pt, "post2", "int", &ctrl.post2, NULL);
    bcma_cli_parse_table_add(&pt, "post3", "int", &ctrl.post3, NULL);

    rv = bcma_cli_parse_table_do_args(&pt, args);
    bcma_cli_parse_table_done(&pt);
    if (rv < 0) {
        sal_printf("%s: Invalid option: %s\n",
                BCMA_CLI_ARG_CMD(args), BCMA_CLI_ARG_CUR(args));
        return BCMA_CLI_CMD_FAIL;
    }

    rv = board_port_phy_tx_ctrl_set(port, &ctrl);
    if (rv != SYS_OK) {
        sal_printf("%board_port_phy_tx_ctrl_set() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    return BCMA_CLI_CMD_OK;
}

static int
porttest_cmd_status(bcma_cli_t *cli, bcma_cli_args_t *args, int port)
{
    int rv, idx;
    board_phy_rx_st_t rx_st;
    board_phy_stats_t stats;

    if (BCMA_CLI_ARG_CNT(args) != 0) {
        return BCMA_CLI_CMD_USAGE;
    }

    rv = board_port_phy_rx_status_get(port, &rx_st);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_phy_rx_status_get() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    rv = board_port_phy_stats_get(port, &stats);
    if (rv != SYS_OK) {
        sal_printf("%sboard_port_phy_stats_get() failed: %d.\n",
                   BCMA_CLI_CONFIG_ERROR_STR, rv);
        return BCMA_CLI_CMD_FAIL;
    }

    sal_printf("Port %d:\n", port);

    sal_printf("%sRx status:\n", str_indent_lv1);
    sal_printf("%sDFE: ", str_indent_lv2);
    for (idx = 0; idx < rx_st.num_of_dfe_taps; idx++) {
        sal_printf("%s%d", idx == 0 ? "" : ",", rx_st.dfe[idx]);
    }
    sal_printf("\n");
    sal_printf("%sVGA: %d\n", str_indent_lv2, rx_st.vga);
    sal_printf("%sPPM: %d\n", str_indent_lv2, rx_st.ppm);

    sal_printf("%sError statistics:\n", str_indent_lv1);
    sal_printf("%sBIP: ", str_indent_lv2);
    for (idx = 0; idx < COUNTOF(stats.bip_count); idx++) {
        sal_printf("%s%d", idx == 0 ? "" : ",", stats.bip_count[idx]);
    }
    sal_printf("\n");
    sal_printf("%sBER: ", str_indent_lv2);
    for (idx = 0; idx < COUNTOF(stats.ber_count); idx++) {
        sal_printf("%s%d", idx == 0 ? "" : ",", stats.ber_count[idx]);
    }
    sal_printf("\n");

    return BCMA_CLI_CMD_OK;
}

/* Sub-command list */
static clicmd_porttest_cmd_t porttest_cmds[] = {
    { "FAULT", porttest_cmd_fault },
    { "FEC", porttest_cmd_fec },
    { "PRBS", porttest_cmd_prbs },
    { "PoWeR", porttest_cmd_pwr },
    { "TXFIR", porttest_cmd_txfir },
    { "STATUS", porttest_cmd_status },
};

int
bcma_clicmd_porttest(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    int idx, uport;
    const char *arg;

    if (!cli) {
        return BCMA_CLI_CMD_FAIL;
    }

    /* Require a port number and a subcommand string at least. */
    if (BCMA_CLI_ARG_CNT(args) < 2) {
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
    for (idx = 0; idx < COUNTOF(porttest_cmds); idx++) {
        if (bcma_cli_parse_cmp(porttest_cmds[idx].name, arg, '\0')) {
            return (*porttest_cmds[idx].func)(cli, args, uport);
        }
    }

    return BCMA_CLI_CMD_USAGE;
}

#endif /* CFG_SDKCLI_INCLUDED && CFG_SDKCLI_PHY_INCLUDED*/
