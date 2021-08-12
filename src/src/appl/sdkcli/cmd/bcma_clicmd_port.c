/*! \file bcma_clicmd_port.c
 *
 * CLI 'port' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_PORT_INCLUDED
#include "cli_porting.h"

#include <appl/sdkcli/bcma_cli.h>
#include <appl/sdkcli/bcma_cli_parse.h>

#include <appl/sdkcli/bcma_clicmd_port.h>
#include "boardapi/port.h"

/*******************************************************************************
 * Local definitions
 */

#define PS_HEADER_FMT \
        "%-6s"      /* port number */ \
        "%-8s"      /* enable */ \
        "%-6s"      /* PHY link state */ \
        "%-8s"      /* speed */ \
        "%-5s"      /* auto negotiate */ \
        "%-7s"      /* pause tx/rx */ \
        "%-7s"      /* max frame */ \
        "%-15s"     /* loopback */ \
        "%-20s"     /* Interface */ \
        "%-8s\n"    /* IFG */ \

#define PS_DATA_FMT \
        "%-6d"      /* port number */ \
        "%-8s"      /* enable */ \
        "%-6s"      /* PHY link state */ \
        "%-8d"      /* speed */ \
        "%-5s"      /* auto negotiate */ \
        "%-7s"      /* pause tx/rx */ \
        "%-7d"      /* max frame */ \
        "%-15s"     /* loopback */ \
        "%-20s"     /* Interface */ \
        "%-8d\n"    /* IFG */ \

#define PS_ERROR_FMT \
        "%-6d"      /* port number */ \
        "%-8s\n"    /* error text */

/* interface type */
static bcma_cli_parse_enum_t intf_str[] = {
    {"NOCXN", PORT_IF_NOCXN},
    {"NULL", PORT_IF_NULL},
    {"MII", PORT_IF_MII},
    {"GMII", PORT_IF_GMII},
    {"SGMII", PORT_IF_SGMII},
    {"TBI", PORT_IF_TBI},
    {"XGMII", PORT_IF_XGMII},
    {"RGMII", PORT_IF_RGMII},
    {"SFI", PORT_IF_SFI},
    {"XFI", PORT_IF_XFI},
    {"KR", PORT_IF_KR},
    {"KR2", PORT_IF_KR2},
    {"KR4", PORT_IF_KR4},
    {"CR", PORT_IF_CR},
    {"CR2", PORT_IF_CR2},
    {"CR4", PORT_IF_CR4},
    {"XLAUI", PORT_IF_XLAUI},
    {"XLAUI2", PORT_IF_XLAUI2},
    {"RXAUI", PORT_IF_RXAUI},
    {"XAUI", PORT_IF_XAUI},
    {"SPAUI", PORT_IF_SPAUI},
    {"QSGMII", PORT_IF_QSGMII},
    {"ILKN", PORT_IF_ILKN},
    {"RCY", PORT_IF_RCY},
    {"FATPIPE", PORT_IF_FAT_PIPE},
    {"SR", PORT_IF_SR},
    {"SR2", PORT_IF_SR2},
    {"CAUI", PORT_IF_CAUI},
    {"LR", PORT_IF_LR},
    {"LR4", PORT_IF_LR4},
    {"SR4", PORT_IF_SR4},
    {"KX", PORT_IF_KX},
    {"ZR", PORT_IF_ZR},
    {"SR10", PORT_IF_SR10},
    {"CR10", PORT_IF_CR10},
    {"KR10", PORT_IF_KR10},
    {"LR10", PORT_IF_LR10},
    {"OTL", PORT_IF_OTL},
    {"CPU", PORT_IF_CPU},
    {"ER", PORT_IF_ER},
    {"ER2", PORT_IF_ER2},
    {"ER4", PORT_IF_ER4},
    {"CX", PORT_IF_CX},
    {"CX2", PORT_IF_CX2},
    {"CX4", PORT_IF_CX4},
    {"CAUIC2C", PORT_IF_CAUI_C2C},
    {"CAUIC2M", PORT_IF_CAUI_C2M},
    {"VSR", PORT_IF_VSR},
    {"LR2", PORT_IF_LR2},
    {"LRM", PORT_IF_LRM},
    {"XLPPI", PORT_IF_XLPPI},
    {"LBG", PORT_IF_LBG},
    {"CAUI4", PORT_IF_CAUI4},
    {"OAMP", PORT_IF_OAMP},
    {"OLP", PORT_IF_OLP},
    {"ERP", PORT_IF_ERP},
    {"SAT", PORT_IF_SAT},
    {"EVENTOR", PORT_IF_EVENTOR},
    {"NIFETH", PORT_IF_NIF_ETH},
    {"FLEXECLIENT", PORT_IF_FLEXE_CLIENT},
    {"VIRTUALFLEXECLIENT", PORT_IF_VIRTUAL_FLEXE_CLIENT},
    {"SCH", PORT_IF_SCH},
    {"CRPS", PORT_IF_CRPS},
    {NULL,   0}
};

