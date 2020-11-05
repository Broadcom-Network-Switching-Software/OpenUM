/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _HTTPD_H_
#define _HTTPD_H_

/* Flags for HTTP service */
typedef enum {
    HTTPSVC_FLAG_HTTPS = 0x01
} HTTPSVC_FLAG;


/* HTTP Services */
typedef struct {
    char name[16];          /* Name of this service */
    uint16 port;    /* TCP port number */
    HTTPSVC_FLAG flags;     /* Flags */
    void *wcp_param;        /* Parameter applied to WCP */
    void *reserved;         /* Reserved for internal use */
} HTTP_SERVICE;

/* uIP.TCP appcall */
void httpd_appcall(void);

/* Initialize HTTPd */
void httpd_init(void);
void httpd_uninit(void);

/* REXMIT feature is to re-transmit previous data again.
 * 
 * Note: 
 *  This is for the support while TCP layer issue a REXMIT request due to the remote peer 
 *  didn't ACK our previoue replied data.
 */
#define HTTPD_REXMIT_SUPPORT


#define HTTPD_TIMER_SUPPORT
#ifdef HTTPD_TIMER_SUPPORT
/* Utility: timer callback (resolution: second) */
#define HTTPD_TIMER_INTERVAL    1000000UL   /* 1 sec */

typedef void *HTTPD_TIMER_HANDLE;
HTTPD_TIMER_HANDLE httpd_create_timer(unsigned int s, void (*func)(void *), void *in_data);
void httpd_delete_timer(HTTPD_TIMER_HANDLE handle);
void httpd_delete_timers_by_callback(void (*func)(void *));
#endif /* HTTPD_TIMER_SUPPORT */

#endif /* _HTTPD_H_ */
