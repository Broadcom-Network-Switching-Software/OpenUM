/*! \file bcma_clicmd_tick.h
 *
 * CLI 'tick' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef BCMA_CLICMD_TICK_H
#define BCMA_CLICMD_TICK_H

#include <appl/sdkcli/bcma_cli.h>

/*! Brief description for CLI command. */
#define BCMA_CLICMD_TICK_DESC \
    "Show current system ticks."

/*!
 * \brief CLI command implementation.
 *
 * \param [in] cli CLI object
 * \param [in] args Argument list
 *
 * \return BCMA_CLI_CMD_xxx return values.
 */
extern int
bcma_clicmd_tick(bcma_cli_t *cli, bcma_cli_args_t *args);

#endif /* BCMA_CLICMD_TICK_H */