/*!
 * \brief Get pause string from value.
 *
 * \param [in] pause_tx Tx pause is enabled or not.
 * \param [in] pause_rx Rx pause is enabled or not.
 * \param [out] buf String buffer.
 * \param [in] buflen buffer length.
 *
 * \return Pause string.
 */
static char *
port_status_pause_str(bool pause_tx, bool pause_rx, char *buf, int buflen)
{
    if (!pause_tx && !pause_rx) {
        sal_strcpy(buf, "None");
    } else {
        sal_snprintf(buf, buflen,
                     "%s%s",
                     pause_tx ? "TX " : "", pause_rx ? "RX" : "");
    }

    return buf;
}

/*!
 * \brief Fields of PC_PORT_STATUS entry.
 */
typedef struct pc_port_status_entry_s {
    int an;
    int speed;
    int enable;
    int link;
    int pause_tx;
    int pause_rx;
    int max_frame_size;
    port_loopback_t lpbk_mode;
    port_if_t intf;
    int ifg;
} pc_port_status_entry_t;

/*!
 * \brief Print port status.
 *
 * \param [in] unit Unit number.
 * \param [in] port User port number.
 */
static void
port_status_print(int port)
{
    int rv;
    char pbuf[32];
    pc_port_status_entry_t eps;
    int intf_idx;

    const static char *loopback_str[] = {
        "LOOPBACK_NONE",
        "LOOPBACK_MAC",
        "LOOPBACK_PHY",
        "LOOPBACK_PHY_REMOTE",
    };

    /* Get each port status items from board api */
    /* Get port enable */
    BOOL get_en;
    rv = board_port_enable_get(port, &get_en);
    eps.enable = (int)get_en;
    if (rv != SYS_OK) {
        cli_out(PS_ERROR_FMT,
                port, "failed to get enable status");
        return;
    }

    /* Get link status */
    BOOL get_link;
    rv = board_port_link_status_get(port, &get_link);
    eps.link = (int)get_link;
    if (rv != SYS_OK) {
        cli_out(PS_ERROR_FMT,
                port, "failed to get link status");
        return;
    }

    /* Get port speed */
    rv = board_port_speed_get(port, &eps.speed);
    if (rv != SYS_OK) {
        cli_out(PS_ERROR_FMT,
                port, "failed to get link speed");
        return;
    }

    /* Get the status of auto neg */
    BOOL get_an;
    rv = board_port_an_get(port, &get_an);
    eps.an = (int)get_an;
    if (rv != SYS_OK) {
        cli_out(PS_ERROR_FMT,
                port, "failed to get autoneg status");
        return;
    }
    BOOL get_pause_tx, get_pause_rx;
    rv = board_port_pause_get(port, &get_pause_tx, &get_pause_rx);
    eps.pause_tx = (int)get_pause_tx;
    eps.pause_rx = (int)get_pause_rx;
    if (rv != SYS_OK) {
        cli_out(PS_ERROR_FMT,
                port, "failed to get pause status");
        return;
    }

    rv = board_port_frame_max_get(port, &eps.max_frame_size);
    if (rv != SYS_OK) {
        cli_out(PS_ERROR_FMT,
                port, "failed to get max frame size");
        return;
    }

    rv = board_port_loopback_get(port, &eps.lpbk_mode);
    if (rv != SYS_OK) {
        cli_out(PS_ERROR_FMT,
                port, "failed to get loopback status");
        return;
    }
    SAL_ASSERT(COUNTOF(loopback_str) == PORT_LOOPBACK_COUNT);
    if (eps.lpbk_mode < 0 || eps.lpbk_mode >= COUNTOF(loopback_str)) {
        cli_out(PS_ERROR_FMT,
                port, "failed to get loopback status");
        return;
    }

    rv = board_port_interface_get(port, &eps.intf);
    if (rv != SYS_OK) {
        cli_out(PS_ERROR_FMT,
                port, "failed to get interface status");
        return;
    }

    for (intf_idx = 0; intf_idx < COUNTOF(intf_str); intf_idx++) {
        if (intf_str[intf_idx].name == NULL) {
            cli_out(PS_ERROR_FMT,
                    port, "failed to get interface status");
            return;
        }
        if (intf_str[intf_idx].val == eps.intf) {
            /* Found */
            break;
        }
    }

    /* Get port ifg */
    rv = board_port_ifg_get(port, &eps.ifg);
    if (rv != SYS_OK) {
        cli_out(PS_ERROR_FMT,
                port, "failed to get ifg");
        return;
    }

    cli_out(PS_DATA_FMT,
            port,
            eps.enable > 0 ? "Yes" : "No",
            eps.link > 0 ? "Up" : "Down",
            eps.speed,
            eps.an > 0 ? "Yes" : "No",
            port_status_pause_str(eps.pause_tx > 0, eps.pause_rx > 0, pbuf, sizeof(pbuf)),
            (int)eps.max_frame_size,
            loopback_str[eps.lpbk_mode],
            intf_str[intf_idx].name,
            eps.ifg);
}

