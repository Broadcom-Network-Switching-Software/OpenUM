/*! \file bcma_cli_unit.h
 *
 * Functions about unit information operation in CLI.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef BCMA_CLICMD_TIME_H
#define BCMA_CLICMD_TIME_H

#include <appl/sdkcli/bcma_cli.h>

/*! Brief description for CLI command. */
#define BCMA_CLICMD_SYNCE_DESC \
    "Set/get SyncE squelch/frequency."
/*! Syntax for SYNCE CLI command. */
#define BCMA_CLICMD_SYNCE_SYNOP \
    "Freq Primary|Secondary [PORT|PLL <port_num> <freq_valu>]\n" \
    "Squelch Primary|Secondary [ENable|DISable]"
/*! Help for SYNCE CLI command. */
#define BCMA_CLICMD_SYNCE_HELP \
    "This command is used for set/get SYNCE.\n" \
    "The command supports the following sub-commands and actions:\n" \
    "    Freq Primary|Secondary [PORT|PLL <port_num> <freq_value>] - \n"\
    "        Set/Get frequency synce MHz.\n" \
    "        PORT  - Clock source is from port.\n" \
    "        PLL   - Clock source is from PLL.\n" \
    "        <port_num> - Source port number.\n" \
    "        <freq_value> - Frequency values.\n" \
    "             1: TSCF    - 23.4375 MHz\n" \
    "             2: SDM     - 25MHz\n" \
    "             3: General - 28.4 MHz\n" \
    "             4: General - 46.875 MHz\n" \
    "             5: General - 125MHz\n" \
    "             6: General - 515.625MHz\n" \
    "             7: SDM     - 7.8125 MHz\n" \
    "             8: SDM     - 12.5 MHz\n" \
    "             9: SDM     - 31.25 MHz\n" \
    "            10: SDM     - 37.5 MHz\n" \
    "            11: General - 156.25 MHz\n" \
    "        If arguments are not specified, current configured value will be shown.\n" \
    "    Squelch Primary|Secondary [ENable|DISable] - Set Squelch SyncE enable.\n" \
    "        ENable|DISable - enable/disable squelch SyncE.\n" \
    "        If argument is not specified, the current state will be shown.\n"
/*! Examples for SYNCE CLI command. */
#define BCMA_CLICMD_SYNCE_EXAMPLES \
    "freq primary PORT 2 2\n" \
    "freq secondary\n" \
    "squelch s en\n" \
    "squ p"

/*!
 * \brief Get/Set the Synce status.
 *
 * This function will ensure that parameters are validated and that a
 * default behavior is defined if no call-back is installed.
 *
 * \param [in] cli CLI object.
 * \param [in] feature Feature to check for.
 *
 * \retval
 */
extern int
bcma_clicmd_synce(bcma_cli_t *cli, bcma_cli_args_t *args);

/*! Brief description for CLI command. */
#define BCMA_CLICMD_TIMESYNC_DESC \
    "TimeSync configurations."

/*! Syntax for CLI command. */
#define BCMA_CLICMD_TIMESYNC_SYNOP \
    "<port> tc [Mode=none|1step|2step]\n" \
    "<port> txinfo \n" \
    "<port>"

/*! Help for CLI command. */
#define BCMA_CLICMD_TIMESYNC_HELP \
    "This command performs timesync congiurations.\n" \
    "<port>: The user port number to operate.\n" \
    "tc:\n" \
    "   Update TC(Transparent Clock) mode. If no parameter is specified,\n" \
    "show the current configuration.\n" \
    "txinfo:\n" \
    "   Show the timestamp saved in MAC/PCS.\n" \
    "Show both TC and txinfo if no parameter is specified."

/*! Examples for CLI command. */
#define BCMA_CLICMD_TIMESYNC_EXAMPLES \
    "1 tc mode=1step\n" \
    "1 ti"

/*!
 * \brief TimeSync command handler.
 *
 * \param [in] cli CLI object.
 * \param [in] feature Feature to check for.
 *
 * \retval
 */
extern int
bcma_clicmd_timesync(bcma_cli_t *cli, bcma_cli_args_t *args);

/*! Brief description for CLI command. */
#define BCMA_CLICMD_TIMESYNCTEST_DESC \
    "TimeSync tests."

/*! Syntax for CLI command. */
#define BCMA_CLICMD_TIMESYNCTEST_SYNOP \
    "[pi=<port_num> pe=<port_num>] [clear]"

/*! Help for CLI command. */
#define BCMA_CLICMD_TIMESYNCTEST_HELP \
    "This command is to setup test configurations for cpu to do tx/rx\n" \
    "TimeSync packets to verify timestamps.\n"\
    "    PortIngress=<value>  - The testing ingress port.\n" \
    "    PortEgress=<value>   - The testing egress port.\n" \
    "    clear                - Clean up test configurations.\n" \
    "Show the current configuration if no parameter is specified.\n" \
    "            ______________\n" \
    "            |            |\n"\
    "    ---<--- | pi <-- CPU |\n" \
    "   |        |            |\n" \
    "   | PHY LB |            |\n" \
    "    --->--- | pi  -->    |\n" \
    "            |       |    |\n" \
    "            |       |    |\n" \
    "    ---<--- | pe <--     |\n" \
    "   |        |            |\n" \
    "   | PHY LB |            |\n" \
    "    --->--- | pe  --> CPU|\n" \
    "            |___________ |\n"

/*! Examples for CLI command. */
#define BCMA_CLICMD_TIMESYNCTEST_EXAMPLES \
    "pi=1 pe=12\n" \
    "clear"

/*!
 * \brief TimeSync command handler.
 *
 * \param [in] cli CLI object.
 * \param [in] feature Feature to check for.
 *
 * \retval
 */
extern int
bcma_clicmd_timesync_test(bcma_cli_t *cli, bcma_cli_args_t *args);

#endif /* BCMA_CLICMD_TIME_H */
