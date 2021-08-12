/*
 * $Id: $
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 *
 * Broadcom Built-in binary files.
 *
 * binfs includes
 * 1. A script to generate binfs data base. ($UM/tools/genbinfs.py)
 * 2. Built-in file access APIs.
 */

#include <system.h>
#include <utils/shr/shr_debug.h>
#include <binfs.h>

int
binfs_file_data_get(char *name, const uint8 **data, int *len)
{
   const binfs_file_t *binfs;
   int i;

   SHR_LOG_DEBUG("Get binfs %s\n", name);

   i = 0;
   binfs = binfs_file_list[i];

   while (binfs) {

       if (sal_strncmp(name, binfs->name, sal_strlen(binfs->name)) == 0) {
           *data = binfs->data;
           *len = binfs->len;
           return SYS_OK;
       }
       i++;
       binfs = binfs_file_list[i];
   }

   return SYS_ERR;
}
