/*! \file bcma_bcmpkt_parse_data.c
 *
 * Parse data drivers for tx command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#ifdef CFG_SDKCLI_INCLUDED
#include "cli_porting.h"

#if CFG_RXTX_SUPPORT_ENABLED

#include <appl/sdkcli/bcma_cli.h>
#include <appl/sdkcli/bcma_cli_parse.h>
#include <utils/shr/shr_pb_format.h>

/* Module header Op-Codes */
#define HIGIG_OP_CPU        0x00 /* CPU Frame */
#define HIGIG_OP_UC         0x01 /* Unicast Frame */
#define HIGIG_OP_BC         0x02 /* Broadcast or DLF frame */
#define HIGIG_OP_MC         0x03 /* Multicast Frame */
#define HIGIG_OP_IPMC       0x04 /* IP Multicast Frame */

static int
parse_bitmap_str(const char *str, SHR_BITDCL *bits, int size)
{
    if (!sal_strcasecmp(str, "none")) {
        SHR_BITCLR_RANGE(bits, 0, size);
        return 0;
    }

    return bcma_cli_parse_bit_list(str, size, bits);
}

static int
bmp_parse_arg(const char *arg, void *data, void **option)
{
    SHR_BITDCL *bits;
    int size;

    if (data == NULL || option == NULL || *option == NULL) {
        cli_out("NULL pointer!\n");
        return -1;
    }
    bits = (SHR_BITDCL *)data;
    size = **(int **)option;
    if (parse_bitmap_str(arg, bits, size) != 0) {
        cli_out("Input error!\n");
        return -1;
    }

    return 0;
}

static char *
bmp_format_arg(const void *data, const void *option, char *buf, int bufsz)
{
    SHR_BITDCL *bits;
    int size, len;
    const char *str;
    shr_pb_t *pb;

    if (data == NULL || option == NULL) {
        cli_out("NULL pointer!\n");
        return NULL;
    }

    bits = (SHR_BITDCL *)data;
    size = *(int *)option;

    pb = shr_pb_create();
    str = shr_pb_format_bit_list(pb, bits, size);
    len = sal_strlen(str);
    if (len < bufsz) {
        sal_memcpy(buf, str, len + 1);
        str = buf;
    } else {
        str = "<overrun>";
    }
    shr_pb_destroy(pb);

    return (char *)str;
}

static bcma_cli_parse_data_t parse_data_bmp = {
    "bmp",
    bmp_parse_arg,
    bmp_format_arg,
    NULL
};

int
bcma_bcmpkt_parse_data_add(void)
{
    bcma_cli_parse_data_add(&parse_data_bmp);

    return BCMA_CLI_CMD_OK;
}
#endif
#endif
