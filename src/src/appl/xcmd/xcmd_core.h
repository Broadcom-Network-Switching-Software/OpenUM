/*
 * $Id: xcmd_core.h,v 1.2 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _XCMD_CORE_H_
#define _XCMD_CORE_H_

#include "system.h"

#ifdef CFG_XCOMMAND_INCLUDED

extern XCMD_ERROR
xcmd_context_generate(const XCNODE_CONTEXT *cxt, XCMD_OUTPUT *pout);

extern XCMD_ERROR
xcmd_process_inputs(
    const char *prompt,
    XCMD_INPUT *ps,
    const XCNODE_CONTEXT *cxt
    );

#endif /* CFG_XCOMMAND_INCLUDED */

#endif /* _XCMD_CORE_H_ */
