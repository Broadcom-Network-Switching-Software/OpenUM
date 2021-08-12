/*
 * $Id: chip.h,v 1.2 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 *
 * Common chip definitions.
 */

#ifndef __CHIP_H__
#define __CHIP_H__


/*!
 * \brief Block information structure.
 *
 * This structure will be auto-generated for each physical block in
 * the device.
 */
typedef struct chip_block_s {
    /*! Block type */
    int type;

    /*! Physical block number */
    int blknum;

    /*! Port Bitmaps */
    pbmp_t pbmps;
} chip_block_t;

/*!
 * \brief Address calculation function for registers and memories.
 *
 * Used to override the default address calculation for a particular
 * Host Management Interface (HMI).
 *
 * A typical use case is transitional devices, which use a new CMIC
 * register interface, but retain the S-channel protocol of the
 * previous CMIC version. Subsequent devices will also use a new
 * S-channel protocol, so the address calculation of the transitional
 * device(s) must be treated as an exception.
 *
 * \param [in] block Block number.
 * \param [in] port Port number for block context.
 * \param [in] offset Base address for register/memory.
 * \param [in] idx Entry number for array-based register/memory.
 *
 * \return Lower 32-bit of register/memory address.
 */
typedef uint32_t (*chip_block_port_addr_f)(int block, int port,
                                           uint32_t offset, uint32_t idx);


/*!
 * \brief Fixed chip information.
 */
typedef struct chip_info_s {
    /*! Number of block types. */
    int nblktypes;

    /*! block types. */
    const char **blktype_names;

    /*! Number of block structures. */
    int nblocks;

    /*! Block structure array. */
    const chip_block_t *blocks;

    /*! Offset/Address Vectors. */
    chip_block_port_addr_f block_port_addr;

    /*! Valid ports for this chip. */
    pbmp_t valid_pbmps;

    /*! Chip symbol table pointer. */
    const symbols_t *symbols;
} chip_info_t;


/* Initializer macros */
#define CHIP_PBMP_1(_w0)                 { { _w0 } }
#define CHIP_PBMP_2(_w0, _w1)            { { _w0, _w1 } }
#define CHIP_PBMP_3(_w0, _w1, _w2)       { { _w0, _w1, _w2 } }
#define CHIP_PBMP_4(_w0, _w1, _w2, _w3)  { { _w0, _w1, _w2, _w3 } }
#define CHIP_PBMP_5(_w0, _w1, _w2, _w3, _w4) \
                                        { { _w0, _w1, _w2, _w3, _w4 } }

#endif /* __CHIP_H__ */
