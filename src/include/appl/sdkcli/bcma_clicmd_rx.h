/*! \file bcma_bcmpktcmd_rx.h
 *
 * CLI command related to RX access.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef BCMA_BCMPKTCMD_RX_H
#define BCMA_BCMPKTCMD_RX_H

#include <appl/sdkcli/bcma_cli.h>

/*! Brief description for CLI command. */
#define BCMA_BCMPKTCMD_RX_DESC \
    "Packet RX debug operations."

/*! Syntax for CLI command. */
#define BCMA_BCMPKTCMD_RX_SYNOP \
    "<subcmd> [<action>] [options]"

/*! Help for CLI command. */
#define BCMA_BCMPKTCMD_RX_HELP \
    "This command is used to create and manage packet watchers. A packet\n" \
    "watcher monitors a specific network interface and dumps all packets\n" \
    "received on this interface in real time.\n" \
    "RX watcher recieved packets can be displayed on console by default.\n" \
    "    Watcher - Create RX watcher on a network interface.\n" \
    "        Create [options]             - Create a RX watcher on a network\n" \
    "                                       interface.\n" \
    "            ShowPacketData=[yes/no]  - Dump packet raw data.\n" \
    "            ShowMetaData=[yes/no]    - Dump packet metadata.\n" \
    "        Destroy                      - Destroy a RX watcher.\n" \
    "        Output [options] - Log RX watcher recieved packets on console. \n" \
    "            ConsoleEnable=[on|off]   - Display data on console. Default\n" \
    "                                       is 'on'."

/*! Examples for CLI command. */
#define BCMA_BCMPKTCMD_RX_EXAMPLES \
    "watcher create\n" \
    "w c spd=yes\n" \
    "w o ce=off\n" \
    "w d"

/*!
 * \brief RX command in CLI.
 *
 * \param[in] cli CLI object
 * \param[in] args CLI arguments list
 *
 * \return BCMA_CLI_CMD_xxx return values.
 */
extern int
bcma_bcmpktcmd_rx(bcma_cli_t *cli, bcma_cli_args_t *args);

#endif /* BCMA_BCMPKTCMD_RX_H */
