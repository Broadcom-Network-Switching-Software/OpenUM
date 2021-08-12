/*! \file bcma_clicmd_porttest.h
 *
 * CLI "porttest" command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef BCMA_CLICMD_PORTTEST_H
#define BCMA_CLICMD_PORTTEST_H

#include <appl/sdkcli/bcma_cli.h>

/*! Brief description for CLI command. */
#define BCMA_CLICMD_PORTTEST_DESC \
    "Port/Serdes test configuration and information."

/*! Syntax for CLI command. */
#define BCMA_CLICMD_PORTTEST_SYNOP \
    "<port> fault [LocalFault=on|off] [RemoteFault=on|off]\\\n" \
    "[ForcedRemoteFault=on|off]\n" \
    "<port> fec [Mode=none|cl74|cl91|cl108]\n" \
    "<port> prbs [tx|rx] [ENable=on|off]\\\n" \
    "[Polynomial=p7|p9|p11|p15|p23|p31|p58] [Invert=0|1]\n" \
    "<port> pwr [ENable=on|off]\n" \
    "<port> txfir [pre=<value>] [main=<value>] [post=<value>]\\\n" \
    "[post2=<value>] [post3=<value>]\n" \
    "<port> status"

/*! Help for CLI command. */
#define BCMA_CLICMD_PORTTEST_HELP \
    "This command performs port test operations.\n" \
    "<port>: The user port number to operate.\n" \
    "fault:\n" \
    "   Update fault detection configurations. If no parameter is\n" \
    "   specified, show the current configuration and status.\n" \
    "fec:\n" \
    "   Update FEC configurations. If no parameter is specified, show the\n" \
    "   current configuration and status.\n" \
    "prbs:\n" \
    "   Update PRBS configurations. If no parameter is specified, show the\n" \
    "   current configuration and status.\n" \
    "pwr:\n" \
    "   Update PHY power configurations. If no parameter is specified, show\n" \
    "   the current configuration and status.\n" \
    "txfir:\n" \
    "   Update FIR configurations for transmitter equalization. If no\n" \
    "   parameter is specified, show the current configuration and status.\n" \
    "status:\n" \
    "   Show the misc port status, e.g. receiver equalization values, BIP\n" \
    "   error count, BER count, and etc."

/*! Examples for CLI command. */
#define BCMA_CLICMD_PORTTEST_EXAMPLES \
    "1 fault lf=on rf=on\n" \
    "1 fault\n" \
    "1 fec mode=cl74\n" \
    "1 fec\n" \
    "1 prbs en=on p=p7 i=0\n" \
    "1 prbs\n" \
    "1 pwr en=on\n" \
    "1 pwr\n" \
    "1 txfir pre=0 main=50 post=10 post2=0 post3=-1\n" \
    "1 txfir\n" \
    "1 status"

/*!
 * \brief Porttest command handler.
 *
 * \param [in] cli CLI object.
 * \param [in] feature Feature to check for.
 *
 * \retval
 */
extern int
bcma_clicmd_porttest(bcma_cli_t *cli, bcma_cli_args_t *args);

#endif /* BCMA_CLICMD_PORTTEST_H */
