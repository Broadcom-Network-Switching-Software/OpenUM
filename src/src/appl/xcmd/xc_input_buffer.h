/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _XC_INPUT_BUFFER_H_
#define _XC_INPUT_BUFFER_H_

#include "system.h"

#ifdef CFG_XCOMMAND_INCLUDED

typedef struct {
    XCMD_INPUT     base;
    int depth;
    const char *buffer;
    unsigned int curpos;
    unsigned int maxlen;
} XCIN_BUFFER;

XCMD_ERROR xcin_buffer_init(XCIN_BUFFER *ps, const char *buffer, unsigned int len);

#endif /* CFG_XCOMMAND_INCLUDED */

#endif /* _XC_INPUT_BUFFER_H_ */
