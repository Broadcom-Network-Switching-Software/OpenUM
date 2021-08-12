/*! \file bcma_clicmd_phy.c
 *
 * CLI 'phy' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_PHY_INCLUDED

#include "cmicx_miim.h"
#include "types.h"

#ifdef CFG_SDKCLI_INCLUDED

#include <appl/sdkcli/bcma_cli.h>
#include <appl/sdkcli/bcma_cli_var.h>
#include <appl/sdkcli/bcma_cli_redir.h>
#include <appl/sdkcli/bcma_cli_parse.h>

#undef _SOC_PHYCTRL_H_
#include <soc/phyctrl.h>

#define BCMA_CLICM_PHY_INT_TO_PTR(x)        ((void *)((uint32)(x)))

static int
bcma_clicmd_phydiag_dsc_dump(bcma_cli_t *cli, uint8 port, bcma_cli_args_t *args)
{
    int unit = cli->cmd_unit;
    int rv = BCMA_CLI_CMD_OK;
    uint32 inst;
    char *cmd_str = BCMA_CLI_ARG_GET(args);

    inst = PHY_DIAG_INSTANCE(PHY_DIAG_DEV_INT, PHY_DIAG_INTF_LINE, PHY_DIAG_LN_DFLT);

    rv = soc_phyctrl_diag_ctrl(unit, port, inst, PHY_DIAG_CTRL_CMD, PHY_DIAG_CTRL_DSC, cmd_str);

    if (rv < 0) {
         sal_printf("Phy diag dsc failed. rv: %d\n", rv);
         return BCMA_CLI_CMD_FAIL;
    }

    return rv;
}

static int
bcma_clicmd_phydiag_prbs(bcma_cli_t *cli, uint8 port, bcma_cli_args_t *args)
{
    int unit = cli->cmd_unit;
    int cmd, enable, status, rv = BCMA_CLI_CMD_OK;
    int poly = 0, invert = 0;
    uint32 inst;
    bcma_cli_parse_table_t pt;
    char *poly_str = NULL, *cmd_str = NULL;

    inst = PHY_DIAG_INSTANCE(PHY_DIAG_DEV_INT, PHY_DIAG_INTF_LINE, PHY_DIAG_LN_DFLT);

    enum { _PHY_PRBS_SET_CMD, _PHY_PRBS_GET_CMD, _PHY_PRBS_CLEAR_CMD };
    enum { _PHY_PRBS_SI_MODE, _PHY_PRBS_HC_MODE };

    if ((cmd_str = BCMA_CLI_ARG_GET(args)) == NULL) {
        return BCMA_CLI_CMD_USAGE;
    }

    if (sal_strcasecmp(cmd_str, "set") == 0) {
        cmd = _PHY_PRBS_SET_CMD;
        enable = 1;
    } else if (sal_strcasecmp(cmd_str, "get") == 0) {
        cmd = _PHY_PRBS_GET_CMD;
        enable = 0;
    } else if (sal_strcasecmp(cmd_str, "clear") == 0) {
        cmd = _PHY_PRBS_CLEAR_CMD;
        enable = 0;
    } else {
        return BCMA_CLI_CMD_USAGE;
    }

    if (cmd == _PHY_PRBS_SET_CMD) {
        bcma_cli_parse_table_init(cli, &pt);
        bcma_cli_parse_table_add(&pt, "Polynomial", "str",
                                 &poly_str, NULL);
        bcma_cli_parse_table_add(&pt, "Invert", "int",
                                 &invert, NULL);

        if (bcma_cli_parse_table_do_args(&pt, args) < 0) {
            sal_printf("%s: Invalid option: %s\n",
                BCMA_CLI_CONFIG_ERROR_STR, BCMA_CLI_ARG_CUR(args));
            bcma_cli_parse_table_done(&pt);

            return BCMA_CLI_CMD_USAGE;
        }

        if (poly_str) {
            if (!sal_strcasecmp(poly_str, "P7") ||
                !sal_strcasecmp(poly_str, "0")) {
                poly = 0;
            } else if (!sal_strcasecmp(poly_str, "P15") ||
                       !sal_strcasecmp(poly_str, "1")  ) {
                poly = 1;
            } else if (!sal_strcasecmp(poly_str, "P23") ||
                       !sal_strcasecmp(poly_str, "2") ) {
                poly = 2;
            } else if (!sal_strcasecmp(poly_str, "P31") ||
                       !sal_strcasecmp(poly_str, "3") ) {
                poly = 3;
            } else if (!sal_strcasecmp(poly_str, "P9") ||
                       !sal_strcasecmp(poly_str, "4")) {
                poly = 4;
            } else if (!sal_strcasecmp(poly_str, "P11") ||
                       !sal_strcasecmp(poly_str, "5") ) {
                poly = 5;
            } else if (!sal_strcasecmp(poly_str, "P58") ||
                       !sal_strcasecmp(poly_str, "6")) {
                poly = 6;
            } else {
                sal_printf("Prbs p must be P7(0), P15(1), P23(2), P31(3), "
                           "P9(4), P11(5), or P58(6).\n");
                bcma_cli_parse_table_done(&pt);

                return BCMA_CLI_CMD_FAIL;
            }
        }

        bcma_cli_parse_table_done(&pt);
    }

    if (cmd == _PHY_PRBS_SET_CMD || cmd == _PHY_PRBS_CLEAR_CMD) {
        rv = soc_phyctrl_diag_ctrl(unit, port, inst, PHY_DIAG_CTRL_SET,
                                       SOC_PHY_CONTROL_PRBS_POLYNOMIAL,
                                       BCMA_CLICM_PHY_INT_TO_PTR(poly));
        if (rv < 0) {
            sal_printf("Setting prbs polynomial failed. rv: %d\n", rv);
            return BCMA_CLI_CMD_FAIL;
        }

        rv = soc_phyctrl_diag_ctrl(unit, port, inst, PHY_DIAG_CTRL_SET,
                                       SOC_PHY_CONTROL_PRBS_TX_INVERT_DATA,
                                       BCMA_CLICM_PHY_INT_TO_PTR(invert));
        if (rv < 0) {
            sal_printf("Setting prbs invertion failed. rv: %d\n", rv);
            return BCMA_CLI_CMD_FAIL;
        }

        rv = soc_phyctrl_diag_ctrl(unit, port, inst, PHY_DIAG_CTRL_SET,
                                       SOC_PHY_CONTROL_PRBS_TX_ENABLE,
                                       BCMA_CLICM_PHY_INT_TO_PTR(enable));
        if (rv < 0) {
            sal_printf("Setting prbs tx enable failed. rv: %d\n", rv);
            return BCMA_CLI_CMD_FAIL;
        }

        rv = soc_phyctrl_diag_ctrl(unit, port, inst, PHY_DIAG_CTRL_SET,
                                       SOC_PHY_CONTROL_PRBS_RX_ENABLE,
                                       BCMA_CLICM_PHY_INT_TO_PTR(enable));
        if (rv < 0) {
            sal_printf("Setting prbs rx enable failed. rv: %d\n", rv);
            return BCMA_CLI_CMD_FAIL;
        }

    } else {   /* _PHY_PRBS_GET_CMD */
        rv = soc_phyctrl_diag_ctrl(unit, port, inst, PHY_DIAG_CTRL_GET,
                                       SOC_PHY_CONTROL_PRBS_RX_STATUS,
                                       (void *)&status);
        if (rv < 0) {
            sal_printf("Getting prbs rx status failed. rv: %d\n", rv);
            return BCMA_CLI_CMD_FAIL;
        }

        switch (status) {
            case 0:
                sal_printf("port %2d: PRBS OK!\n", port);
                break;
            case -1:
                sal_printf("port %2d: PRBS Failed!\n", port);
                break;
            default:
                sal_printf("port %2d: PRBS has %d errors\n", port, status);
                break;
        }
    }

    return BCMA_CLI_CMD_OK;
}

