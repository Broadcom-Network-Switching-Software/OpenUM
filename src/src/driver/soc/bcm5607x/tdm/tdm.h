/*
 * $Id: tdm.h,v 1.1 Broadcom SDK $
 * 
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __TDM_H__
#define __TDM_H__

//#include <cdk/cdk_types.h>
#include "tdm_defines.h"
#include "tdm_soc.h"
#include "tdm_top.h"

#define bcm5607x_a0_sel_tdm             SOC_SEL_TDM
#define bcm5607x_a0_set_tdm_tbl         _soc_set_tdm_tbl
#define bcm5607x_a0_set_iarb_tdm_table  tdm_mn_set_iarb_tdm

#endif /* __TDM_H__ */
