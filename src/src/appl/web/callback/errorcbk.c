/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "appl/ssp.h"
#include "utilcbk.h"

const char err_button_retry[] = " Retry ";
const char err_action_back[] = "history.go(-1)";

void
sspvar_error_tag_title(SSPTAG_PARAM *params, SSPVAR_RET *ret, SSP_PSMH psmem)
{
    char *str = ssputil_psmem_get(psmem, sspvar_error_tag_title);
    if (str == NULL) {
        return;
    }
    
    ret->type = SSPVAR_RET_STRING;
    ret->val_data.string = str;
}

void
sspvar_error_tag_message(SSPTAG_PARAM *params, SSPVAR_RET *ret, SSP_PSMH psmem)
{
    char *str = ssputil_psmem_get(psmem, sspvar_error_tag_message);
    if (str == NULL) {
        return;
    }
    
    ret->type = SSPVAR_RET_STRING;
    ret->val_data.string = str;
}

void
sspvar_error_tag_button(SSPTAG_PARAM *params, SSPVAR_RET *ret, SSP_PSMH psmem)
{
    char *str = ssputil_psmem_get(psmem, sspvar_error_tag_button);
    if (str == NULL) {
        return;
    }
    
    ret->type = SSPVAR_RET_STRING;
    ret->val_data.string = str;
}

void
sspvar_error_tag_action(SSPTAG_PARAM *params, SSPVAR_RET *ret, SSP_PSMH psmem)
{
    char *str = ssputil_psmem_get(psmem, sspvar_error_tag_action);
    if (str == NULL) {
        return;
    }
    
    ret->type = SSPVAR_RET_STRING;
    ret->val_data.string = str;
}

void
webutil_show_error(SSP_HANDLER_CONTEXT *cxt, 
                   SSP_PSMH psmem,
                   const char *title,
                   const char *message,
                   const char *button,
                   const char *action
                   )
{
    char *str;
    
    if (cxt == NULL) {
        return;
    }
    
    if (title != NULL) {
        str = ssputil_psmem_alloc(
                psmem, 
                sspvar_error_tag_title, 
                sal_strlen(title) + 1
                );
        sal_strcpy(str, title);
    }
    
    if (message != NULL) {
        str = ssputil_psmem_alloc(
                psmem, 
                sspvar_error_tag_message, 
                sal_strlen(message) + 1
                );
        sal_strcpy(str, message);
    }
    
    if (button != NULL) {
        str = ssputil_psmem_alloc(
                psmem, 
                sspvar_error_tag_button, 
                sal_strlen(button) + 1
                );
        sal_strcpy(str, button);
    }
    
    if (action != NULL) {
        str = ssputil_psmem_alloc(
                psmem, 
                sspvar_error_tag_action, 
                sal_strlen(action) + 1
                );
        sal_strcpy(str, action);
    }
    
    cxt->page = ssputil_locate_file(psmem, "/errormsg.htm", NULL);
    cxt->flags &= ~SSPF_FORCE_CACHE;
    cxt->flags |= SSPF_NO_CACHE;
}
