/*! \file bcma_clicmd_phy.h
 *
 * CLI 'phy' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef BCMA_CLICMD_PHY_H
#define BCMA_CLICMD_PHY_H

#include <appl/sdkcli/bcma_cli.h>

/*! Brief description for CLI command. */
#define BCMA_CLICMD_PHY_DESC \
    "PHY debug information"

#define BCMA_CLICMD_PHY_SYNOP \
    "info\n" \
    "<port> [list] <regsym> [<value>]\n" \
    "raw sbus|c45 <phyid> <devaddr> <regaddr> [<value>]\n" \
    "raw <phyid> <regaddr> [<value>]\n" \
    "diag <port> dsc|prbs <options>"

#define BCMA_CLICMD_PHY_HELP \
    "This command is used for PHY diag\n" \
    "  Default: <port> [list] <regsym> [<value>]\n" \
    "           read/write/list register with register name \n" \
    "  Subcommands: raw, info, diag... \n" \
    "  raw  - read/write register with Phy address via S-BUS, CL45 or CL22 \n" \
    "    phy raw sbus|c45 <phyid> <devaddr> <regaddr> [<value>] \n"  \
    "    phy raw <phyid> <regaddr> [<value>] \n"  \
    "           phyid   -- Phy address \n" \
    "           devaddr -- Phy Device address: \n" \
    "                      0 - Reserved; 1 - PMD/PMA; 3 - PCS; 7 - AN; \n" \
    "           regaddr -- Phy register address \n" \
    "  info - display PHY device ID and attached PHY drivers\n" \
    "  diag - phy diagnostic commands\n" \
    "    phy diag <port> <sub_cmd> [sub cmd parameters]\n" \
    "    The list of phy diag sub commands:\n" \
    "    dsc  - display lane information, include sigdet, lock etc. \n" \
    "           phy diag <port> dsc [config|std|cl72] \n" \
    "    prbs - perform various PRBS functions.\n" \
    "           phy diag <port> prbs set [pol=<polynomial>] [invert=0|1] \n" \
    "           phy diag <port> prbs get|clear\n"

#define BCMA_CLICMD_PHY_EXAMPLES \
    "1 *\n" \
    "1 list main\n" \
    "1 MAIN0_SETUPr\n" \
    "raw sbus 0xc1 1 2\n" \
    "raw c45 0xc1 1 2\n" \
    "raw 0xc1 2\n" \
    "diag 1 dsc\n" \
    "diag 1 dsc config\n" \
    "diag 1 prbs set p=3\n" \
    "diag 1 prbs get\n" \
    "diag 1 prbs clear"

/*!
 * \brief CLI command implementation.
 *
 * \param [in] cli CLI object
 * \param [in] args Argument list
 *
 * \return BCMA_CLI_CMD_xxx return values.
 */
extern int
bcma_clicmd_phy(bcma_cli_t *cli, bcma_cli_args_t *args);

#endif /* BCMA_CLICMD_PHY_H */