/*!
 * \brief Print header for ps command.
 */
static void
port_status_header_print(void)
{
    cli_out(PS_HEADER_FMT,
            "Port",             /* port number */
            "Enable",           /* enable */
            "Link",             /* PHY link state */
            "Speed",            /* speed */
            "AN",               /* auto negotiate */
            "Pause",            /* pause tx/rx */
            "Frame",            /* max frame */
            "LoopBack",         /* loopback */
            "INTerFace",        /* Interface */
            "IFG");             /* IFG */
}

int
bcma_clicmd_portstatus(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    const char *arg;
    SHR_BITDCL uport_array;
    int num_uports = board_uport_count() + 1;
    int uport;
    int ret_val;

    COMPILER_REFERENCE(cli);
    SHR_BITCLR_RANGE(&uport_array, 0, num_uports);

    /* Parse the input ports if any */
    /* Get the valid ports */
    arg = BCMA_CLI_ARG_GET(args);
    if (arg) {
        ret_val = bcma_cli_parse_bit_list(arg, num_uports, &uport_array);
        if (ret_val >= 0) {
            /* uport '0' is invalid port */
            if (uport_array & 0x1) {
                cli_out("%sInvalid user port(s).\n",
                        BCMA_CLI_CONFIG_ERROR_STR);
                return BCMA_CLI_CMD_BAD_ARG;
            }
        } else if (ret_val == -2) {
                cli_out("%sInvalid user port(s).\n",
                        BCMA_CLI_CONFIG_ERROR_STR);
            return BCMA_CLI_CMD_BAD_ARG;
        } else {
            return BCMA_CLI_CMD_USAGE;
        }
    } else {
        int lport;
        uint16 uport16;
        uport_array = 0x0;
        /* get all ports */
        SOC_LPORT_ITER(lport) {
            board_lport_to_uport(0, lport, &uport16);
            SHR_BITSET(&uport_array, uport16);
        }
    }
    /* Show header */
    port_status_header_print();

    /* Show uport status */
    SHR_BIT_ITER(&uport_array, num_uports, uport) {
        port_status_print(uport);
    }

    return BCMA_CLI_CMD_OK;
}

int
bcma_clicmd_portreinit(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    const char *arg;
    SHR_BITDCL uport_array;
    int num_uports = board_uport_count() + 1;
    int uport;
    int ret_val;

    COMPILER_REFERENCE(cli);
    SHR_BITCLR_RANGE(&uport_array, 0, num_uports);

    /* Parse the input ports if any */
    /* Get the valid ports */
    arg = BCMA_CLI_ARG_GET(args);
    if (arg) {
        ret_val = bcma_cli_parse_bit_list(arg, num_uports, &uport_array);
        if (ret_val >= 0) {
            /* uport '0' is invalid port */
            if (uport_array & 0x1) {
                cli_out("%sInvalid user port(s).\n",
                        BCMA_CLI_CONFIG_ERROR_STR);
                return BCMA_CLI_CMD_BAD_ARG;
            }
        } else if (ret_val == -2) {
                cli_out("%sInvalid user port(s).\n",
                        BCMA_CLI_CONFIG_ERROR_STR);
            return BCMA_CLI_CMD_BAD_ARG;
        } else {
            return BCMA_CLI_CMD_USAGE;
        }
    } else {
        return BCMA_CLI_CMD_USAGE;
    }

    /* Calls port re-init */
    SHR_BIT_ITER(&uport_array, num_uports, uport) {
        board_port_reinit(uport);
    }

    return BCMA_CLI_CMD_OK;
}

