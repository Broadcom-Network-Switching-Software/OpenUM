/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _ERROR_H_
#define _ERROR_H_

typedef enum {
    SYS_OK                          =     0,
    SYS_ERR_FALSE                   =    -1,   /* FALSE condition */
    SYS_ERR                         =    -2,   /* generic error */
    SYS_ERR_CANCELLED               =    -3,
    SYS_ERR_PARAMETER               =    -4,
    SYS_ERR_TIMEOUT                 =    -5,
    SYS_ERR_FULL                    =    -6,
    SYS_ERR_STATE                   =    -7,
    SYS_ERR_OUT_OF_RESOURCE         =    -8,
    SYS_ERR_NOT_IMPLEMENTED         =    -9,
    SYS_ERR_EXISTS                  =    -10,
    SYS_ERR_NOT_FOUND               =    -11
} sys_error_t;

#endif /* _ERROR_H_ */
