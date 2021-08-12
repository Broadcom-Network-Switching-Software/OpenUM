/*
 * $Id: util_symbol_blk.h,v 1.2 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 *
 * Chip symbol table definitions.
 */

#ifndef __UTIL_SYMBOL_BLK_H__
#define __UTIL_SYMBOL_BLK_H__

/*!
 * \brief Get block from physical block id.
 *
 * Convert physical block id to block structure.
 *
 * \param [in] unit Unit number.
 * \param [in] blknum Physical block id.
 *
 * \retval Device-specified block structure on success.
 * \retval NULL on failure.
 */
extern const chip_block_t* util_blk_get(int unit, int blknum);

/*!
 * \brief Get the name of block type from symbol.
 *
 * Get the name of block type from symbol.
 *
 * \param [in] unit Unit number.
 * \param [in] symbol Symbol information structure
 *
 * \retval Name of block type on success.
 * \retval NULL on failure.
 */
const char* util_blktype_name_get(int unit, const symbol_t *symbol);

/*!
 * \brief Get the instance index accoring to the block type of the symbol.
 *
 * ex. if the block type of symbol is BLKTYPE_GPORT,
 *     and the chip_info->blocks is listed as following:
 *     { BLKTYPE_GPORT, 29}
 *     { BLKTYPE_GPORT, 33}
 *     { BLKTYPE_GPORT, 37}
 *     { BLKTYPE_GPORT, 41}
 *     { BLKTYPE_GPORT, 42}
 *     { BLKTYPE_GPORT, 45}
 *   ==> util_blktype_inst_get(symbol, 29) = 0
 *       util_blktype_inst_get(symbol, 33) = 1
 *       util_blktype_inst_get(symbol, 42) = 4
 *
 * \param [in] unit Unit number.
 * \param [in] symbol Symbol information structure.
 * \param [in] blknum Physical block id.
 *
 * \retval Non-negative instance index on success.
 * \retval -1 on failure.
 */
int util_blktype_inst_get(int unit, const symbol_t *symbol, int blknum);

/*!
 * \brief Get the range of physical block id according to the block type.
 *
 * ex. Asseumed that chip_info->blocks is listed as following:
 *     { BLKTYPE_GPORT, 29}
 *     { BLKTYPE_GPORT, 33}
 *     { BLKTYPE_GPORT, 37}
 *     { BLKTYPE_GPORT, 41}
 *     { BLKTYPE_GPORT, 42}
 *     { BLKTYPE_GPORT, 45}
 *  ==> util_blk_range_get("gport", -1) ==> start = 29, end = 45
 *      util_blk_range_get("gport", 0) ==> start = 29, end = 29
 *      util_blk_range_get("gport", 2) ==> start = 37, end = 37
 *      util_blk_range_get("gport", 6) ==> reurn -1
 *
 * \param [in] unit Unit number.
 * \param [in] blktype_name Block name.
 * \param [in] blkinst Block instance index, uses -1 if not specified
 * \param [out] blk_num_start The beginning physical block id.
 * \param [out] blk_num_end The ended physical block id.
 *
 * \retval 0 on success.
 * \retval -1 on failure.
 */
int util_blk_range_get(int unit, const char *blktype_name, int blkinst,
                       int *blk_num_start, int *blk_num_end);
#endif /* __UTIL_SYMBOL_BLK_H__ */
