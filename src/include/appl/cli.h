/*
 * $Id: cli.h,v 1.2 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _CLI_H_
#define _CLI_H_

/* CLI command function prototype */
typedef enum {
    CLI_CMD_OP_EXEC,
    CLI_CMD_OP_DESC,
    CLI_CMD_OP_HELP
} CLI_CMD_OP;
typedef void (*CLI_CMD_FUNC)(CLI_CMD_OP) REENTRANT;

/* Add command to CLI command list */
extern BOOL cli_add_cmd(char cmd, CLI_CMD_FUNC func) REENTRANT;

/* Remove command from CLI command list */
extern void cli_remove_cmd(char cmd) REENTRANT;

/* Start CLI */
extern void cli(void) REENTRANT;







#endif /* _CLI_H_ */
