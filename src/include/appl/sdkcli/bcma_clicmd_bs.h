/*! \file bcma_clicmd_bs.h
 *
 * Functions about unit information operation in CLI.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef BCMA_CLICMD_BS_H
#define BCMA_CLICMD_BS_H

#include <appl/sdkcli/bcma_cli.h>

/*! Brief description for CLI command. */
#define BCMA_CLICMD_BS_DESC \
    "BroadSync Configuration, Information and Debug."

/*! Syntax for CLI command. */
#define BCMA_CLICMD_BS_SYNOP \
    "<sub-command> [<params>]"

/*! Help for CLI command. */
#define BCMA_CLICMD_BS_HELP \
    "This command performs BroadSync operations\n" \
    "   VERsion             - Shows the BroadSync Firmware version\n"\
    "                         and Build time.\n"\
    "   ConFiG Master|Slave <bit_clk> <hb_clk>\n"\
    "                       - Re-configure Broadsync0 interface.\n"\
    "                           <bit_clk> and <hb_clk> values in Hz.\n"\
    "   FreqOffset <value>  - Configure a frequency offset to Broadsync clock.\n"\
    "   PhaseOffset <sec> <nsec> <is_negative>\n"\
    "                       - Configure a phase offset to Broadsync time.\n"\
    "                           <is_negative> = True|False\n"\
    "   NtpOffset <sec> <nsec> <is_negative>\n"\
    "                       - Configure a phase offset to NTP time.\n"\
    "                         (relative to BS time).\n"\
    "                           <is_negative> = True|False\n"\
    "   1pps oN|oFf         - Enable/Disable 1PPS signal\n"\
    "   DeBuG <mask_val>|PRinT|CoreDump\n"\
    "                        ]- Set debug log level.\n"\
    "                          or print the last line printed by the BS FW.\n"\
    "                          <mask_val>is an 8-bit mask that sets log level.\n"\
    "                            0xff = BS_DEBUG_LEVEL_ALL\n"\
    "                            0x00 = BS_DEBUG_LEVEL_NONE\n"\
    "                          or print the core dump if any\n"\
    "   TimeStamps          - Prints the latest timestamps info from TS FIFO."\
    "   ReInit              - Reset and reinitialize BroadSync Firmware."

/*! Examples for CLI command. */
#define BCMA_CLICMD_BS_EXAMPLES \
    "version\n" \
    "config m 10000000 4000\n" \
    "freqOffset 20\n"\
    "phaseoffset 2 0 t\n"\
    "ntpoffset 5 0 t\n"\
    "1pps on\n"\
    "debug 0xff\n"\
    "timestamps\n"\
    "reinit"

/*!
 * \brief CLI command implementation.
 *
 * \param [in] cli CLI object
 * \param [in] args Argument list
 *
 * \return BCMA_CLI_CMD_xxx return values.
 */
extern int
bcma_clicmd_bs(bcma_cli_t *cli, bcma_cli_args_t *args);

#endif /* BCMA_CLICMD_BS_H */
