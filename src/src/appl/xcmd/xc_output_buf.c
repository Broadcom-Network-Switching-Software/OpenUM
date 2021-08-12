/*
 * $Id: xc_output_buf.c,v 1.3 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"

#ifdef CFG_XCOMMAND_INCLUDED

#include "appl/xcmd/xcmd_internal.h"
#include "xcmd_core.h"
#include "xc_output_buf.h"

/* Use DOS format as default */
#define DOS_FORMAT_NEWLINE

#define THIS ((XCOUT_BUFFER *)pstream)
#define OVERFLOW(x) (THIS->curpos + x > THIS->maxlen)

static XCMD_ERROR
handle_BOL(void *pstream)
{
    int i;
    
    if (pstream == NULL) {
        return XCMD_ERR_INTERNAL_INVALID_STREAM_CALL;
    }
    
    if (THIS->bol) {
    
        /* Add comment to separate commands if it's at first level */
        if (THIS->depth == 0) {
            if (OVERFLOW(2)) {
                return XCMD_ERR_BUFFER_OVERFLOW;
            }
            THIS->buffer[THIS->curpos++] = DEFAULT_CFGFILE_COMMENT_BEGIN;
#ifdef DOS_FORMAT_NEWLINE
            THIS->buffer[THIS->curpos++] = '\r';
#endif /* DOS_FORMAT_NEWLINE */
            THIS->buffer[THIS->curpos++] = '\n';
        }
    
        /* Add spaces to indicate depth */
        for(i=0; i<THIS->depth; i++) {
            if (OVERFLOW(1)) {
                return XCMD_ERR_BUFFER_OVERFLOW;
            }
            THIS->buffer[THIS->curpos++] = DEFAULT_CFGFILE_DEPTH_CHAR;
        }
        
        THIS->bol = 0;
    }
    
    return XCMD_ERR_OK;
}

static XCMD_ERROR
write_str(void *pstream, const char *str, unsigned int len)
{
    XCMD_ERROR r;
    unsigned int i;
    
    if (pstream == NULL || str == NULL || THIS->depth < 0) {
        return XCMD_ERR_INTERNAL_INVALID_STREAM_CALL;
    }
    
    if (len == 0) {
        return XCMD_ERR_OK;
    }
    
    /* Check if it has EOL in it */
    for(i=0; i<len; i++) {
        if (str[i] == '\r' || str[i] == '\n') {
            /* Actuall we can handle it, but for what? */
            return XCMD_ERR_INTERNAL_INVALID_STREAM_CALL;
        }
    }
    
    /* Add line prefixes */
    r = handle_BOL(pstream);
    if (r != XCMD_ERR_OK) {
        return r;
    }
    
    /* write to buffer if not overflow */
    if (OVERFLOW(len)) {
        return XCMD_ERR_BUFFER_OVERFLOW;
    }
    sal_strncpy(&THIS->buffer[THIS->curpos], str, len);
    THIS->curpos += len;
    
    return XCMD_ERR_OK;
}

static XCMD_ERROR
write_cr(void *pstream)
{
    XCMD_ERROR r;

    if (pstream == NULL || THIS->depth < 0) {
        return XCMD_ERR_INTERNAL_INVALID_STREAM_CALL;
    }
    
    /* Check for continuous CR */
    r = handle_BOL(pstream);
    if (r != XCMD_ERR_OK) {
        return r;
    }

    /* Append CR */
    if (OVERFLOW(1)) {
        return XCMD_ERR_BUFFER_OVERFLOW;
    }

#ifdef DOS_FORMAT_NEWLINE
    THIS->buffer[THIS->curpos++] = '\r';
#endif /* DOS_FORMAT_NEWLINE */
    THIS->buffer[THIS->curpos++] = '\n';

    THIS->bol = 1;
    
    return XCMD_ERR_OK;
}

