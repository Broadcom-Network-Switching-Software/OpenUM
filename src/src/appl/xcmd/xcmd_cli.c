/*
 * $Id: xcmd_cli.c,v 1.3 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"

#ifdef CFG_XCOMMAND_INCLUDED

#include "appl/xcmd/xcmd_internal.h"
#include "xcmd_core.h"
#include "xc_input_cli.h"
#include "xc_input_buffer.h"
#include "xc_output_buf.h"

extern const XCNODE_CONTEXT xcmd_context_global;

/* #ifdef FEAT_BROADCOM_XCMD_XCLI */
#define XCLI_START_CONTEXT (&xcmd_context_global)
#define XCPRINT sal_printf


XCMD_ERROR
xcli_start_shell(const char *prompt, const char *user, const char *password)
{
    XCIN_CLI in;
    XCMD_ERROR r;
    char prompt_with_user[32];

    xcin_cli_init(&in);

    in.flags = xcli_auth_login(user, password);

    if (in.flags == 0) {

        XCPRINT("user name or password error\n");
        
        return XCMD_ERR_AUTH_FAIL;
    }

    sal_sprintf(prompt_with_user, "%s/%s:", prompt, user);
    
    r = xcmd_process_inputs(
            prompt_with_user , 
            (XCMD_INPUT *)&in,
            XCLI_START_CONTEXT
            );
    
    return r;
}

XCMD_ERROR
xcli_execute_commands_from_buffer(const char *buffer, unsigned int len)
{
    XCIN_BUFFER in;
    XCMD_ERROR r;
    
    xcin_buffer_init(&in, buffer, len);
    
    r = xcmd_process_inputs(
            "", 
            (XCMD_INPUT *)&in, 
            &xcmd_context_global
            );
    
    return r;
}

XCMD_ERROR
xcli_build_commands_to_buffer(char *buffer, unsigned int *plen)
{
    XCOUT_BUFFER out;
    XCMD_ERROR r;
    
    if (buffer == NULL || plen == NULL) {
        return XCMD_ERR_INTERNAL_INVALID_FUNC_PARAM;
    }
    
    xcout_buffer_init(&out, buffer, *plen);
    
    r = xcmd_context_generate(&xcmd_context_global, (XCMD_OUTPUT *)&out);
    out.buffer[out.curpos] = 0;
    *plen = out.curpos;
    
    return r;    
}

#endif /* CFG_XCOMMAND_INCLUDED */