static bcma_cli_parse_enum_t pc_str[] = {
    { "FieldLookup", PortClassFieldLookup },
    { "FieldIngress", PortClassFieldIngress },
    { "FieldEgress", PortClassFieldEgress },
    { "VlanTranslateEgress", PortClassVlanTranslateEgress },
    { NULL, 0 }
};
int
bcma_clicmd_portclass(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    const char *arg;
    SHR_BITDCL uport_array;
    bcma_cli_parse_table_t pt;
    int num_uports = board_uport_count() + 1;
    int uport;
    int ret_val;
    port_class_t pclass;
    uint32 id, get_id;
    int pc_idx;

    COMPILER_REFERENCE(cli);
    SHR_BITCLR_RANGE(&uport_array, 0, num_uports);

    /* Parse the input ports if any */
    /* Get the valid ports */
    arg = BCMA_CLI_ARG_GET(args);
    if (arg) {
        ret_val = bcma_cli_parse_bit_list(arg, num_uports, &uport_array);
        if (ret_val >= 0) {
            /* uport '0' is invalid port */
            if (uport_array & 0x1) {
                cli_out("%sInvalid user port(s).\n",
                        BCMA_CLI_CONFIG_ERROR_STR);
                return BCMA_CLI_CMD_BAD_ARG;
            }
        } else if (ret_val == -2) {
                cli_out("%sInvalid user port(s).\n",
                        BCMA_CLI_CONFIG_ERROR_STR);
            return BCMA_CLI_CMD_BAD_ARG;
        } else {
            return BCMA_CLI_CMD_USAGE;
        }
    } else {
        int lport;
        uint16 uport16;
        uport_array = 0x0;
        /* get all ports */
        SOC_LPORT_ITER(lport) {
            board_lport_to_uport(0, lport, &uport16);
            SHR_BITSET(&uport_array, uport16);
        }
    }

    /* Check if assign pclass */
    if (BCMA_CLI_ARG_CNT(args) == 0) {
        return BCMA_CLI_CMD_USAGE;
    }

    pclass = (port_class_t)-1;
    arg = BCMA_CLI_ARG_GET(args);
    if (bcma_cli_parse_cmp("FieldINGress", arg,  ' ')) {
        pclass = PortClassFieldIngress;
    } else {
        /* Unsupport on API */
        return BCMA_CLI_CMD_USAGE;
    }
    for (pc_idx = 0; pc_idx < COUNTOF(pc_str); pc_idx++) {
        if (pc_str[pc_idx].name == NULL) {
            return BCMA_CLI_CMD_USAGE;
        }
        if (pc_str[pc_idx].val == pclass) {
            /* Found */
            break;
        }
    }

    id = (uint32)-1;
    bcma_cli_parse_table_init(cli, &pt);
    bcma_cli_parse_table_add(&pt, "ID", "int", &id, NULL);
    ret_val = bcma_cli_parse_table_do_args(&pt, args);
    bcma_cli_parse_table_done(&pt);
    if (ret_val < 0) {
        cli_out("%s: Invalid option: %s\n",
                BCMA_CLI_ARG_CMD(args), BCMA_CLI_ARG_CUR(args));
        return BCMA_CLI_CMD_FAIL;
    }

    /* Check if assign id */
    if (id == (uint32)-1) {
        /* Get */
        SHR_BIT_ITER(&uport_array, num_uports, uport) {
            board_port_class_get(uport, pclass, &get_id);
            cli_out("port %d %s id=%d\n", uport, pc_str[pc_idx].name, get_id);
        }
        return BCMA_CLI_CMD_OK;
    }

    /* Set the pclass ID */
    SHR_BIT_ITER(&uport_array, num_uports, uport) {
        board_port_class_set(uport, pclass, id);
    }

    return BCMA_CLI_CMD_OK;
}

