/*! \file bcma_clicmd_gpio.c
 *
 * CLI 'gpio' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_GPIO_INCLUDED

#include <appl/sdkcli/bcma_cli.h>
#include <appl/sdkcli/bcma_cli_parse.h>
#include <appl/sdkcli/bcma_clicmd_gpio.h>

#include <boardapi/gpio.h>

#include <utgpio.h>

/*******************************************
 *   Local definition.
 */
/* Maximun number of parameter options. */
#define OPTION_MAX        6

/* Interrupt type options. */
#define INTR_TYPE_OPTIONS \
    "DualEdge|FallingEdge|RisingEdge|DualEdge|HighLevel|LowLevel"

/* Interrupt enable options. */
#define INTR_ENABLE_OPTIONS \
    "Disable|Enable"

/* Interrupt value options. */
#define VALUE_OPTIONS \
    "0|1"

/* Interrupt mode options. */
#define MODE_OPTIONS \
    "in|out"

/* Test list */
#define TEST_LIST \
    "Available tests are listed below.\n"\
    "    intr\n"

/* GPIO sub-command list. */
static const char *subcmd_list[] = {
    "show",
    "mode",
    "out",
    "intr",
    "test"
};

/* GPIO test list. */
static const char *test_list[] = {
#if defined(CFG_INTR_INCLUDED)
    "intr",
#endif
};


static void
bcma_gpio_status_show(int gpio) {

    int start, end, i;
    bool value;

    if (gpio < 0) {
        start = 0;
        end = 31;
    } else {
        start = gpio;
        end = gpio;
    }

    sal_printf("GPIO   value\n");
    sal_printf("===================\n");

    for (i = start; i <= end; i++) {
        board_gpio_value_get(i, &value);
        sal_printf("%02d         %1d\n", i,
                   value);
    }

    sal_printf("===================\n");
}

static int
clicmd_gpio_parse_option(const char *ref,
                         const char *str,
                         const char *separator,
                         int *opt)
{
     char *sr, *sr2, *token;
     int option = -1;
     char tmp[128];

     sal_strncpy(tmp, ref, sal_strlen(ref));
     token = sal_strtok_r(tmp, separator, &sr2);
     while(token) {
         option++;
         if (option == OPTION_MAX) {
             return -1;
         }

         if (bcma_cli_parse_cmp(token, str, '\0') == TRUE) {
             *opt = option;
             return 0;
         }
         sr = sr2;
         token = sal_strtok_r(sr, separator, &sr2);
     }

     return -1;
}

static int
clicmd_gpio_list_show(void)
{
    /* List all test cases. */
    sal_printf(TEST_LIST);
    return BCMA_CLI_CMD_OK;
}

static int
bcma_clicmd_gpio_test(bcma_cli_t *cli, bcma_cli_args_t *args)
{
     const char *str;
     int i;

     if (COUNTOF(test_list) == 0) {
         sal_printf("No test case avaliable.\n");
         return BCMA_CLI_CMD_OK;
     }

     /* Get the test case name. */
     str = BCMA_CLI_ARG_GET(args);
     if (str == NULL) {
         return clicmd_gpio_list_show();
     }

     /* Match test case name. */
     for (i = 0; i <= COUNTOF(test_list); i++) {
         if (i == COUNTOF(test_list)) {
             return BCMA_CLI_CMD_USAGE;
         }
         if (bcma_cli_parse_cmp(test_list[i], str, '\0') == FALSE) {
             continue;
         }
         break;
     }

     /* Run test case. */
     switch (i) {
     /* "intr" test case */
     case 0:
         utgpio_intr();
         break;
     default:
         return clicmd_gpio_list_show();
     }

     return BCMA_CLI_CMD_OK;
}

static int
bcma_clicmd_gpio(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    int gpio, subcmd, output, value, option;
    int intr_enable, intr_type;
    const char *str, *cmd;

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

        /* If sub-command is not found, then show more information. */
        if (subcmd == COUNTOF(subcmd_list)) {
            return BCMA_CLI_CMD_USAGE;
        }

        /* Match sub-command. */
        if (bcma_cli_parse_cmp(subcmd_list[subcmd], cmd, '\0') == FALSE) {
            continue;
        }

        break;
   }

   /* Get the GPIO number except for "test" (subcmd = 4). */
   if (subcmd != 4) {
       str = BCMA_CLI_ARG_GET(args);
       gpio = -1;
       option = 0;

       /* For "show" (subcmd = 0), the pin number is optional. */
       if (subcmd == 0) {
           option = 1;
       }

       /* If <pin> is mandatory in sub-command then show more info. */
       if ((str == NULL || bcma_cli_parse_int(str, &gpio) < 0) &&
           !option) {
           return BCMA_CLI_CMD_USAGE;
       }
   }

   switch (subcmd) {
   case 0:
       bcma_gpio_status_show(gpio);
       break;
   case 1:
       /* Get GPIO mode. */
       str = BCMA_CLI_ARG_GET(args);
       if (str == NULL ||
           clicmd_gpio_parse_option(MODE_OPTIONS,
                                    str,
                                    "|",
                                    &output)) {
           return BCMA_CLI_CMD_USAGE;
       }

       /* Set GPIO mode. */
       board_gpio_mode_set(gpio, output);
       break;
   case 2:

       /* Get GPIO value. */
       str = BCMA_CLI_ARG_GET(args);
       if (str == NULL ||
           clicmd_gpio_parse_option(VALUE_OPTIONS,
                                    str,
                                    "|",
                                    &value)) {
           return BCMA_CLI_CMD_USAGE;
       }

       /* Set GPIO value. */
       board_gpio_value_set(gpio, value);
       break;
   case 3:

       /* Get interrupt disable/enable. */
       str = BCMA_CLI_ARG_GET(args);
       if (str == NULL ||
           clicmd_gpio_parse_option(INTR_ENABLE_OPTIONS,
                                    str,
                                    "|",
                                    &intr_enable)) {
           return BCMA_CLI_CMD_USAGE;
       }

       /* Get interrupt type. */
       str = BCMA_CLI_ARG_GET(args);
       intr_type = BOARD_GPIO_INTR_DUAL_EDGE_TRIGGER;
       if (str != NULL) {
           if (clicmd_gpio_parse_option(INTR_TYPE_OPTIONS,
                                        str,
                                        "|",
                                        &intr_type)) {
               return BCMA_CLI_CMD_USAGE;
           }
       }

       /* Set GPIO interrupt. */
       board_gpio_intr_type_set(gpio, intr_type);
       board_gpio_intr_enable_set(gpio, intr_enable);
       break;
   case 4:
       /* Run test case. */
       return bcma_clicmd_gpio_test(cli, args);
   default:
       return BCMA_CLI_CMD_USAGE;
   }

    return BCMA_CLI_CMD_OK;
}

/*! GPIO command structure. */
static bcma_cli_command_t cmd_gpio = {
    .name = "gpio",
    .func = bcma_clicmd_gpio,
    .desc = BCMA_CLICMD_GPIO_DESC,
    .synop = BCMA_CLICMD_GPIO_SYNOP,
    .help = {BCMA_CLICMD_GPIO_HELP},
    .examples = BCMA_CLICMD_GPIO_EXAMPLES
};

/*******************************************
 *   Public function.
 */
int
bcma_clicmd_add_gpio_cmd(bcma_cli_t *cli)
{
    bcma_cli_add_command(cli, &cmd_gpio, 0);
    return 0;
}

#endif /* CFG_SDKCLI_INCLUDED */
