/*
 * $Id: xc_output_buf.h,v 1.2 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _XC_OUTPUT_BUF_H_
#define _XC_OUTPUT_BUF_H_

#include "system.h"

#ifdef CFG_XCOMMAND_INCLUDED

#define DEFAULT_CFGFILE_ERROR_FORMAT        "ERROR(%d): "

typedef struct {
    XCMD_OUTPUT     base;
    char *buffer;
    unsigned int curpos;
    unsigned int maxlen;
    int depth;
    int bol; /* begin of line */
} XCOUT_BUFFER;

XCMD_ERROR xcout_buffer_init(XCOUT_BUFFER *ps, char *buffer, unsigned int len);

#endif /* CFG_XCOMMAND_INCLUDED */

#endif /* _XC_OUTPUT_BUF_H_ */