/* Loopback mode */
static bcma_cli_parse_enum_t lb_str[] = {
    {"NONE", PORT_LOOPBACK_NONE},
    {"MAC", PORT_LOOPBACK_MAC},
    {"PHY", PORT_LOOPBACK_PHY},
    {"EXT", PORT_LOOPBACK_PHY_REMOTE},
    {NULL, 0}
};

int
bcma_clicmd_port(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    char *arg;
    SHR_BITDCL uport_array;
    int num_uports = board_uport_count() + 1;
    int uport;
    bcma_cli_parse_table_t pt;
    pc_port_status_entry_t eps;
    int ret_val;
    int rv;

    COMPILER_REFERENCE(cli);

    /* Get port ID or port range */
    SHR_BITCLR_RANGE(&uport_array, 0, num_uports);
    /* Parse the input ports if any */
    /* Get the valid ports */

    arg = BCMA_CLI_ARG_GET(args);

    if (BCMA_CLI_ARG_CNT(args) == 0) {
        return BCMA_CLI_CMD_USAGE;
    }

    ret_val = bcma_cli_parse_bit_list(arg, num_uports, &uport_array);
    if (ret_val >= 0) {
        /* uport '0' is invalid port */
        if (uport_array & 0x1) {
                cli_out("%sInvalid user port(s).\n",
                        BCMA_CLI_CONFIG_ERROR_STR);
            return BCMA_CLI_CMD_BAD_ARG;
        }
    } else if (ret_val == -2) {
                cli_out("%sInvalid user port(s).\n",
                        BCMA_CLI_CONFIG_ERROR_STR);
        return BCMA_CLI_CMD_BAD_ARG;
    } else {
        return BCMA_CLI_CMD_USAGE;
    }

    /* uport '0' is invalid port */
    if (uport_array & 0x1) {
        cli_out("%sInvalid port range format: %s\n",
                BCMA_CLI_CONFIG_ERROR_STR, arg);
        return BCMA_CLI_CMD_BAD_ARG;
    }
    /* Set all parameters default value to '-1' */
    eps.speed = -1;
    eps.an = -1;
    eps.pause_tx = -1;
    eps.pause_rx = -1;
    eps.enable = -1;
    eps.max_frame_size = -1;
    eps.lpbk_mode = (port_loopback_t)-1;
    eps.intf = (port_if_t)-1;
    eps.ifg = -1;
    bcma_cli_parse_table_init(cli, &pt);
    bcma_cli_parse_table_add(&pt, "AutoNeg", "bool", &eps.an, NULL);
    bcma_cli_parse_table_add(&pt, "SPeed", "int", &eps.speed, NULL);
    bcma_cli_parse_table_add(&pt, "Enable", "bool", &eps.enable, NULL);
    bcma_cli_parse_table_add(&pt, "TxPAUse", "bool", &eps.pause_tx, NULL);
    bcma_cli_parse_table_add(&pt, "RxPAUse", "bool", &eps.pause_rx, NULL);
    bcma_cli_parse_table_add(&pt, "FrameMax", "int", &eps.max_frame_size, NULL);
    bcma_cli_parse_table_add(&pt, "LoopBack", "enum", &eps.lpbk_mode, lb_str);
    bcma_cli_parse_table_add(&pt, "INTerFace", "enum", &eps.intf, intf_str);
    bcma_cli_parse_table_add(&pt, "IFG", "int", &eps.ifg, NULL);
    ret_val = bcma_cli_parse_table_do_args(&pt, args);
    bcma_cli_parse_table_done(&pt);
    if (ret_val < 0) {
        cli_out("%s: Invalid option: %s\n",
                BCMA_CLI_ARG_CMD(args), BCMA_CLI_ARG_CUR(args));
        return BCMA_CLI_CMD_FAIL;
    }

    SHR_BIT_ITER(&uport_array, num_uports, uport) {
        /* Set autoneg */
        if (eps.an != -1) {
            BOOL cur_an;
            rv = board_port_an_get(uport, &cur_an);
            if (rv != SYS_OK) {
                cli_out(PS_ERROR_FMT,
                    uport, "failed to get autoneg status");
            }
            if (cur_an != (BOOL)eps.an) {
                rv = board_port_an_set(uport, eps.an);
                if (rv != SYS_OK) {
                    cli_out(PS_ERROR_FMT,
                        uport, "failed to set autoneg status");
                }
            }
        }

        /* Set port speed */
        if (eps.speed != -1) {
            rv = board_port_speed_set(uport, eps.speed);
            if (rv != SYS_OK) {
                cli_out(PS_ERROR_FMT,
                    uport, "failed to set port speed");
            }
        }

        /* Set tx/rx pause */
        int set_pause_tx = 0;
        int set_pause_rx = 0;
        if (eps.pause_tx != -1 || eps.pause_rx != -1) {
            BOOL pause_tx;
            BOOL pause_rx;
            rv = board_port_pause_get(uport, &pause_tx, &pause_rx);
            if (rv != SYS_OK) {
                cli_out(PS_ERROR_FMT,
                    uport, "failed to get pause status");
            }

            if (eps.pause_tx != -1 && eps.pause_tx != (int)pause_tx) {
                pause_tx = eps.pause_tx;
                set_pause_tx = 1;
            }

            if (eps.pause_rx != -1 && eps.pause_rx != (int)pause_rx) {
                pause_rx = eps.pause_rx;
                set_pause_rx = 1;
            }
            if (set_pause_tx == 1 || set_pause_rx == 1) {
                /* Call api only while it differs from current status */
                rv = board_port_pause_set(uport, pause_tx, pause_rx);
                if (rv != SYS_OK) {
                    cli_out(PS_ERROR_FMT,
                        uport, "failed to set pause status");
                }
            }
        }

        /* Set port enable */
        if (eps.enable != -1) {
            BOOL cur_en;
            rv = board_port_enable_get(uport, &cur_en);
            if (rv != SYS_OK) {
                cli_out(PS_ERROR_FMT,
                    uport, "failed to get port enable/disable mode");
            }
            if (cur_en != (BOOL)eps.enable) {
                rv = board_port_enable_set(uport, eps.enable);
                if (rv != SYS_OK) {
                    cli_out(PS_ERROR_FMT,
                        uport, "failed to set port enable/disable mode");
                }
            }
        }

        /* Set max frame size */
        if (eps.max_frame_size != -1) {
            int cur_mfs;
            rv = board_port_frame_max_get(uport, &cur_mfs);
            if (rv != SYS_OK) {
                cli_out(PS_ERROR_FMT,
                    uport, "failed to get max frame size");
            }
            if (cur_mfs != eps.max_frame_size) {
                rv = board_port_frame_max_set(uport, eps.max_frame_size);
                if (rv != SYS_OK) {
                    cli_out(PS_ERROR_FMT,
                        uport, "failed to set max frame size");
                }
            }
        }
        /* Set port loopback */
        if (eps.lpbk_mode != (port_loopback_t)-1) {
            port_loopback_t cur_lb;
            rv = board_port_loopback_get(uport, &cur_lb);
            if (rv != SYS_OK) {
                cli_out(PS_ERROR_FMT,
                    uport, "failed to get loopback mode");
            }
            if (cur_lb != eps.lpbk_mode) {
                rv = board_port_loopback_set(uport, eps.lpbk_mode);
                if (rv != SYS_OK) {
                    cli_out(PS_ERROR_FMT,
                        uport, "failed to set loopback mode");
                }
            }
        }
        /* Set port interface */
        if (eps.intf != (port_if_t)-1) {
            port_if_t cur_intf;
            rv = board_port_interface_get(uport, &cur_intf);
            if (rv != SYS_OK) {
                cli_out(PS_ERROR_FMT,
                    uport, "failed to get port interface");
            }
            if (cur_intf != eps.intf) {
                rv = board_port_interface_set(uport, eps.intf);
                if (rv != SYS_OK) {
                    cli_out(PS_ERROR_FMT,
                        uport, "failed to set port interface");
                }
            }
        }
        /* Set port ifg */
        if (eps.ifg != -1) {
            rv = board_port_ifg_set(uport, eps.ifg);
            if (rv != SYS_OK) {
                cli_out(PS_ERROR_FMT,
                    uport, "failed to set port ifg");
            }
        }
    }

    return BCMA_CLI_CMD_OK;
}

#endif /* CFG_SDKCLI_PORT_INCLUDED */
