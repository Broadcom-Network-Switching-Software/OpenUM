/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#include "httpd_arch.h"

char *
gettoken(char **ptr)
{
    char *p = *ptr;
    char *ret;

    /* skip white space */

    while(*p && isspace(*p))
        p++;
    ret = p;

    /* check for end of string */

    if (!*p) {
        *ptr = p;
        return NULL;
    }

    /* skip non-whitespace */

    while(*p) {
        if (isspace(*p))
            break;

        /* do quoted strings */

        if (*p == '"') {
            p++;
            ret = p;
            while(*p && (*p != '"'))
                p++;
            if (*p == '"')
                *p = '\0';
        }

        p++;

    }

    if (*p) {
        *p++ = '\0';
    }
    *ptr = p;

    return ret;
}


/* uIP.TCP adaptor for HTTPd */
int  
uiptcp_send(uint8 * buf, int32 len)
{
    /* To send httpd data through uIP, the length must not exceeed the 
     *  defined max segement size in uIP. This size is the max length that 
     *  we can send through uip_send and this value will be different 
     *  while device is or isn't in LAN area. 
     *
     *  Normally, the ps->mss is expected be no larger than uip_mss().
     *    Once the len > uip_mss condition occurred, the reason might be the 
     *  limitation of uIP native arch. An complex network environment might 
     *  causes the uip_mss been updated due to different network connection 
     *  been established(TCP/UDP, IPv4/IPv6...).
     *
     *    Here we need to ensure the sending data is under the most current 
     *  uip_mss() to prevent the unexpect behavior. 
     */
#if CFG_IP_FRAGMENT_INCLUDED
    /* 
    * keep the packet size even if > uip_mss(), which is 1460B
    */
#else
    if (len > uip_mss()){
#ifndef __BOOTLOADER__
        HTTPD_LOG(("uiptcp_send: failed due to len=%d exceeds mss=%d\n", (int)len, uip_mss()));
#endif /* __BOOTLOADER__ */
        len = uip_mss();
    }
#endif//CFG_IP_FRAGMENT_INCLUDED
    HTTPD_CONTENT_PRINTF(("uiptcp_send:len=%d:\n%s\n",(int)len,buf));
    uip_send(buf, (int)len);
    return 0;
}

int32
uiptcp_recv(uint8 * buf, int32 len)
{
    int ret_len;

    ret_len = uip_datalen();
    if (len > UIP_BUFSIZE) {
        len = UIP_BUFSIZE;
    }

    if (len < ret_len){
        ret_len = len;
    }

    sal_memcpy(buf, uip_appdata, ret_len);
    HTTPD_CONTENT_PRINTF(("uiptcp_recv:\n%s\n",buf));
    return (int32)ret_len;
}


