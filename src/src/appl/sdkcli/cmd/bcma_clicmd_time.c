/*! \file bcma_clicmd_time.c
 *
 * CLI 'time synce' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_INCLUDED
#ifdef CFG_SWITCH_SYNCE_INCLUDED

#include <appl/sdkcli/bcma_cli.h>
#include <appl/sdkcli/bcma_cli_parse.h>

#include <appl/sdkcli/bcma_clicmd_time.h>

int
bcma_clicmd_getsynce(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    char *input_arg;
    uint8 unit = 0;
    int clk_control = 0;
    bcm_time_synce_clock_source_config_t clk_src_config;
    int ret_val;

    COMPILER_REFERENCE(cli);
    COMPILER_REFERENCE(args);

    if (BCMA_CLI_ARG_CNT(args) == 0) {
        /* No args, return 0 */
        return 0;
    }

    /* First parameter only can be -f(Frequency) or -s(Squelch) */
    input_arg = BCMA_CLI_ARG_CUR(args);
    if (sal_strcmp(input_arg, "-f") == 0) {
        /* -f(Frequeycy) */
        clk_control = 1;
        BCMA_CLI_ARG_NEXT(args);
        /* -p(Primary) or -s(Secondary) */
        input_arg = BCMA_CLI_ARG_CUR(args);
        if (sal_strcmp(input_arg, "-p") == 0) {
            /* bcmTimeSynceClockSourcePrimary */
            clk_src_config.clk_src = 0;
            BCMA_CLI_ARG_NEXT(args);
        } else if (sal_strcmp(input_arg, "-s") == 0) {
            /* bcmTimeSynceClockSourceSecondary */
            clk_src_config.clk_src = 1;
            BCMA_CLI_ARG_NEXT(args);
        } else {
            return BCMA_CLI_CMD_FAIL;
        }
        /* -port(SrcTypePort) or -pll(SrcTypePLL), default=-port */
        input_arg = BCMA_CLI_ARG_CUR(args);
        if (sal_strcmp(input_arg, "-pll") == 0) {
            /* bcmTimeSynceClockSourceSecondary */
            sal_printf("Firelight doesn't support PLL source type.\n");
            BCMA_CLI_ARG_NEXT(args);
            return BCMA_CLI_CMD_FAIL;
        } else if (sal_strcmp(input_arg, "-port") == 0) {
            int lport;

            /* bcmTimeSynceClockSourcePrimary and default */
            clk_src_config.input_src = 0;
            BCMA_CLI_ARG_NEXT(args);
            /* lport */
            /* enable (1 or 0) */
            input_arg = BCMA_CLI_ARG_CUR(args);
            if (bcma_cli_parse_int(input_arg, &lport) != -1) {
                clk_src_config.port = lport;
                BCMA_CLI_ARG_NEXT(args);
            } else {
                return BCMA_CLI_CMD_FAIL;
            }
        } else {
            return BCMA_CLI_CMD_FAIL;
        }

    } else if (sal_strcmp(input_arg, "-s") == 0) {
        /* -s(Squelch) */
        clk_control = 0;
        BCMA_CLI_ARG_NEXT(args);
        /* -p(Primary) or -s(Secondary) */
        input_arg = BCMA_CLI_ARG_CUR(args);
        if (sal_strcmp(input_arg, "-p") == 0) {
            /* bcmTimeSynceClockSourcePrimary */
            clk_src_config.clk_src = 0;
            BCMA_CLI_ARG_NEXT(args);
        } else if (sal_strcmp(input_arg, "-s") == 0) {
            /* bcmTimeSynceClockSourceSecondary */
            clk_src_config.clk_src = 1;
            BCMA_CLI_ARG_NEXT(args);
        } else {
            return BCMA_CLI_CMD_FAIL;
        }
    } else {
        return BCMA_CLI_CMD_FAIL;
    }

    board_time_synce_clock_source_control_get(unit, &clk_src_config, clk_control,
                          &ret_val);
    if (clk_control == 0) {
        /* bcmTimeSynceClockSourceControlSquelch,
           get TOP_L1_RCVD_CLK_CONTROLr__L1_RCVD_SW_OVWR_ENf  */
        sal_printf("Squelch get_enable = %d\n", ret_val);
    } else {
        /* bcmTimeSynceClockSourceControlFrequency */
        sal_printf("Frequency get_MHz = %d\n", ret_val);
    }

    return BCMA_CLI_CMD_OK;
}

