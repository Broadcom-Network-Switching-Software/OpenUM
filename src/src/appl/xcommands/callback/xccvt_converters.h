/*
 * $Id: xccvt_converters.h,v 1.5 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _XCCVT_CONVERTERS_H_
#define _XCCVT_CONVERTERS_H_


#include "system.h"
#if defined(CFG_XCOMMAND_INCLUDED) || defined(CFG_VENDOR_CONFIG_SUPPORT_INCLUDED)


#include "appl/xcmd/xcmd_public.h"


typedef struct {    
  uint8 bytes[MAX_UPLIST_WIDTH];
} uplist_t;

extern XCMD_ERROR xc_uplist_from_string(const char *string,unsigned int len, void *prop, uplist_t *puplist);
extern XCMD_ERROR xc_uplist_to_string(const uplist_t *puplist, void *prop,char **pstring);
extern XCMD_ERROR xc_pbmp_from_string(const char *string, int len, void *prop, pbmp_t *pbmp);
extern XCMD_ERROR xc_pbmp_to_string(const pbmp_t pbmp, void *prop, char **pstring);

#endif

#endif /* _XCCVT_CONVERTERS_H_ */