static struct {
    const char *name;
    int (*func)(bcma_cli_t *cli, uint8 port, bcma_cli_args_t *arg);
    int (*cleanup)(bcma_cli_t *cli);
} phydiag_cmds[] = {
    {"dsc",      bcma_clicmd_phydiag_dsc_dump, NULL},
    {"prbs",     bcma_clicmd_phydiag_prbs,     NULL}
};

static int
bcma_clicmd_phydiag(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    int   idx, uport = -1;
    uint8 lport = 0, unit = cli->cmd_unit;
    const char *arg;

    arg = BCMA_CLI_ARG_GET(args);
    if (arg == NULL) {
       return BCMA_CLI_CMD_USAGE;
    }

    if (bcma_cli_parse_int(arg, &uport) != -1) {
        if (SAL_UPORT_IS_NOT_VALID(uport)) {
            sal_printf("Invalid user port %d!\r\n", uport);
            sal_printf("User port range from %d to %d\n", SAL_NZUPORT_TO_UPORT(1), board_uport_count());
            return BCMA_CLI_CMD_FAIL;
        }
        board_uport_to_lport(uport, &unit, &lport);
        sal_printf("Phy diag: user port %d logical port %d\r\n", uport, lport);
    } else {
        return BCMA_CLI_CMD_USAGE;
    }

    arg = BCMA_CLI_ARG_GET(args);

    for (idx = 0; idx < COUNTOF(phydiag_cmds); idx++) {
        if (sal_strcasecmp(arg, phydiag_cmds[idx].name) == 0) {
            return phydiag_cmds[idx].func(cli, lport, args);
        }
    }

    return BCMA_CLI_CMD_USAGE;
}