static XCMD_ERROR
report_error(void *pstream, XCMD_ERROR err, const char *command)
{
    XCMD_ERROR r;
    char buffer[32];
    int len;

    if (pstream == NULL || THIS->depth < 0) {
        return XCMD_ERR_INTERNAL_INVALID_STREAM_CALL;
    }
    
    if (!THIS->bol) {
        /* If this is not called at the beginning of line */
        r = write_cr(pstream);
        if (r != XCMD_ERR_OK) {
            return r;
        }
    }
    
    /* This line should also be aligned to the same level */
    r = handle_BOL(pstream);
    if (r != XCMD_ERR_OK) {
        return r;
    }
    
    /* Error description starts as a comment */
    if (OVERFLOW(1)) {
        return XCMD_ERR_BUFFER_OVERFLOW;
    }
    THIS->buffer[THIS->curpos++] = DEFAULT_CFGFILE_COMMENT_BEGIN;
    
    /* Append error header */
    sal_sprintf(buffer, DEFAULT_CFGFILE_ERROR_FORMAT, err);
    buffer[sizeof(buffer) - 1] = 0;
    len = sal_strlen(buffer);
    if (THIS->curpos + len > THIS->maxlen) {
        return XCMD_ERR_BUFFER_OVERFLOW;
    }
    sal_strncpy(&THIS->buffer[THIS->curpos], buffer, len);
    THIS->curpos += len;
    
    /* Append problematic command */
    if (command) {
        len = sal_strlen(command);
        if (THIS->curpos + len > THIS->maxlen) {
            return XCMD_ERR_BUFFER_OVERFLOW;
        }
        sal_strncpy(&THIS->buffer[THIS->curpos], command, len);
        THIS->curpos += len;
    }
    
    /* Write CR */
    THIS->bol = 0;
    r = write_cr(pstream);
    if (r != XCMD_ERR_OK) {
        return r;
    }
    
    return XCMD_ERR_OK;
}

static XCMD_ERROR
open(void *pstream)
{
    XCMD_ERROR r;

    if (pstream == NULL) {
        return XCMD_ERR_INTERNAL_INVALID_STREAM_CALL;
    }

    THIS->depth++;

    if (!THIS->bol) {
        /* If this is not called at the beginning of line */
        r = write_cr(pstream);
        if (r != XCMD_ERR_OK) {
            return r;
        }
    }
    
    return XCMD_ERR_OK;
}

static XCMD_ERROR
close(void *pstream)
{
    XCMD_ERROR r;

    if (pstream == NULL || THIS->depth < 0) {
        return XCMD_ERR_INTERNAL_INVALID_STREAM_CALL;
    }
    
    if (!THIS->bol) {
        /* If this is not called at the beginning of line */
        r = write_cr(pstream);
        if (r != XCMD_ERR_OK) {
            return r;
        }
    }
    
    /* Make the ending more beautiful */
    if (THIS->depth == 0) {
        THIS->buffer[THIS->curpos++] = DEFAULT_CFGFILE_COMMENT_BEGIN;
#ifdef DOS_FORMAT_NEWLINE
        THIS->buffer[THIS->curpos++] = '\r';
#endif /* DOS_FORMAT_NEWLINE */
        THIS->buffer[THIS->curpos++] = '\n';
    }

    THIS->depth--;

    return XCMD_ERR_OK;
}

XCMD_ERROR
xcout_buffer_init(XCOUT_BUFFER *ps, char *buffer, unsigned int len)
{
    if (ps == NULL || (buffer != NULL && len == 0)) {
        return XCMD_ERR_INTERNAL_INVALID_FUNC_PARAM;
    }
    
    ps->base.open = open;
    ps->base.write_str = write_str;
    ps->base.write_cr = write_cr;
    ps->base.report_error = report_error;
    ps->base.close = close;
    
    ps->buffer = buffer;
    ps->maxlen = len;
    ps->curpos = 0;
    ps->depth = -1;
    ps->bol = 1;

    /* For counting only, call with buffer = NULL */
    if (buffer == NULL) {
        ps->maxlen = (unsigned int)-1;
    }
    
    return XCMD_ERR_OK;
}

#endif /* CFG_XCOMMAND_INCLUDED */
