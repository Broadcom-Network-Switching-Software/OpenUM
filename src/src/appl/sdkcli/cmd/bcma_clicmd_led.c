/*! \file bcma_clicmd_led.c
 *
 * CLI 'led' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_LED_INCLUDED

#include <cmicx_customer_led_common.h>
#include <appl/sdkcli/bcma_cli.h>
#include <appl/sdkcli/bcma_cli_parse.h>
#include <appl/sdkcli/bcma_clicmd_led.h>

#include <utils/shr/shr_debug.h>

#include <boardapi/base.h>
#include <boardapi/led.h>
#include <boardapi/link.h>

#include <binfs.h>
#include <utled.h>

/*******************************************
 *   Local definition.
 */
static int led_auto_on = 0;

/* LED sub-command list. */
typedef struct {
     const char *name;
     int (*func)(bcma_cli_t *cli, bcma_cli_args_t *arg);
} led_subcmd_t;

static int
bcma_led_status_show(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    int start;
    int led_uc_max, led_uc_min, led_uc_num, led_uc;
    char *str;

    SHR_FUNC_ENTER(0);

    SHR_IF_ERR_EXIT
        (board_led_uc_num_get(&led_uc_num));

    str = BCMA_CLI_ARG_GET(args);
    if (str == NULL || bcma_cli_parse_int(str, &led_uc) < 0) {
        led_uc_min = 0;
        led_uc_max = led_uc_num - 1;
    } else {
        if (led_uc >= led_uc_num) {
            SHR_ERR_EXIT(SYS_ERR_PARAMETER);
        }
        led_uc_min = led_uc_max = led_uc;
    }

    for (led_uc = led_uc_min; led_uc <= led_uc_max; led_uc++) {

        SHR_IF_ERR_EXIT
            (board_led_fw_start_get(led_uc, &start));

        if (start) {
            sal_printf("LED uC %d started.\n", led_uc);
        } else {
            sal_printf("LED uC %d stopped.\n", led_uc);
        }
    }

    sal_printf("LED auto update %s\n", led_auto_on ? "on." : "off.");

exit:

    SHR_FUNC_EXIT();
}

static int
bcma_led_uc_start_set(bcma_cli_t *cli, bcma_cli_args_t *args, int start)
{
    int led_uc;
    int led_uc_max, led_uc_min, led_uc_num;
    char *str;

    SHR_FUNC_ENTER(0);

    SHR_IF_ERR_EXIT
        (board_led_uc_num_get(&led_uc_num));

    str = BCMA_CLI_ARG_GET(args);
    if (str == NULL || bcma_cli_parse_int(str, &led_uc) < 0) {
        led_uc_min = 0;
        led_uc_max = led_uc_num - 1;
    } else {
        led_uc_min = led_uc_max = led_uc;
    }

    for (led_uc = led_uc_min; led_uc <= led_uc_max; led_uc++) {
        SHR_IF_ERR_EXIT
            (board_led_fw_start_set(led_uc, start));
    }

exit:
    SHR_FUNC_EXIT();
}

static int
bcma_led_uc_start(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    return bcma_led_uc_start_set(cli, args, 1);
}

static int
bcma_led_uc_stop(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    return bcma_led_uc_start_set(cli, args, 0);
}

static int
bcma_led_uc_load_fw(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    char *name;
    const uint8 *program;
    int len, led_uc;
    char *str;

    SHR_FUNC_ENTER(0);

    name = BCMA_CLI_ARG_GET(args);
    if (name == NULL) {
        sal_printf("File name is unspecified\n");
        SHR_ERR_EXIT(SYS_ERR_PARAMETER);
    }

    str = BCMA_CLI_ARG_GET(args);
    if (str == NULL || bcma_cli_parse_int(str, &led_uc) < 0) {
        led_uc = 0;
    }

    if (SHR_FAILURE(binfs_file_data_get(name, &program, &len))) {
        sal_printf("File is not found\n");
        SHR_ERR_EXIT(SYS_ERR_NOT_FOUND);
    }

    if (SHR_FAILURE(board_led_fw_load(led_uc, program, len))) {
        SHR_ERR_EXIT(SYS_ERR);
    }

exit:
    SHR_FUNC_EXIT();
}

