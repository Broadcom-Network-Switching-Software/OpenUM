/*! \file bcma_clicmd_setget.h
 *
 * CLI 'set' and 'get' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef BCMA_CLICMD_SETGET_H
#define BCMA_CLICMD_SETGET_H

#include <appl/sdkcli/bcma_cli.h>

/*! Brief description for get command. */
#define BCMA_CLICMD_GET_DESC \
    "Get chip register/memory contents."

/*! Syntax for get command. */
#define BCMA_CLICMD_GET_SYNOP_SUB1 \
    "[NonZero] [Raw] [CompactFormat] <symbol>\n"

#define BCMA_CLICMD_GET_SYNOP_SUB2 \
    "[NonZero] <symbol>\n"

#define BCMA_CLICMD_GET_SYNOP_SUB3 \
    "reg|mem <block id> <addr> [<len>]\n" \
    "<addr> [<len>]"

#ifdef CFG_CHIP_SYMBOLS_INCLUDED
#ifdef CFG_CHIP_SYMBOLS_FIELD_INCLUDED
#define BCMA_CLICMD_GET_SYNOP \
    BCMA_CLICMD_GET_SYNOP_SUB1 \
    BCMA_CLICMD_GET_SYNOP_SUB3
#else /* CFG_CHIP_SYMBOLS_FIELD_INCLUDED */
#define BCMA_CLICMD_GET_SYNOP \
    BCMA_CLICMD_GET_SYNOP_SUB2 \
    BCMA_CLICMD_GET_SYNOP_SUB3
#endif /* CFG_CHIP_SYMBOLS_FIELD_INCLUDED */
#else /* CFG_CHIP_SYMBOLS_INCLUDED */
#define BCMA_CLICMD_GET_SYNOP \
    BCMA_CLICMD_GET_SYNOP_SUB3
#endif /* CFG_CHIP_SYMBOLS_INCLUDED */

/*! Help for get command. */
#define BCMA_CLICMD_GET_HELP_SUB1 \
    "Read and decode chip register or memory contents, where <symbol> is a\n" \
    "register/memory expression in one of the following forms:\n\n" \
    "<name>\n" \
    "<name>.<block>\n" \
    "<name>.<block>.<port>\n" \
    "<name>.uport.<port>\n" \
    "<name>[<index>]\n" \
    "<name>[<index>].<block>\n" \
    "<name>[<index>].<block>.<port>\n" \
    "<name>[<index>].uport.<port>\n" \
    "where <port> is the block port index in the corresponding <block>.\n" \
    "Or you can specify <port> as user port index by uport.<port>\n\n" \
    "If the nz option is specified, then only registers/memories with\n" \
    "non-zero contents will be displayed.\n"

#define BCMA_CLICMD_GET_HELP_SUB2 \
    "The raw option suppresses the decoding of individual fields.\n" \
    "The cf option selects a compact-field output format,\n" \
    "which is useful for slow terminals and high-volume output.\n"

#define BCMA_CLICMD_GET_HELP_SUB3 \
    "Use reg | mem <block id> <addr> to access chip register or memory\n" \
    "in raw format, \n" \
    "or simply <addr> to get the register contents in system address space.\n\n"

#define BCMA_CLICMD_GET_HELP_SUB4 \
    "It also supports wildcard in <name>."

#ifdef CFG_CHIP_SYMBOLS_INCLUDED
#ifdef CFG_CHIP_SYMBOLS_FIELD_INCLUDED
#define BCMA_CLICMD_GET_HELP \
    BCMA_CLICMD_GET_HELP_SUB1 \
    BCMA_CLICMD_GET_HELP_SUB2 \
    BCMA_CLICMD_GET_HELP_SUB3 \
    BCMA_CLICMD_GET_HELP_SUB4
#else /* CFG_CHIP_SYMBOLS_FIELD_INCLUDED */
#define BCMA_CLICMD_GET_HELP \
    BCMA_CLICMD_GET_HELP_SUB1 \
    BCMA_CLICMD_GET_HELP_SUB3 \
    BCMA_CLICMD_GET_HELP_SUB4
#endif /* CFG_CHIP_SYMBOLS_FIELD_INCLUDED */
#else /* CFG_CHIP_SYMBOLS_INCLUDED */
#define BCMA_CLICMD_GET_HELP \
    BCMA_CLICMD_GET_HELP_SUB3
