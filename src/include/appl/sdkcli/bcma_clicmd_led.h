/*! \file bcma_clicmd_led.h
 *
 *  LED CLI commands.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef BCMA_CLICMD_LED_H
#define BCMA_CLICMD_LED_H

#include <appl/sdkcli/bcma_cli.h>

/*! Brief description for CLI command. */
#define BCMA_CLICMD_LED_DESC \
    "LED microcontroller control, information and test."

/*! Syntax for CLI command. */
#define BCMA_CLICMD_LED_SYNOP \
    "<sub-command> [<led_uc>]"

/*! Help for CLI command. */
#define BCMA_CLICMD_LED_HELP \
    "This command performs operations on GPIO\n" \
    "    show [<led_uc>]         - Show LED controller status.\n"\
    "                              If led_uc is unspecified,"\
    "                              all led uCs' status will be shown.\n"\
    "    start [<led_uc>]        - Start LED controller.\n"\
    "                              If led_uc is unspecified, start all led uCs.\n"\
    "    stop [<led_uc>]         - Stop LED controller.\n"\
    "                              If led_uc is unspecified, stop all led uCs.\n"\
    "    auto [on|off]           - Turn on/off automatically LED updates as port link change.\n"\
    "    load <filename> [<led_uc>]\n"\
    "                            - Load firmware from binfs.\n"\
    "                              If led_uc is unspecified, led_uc = 0.\n"\
    "    test                    - Perform LED unit tests.\n"\
    "                              For example,\n"\
    "                              led test\n"

/*! Examples for CLI command. */
#define BCMA_CLICMD_LED_EXAMPLES \
    "show\n" \
    "start\n" \
    "stop\n" \
    "auto on\n"\
    "auto off\n"

/*!
 * \brief CLI 'led' command.
 *
 * \param [in] cli CLI object
 *
 * \return BCMA_CLI_CMD_xxx return values.
 */
extern int
bcma_clicmd_add_led_cmd(bcma_cli_t *cli);

#endif /* BCMA_CLICMD_LED_H */
