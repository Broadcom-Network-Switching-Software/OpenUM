/*! \file bcma_bcmpktcmd_tx.h
 *
 * CLI command related to TX.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef BCMA_BCMPKTCMD_TX_H
#define BCMA_BCMPKTCMD_TX_H

#include <appl/sdkcli/bcma_cli.h>

/*! Brief description for CLI command. */
#define BCMA_BCMPKTCMD_TX_DESC \
    "Transmit packets."

/*! Syntax for CLI command. */
#define BCMA_BCMPKTCMD_TX_SYNOP \
    "<count> [options]"

/*! Help for CLI command. */
#define BCMA_BCMPKTCMD_TX_HELP \
    "Transmit one or more packets. The default packet contents can be\n" \
    "overridden using command line options.\n" \
    "Options:\n" \
    "    PortList=<portlist>  - Destination port list.\n" \
    "    DATA=<packet>        - Parse string given by user as packet payload\n" \
    "    LENgth=<value>       - Specify the total length of the packet,\n" \
    "                           including header, possible tag, and CRC.\n" \
    "    EtherType=<value>    - EtherType for packet generation.\n" \
    "    TPID=<value>         - TPID for packet generation.\n" \
    "    UnTagged=[yes/no]    - Without 802.1Q TAG.\n" \
    "    DEI=<value>          - 802.1Q Drop eligible indicator.\n" \
    "    PCP=<value>          - 802.1Q Priority code point.\n" \
    "    VlanID=<value>       - 802.1Q VLAN identifier.\n" \
    "    IntPrio=<value>      - Internal Priority.\n" \
    "    PATtern=<value>      - Specify 32-bit data pattern used.\n" \
    "    PATternInc=<value>   - Value by which each word of the data\n" \
    "                           pattern is incremented.\n" \
    "    PATternRandom=[yes/no]- Use Random data pattern\n" \
    "    PerPortSrcMac=[yes/no]- Associate specific (different) src macs\n" \
    "                          with each source port.\n" \
    "    SourceMac=<value>    - Source MAC address in packet.\n" \
    "    SourceMacInc=<val>   - Source MAC increment.\n" \
    "    DestMac=<value>      - Destination MAC address in packet.\n" \
    "    DestMacInc=<value>   - Destination MAC increment.\n" \
    "\n" \
    "<portlist> is one or more comma-separated ports or port ranges, for\n" \
    "example \"pl=2,4-8,21\". An empty list is specified as \"pl=none\".\n" \
    "When configure packet metadata's multiple view fields, the view type\n" \
    "must be input before it's fields."

/*! Examples for CLI command. */
#define BCMA_BCMPKTCMD_TX_EXAMPLES \
    "1 pl=1 len=128\n" \
    "1 pl=none data=00010203...."

/*!
 * \brief Packet transmit command in CLI.
 *
 * \param[in] cli CLI object
 * \param[in] args CLI arguments list
 *
 * \return BCMA_CLI_CMD_xxx return values.
 */
extern int
bcma_bcmpktcmd_tx(bcma_cli_t *cli, bcma_cli_args_t *args);

#endif /* BCMA_BCMPKTCMD_TX_H */
