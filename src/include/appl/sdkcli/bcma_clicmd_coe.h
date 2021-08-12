/*! \file bcma_clicmd_coe.h
 *
 * CLI 'coe' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef BCMA_CLICMD_COE_H
#define BCMA_CLICMD_COE_H

#include <appl/sdkcli/bcma_cli.h>

/*! Brief description for CLI command. */
/*! Brief description for CLI command. */
#define BCMA_BCMCOECMD_DESC \
    "COE debug operations."

/*! Syntax for CLI command. */
#define BCMA_BCMCOECMD_SYNOP \
    "<subcmd> [options]"

/*! Help for CLI command. */
#define BCMA_BCMCOECMD_HELP \
    "This command performs operations on COE\n" \
    "coe ChaSsis Mapping Up|Down <source_ports> <dest_ports> <channel_id>\n" \
    "                                - Configure upstream or downstream's\n" \
    "                                  port and channel ID mapping for the\n" \
    "                                  COE chassis mode.\n" \
    "      Up|Down          Specify upstream or downstream direction.\n" \
    "      <source_ports>   Specify a single port or dual ports separated\n" \
    "                       by comma (',').\n" \
    "      <dest_ports>     Specify a single port or dual ports separated\n" \
    "                       by comma (','). When dual ports are specified\n" \
    "                       in upstream, the first port is an active\n" \
    "                       backplane port and the second one is a standby\n" \
    "                       backplane port.\n" \
    "      <channel_id>     Specify a channel ID of the traffic flow.\n" \
    "\ncoe ActivePortClass [<class_id>]- Get or set COE active port class ID.\n" \
    "                                  Current configuration is shown if no\n" \
    "                                  argument is specified.\n" \
    "\ncoe PassThru - COE pass-through mode configuration.\n" \
    "      Mapping <source_ports> <dest_ports> - Configure port mapping\n" \
    "          <source_ports> Specify a single port or dual ports separated\n" \
    "                         by comma (‘,’).\n" \
    "          <dest_ports>   Specify a single port or dual ports separated\n" \
    "                         by comma (‘,’).\n" \
    "\ncoe Show - Display COE chassis or pass-through configuration\n"

/*! Examples for CLI command. */
#define BCMA_BCMCOECMD_EXAMPLES \
    "apc\n" \
    "apc 0x10\n" \
    "chassis mapping up sp=1 dp=11,12 cid=2\n" \
    "chassis mapping down sp=11,12 dp=1 cid=2\n" \
    "passthru mapping 1 2,3\n" \
    "show\n"

/*!
 * \brief CLI command implementation.
 *
 * \param [in] cli CLI object
 * \param [in] args Argument list
 *
 * \return BCMA_CLI_CMD_xxx return values.
 */
extern int
bcma_clicmd_coe(bcma_cli_t *cli, bcma_cli_args_t *args);

#endif /* BCMA_CLICMD_COE_H */
