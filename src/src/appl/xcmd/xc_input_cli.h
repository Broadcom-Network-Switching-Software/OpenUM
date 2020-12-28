/*
 * $Id: xc_input_cli.h,v 1.4 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _XC_INPUT_CLI_H_
#define _XC_INPUT_CLI_H_

#include "system.h"

#if defined(CFG_XCOMMAND_INCLUDED) || defined(CFG_VENDOR_CONFIG_SUPPORT_INCLUDED)

#define DEFAULT_CLI_COMMENT_BEGIN   '!'
#define MAX_XCINPUT_CLI_BUFFER      (512)
#define MAX_XCINPUT_CLI_LEVELS      (8)

typedef struct {
    XCMD_INPUT     base;  /* must be the top of this structure */
    int depth;
    char buffer[MAX_XCINPUT_CLI_BUFFER];
    unsigned int curpos;
    unsigned int stack[MAX_XCINPUT_CLI_LEVELS];
    unsigned int count;
    char *prompt;
    unsigned int flags;      /* for authentication*/
} XCIN_CLI;

XCMD_ERROR xcin_cli_init(XCIN_CLI *ps);

#endif /* CFG_XCOMMAND_INCLUDED */

#endif /* _XC_INPUT_CLI_H_ */
