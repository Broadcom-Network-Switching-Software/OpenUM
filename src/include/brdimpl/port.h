/*
 * $Id: port.h,v 1.6 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _BRDIMPL_PORT_H_
#define _BRDIMPL_PORT_H_

/*
 * Cable Diagnostics
 */
extern sys_error_t brdimpl_port_cable_diag(uint16 uport, 
                                           port_cable_diag_t *status);


#endif /* _BRDIMPL_PORT_H_ */
