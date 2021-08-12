/*! \file bcma_clicmd_debugcmd.h
 *
 * CLI 'debugcmd' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef BCMA_CLICMD_DEBUGCMD_H
#define BCMA_CLICMD_DEBUGCMD_H

#include <appl/sdkcli/bcma_cli.h>

/*! Brief description for CLI command. */
#define BCMA_CLICMD_DEBUGCMD_DESC \
    "Enter original UM CLI mode for debugging."

/*!
 * \brief CLI 'debugcmd' command.
 *
 * \param [in] cli CLI object
 * \param [in] args Argument list
 *
 * \return BCMA_CLI_CMD_xxx return values.
 */
extern int
bcma_clicmd_debugcmd(bcma_cli_t *cli, bcma_cli_args_t *args);

#endif /* BCMA_CLICMD_DEBUGCMD_H */
