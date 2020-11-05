/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

/*
 * Interfaces of WCP - Web Content Provider for HTTPd
 */

#ifndef _HTTPD_WCP_H_
#define _HTTPD_WCP_H_

#include "appl/httpd.h"

#ifdef HTTPD_REXMIT_SUPPORT
#define WCP_REXMIT_SUPPORT
#endif  /* HTTPD_REXMIT_SUPPORT */

typedef void *WCP_HANDLE; 

/*  *********************************************************************
    *  wcp_open(post,path,filename,params)
    *  
    *  Open a web page from the WCP.
    *  
    *  Input parameters: 
    *       post - 1 if it's a POST request. 0 if GET.
    *       path - path and filename of the URL
    *       params - parameters after URL (normally for GET request)
    *       sock - socket for this connection
    *       service - HTTP service it's requesting
    *      
    *  Return value:
    *       Handle to this WCP session.
 */
WCP_HANDLE wcp_open(uint8, const char *, const char *, int, HTTP_SERVICE *);

void wcp_req_header(WCP_HANDLE wh, const char *name, const char *value);

void wcp_req_content_length(WCP_HANDLE wh, uint32 len);

void wcp_req_post_data(WCP_HANDLE wh, const uint8 *buf, uint32 len);

/* returns: status code string */
const char *wcp_req_end(WCP_HANDLE wh, int32 *ctlen);

void wcp_close(WCP_HANDLE wh);

void *wcputil_memcpy(void *dest, const void *src, size_t n, WCP_HANDLE wh);

#ifdef WCP_REXMIT_SUPPORT

void wcp_rexmit_clear(WCP_HANDLE wh);
const char *wcp_reply_headers(WCP_HANDLE wh, int rexmit);
const uint8 *wcp_reply_content_data(WCP_HANDLE wh, uint32 *len, int rexmit);
#else  /* WCP_REXMIT_SUPPORT */

const char *wcp_reply_headers(WCP_HANDLE wh);
const uint8 *wcp_reply_content_data(WCP_HANDLE wh, uint32 *len);
#endif  /* WCP_REXMIT_SUPPORT */

#endif /* _HTTPD_WCP_H_ */