static int
bcma_clicmd_phyinfo(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    uint8 i, lport = 0, unit = cli->cmd_unit;
    int port_max_cnt = board_uport_count();

    sal_printf("%6s %6s %5s %7s %13s\n",
            "  port", "id0", "id1", "addr", "name");

    for (i = SAL_NZUPORT_TO_UPORT(1); i <= port_max_cnt; i++) {
        if (SAL_UPORT_IS_NOT_VALID(i)) {
            continue;
        }
        board_uport_to_lport(i, &unit, &lport);
        sal_printf("%3d(%3d) 0x%4x 0x%4x 0x%2x %23s\n",
        i, lport,
        soc_phy_id0reg_get(unit, lport),
        soc_phy_id1reg_get(unit, lport),
        soc_phy_addr_of_port(unit, lport),
        soc_phy_name_get(unit, lport));
    }

    return BCMA_CLI_CMD_OK;
}

static int
bcma_clicmd_miim_read(int unit, uint16 phy_id,
                          uint8 phy_reg_addr, uint16 *phy_rd_data)
{
    int rv;
    uint32_t data = 0;
    cmicx_miim_op_t miim_op, *op = &miim_op;

    sal_memset(op, 0, sizeof(*op));

    op->opcode = CMICX_MIIM_OPC_CL22_READ;
    op->internal = PCM_PHY_ID_IS_INTERNAL_BUS(phy_id);
    op->busno = PCM_PHY_ID_BUS_NUM(phy_id);
    op->phyad = PCM_PHY_ID_PHY_ADDR(phy_id);
    op->regad = phy_reg_addr & 0x1f;

    rv = cmicx_miim_op(unit, op, &data);
    *phy_rd_data = data & 0xffff;

    return rv;
}

static int
bcma_clicmd_miimc45_read(int unit, uint16 phy_id, uint8 phy_devad,
                                uint16 phy_reg_addr, uint16 *phy_rd_data)
{
    int rv;
    uint32_t data = 0;
    cmicx_miim_op_t miim_op, *op = &miim_op;

    sal_memset(op, 0, sizeof(*op));

    op->opcode = CMICX_MIIM_OPC_CL45_READ;
    op->internal = PCM_PHY_ID_IS_INTERNAL_BUS(phy_id);
    op->busno = PCM_PHY_ID_BUS_NUM(phy_id);
    op->phyad = PCM_PHY_ID_PHY_ADDR(phy_id);
    op->regad = phy_reg_addr;
    op->devad = phy_devad;

    rv = cmicx_miim_op(unit, op, &data);
    *phy_rd_data = data & 0xffff;

    return rv;
}

static int
bcma_clicmd_miim_write(int unit, uint16 phy_id,
                           uint8 phy_reg_addr, uint16 phy_wr_data)
{
    uint32_t data = phy_wr_data;
    cmicx_miim_op_t miim_op, *op = &miim_op;

    sal_memset(op, 0, sizeof(*op));

    op->opcode = CMICX_MIIM_OPC_CL22_WRITE;
    op->internal = PCM_PHY_ID_IS_INTERNAL_BUS(phy_id);
    op->busno = PCM_PHY_ID_BUS_NUM(phy_id);
    op->phyad = PCM_PHY_ID_PHY_ADDR(phy_id);
    op->regad = phy_reg_addr & 0x1f;

    return cmicx_miim_op(unit, op, &data);
}