static void
linkchange_handler(uint16 port, BOOL link, void *arg)
{
    uint8 color;
    int led_uc;
    int led_uc_port, pport;
    uint8 unit, lport;

    SHR_FUNC_ENTER(0);

    if (link) {
        sal_printf("port %d link up.\n", port);
        color = (LED_GREEN << 1) | LED_SW_LINK_UP;
    } else {
        sal_printf("port %d link down.\n", port);
        color = LED_OFF;
    }

    board_uport_to_lport(port, &unit, &lport);

    pport = SOC_PORT_L2P_MAPPING(lport);

    SHR_IF_ERR_EXIT
        (board_led_pport_to_led_uc_port(pport, &led_uc, &led_uc_port));

    SHR_IF_ERR_EXIT
        (board_led_control_data_write(led_uc, led_uc_port,
                                      &color, sizeof(color)));
exit:
    return;
}

static int
bcma_led_auto_on(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    char *str;

    SHR_FUNC_ENTER(0);

    /* Get sub-command string. */
    str = BCMA_CLI_ARG_GET(args);

    /* No sub-command, then more information. */
    if (str == NULL) {
        SHR_ERR_EXIT(SYS_ERR_PARAMETER);
    }

    if (bcma_cli_parse_cmp(str, "on", '\0')) {
        if (led_auto_on == 0 &&
           sys_register_linkchange(linkchange_handler, NULL) == TRUE) {
           led_auto_on = 1;
        }
    } else if (bcma_cli_parse_cmp(str, "off", '\0')) {
        sys_unregister_linkchange(linkchange_handler);
        led_auto_on = 0;
    } else {
        SHR_ERR_EXIT(SYS_ERR_PARAMETER);
    }

exit:
    SHR_FUNC_EXIT();
}

static int
bcma_led_test(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    SHR_FUNC_ENTER(0);

    SHR_IF_ERR_EXIT
        (utled_test());

exit:
    SHR_FUNC_EXIT();
}

static led_subcmd_t subcmd_list[] = {
   { "show",  bcma_led_status_show },
   { "start", bcma_led_uc_start    },
   { "stop",  bcma_led_uc_stop     },
   { "auto",  bcma_led_auto_on     },
   { "load",  bcma_led_uc_load_fw  },
   { "test",  bcma_led_test     }
};

static int
bcma_clicmd_led(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    int subcmd, rv;
    const char *cmd;

    if (BCMA_CLI_ARG_CNT(args) == 0) {
        return BCMA_CLI_CMD_USAGE;
    }

    /* Get sub-command string. */
    cmd = BCMA_CLI_ARG_GET(args);

    /* No sub-command, then more information. */
    if (cmd == NULL) {
        return BCMA_CLI_CMD_USAGE;
    }

    /* Find sub-command in subcmd_list. */
    for (subcmd = 0; subcmd <= COUNTOF(subcmd_list); subcmd++) {

        /* Match sub-command. */
        if (bcma_cli_parse_cmp(subcmd_list[subcmd].name, cmd, '\0')) {

            rv = subcmd_list[subcmd].func(cli, args);
            if (SHR_SUCCESS(rv)) {
                return BCMA_CLI_CMD_OK;
            }
            if (rv == SYS_ERR_PARAMETER) {
                return BCMA_CLI_CMD_USAGE;
            }
            return BCMA_CLI_CMD_FAIL;
        }
   }

   return BCMA_CLI_CMD_USAGE;
}

/*! LED command structure. */
static bcma_cli_command_t cmd_led = {
    .name = "led",
    .func = bcma_clicmd_led,
    .desc = BCMA_CLICMD_LED_DESC,
    .synop = BCMA_CLICMD_LED_SYNOP,
    .help = {BCMA_CLICMD_LED_HELP},
    .examples = BCMA_CLICMD_LED_EXAMPLES
};

/*******************************************
 *   Public function.
 */
int
bcma_clicmd_add_led_cmd(bcma_cli_t *cli)
{
    bcma_cli_add_command(cli, &cmd_led, 0);
    return 0;
}

#endif /* CFG_SDKCLI_INCLUDED */
