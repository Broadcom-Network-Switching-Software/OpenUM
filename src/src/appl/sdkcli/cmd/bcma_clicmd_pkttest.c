/*! \file bcma_bcmpktcmd_pkttest.c
 *
 * Packet I/O  performance test commands in CLI.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_INCLUDED

#include "cli_porting.h"

#if CFG_RXTX_SUPPORT_ENABLED

#include <appl/sdkcli/bcma_cli.h>
#include <appl/sdkcli/bcma_cli_parse.h>

#include <appl/sdkcli/bcma_clicmd_pkttest.h>

#include <appl/snaketest.h>

#define DEF_TEST_DURATION      10 /* 10 seconds */

static bcma_cli_parse_enum_t pkt_test_type[] = {
    {"MAC", SNAKETEST_TYPE_INT_MAC},
    {"INTPHY", SNAKETEST_TYPE_INT_PHY},
    {"EXTPHY", SNAKETEST_TYPE_EXT},
    {"PAIRED", SNAKETEST_TYPE_PORT_PAIR},
    {"PKTGEN", SNAKETEST_TYPE_PKT_GEN},
    {NULL, 1}
};

typedef struct bcmpkttest_cfg_s {
    snaketest_type_t rm_type;
    int min_port;
    int max_port;
    int test_duration;
    bool init;
} bcmpkttest_cfg_t;

static int
pkttest_parser(bcma_cli_t *cli, bcma_cli_args_t *args, bcmpkttest_cfg_t *cfg)
{
    int rv = BCMA_CLI_CMD_OK;
    bcma_cli_parse_table_t pt;

    /* Default values */
    cfg->min_port = SAL_NZUPORT_TO_UPORT(1);
    cfg->max_port = SAL_NZUPORT_TO_UPORT(board_uport_count());
    cfg->test_duration = DEF_TEST_DURATION;

    bcma_cli_parse_table_init(cli, &pt);
    bcma_cli_parse_table_add(&pt, "RunMode", "enum",
                             &cfg->rm_type, pkt_test_type);
    bcma_cli_parse_table_add(&pt, "PortStart", "int",
                             &cfg->min_port, NULL);
    bcma_cli_parse_table_add(&pt, "PortEnd", "int",
                             &cfg->max_port, NULL);
    bcma_cli_parse_table_add(&pt, "Time", "int",
                             &cfg->test_duration, NULL);

    rv = bcma_cli_parse_table_do_args(&pt, args);
    bcma_cli_parse_table_done(&pt);
    if (rv < 0) {
        cli_out("%s: Invalid option: %s\n",
                BCMA_CLI_ARG_CMD(args), BCMA_CLI_ARG_CUR(args));
        return BCMA_CLI_CMD_USAGE;
    }

    if (SAL_UPORT_IS_NOT_VALID(cfg->min_port) || SAL_UPORT_IS_NOT_VALID(cfg->max_port)) {
        cli_out("%s: port out of range\n", __func__);
        return BCMA_CLI_CMD_USAGE;
    }

    return rv;
}

int
bcma_bcmpktcmd_pkttest(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    int rv;
    bcmpkttest_cfg_t cfg;

    sal_memset(&cfg, 0, sizeof(bcmpkttest_cfg_t));

    rv = pkttest_parser(cli, args, &cfg);
    if (rv < 0) {
        return rv;
    }

    snaketest(cfg.rm_type, cfg.min_port, cfg.max_port, cfg.test_duration);

    return BCMA_CLI_CMD_OK;
}
#endif
#endif