int
bcma_clicmd_setsynce(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    char *input_arg;
    uint8 unit = 0;
    int clk_control = 0;
    bcm_time_synce_clock_source_config_t clk_src_config;
    int set_val;

    COMPILER_REFERENCE(cli);
    COMPILER_REFERENCE(args);

    if (BCMA_CLI_ARG_CNT(args) == 0) {
        /* No args, return 0 */
        return 0;
    }

    /* First parameter only can be -f(Frequency) or -s(Squelch) */
    input_arg = BCMA_CLI_ARG_CUR(args);
    if (sal_strcmp(input_arg, "-f") == 0) {
        /* -f(Frequeycy) */
        clk_control = 1;
        BCMA_CLI_ARG_NEXT(args);
        /* -p(Primary) or -s(Secondary) */
        input_arg = BCMA_CLI_ARG_CUR(args);
        if (sal_strcmp(input_arg, "-p") == 0) {
            /* bcmTimeSynceClockSourcePrimary */
            clk_src_config.clk_src = 0;
            BCMA_CLI_ARG_NEXT(args);
        } else if (sal_strcmp(input_arg, "-s") == 0) {
            /* bcmTimeSynceClockSourceSecondary */
            clk_src_config.clk_src = 1;
            BCMA_CLI_ARG_NEXT(args);
        } else {
            return BCMA_CLI_CMD_FAIL;
        }
        /* -port(SrcTypePort) or -pll(SrcTypePLL), default=-port */
        input_arg = BCMA_CLI_ARG_CUR(args);
        if (sal_strcmp(input_arg, "-pll") == 0) {
            /* bcmTimeSynceClockSourceSecondary */
            sal_printf("Firelight doesn't support PLL source type.\n");
            BCMA_CLI_ARG_NEXT(args);
            return BCMA_CLI_CMD_FAIL;
        } else if (sal_strcmp(input_arg, "-port") == 0) {
            int lport;

            /* bcmTimeSynceInputSourceTypePort and default */
            clk_src_config.input_src = 0;
            BCMA_CLI_ARG_NEXT(args);
            /* lport */
            /* enable (1 or 0) */
            input_arg = BCMA_CLI_ARG_CUR(args);
            if (bcma_cli_parse_int(input_arg, &lport) != -1) {
                clk_src_config.port = lport;
                BCMA_CLI_ARG_NEXT(args);
            } else {
                return BCMA_CLI_CMD_FAIL;
            }
            /* Frequency range=1~11 */
            input_arg = BCMA_CLI_ARG_CUR(args);
            if (bcma_cli_parse_int(input_arg, &set_val) != -1 &&
                (set_val >= 1 && set_val <= 11)) {
                sal_printf("\n set_val = %d", set_val);
                BCMA_CLI_ARG_NEXT(args);
            } else {
                return BCMA_CLI_CMD_FAIL;
            }
        } else {
            return BCMA_CLI_CMD_FAIL;
        }

    } else if (sal_strcmp(input_arg, "-s") == 0) {
        /* -s(Squelch) */
        clk_control = 0;
        BCMA_CLI_ARG_NEXT(args);
        /* -p(Primary) or -s(Secondary) */
        input_arg = BCMA_CLI_ARG_CUR(args);
        if (sal_strcmp(input_arg, "-p") == 0) {
            /* bcmTimeSynceClockSourcePrimary */
            clk_src_config.clk_src = 0;
            BCMA_CLI_ARG_NEXT(args);
        } else if (sal_strcmp(input_arg, "-s") == 0) {
            /* bcmTimeSynceClockSourceSecondary */
            clk_src_config.clk_src = 1;
            BCMA_CLI_ARG_NEXT(args);
        } else {
            return BCMA_CLI_CMD_FAIL;
        }
        /* enable (1 or 0) */
        input_arg = BCMA_CLI_ARG_CUR(args);
        if (bcma_cli_parse_int(input_arg, &set_val) != -1 &&
           (set_val == 1 || set_val == 0)) {
            sal_printf("\n en = %d \n", set_val);
            BCMA_CLI_ARG_NEXT(args);
        } else {
            return BCMA_CLI_CMD_FAIL;
        }
    } else {
        return BCMA_CLI_CMD_FAIL;
    }

    board_time_synce_clock_source_control_set(unit, &clk_src_config, clk_control,
                          set_val);

    return BCMA_CLI_CMD_OK;
}

#endif /* CFG_SWITCH_SYNCE_INCLUDED */
#endif /* CFG_SDKCLI_INCLUDED */
