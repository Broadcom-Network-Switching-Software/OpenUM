/*! \file bcma_cli_unit.h
 *
 * Functions about unit information operation in CLI.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef BCMA_CLICMD_PORT_H
#define BCMA_CLICMD_PORT_H

#include <appl/sdkcli/bcma_cli.h>

/*! Brief description for CLI command. */
#define BCMA_CLICMD_PORTSTATUS_DESC \
    "Display port status (by user ports)."

/*! Syntax for CLI command. */
#define BCMA_CLICMD_PORTSTATUS_SYNOP \
    "[<port_range>]"

/*! Help for CLI command. */
#define BCMA_CLICMD_PORTSTATUS_HELP \
    "<port_range> consists of port ranges and single ports separated by\n" \
    " comma(,). Ranges are specified with start port and end port connected\n"\
    " by dash(-)."


/*! Examples for CLI command. */
#define BCMA_CLICMD_PORTSTATUS_EXAMPLES \
    "\n"\
    "1-5,7,9-12"

/*! Brief description for CLI command. */
#define BCMA_CLICMD_PORTREINIT_DESC \
    "Re-initial port (by user ports)."

/*! Syntax for CLI command. */
#define BCMA_CLICMD_PORTREINIT_SYNOP \
    "<port_range>"

/*! Help for CLI command. */
#define BCMA_CLICMD_PORTREINIT_HELP \
    "<port_range> consists of port ranges and single ports separated by\n" \
    " comma(,). Ranges are specified with start port and end port connected\n"\
    " by dash(-)."

/*! Examples for CLI command. */
#define BCMA_CLICMD_PORTREINIT_EXAMPLES \
    "1-5,7,9-12"

/*! Brief description for CLI command. */
#define BCMA_CLICMD_PORTCLASS_DESC \
    "Set/Get port class-id(by user ports)."

/*! Syntax for CLI command. */
#define BCMA_CLICMD_PORTCLASS_SYNOP \
    "<port_range><port_class>\\\n"\
    "[ID=<value>]"

/*! Help for CLI command. */
#define BCMA_CLICMD_PORTCLASS_HELP \
    "<port_range> consists of port ranges and single ports separated by\n" \
    " comma(,). Ranges are specified with start port and end port connected\n"\
    " by dash(-).\n" \
    "<port_class> specifies port class used by FieldINGress\n" \
    "<ID>:Set the class ID. Without specifying the class ID, this command\n" \
    " would get the class ID."


/*! Examples for CLI command. */
#define BCMA_CLICMD_PORTCLASS_EXAMPLES \
    "1-5 fing -- Get the class ID for port class used by FieldIngress\n" \
    "1-5 fing id=1 -- Set the class ID=1 for port class used by FieldIngress"


/*! Brief description for CLI command. */
#define BCMA_CLICMD_PORT_DESC \
    "Set port characteristics (by user ports)."
/*! Syntax for PORT CLI command. */
#define BCMA_CLICMD_PORT_SYNOP \
    "<port_range>[AutoNeg=on|off]\\\n"\
    "[SPeed=1000|2500|5000|10000|25000|40000|50000|100000]\\\n"\
    "[TxPAUse=on|off] [RxPAUse=on|off] [Enable=true|false]\\\n"\
    "[FrameMax=<value>][LoopBack=none|mac|phy][INTerFace=<interface>]\\\n"\
    "[IFG=<value>]"

/*! Help for PORT CLI command. */
#define BCMA_CLICMD_PORT_HELP \
    "<port_range> consists of port ranges and single ports separated by\n"\
    " comma(,). Ranges are specified with start port and end port connected\n"\
    " by dash(-).\n"\
    "<interface>: available interfaces can be retrieved by not providing the\n"\
    " value, e.g. intf=\n"\
    "<IFG>: available ifg can be retrieved by not providing the value,\n"\
    " e.g. ifg="
/*! Examples for PORT CLI command. */
#define BCMA_CLICMD_PORT_EXAMPLES \
    "1-5,7,9-12 an=1 en=1"

/*!
 * \brief Display port status (by user ports).
 *
 * \param [in] cli CLI object.
 * \param [in] feature Feature to check for.
 *
 * \retval
 */
extern int
bcma_clicmd_portstatus(bcma_cli_t *cli, bcma_cli_args_t *args);

/*!
 * \brief Reinitial port (by user ports).
 *
 * \param [in] cli CLI object.
 * \param [in] feature Feature to check for.
 *
 * \retval
 */
extern int
bcma_clicmd_portreinit(bcma_cli_t *cli, bcma_cli_args_t *args);

/*!
 * \brief set the ports class ID (by user ports).
 *
 * \param [in] cli CLI object.
 * \param [in] feature Feature to check for.
 *
 * \retval
 */
extern int
bcma_clicmd_portclass(bcma_cli_t *cli, bcma_cli_args_t *args);

/*!
 * \brief Set port characteristics (by user ports).
 *
 * \param [in] cli CLI object.
 * \param [in] feature Feature to check for.
 *
 * \retval
 */
extern int
bcma_clicmd_port(bcma_cli_t *cli, bcma_cli_args_t *args);

#endif /* BCMA_CLICMD_PORT_H */
