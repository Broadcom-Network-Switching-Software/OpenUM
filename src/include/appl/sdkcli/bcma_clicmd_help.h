/*! \file bcma_clicmd_help.h
 *
 * CLI 'help' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef BCMA_CLICMD_HELP_H
#define BCMA_CLICMD_HELP_H

#include <appl/sdkcli/bcma_cli.h>

/*! Brief description for CLI command. */
#define BCMA_CLICMD_HELP_DESC \
    "Show help."

/*! Syntax for CLI command. */
#define BCMA_CLICMD_HELP_SYNOP \
    "[<command>]"

/*!
 * \brief CLI command implementation.
 *
 * \param [in] cli CLI object
 * \param [in] args Argument list
 *
 * \return BCMA_CLI_CMD_xxx return values.
 */
extern int
bcma_clicmd_help(bcma_cli_t *cli, bcma_cli_args_t *args);

#endif /* BCMA_CLICMD_HELP_H */
