/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Shell diagnostics of Phymod    
 */

#ifndef _PHYMOD_UTIL_H_
#define _PHYMOD_UTIL_H_

#include <phymod/phymod.h>


/******************************************************************************
Functions
 */


int phymod_util_lane_config_get(const phymod_access_t *phys, int *start_lane, int *num_of_lane);

#endif /*_PHYMOD_UTIL_H_*/
