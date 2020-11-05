/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

/***** GENERATED FILE; DO NOT EDIT. *****/

#ifndef _SSPFILE_ERRORMSG_HTM_H_
#define _SSPFILE_ERRORMSG_HTM_H_

extern RES_CONST_DECL SSP_DATA_ENTRY CODE sspfile_errormsg_htm[];

extern void
webutil_show_error(SSP_HANDLER_CONTEXT *cxt, 
                   SSP_PSMH psmem,
                   const char *title,
                   const char *message,
                   const char *button,
                   const char *action
                   );

extern const char err_button_retry[];
extern const char err_action_back[];

#endif /* _SSPFILE_ERRORMSG_HTM_H_ */
