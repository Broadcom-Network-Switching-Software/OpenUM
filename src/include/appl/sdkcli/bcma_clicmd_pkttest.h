/*! \file bcma_bcmpktcmd_pkttest.h
 *
 * CLI command related to PKTTEST.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef BCMA_BCMPKTCMD_PKTTEST_H
#define BCMA_BCMPKTCMD_PKTTEST_H

#include <appl/sdkcli/bcma_cli.h>

/*! Brief description for CLI command. */
#define BCMA_BCMPKTCMD_PKTTEST_DESC \
    "Packet IO snake test."

/*! Syntax for CLI command. */
#define BCMA_BCMPKTCMD_PKTTEST_SYNOP \
    "[options] "

/*! Help for CLI command. */
#define BCMA_BCMPKTCMD_PKTTEST_HELP \
    "This command can be used to conduct snake test for switch ports.\n"\
    "The options supported for each type of test is shown below.\n\n"\
    "    RunMode=<runmode>    - The testing mode (default=MAC).\n"\
    "    PortStart=<value>    - The first test port (default=1).\n"\
    "    PortEnd=<value>      - The last test port (default=MAX).\n"\
    "    Time=<value>         - The test duration (default=10).\n"\
    "<runmode> is one of the options : MAC, INTPHY, EXTPHY, PAIRED, PKTGEN"

/*! Examples for CLI command. */
#define BCMA_BCMPKTCMD_PKTTEST_EXAMPLES \
    "rm=mac ps=1 pe=12 t=20"

/*!
 * \brief PKTTEST command in CLI.
 *
 * \param[in] cli CLI object
 * \param[in] args CLI arguments list
 *
 * \return BCMA_CLI_CMD_xxx return values.
 */
extern int
bcma_bcmpktcmd_pkttest(bcma_cli_t *cli, bcma_cli_args_t *args);

#endif /* BCMA_BCMPKTCMD_PKTTEST_H */
