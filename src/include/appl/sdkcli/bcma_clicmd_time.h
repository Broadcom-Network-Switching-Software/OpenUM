/*! \file bcma_cli_unit.h
 *
 * Functions about unit information operation in CLI.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef BCMA_CLI_UNIT_H
#define BCMA_CLI_UNIT_H

#include <appl/sdkcli/bcma_cli.h>

/*! Brief description for CLI command. */
#define BCMA_CLICMD_GETSYNCE_DESC \
    "Show SyncE squlth/frequency."
/*! Syntax for GETSYNCE CLI command. */
#define BCMA_CLICMD_GETSYNCE_SYNOP \
    "<subcmd> <action> <action> [<action>]"
/*! Help for GETSYNCE CLI command. */
#define BCMA_CLICMD_GETSYNCE_HELP \
    "This command is used for get SYNCE enable/disable status.\n" \
    "The command support the following sub-commands and actions:\n" \
    "    -f <action1> <action2> <value1> - Get frequency synce MHz(1~11).\n"\
    "        -p=<action1>  - Primary.\n" \
    "        -s=<action1>  - Secondary.\n" \
    "        -port=<action2>  - SrcTypePort.\n" \
    "        -pll=<action2>  - SrcTypePll.\n" \
    "        lport_num=[value1] - SrcPort number.\n" \
    "        frequency= 1~11 Frequency values.\n" \
    "            bcmTimeSyncE23MHz = 1,  TSCF - 23.4375 MHz\n" \
    "            bcmTimeSyncE25MHz = 2,  SDM - 25MHz\n" \
    "            bcmTimeSyncE28MHz = 3,  General - 28.4 MHz\n" \
    "            bcmTimeSyncE46MHz = 4,  General - 46.875 MHz\n" \
    "            bcmTimeSyncE125MHz = 5, General - 125MHz\n" \
    "            bcmTimeSyncE515MHz = 6, General - 515.625MHz\n" \
    "            bcmTimeSyncE7MHz = 7,   SDM - 7.8125 MHz\n" \
    "            bcmTimeSyncE12MHz = 8,  SDM - 12.5 MHz\n" \
    "            bcmTimeSyncE31MHz = 9,  SDM - 31.25 MHz\n" \
    "            bcmTimeSyncE37MHz = 10, SDM - 37.5 MHz\n" \
    "            bcmTimeSyncE156MHz = 11 General - 156.25 MHz\n" \
    "    -s <action> - Get Squelch synce status.\n" \
    "        -p=<action>  - Primary.\n" \
    "        -s=<action>  - Secondary.\n" \
/*! Examples for GETSYNCE CLI command. */
#define BCMA_CLICMD_GETSYNCE_EXAMPLES \
    "setsynce -f -s -port 2\n" \
    "setsynce -s -p"

#define BCMA_CLICMD_SETSYNCE_DESC \
    "Set SyncE squlth/frequency."
/*! Syntax for GETSYNCE CLI command. */
#define BCMA_CLICMD_SETSYNCE_SYNOP \
    "<subcmd> <action> <value|action> [<action> <value>]"
/*! Help for GETSYNCE CLI command. */
#define BCMA_CLICMD_SETSYNCE_HELP \
    "This command is used for set SYNCE.\n" \
    "The command support the following sub-commands and actions:\n" \
    "    -f <action1> <action2> <value1> <value2> - Set frequency synce MHz.\n"\
    "        -p=<action1>  - Primary.\n" \
    "        -s=<action1>  - Secondary.\n" \
    "        -port=<action2>  - SrcTypePort.\n" \
    "        -pll=<action2>  - SrcTypePll.\n" \
    "        lport_num=[value1] - SrcPort number.\n" \
    "        frequency=[value2] - 1~11 Frequency values.\n" \
    "            bcmTimeSyncE23MHz = 1,  TSCF - 23.4375 MHz\n" \
    "            bcmTimeSyncE25MHz = 2,  SDM - 25MHz\n" \
    "            bcmTimeSyncE28MHz = 3,  General - 28.4 MHz\n" \
    "            bcmTimeSyncE46MHz = 4,  General - 46.875 MHz\n" \
    "            bcmTimeSyncE125MHz = 5, General - 125MHz\n" \
    "            bcmTimeSyncE515MHz = 6, General - 515.625MHz\n" \
    "            bcmTimeSyncE7MHz = 7,   SDM - 7.8125 MHz\n" \
    "            bcmTimeSyncE12MHz = 8,  SDM - 12.5 MHz\n" \
    "            bcmTimeSyncE31MHz = 9,  SDM - 31.25 MHz\n" \
    "            bcmTimeSyncE37MHz = 10, SDM - 37.5 MHz\n" \
    "            bcmTimeSyncE156MHz = 11 General - 156.25 MHz\n" \
    "    -s <action> <value> - Set Squelch synce enable.\n" \
    "        -p=<action>  - Primary.\n" \
    "        -s=<action>  - Secondary.\n" \
    "        enable=[value] - 1/0 for enable/disable squelch synce.\n"
/*! Examples for GETSYNCE CLI command. */
#define BCMA_CLICMD_SETSYNCE_EXAMPLES \
    "setsynce -f -s -port 2 2\n" \
    "setsynce -s -p 1"

/*!
 * \brief Get the Synce status.
 *
 * This function will ensure that parameters are validated and that a
 * default behavior is defined if no call-back is installed.
 *
 * \param [in] cli CLI object.
 * \param [in] feature Feature to check for.
 *
 * \retval
 */
extern int
bcma_clicmd_getsynce(bcma_cli_t *cli, bcma_cli_args_t *args);

/*!
 * \brief Set the Synce status.
 *
 * This function will ensure that parameters are validated and that a
 * default behavior is defined if no call-back is installed.
 *
 * \param [in] cli CLI object.
 * \param [in] feature Feature to check for.
 *
 * \retval
 */
extern int
bcma_clicmd_setsynce(bcma_cli_t *cli, bcma_cli_args_t *args);

#endif /* BCMA_CLI_UNIT_H */
