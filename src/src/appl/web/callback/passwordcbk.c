/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "appl/ssp.h"
#include "utilcbk.h"
#include "utils/net.h"
#include "appl/persistence.h"
#include "../content/errormsg_htm.h"

#ifdef CFG_SYSTEM_PASSWORD_INCLUDED
/*
 * For performace on checking, we save the current corresponding cookie value
 */
#define MAX_COOKIE_LEN      (24)
static char cached_cookie_value[MAX_SSS_USER_COUNT][MAX_COOKIE_LEN + 1];
/* static BOOL cached_cookie_initialized = FALSE; */

/* Forwards */
/* static void make_cookie_cache(int user_id); */

SSP_HANDLER_RETVAL
ssphandler_password_cgi(SSP_HANDLER_CONTEXT *cxt, SSP_PSMH psmem)
{
    char lpwd[MAX_USERNAME_LEN + 1];
    int  user_id = 0;

    if (cxt->type == SSP_HANDLER_REQ_COOKIE) {
        return SSP_HANDLER_RET_INTACT;
    }

    if (cxt->type != SSP_HANDLER_QUERY_STRINGS) {
        return SSP_HANDLER_RET_INTACT;
    }

    if (cxt->count != 3) {
        return SSP_HANDLER_RET_INTACT;
    }

    get_login_password(lpwd, sizeof(lpwd));

    if (sal_strcmp(cxt->pairs[0].value, lpwd)) {
          webutil_show_error(
            cxt, psmem,
            "Change Password",
            "Incorrect old password!",
            err_button_retry,
            err_action_back
            );
        return SSP_HANDLER_RET_MODIFIED;
    }

    if (sal_strcmp(cxt->pairs[1].value, cxt->pairs[2].value)) {
          webutil_show_error(
            cxt, psmem,
            "Change Password",
            "New password unmatched!",
            err_button_retry,
            err_action_back
            );
        return SSP_HANDLER_RET_MODIFIED;
    }

    if (sal_strlen(cxt->pairs[1].value) == 0) {
        sal_sprintf(ssputil_shared_buffer,
                "Can't be without password !");
        webutil_show_error(
            cxt, psmem,
            "Change Password",
            ssputil_shared_buffer,
            err_button_retry,
            err_action_back
            );
        return SSP_HANDLER_RET_MODIFIED;
    }

    if (sal_strlen(cxt->pairs[1].value) > MAX_PASSWORD_LEN) {
        sal_sprintf(ssputil_shared_buffer,
                "Password cannot exceed %d characters!",
                MAX_PASSWORD_LEN);
        webutil_show_error(
            cxt, psmem,
            "Change Password",
            ssputil_shared_buffer,
            err_button_retry,
            err_action_back
            );
        return SSP_HANDLER_RET_MODIFIED;
    }

    set_login_password(cxt->pairs[1].value);

    /* Forced cached cookie to be re-calculated */
    cached_cookie_value[user_id][0] = 0;
    /* make_cookie_cache(user_id); */

    /* Save it to persistent medium */
#if CFG_PERSISTENCE_SUPPORT_ENABLED
    persistence_save_current_settings("password");
#endif

    return SSP_HANDLER_RET_INTACT;
}

#endif  /* CFG_SYSTEM_PASSWORD_INCLUDED */

