/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _UTILCBK_H_
#define _UTILCBK_H_

#include "appl/ssp.h" 

/* Utility: show web page of error messages.
 *          Used only in SSP handlers. 
 *          Must return SSP_HANDLER_RET_MODIFIED after calling this.
 */
void webutil_show_error(SSP_HANDLER_CONTEXT *cxt, 
           SSP_PSMH psmem,
           const char *title,
           const char *message,
           const char *button,
           const char *action
           );
           
/* Common string used in webutil_show_error() */
extern const char err_button_retry[];
extern const char err_action_back[];

#define MACADDR_STR_LEN 18              /* Formatted MAC address */
#define IPADDR_STR_LEN  16              /* Formatted IP address */

/*
 * util_format_macaddr requires a buffer of 18 bytes minimum.
 * It does not use sprintf so it can be called from an interrupt context.
 */
#ifdef ACL_MAC_SUPPORT
void utilcbk_format_macaddr(char buf[MACADDR_STR_LEN], unsigned char *macaddr);
#endif

/*
 * util_format_ipaddr requires a buffer of 16 bytes minimum.
 * It does not use sprintf so it can be called from an interrupt context.
 */
void utilcbk_format_ipaddr(char buf[IPADDR_STR_LEN], unsigned char * ipaddr);

/*
 * check whether ip mask is correct or not
 */
int utilcbk_ip_mask_check(unsigned char *ipmask);

#if 0
/*
 * Gets IP address in numberic form for the current HTTP session
 */
in_addr_t utilcbk_get_ip_address(SSP_PSMH psmem);
#endif

#endif /* _UTILCBK_H_ */