static int
bcma_clicmd_miimc45_write(int unit, uint16 phy_id, uint8 phy_devad,
                                 uint16 phy_reg_addr, uint16 phy_wr_data)
{
    uint32_t data = phy_wr_data;
    cmicx_miim_op_t miim_op, *op = &miim_op;

    sal_memset(op, 0, sizeof(*op));

    op->opcode = CMICX_MIIM_OPC_CL45_WRITE;
    op->internal = PCM_PHY_ID_IS_INTERNAL_BUS(phy_id);
    op->busno = PCM_PHY_ID_BUS_NUM(phy_id);
    op->phyad = PCM_PHY_ID_PHY_ADDR(phy_id);
    op->regad = phy_reg_addr;
    op->devad = phy_devad;

    return cmicx_miim_op(unit, op, &data);
}


static int
bcma_clicmd_phy_raw_acc(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    char *c, *bus_name;
    int is_c45 = 0;
    int is_sbus = 0;
    int is_mpp1 = 0;
    uint16 phy_data, phy_devad = 0;
    uint16 phy_addr, phy_lane = 0, phy_pindex = 0;
    uint32 phy_reg, phy_wrmask;
    uint32 phy_data32, phy_aer = 0;
    int rv = 0, u = 0;

    if ((c = BCMA_CLI_ARG_GET(args)) == NULL) {
        return BCMA_CLI_CMD_USAGE;
    }

    bus_name = "miim";
    if (sal_strcasecmp(c, "sbus") == 0) {
        is_sbus = 1;
        bus_name = "sbus_mdio";
        if ((c = BCMA_CLI_ARG_GET(args)) == NULL) {
            return BCMA_CLI_CMD_USAGE;
        }
    } else if (sal_strcasecmp(c, "c45") == 0) {
        bus_name = "miimc45";
        is_c45 = 1;
        if ((c = BCMA_CLI_ARG_GET(args)) == NULL) {
            return BCMA_CLI_CMD_USAGE;
        }
    }
    phy_addr = sal_strtoul(c, NULL, 0);
    if ((c = BCMA_CLI_ARG_GET(args)) == NULL) { /* Get register number */
        return BCMA_CLI_CMD_USAGE;
    }
    if (is_sbus) {
        phy_devad = sal_strtoul(c, NULL, 0);
        if (phy_devad > 0x1f) {
            sal_printf("ERROR: Invalid devad 0x%x, max=0x%x\n",
                    phy_devad, 0x1f);
            return BCMA_CLI_CMD_FAIL;
        }
        /*
         * If DEVAD is specified as X.Y then X is the DEVAD and Y
         * is the lane number. For example DEVAD=1 and lane=2 is
         * specified as 1.2 and encoded in phy_aer as 0x0802.
         */
        if ((c = sal_strchr(c, '.')) != NULL) {
            c++;
            phy_lane = sal_strtoul(c, NULL, 0);
            /* next check lane number is greater than 7 for TSCBH */
            if (phy_lane > 7) {
                sal_printf("ERROR: Invalid phy_lane 0x%x, max=0x%x\n",
                        phy_lane, 0x7);
                return BCMA_CLI_CMD_FAIL;
            } else {
                if (phy_lane > 3) {
                    is_mpp1 = 1;
                }
            }
            if((c = sal_strchr(c, '.')) != NULL) {
                c++;
                phy_pindex = sal_strtoul(c, NULL, 0);
            }
        }

        /* need to check if TSCBH 8 lane, for pcs register lane 0-lane3 using MPP0
        and lane 4-7 using MPP1, the bit location for MPP0 is 24 and MPP1 is 25 */
        if (phy_devad == 0) {
            if (is_mpp1) {
                phy_aer = 1 << 9;
            } else {
                phy_aer = 1 << 8;
            }
            phy_aer |= (phy_devad << 11) | (phy_lane % 4);
        } else {
            phy_aer = (phy_devad << 11) | phy_lane | (phy_pindex << 8);
        }
        if ((c = BCMA_CLI_ARG_GET(args)) == NULL) {     /* Get register number */
            return BCMA_CLI_CMD_USAGE;
        }
    } else  if (is_c45) {
        phy_devad = sal_strtoul(c, NULL, 0);
        if (phy_devad > 0x1f) {
            sal_printf("ERROR: Invalid devad 0x%x, max=0x%x\n",
                    phy_devad, 0x1f);
            return BCMA_CLI_CMD_FAIL;
        }
        if ((c = BCMA_CLI_ARG_GET(args)) == NULL) {     /* Get register number */
            return BCMA_CLI_CMD_USAGE;
        }
    }
    phy_reg = sal_strtoul(c, NULL, 0);
    if ((c = BCMA_CLI_ARG_GET(args)) == NULL) { /* Read register */
        if (is_sbus) {
            phy_reg = (phy_aer << 16) | phy_reg;
            rv = pcm_phy_reg_sbus_get(u, phy_addr, phy_reg, &phy_data32);
            phy_data = (uint16)phy_data32;
        } else if (is_c45) {
            rv = bcma_clicmd_miimc45_read(u, phy_addr, phy_devad,
                                  phy_reg, &phy_data);
        } else {
            rv = bcma_clicmd_miim_read(u, phy_addr, phy_reg, &phy_data);
        }
        if (rv < 0) {
            sal_printf("ERROR: MII Addr %d: soc_%s_read failed: %d\n",
                    phy_addr, bus_name, rv);
            return BCMA_CLI_CMD_FAIL;
        }
        sal_printf("%s\t0x%02x: 0x%04x\n", "", phy_reg, phy_data);
    } else {                /* write */
        phy_data = sal_strtoul(c, NULL, 0);
        if (is_sbus) {
            /*
             * If register data is specified as X/Y then X is the
             * data value and Y is the data mask. For example
             * data=0x50 and mask=0xf0 is specified as 0x50/0xf0
             * and encoded in phy_data32 as 0x0f000500.  Note that
             * if the mask is not specified (or zero) then it is
             * implicitly assumed to be 0xffff (all bits valid).
             */
            phy_data32 = phy_data;
            phy_wrmask = 0;
            if ((c = sal_strchr(c, '/')) != NULL) {
                c++;
                phy_wrmask = sal_strtoul(c, NULL, 0);
                phy_data32 |= (phy_wrmask << 16);
            }
            phy_reg = (phy_aer << 16) | phy_reg;
            rv = pcm_phy_reg_sbus_set(u, phy_addr, phy_reg, phy_data32);
        } else if (is_c45) {
            rv = bcma_clicmd_miimc45_write(u, phy_addr, phy_devad, phy_reg,
                                   phy_data);
        } else {
            rv = bcma_clicmd_miim_write(u, phy_addr, phy_reg, phy_data);
        }
        if (rv < 0) {
            sal_printf("ERROR: MII Addr %d: soc_%s_write failed: %d\n",
                    phy_addr, bus_name, rv);
            return BCMA_CLI_CMD_FAIL;
        }
    }
    return BCMA_CLI_CMD_OK;
}

