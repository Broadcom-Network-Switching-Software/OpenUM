/*
 * $Id: xc_input_buffer.c,v 1.3 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#include "system.h"

#ifdef CFG_XCOMMAND_INCLUDED

#include "appl/xcmd/xcmd_internal.h"
#include "xcmd_core.h"
#include "xc_input_buffer.h"

#define THIS ((XCIN_BUFFER *)pstream)
static const char CSTR_EXIT[] = "exit";

static int
char_is_eol(char ch)
{
    if (ch == '\r' || 
        ch == '\n' ||
        ch == '\0') {
        
        return 1;
    }
    
    return 0;
}

static void
skip_line(void *pstream)
{
    while(THIS->curpos < THIS->maxlen && 
          !char_is_eol(THIS->buffer[THIS->curpos])) {
        THIS->curpos++;
    }
    while(THIS->curpos < THIS->maxlen &&  
          char_is_eol(THIS->buffer[THIS->curpos])) {

        THIS->curpos++;
    }
}

static XCMD_ERROR
get_line(void *pstream, const char **pline, unsigned int *plen, XCMDI_OP *pop)
{
    if (pstream == NULL || pline == NULL || plen == NULL || pop == NULL) {
        return XCMD_ERR_INTERNAL_INVALID_STREAM_CALL;
    }
    
    if (THIS->depth < 0) {
        return XCMD_ERR_INTERNAL_INVALID_STREAM_CALL;
    }
    
    for(;;) {
        unsigned int d;
        
        if (THIS->curpos == THIS->maxlen) {
            /* No more data */
            *plen = 0;
            return XCMD_ERR_EXIT;
        }
        
        /* Check depth by spaces */
        d = 0;
        while(THIS->curpos + d < THIS->maxlen && 
              THIS->buffer[THIS->curpos + d] == ' ') {
            d++;
        }
        if (d != THIS->depth) {
            
            /* If spaces < current depth, send a EXIT signal */
            if (d < THIS->depth) {
                
                /* Emulate we got a 'exit' */
                *pline = CSTR_EXIT;
                *plen = sal_strlen(CSTR_EXIT);
                *pop = XCMDI_OP_EXECUTE;
                return XCMD_ERR_OK;
            }
            
            skip_line(pstream);
            continue;
        }
        
        THIS->curpos += d;
        if (THIS->curpos  == THIS->maxlen) {
            /* No more data */
            *plen = 0;
            return XCMD_ERR_EXIT;
        }
        
        /* Check comment line */
        if (THIS->buffer[THIS->curpos] == DEFAULT_CFGFILE_COMMENT_BEGIN) {
            skip_line(pstream);
            continue;
        }
        
        /* Check empty line */
        if (char_is_eol(THIS->buffer[THIS->curpos])) {
            skip_line(pstream);
            continue;
        }

        /* Calculate how many bytes we can provide */
        *plen = 0;
        while(THIS->curpos + (*plen)< THIS->maxlen && 
              !char_is_eol(THIS->buffer[THIS->curpos + (*plen)])) {
                
            (*plen)++;
        }
        while(THIS->curpos + (*plen) < THIS->maxlen && 
              char_is_eol(THIS->buffer[THIS->curpos + (*plen)])) {
                
            (*plen)++;
        }
        
        /* Return the data */
        *pline = &THIS->buffer[THIS->curpos];
        *pop = XCMDI_OP_EXECUTE;

        /* Advance data pointer */
        THIS->curpos += *plen;
        
        break;
    }

    return XCMD_ERR_OK;
}

static XCMD_ERROR
report_error(void *pstream, XCMD_ERROR err, const char *line, unsigned int len)
{
    return XCMD_ERR_OK;
}

static XCMD_ERROR
open(void *pstream, const char *prompt)
{
    if (pstream == NULL) {
        return XCMD_ERR_INTERNAL_INVALID_FUNC_PARAM;
    }
    
    THIS->depth++;

    return XCMD_ERR_OK;
}

static XCMD_ERROR
close(void *pstream)
{
    if (pstream == NULL || THIS->depth < 0) {
        return XCMD_ERR_INTERNAL_INVALID_FUNC_PARAM;
    }

    THIS->depth--;

    return XCMD_ERR_OK;
}

XCMD_ERROR
xcin_buffer_init(XCIN_BUFFER *ps, const char *buffer, unsigned int len)
{
    if (ps == NULL || buffer == NULL || len == 0) {
        return XCMD_ERR_INTERNAL_INVALID_FUNC_PARAM;
    }
    
    ps->base.open = open;
    ps->base.get_line = get_line;
    ps->base.report_error = report_error;
    ps->base.close = close;

    ps->buffer = buffer;
    ps->curpos = 0;
    ps->maxlen = len;
    ps->depth = -1;
    
    return XCMD_ERR_OK;
}

#endif /* CFG_XCOMMAND_INCLUDED */
