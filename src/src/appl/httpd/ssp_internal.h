/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _SSP_INTERNAL_H_
#define _SSP_INTERNAL_H_

#include "appl/ssp.h"

/* adjust ssp buffer size from 4096 to 1024 due to most of the web constant 
 *  for UM web pages under 1K size. 
 */
#define SSP_SESSION_BUFFER_SIZE     (3072)
#define SSP_MAX_NUM_QUERY_STRINGS   (256)
#define SSP_MAX_LOOP_DEPTH          (3)
#define SSP_MAX_NUM_PARAMETERS      (3)

#endif /* _SSP_INTERNAL_H_ */

