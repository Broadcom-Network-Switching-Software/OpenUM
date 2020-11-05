/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _BRDIMPL_H_
#define _BRDIMPL_H_

#include "soc/soc.h"

/*
 * Typical board function implementation (most boards would fit)
 */
#include "brdimpl/rxtx.h"
#include "brdimpl/vlan.h"
#include "brdimpl/port.h"

#ifdef CFG_RESET_BUTTON_INCLUDED
void brdimpl_reset_button_detect(void *param) REENTRANT;
#endif /* CFG_RESET_BUTTON_INCLUDED */

#endif /* _BRDIMPL_H_ */
