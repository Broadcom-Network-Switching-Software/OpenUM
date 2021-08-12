/*! \file bcma_clicmd_gpio.h
 *
 * GPIO CLI commands.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef BCMA_CLICMD_GPIO_H
#define BCMA_CLICMD_GPIO_H

#include <appl/sdkcli/bcma_cli.h>

/*! Brief description for CLI command. */
#define BCMA_CLICMD_GPIO_DESC \
    "GPIO configuration, information and test."

/*! Syntax for CLI command. */
#define BCMA_CLICMD_GPIO_SYNOP \
    "<sub-command> [<pin>] [options]"

/*! Help for CLI command. */
#define BCMA_CLICMD_GPIO_HELP \
    "This command performs operations on GPIO\n" \
    "    show [<pin>]            - Show GPIO status including GPIO mode,\n"\
    "                              interrupt and current value.\n"\
    "                              No pin specified, all pins will be shown.\n"\
    "    mode <pin> in|out       - Set GPIO mode to input or output.\n" \
    "    out <pin> 0|1           - Write GPIO output value (effective when\n"\
    "                              mode is output). Value should be 0 or 1.\n"\
    "    intr <pin> Enable|Disable [<intr_mode>]\n"\
    "                            - Enable or disable GPIO interrupt with \n"\
    "                              specified interrupt mode.\n"\
    "                              Interrupt modes can be FallingEdge, \n"\
    "                              RisingEdge, DualEdge, HighLevel or"\
    "                              LowLevel.\n"\
    "    test [<case>] [<param>] - Perform GPIO unit tests.\n"\
    "                              If no test case specified, all available\n"\
    "                              test cases will be shown.\n"\
    "                              For example,\n"\
    "                              gpio test intr\n"\
    "                              gpio test intr_latency\n"\
    "                              gpio test intr_regression\n"

/*! Examples for CLI command. */
#define BCMA_CLICMD_GPIO_EXAMPLES \
    "show\n" \
    "mode 1 in|out\n" \
    "out 2 1\n" \
    "intr 4 enable fe"

/*!
 * \brief CLI 'gpio' command.
 *
 * \param [in] cli CLI object
 *
 * \return BCMA_CLI_CMD_xxx return values.
 */
extern int
bcma_clicmd_add_gpio_cmd(bcma_cli_t *cli);

#endif /* BCMA_CLICMD_GPIO_H */