#endif /* CFG_CHIP_SYMBOLS_INCLUDED */

/*! Examples for get command. */
#define BCMA_CLICMD_GET_EXAMPLES_SUB1 \
    "PGW_XL_CONFIGr\n" \
    "PGW_XL_CONFIGr.xlport\n" \
    "PGW_XL_CONFIGr.xlport3\n" \
    "PGW_XL_CONFIGr.xlport3.2\n" \
    "PGW_XL_CONFIGr.uport.9\n" \
    "CLPORT_WC_UCMEM_DATAm[3].clport0\n"\
    "MHOST_0_MHOST_INTR_MASKr[5]\n"\
    "nz L2_ENTRYm\n"

#define BCMA_CLICMD_GET_EXAMPLES_SUB2 \
    "reg 40 0x00080002\n" \
    "mem 10 0x1c040000 6\n" \
    "0x0321002c\n"

#define BCMA_CLICMD_GET_EXAMPLES_SUB3 \
    "*MAC*CTRLr\n" \
    "nz *m"

#ifdef CFG_CHIP_SYMBOLS_INCLUDED
#define BCMA_CLICMD_GET_EXAMPLES \
    BCMA_CLICMD_GET_EXAMPLES_SUB1 \
    BCMA_CLICMD_GET_EXAMPLES_SUB2 \
    BCMA_CLICMD_GET_EXAMPLES_SUB3
#else /* CFG_CHIP_SYMBOLS_INCLUDED */
#define BCMA_CLICMD_GET_EXAMPLES \
    BCMA_CLICMD_GET_EXAMPLES_SUB2
#endif /* CFG_CHIP_SYMBOLS_INCLUDED */

/*! Brief description for set command. */
#define BCMA_CLICMD_SET_DESC \
    "Modify chip register/memory contents."

/*! Syntax for set command. */
#define BCMA_CLICMD_SET_SYNOP_SUB1 \
    "<symbol> <value>... | <field>=<value> ... | all=<value>\n"

#define BCMA_CLICMD_SET_SYNOP_SUB2 \
    "<symbol> <value>... | all=<value>\n"

#define BCMA_CLICMD_SET_SYNOP_SUB3 \
    "reg|mem <block id> <addr> <value>...\n" \
    "<addr> <value>..." \

#ifdef CFG_CHIP_SYMBOLS_INCLUDED
#ifdef CFG_CHIP_SYMBOLS_FIELD_INCLUDED
#define BCMA_CLICMD_SET_SYNOP \
    BCMA_CLICMD_SET_SYNOP_SUB1 \
    BCMA_CLICMD_SET_SYNOP_SUB3
#else /* CFG_CHIP_SYMBOLS_FIELD_INCLUDED */
#define BCMA_CLICMD_SET_SYNOP \
    BCMA_CLICMD_SET_SYNOP_SUB2 \
    BCMA_CLICMD_SET_SYNOP_SUB3
#endif /* CFG_CHIP_SYMBOLS_FIELD_INCLUDED */
#else /* CFG_CHIP_SYMBOLS_INCLUDED */
#define BCMA_CLICMD_SET_SYNOP \
    BCMA_CLICMD_SET_SYNOP_SUB3
#endif /* CFG_CHIP_SYMBOLS_INCLUDED */

/*! Help for set command. */
#define BCMA_CLICMD_SET_HELP_SUB1 \
    "Modify chip register or memory contents, where <symbol> is a\n" \
    "register/memory expression in one of the following forms:\n\n" \
    "<name>\n" \
    "<name>.<block>\n" \
    "<name>.<block>.<port>\n" \
    "<name>.uport.<port>\n" \
    "<name>[<index>]\n" \
    "<name>[<index>].<block>\n" \
    "<name>[<index>].<block>.<port>\n" \
    "<name>[<index>].uport.<port>\n" \
    "where <port> is the block port index in the corresponding <block>.\n" \
    "Or you can specify <port> as user port index by uport.<port>\n\n"

#define BCMA_CLICMD_SET_HELP_SUB2 \
    "Contents may be modified either by entering raw 32-bit word values\n" \
    "or by changing each field separately.\n" \
    "Use all=<value> to write the entry with the same 32bit value.\n"

