/*! \file util_symbol_blk.c
 *
 * The related APIs to handle the block of the symbol
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"

#ifdef CFG_CHIP_SYMBOLS_INCLUDED
#include <utils/sym/util_symbol_blk.h>
const chip_block_t*
util_blk_get(int unit, int blknum)
{
    chip_info_t *chip_info = board_get_chipinfo_by_unit(unit);
    int i;

    if (!chip_info) {
        return NULL;
    }

    for (i = 0; i < chip_info->nblocks; i++) {
        const chip_block_t* block = &(chip_info->blocks[i]);
        if (block->blknum == blknum) {
            return block;
        }
    }
    return NULL;
}

const char*
util_blktype_name_get(int unit, const symbol_t *symbol)
{
    chip_info_t *chip_info = board_get_chipinfo_by_unit(unit);
    int i;

    if (!chip_info || !symbol) {
        return NULL;
    }

    for (i = 0; i < chip_info->nblktypes; i++) {
        if ((1 << i) & SYMBOL_FLAG_BLKTYPES(symbol->flags)) {
            return chip_info->blktype_names[i];
        }
    }
    return NULL;
}

int
util_blktype_inst_get(int unit, const symbol_t *symbol, int blknum)
{
    chip_info_t *chip_info = board_get_chipinfo_by_unit(unit);
    int i, inst = 0;

    if (!chip_info || !symbol) {
        return -1;
    }

    for (i = 0; i < chip_info->nblocks; i++) {
        const chip_block_t* block = &(chip_info->blocks[i]);
        if (block->blknum == blknum) {
            return inst;
        } else if ((1 << block->type) & SYMBOL_FLAG_BLKTYPES(symbol->flags)) {
            inst++;
        }
    }
    return -1;
}

int
util_blk_range_get(int unit, const char *blktype_name, int blkinst,
                   int *blk_num_start, int *blk_num_end)
{
    chip_info_t *chip_info = board_get_chipinfo_by_unit(unit);
    int blktype_id = -1;
    int i;
    int blk_s, blk_e, blk_num;

    if (!chip_info || !blktype_name || !blk_num_start || !blk_num_end) {
        return -1;
    }

    for (i = 0; i < chip_info->nblktypes; i++) {
        if (sal_strcmp(chip_info->blktype_names[i], blktype_name) == 0) {
            blktype_id = i;
            break;
        }
    }
    if (blktype_id == -1) {
        sal_printf("unknown blktype name %s\n", blktype_name);
        return -1;
    }

    /* start to traverse each physical block */
    blk_s = -1;
    blk_e = -1;
    blk_num = 0;
    for (i = 0; i < chip_info->nblocks; i++) {
        if (chip_info->blocks[i].type == blktype_id) {
            if (blkinst < 0) {
                /* get the range of blocks with same blktype */
                if (blk_s == -1) {
                    blk_s = chip_info->blocks[i].blknum;
                }
                blk_e = chip_info->blocks[i].blknum;
            } else {
                /* if the user has specified block inst id */
                if (blk_num == blkinst) {
                    blk_s = chip_info->blocks[i].blknum;
                    blk_e = chip_info->blocks[i].blknum;
                    break;
                }
                blk_num++;
            }
        }
    }

    if (blk_s == -1 || blk_e == -1) {
        sal_printf("cannot find any blocks with type %s", blktype_name);
        if (blkinst >= 0) {
            sal_printf(" and inst %d (max : %d)", blkinst, blk_num - 1);
        }
        sal_printf("\n");
        return -1;
    }

    *blk_num_start = blk_s;
    *blk_num_end = blk_e;
    return 0;
}
#endif /* CFG_CHIP_SYMBOLS_INCLUDED */
