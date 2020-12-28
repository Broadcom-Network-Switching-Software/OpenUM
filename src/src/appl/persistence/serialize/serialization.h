/*
 * $Id: serialization.h,v 1.4 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _SERIALIZATION_H_
#define _SERIALIZATION_H_

#include "appl/serialize.h"
#include "appl/medium.h"

extern int32 serialize_by_groups(uint32 groups, SERIALIZE_OP op, PERSISTENT_MEDIUM *medium);
extern BOOL serialize_by_name(const char *name, SERIALIZE_OP op, PERSISTENT_MEDIUM *medium);
extern int32 serialize_medium_for_validation(PERSISTENT_MEDIUM *med, BOOL defaults);

#endif /* _SERIALIZATION_H_ */