#define BCMA_CLICMD_SET_HELP_SUB3 \
    "Contents can be modified by entering raw 32-bit word values,\n" \
    "or use all=<value> to write the entry with the same 32bit value.\n"

#define BCMA_CLICMD_SET_HELP_SUB4 \
    "Use reg | mem <block id> <addr> <value>... to access\n" \
    "chip register or memory in raw format,\n" \
    "or simply <addr> <value>... to modify the register contents \n" \
    "in system address space."

#ifdef CFG_CHIP_SYMBOLS_INCLUDED
#ifdef CFG_CHIP_SYMBOLS_FIELD_INCLUDED
#define BCMA_CLICMD_SET_HELP  \
    BCMA_CLICMD_SET_HELP_SUB1 \
    BCMA_CLICMD_SET_HELP_SUB2 \
    BCMA_CLICMD_SET_HELP_SUB4
#else /* CFG_CHIP_SYMBOLS_FIELD_INCLUDED */
#define BCMA_CLICMD_SET_HELP  \
    BCMA_CLICMD_SET_HELP_SUB1 \
    BCMA_CLICMD_SET_HELP_SUB3 \
    BCMA_CLICMD_SET_HELP_SUB4
#endif /* CFG_CHIP_SYMBOLS_FIELD_INCLUDED */
#else /* CFG_CHIP_SYMBOLS_INCLUDED */
#define BCMA_CLICMD_SET_HELP \
    BCMA_CLICMD_SET_HELP_SUB4
#endif /* CFG_CHIP_SYMBOLS_INCLUDED */

/*! Examples for set command. */
#define BCMA_CLICMD_SET_EXAMPLES_SUB1 \
    "PGW_XL_CONFIGr.xlport3.2 0xe\n" \
    "PGW_XL_CONFIGr.uport.9 0xe\n" \
    "L2_ENTRYm[0] all=0xffffffff\n"\
    "L2_ENTRYm[3] 0x00000001 0x00000002 0x00000003 0x00000004\n"

#define BCMA_CLICMD_SET_EXAMPLES_SUB2 \
    "L2_ENTRYm[3] KEY_TYPE=0 L2:MAC_ADDR=0x1122_33445566 L2:VLAN_ID=100\n"

#define BCMA_CLICMD_SET_EXAMPLES_SUB3 \
    "reg 10 0x40003835 0x0000000e\n" \
    "mem 10 0x1c040000 0x00000001 0x00000002 0x00000003 0x00000004 \n" \
    "0x183200a0 0x0000eeff"

#ifdef CFG_CHIP_SYMBOLS_INCLUDED
#ifdef CFG_CHIP_SYMBOLS_FIELD_INCLUDED
#define BCMA_CLICMD_SET_EXAMPLES \
    BCMA_CLICMD_SET_EXAMPLES_SUB1 \
    BCMA_CLICMD_SET_EXAMPLES_SUB2 \
    BCMA_CLICMD_SET_EXAMPLES_SUB3
#else /* CFG_CHIP_SYMBOLS_FIELD_INCLUDED */
#define BCMA_CLICMD_SET_EXAMPLES \
    BCMA_CLICMD_SET_EXAMPLES_SUB1 \
    BCMA_CLICMD_SET_EXAMPLES_SUB3
#endif /* CFG_CHIP_SYMBOLS_FIELD_INCLUDED */
#else /* CFG_CHIP_SYMBOLS_INCLUDED */
#define BCMA_CLICMD_SET_EXAMPLES \
    BCMA_CLICMD_SET_EXAMPLES_SUB3
#endif /* CFG_CHIP_SYMBOLS_INCLUDED */

/*!
 * \brief Get command implementation.
 *
 * \param [in] cli CLI object
 * \param [in] args Argument list
 *
 * \return BCMA_CLI_CMD_xxx return values.
 */
extern int
bcma_clicmd_get(bcma_cli_t *cli, bcma_cli_args_t *args);

/*!
 * \brief Set command implementation.
 *
 * \param [in] cli CLI object
 * \param [in] args Argument list
 *
 * \return BCMA_CLI_CMD_xxx return values.
 */
extern int
bcma_clicmd_set(bcma_cli_t *cli, bcma_cli_args_t *args);
#endif /* BCMA_CLICMD_SETGET_H */
