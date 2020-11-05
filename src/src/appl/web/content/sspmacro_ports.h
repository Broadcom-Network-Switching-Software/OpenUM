/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

/***** GENERATED FILE; DO NOT EDIT. *****/

#ifndef _SSPMACRO_PORTS_H_
#define _SSPMACRO_PORTS_H_


#define SSPMACRO_PORTS_ALLPORTS          (0)    /* Assigned */
#define SSPMACRO_PORTS_COLUMNPORTS     (100)    /* Assigned */
#define SSPMACRO_PORTS_RCOLUMNPORTS    (101)    /* Assigned */
#define SSPMACRO_PORTS_NOALIGN           (3)
#define SSPMACRO_PORTS_ONEROW        (5)
#define SSPMACRO_PORTS_LEFT_HALF        (24)
#define SSPMACRO_PORTS_RIGHT_HALF        (4)
#define SSPMACRO_PORTS_ROWSPAN       (1)
#define SSPMACRO_PORTS_NEWROW            (2)
#define SSPMACRO_PORTS_TWOROWS       (6)
#define SSPMACRO_PORTS_TOTAL_COUNT       (20)
#define SSPMACRO_PORTS_LINK              (7)
#define SSPMACRO_PORTS_FLOWCTRL          (8)
#define SSPMACRO_PORTS_PVID              (9)
#define SSPMACRO_PORTS_SPEED            (11)
#define SSPMACRO_PORTS_TYPE             (12)
#define SSPMACRO_PORTS_NUM              (13)
#define SSPMACRO_PORTS_ADMIN            (14)
#define SSPMACRO_PORTS_AUTONEGO         (15)
#define SSPMACRO_PORTS_PORTDESC		(16)
#define SSPMACRO_PORTS_PORTEN		(17)
#define SSPMACRO_PORTS_LEDPORTS		(18)
#define SSPMACRO_PORTS_LEDBASE		(19)

#define MAX_PORTS_PER_ROW 32

void sspvar_ports_tag_status(SSPTAG_PARAM *params, SSPVAR_RET *ret, SSP_PSMH psmem) REENTRANT;


#endif /* _SSPMACRO_PORTS_H_ */
