/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "appl/ssp.h"

extern SSP_FILE_ENTRY CODE ssp_fs_root[];

struct SSP_FILESYSTEM ssp_filesystem_table[] = {
    { "/", ssp_fs_root, NULL, 0 },
    { NULL, NULL, NULL, 0 }
};