extern int phymod_symop_access(int unit, bcma_cli_args_t *a, uint8 lport);

static int
bcma_clicmd_phy_sym_acc(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    int   uport = -1, rv = 0;
    uint8 lport = 0, unit = cli->cmd_unit;
    const char *arg;

    arg = BCMA_CLI_ARG_GET(args);
    if (arg == NULL) {
       return BCMA_CLI_CMD_USAGE;
    }

    if (bcma_cli_parse_int(arg, &uport) != -1) {
        if (SAL_UPORT_IS_NOT_VALID(uport)) {
            sal_printf("Invalid user port %d!\r\n", uport);
            sal_printf("User port range from %d to %d\n", SAL_NZUPORT_TO_UPORT(1), board_uport_count());
            return BCMA_CLI_CMD_FAIL;
        }
        board_uport_to_lport(uport, &unit, &lport);
        sal_printf("Phy diag: user port %d logical port %d\r\n", uport, lport);
    } else {
        sal_printf("Phy sym access port parse error failed.\n");
        return BCMA_CLI_CMD_FAIL;
    }

    rv = phymod_symop_access(unit, args, lport);
    if (rv < 0) {
         sal_printf("Phy sym access failed. rv: %d\n", rv);
         return BCMA_CLI_CMD_FAIL;
    }

    return BCMA_CLI_CMD_OK;
}

int
bcma_clicmd_phy(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    const char *arg;

    arg = BCMA_CLI_ARG_GET(args);
    if (arg == NULL) {
        return BCMA_CLI_CMD_USAGE;
    }

    /* Diagnostics commands  */
    if (sal_strcasecmp(arg, "diag") == 0) {
        return bcma_clicmd_phydiag(cli, args);
    }

    /* phy driver information */
    if (sal_strcasecmp(arg, "info") == 0) {
        return bcma_clicmd_phyinfo(cli, args);
    }

    /* PHY raw access commands */
    if (sal_strcasecmp(arg, "raw") == 0) {
        return bcma_clicmd_phy_raw_acc(cli, args);
    }

    /* PHY symbol access command */
    BCMA_CLI_ARG_PREV(args);
    return bcma_clicmd_phy_sym_acc(cli, args);
}
#endif /* CFG_SDKCLI_INCLUDED */

#endif
