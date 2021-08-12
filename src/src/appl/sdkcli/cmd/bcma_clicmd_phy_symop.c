/*
 * $Id$
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 *
 * This file contains a set of functions which can be used to parse an
 * SDK CLI command into a symbolic PHY operation.
 *
 * For example usage, please refer to e.g. $SDK/src/appl/diag/port.c.
 */

#include "system.h"
#ifdef CFG_SDKCLI_PHY_INCLUDED

#include <appl/sdkcli/bcma_cli.h>
#include <appl/sdkcli/bcma_cli_var.h>
#include <appl/sdkcli/bcma_cli_redir.h>
#include <appl/sdkcli/bcma_cli_parse.h>

#undef _SOC_PHYCTRL_H_
#include <soc/phyctrl.h>
#include <soc/phy/phyctrl.h>

#if defined(PHYMOD_SUPPORT)
#include <phymod/phymod.h>
#include <phymod/phymod_config.h>
#include <phymod/phymod_symbols.h>
#include <phymod/phymod_reg.h>
#ifdef PHYMOD_TSCE16_SUPPORT
#include <phymod/chip/bcmi_tsce16_xgxs_sym.h>
#endif
#ifdef PHYMOD_TSCF16_GEN3_SUPPORT
#include <phymod/chip/bcmi_tscf16_gen3_xgxs_sym.h>
#endif /*PHYMOD_TSCF16_GEN3_SUPPORT  */

#define PHY_FIELD_NAME_MAX      80
#define PHY_FIELDS_MAX          32

typedef struct phy_field_s {
    char name[PHY_FIELD_NAME_MAX + 1];
        uint32 val;
} phy_field_t;

typedef struct phy_field_info_s {
    uint32 regval; /* raw value */
    int num_fields;
    phy_field_t field[PHY_FIELDS_MAX];
} phy_field_info_t;

typedef struct phy_iter_s {
    const char *name;
    const char *found_name;
#define PHY_SHELL_SYM_RAW       0x1     /* Do not decode fields */
#define PHY_SHELL_SYM_LIST      0x2     /* List symbol only */
#define PHY_SHELL_SYM_NZ        0x4     /* Skip if reg/field is zero  */
#define PHY_SHELL_SYM_RESET     0x8     /* Reset register */
    uint32 flags;
    uint32 addr;
    int lane;
    int pll_idx;
    int count;
    const phymod_symbols_t *symbols;
    phymod_phy_access_t *phy_access;
    phy_field_info_t *finfo;
    char tmpstr[64];
} phy_iter_t;

STATIC int
_phymod_encode_field(const phymod_symbol_t *symbol,
                     const char **fnames,
                     const char *field, uint32 value,
                     uint32 *and_masks, uint32 *or_masks)
{
#if PHYMOD_CONFIG_INCLUDE_FIELD_NAMES == 1
    phymod_field_info_t finfo;
    uint32 zero_val;

    PHYMOD_SYMBOL_FIELDS_ITER_BEGIN(symbol->fields, finfo, fnames) {

        if (sal_strcasecmp(finfo.name, field)) {
            continue;
        }

        zero_val = 0;
        phymod_field_set(and_masks, finfo.minbit, finfo.maxbit, &zero_val);
        /* coverity[callee_ptr_arith] */
        phymod_field_set(or_masks, finfo.minbit, finfo.maxbit, &value);

        return 0;
    } PHYMOD_SYMBOL_FIELDS_ITER_END();

#endif /* PHYMOD_CONFIG_INCLUDE_FIELD_NAMES */

    return -1;
}

#if PHYMOD_CONFIG_INCLUDE_FIELD_INFO == 1
STATIC int
_phymod_print_str(const char *str)
{
    cli_out("%s", str);
    return 0;
}
#endif

STATIC int
_phymod_sym_list(const phymod_symbol_t *symbol, uint32 flags, const char **fnames)
{
    uint32 reg_addr;
    uint32 num_copies;

    cli_out("Name:     %s", symbol->name);
    if (symbol->alias != NULL) {
        cli_out(" (%s)", symbol->alias);
    }
    cli_out("\n");
    reg_addr = symbol->addr & 0x00ffffff;
    switch (PHYMOD_REG_ACCESS_METHOD(symbol->addr)) {
    case PHYMOD_REG_ACC_AER_IBLK:
    case PHYMOD_REG_ACC_TSC_IBLK:
        cli_out("Address:  0x%"PRIx32"", reg_addr & 0xfffff);
        num_copies = (reg_addr >> 20) & 0xf;
        if (num_copies == 1) {
            cli_out(" (1 copy only)");
        } else if (num_copies == 2) {
            cli_out(" (2 copies only)");
        }
        cli_out("\n");
        break;
    case PHYMOD_REG_ACC_RAW:
    default:
        cli_out("Address:  0x%"PRIx32"\n", symbol->addr);
        break;
    }
    if (flags & PHY_SHELL_SYM_RAW) {
        cli_out("\n");

        /* Keep the watchdog timer alive */
        board_wdt_ping();
        return 0;
    }
#if PHYMOD_CONFIG_INCLUDE_RESET_VALUES == 1
    cli_out("Reset:    0x%"PRIx32" (%"PRIu32")\n",
            symbol->resetval, symbol->resetval);
#endif
#if PHYMOD_CONFIG_INCLUDE_FIELD_INFO == 1
    if (symbol->fields) {
        cli_out("Fields:   %"PRIu32"\n",
                phymod_field_info_count(symbol->fields));
        phymod_symbol_show_fields(symbol, fnames, NULL, 0,
                                  _phymod_print_str,
                                  NULL, NULL);
    }
#endif
    return 0;
}

STATIC int
_phymod_sym_iter_count(const phymod_symbol_t *symbol, void *vptr)
{
    phy_iter_t *phy_iter = (phy_iter_t *)vptr;

    return ++(phy_iter->count);
}

STATIC int
_phymod_sym_find_hex(const phymod_symbol_t *symbol, void *vptr)
{
    phy_iter_t *phy_iter = (phy_iter_t *)vptr;

    if (phy_iter->symbols == NULL) {
        return -1;
    }

    if ((symbol->addr & 0xffff) == phy_iter->addr) {
        phy_iter->found_name = symbol->name;
    }
    return 0;
}

STATIC int
_phymod_sym_iter_op(const phymod_symbol_t *symbol, void *vptr)
{
    int rv;
    uint32 data;
    uint32 and_mask;
    uint32 or_mask;
    uint32 lane_mask;
    phymod_phy_access_t lpa, *pa;
    phy_iter_t *phy_iter = (phy_iter_t *)vptr;
    phy_field_t *fld;
    uint32 reg_addr;
    uint32 fdx;
    char lane_str[16];
    char pIdx_str[16];
    const char **fnames;

    if (phy_iter->symbols == NULL) {
        return -1;
    }
    fnames = phy_iter->symbols->field_names;

    if (phy_iter->flags & PHY_SHELL_SYM_LIST) {
        _phymod_sym_list(symbol, phy_iter->flags, fnames);
        return 0;
    }

    pa = phy_iter->phy_access;
    /*There are only two PLL at most in a SerDes core.
      0 : PLL0
      1 : PLL1
    */
    if ((phy_iter->pll_idx >= 0) && (phy_iter->pll_idx <= 1)) {
        pa->access.pll_idx = phy_iter->pll_idx;
    }
    if (phy_iter->lane >= 0) {
        lane_mask = 1 << phy_iter->lane;
        if (lane_mask != pa->access.lane_mask) {
            sal_memcpy(&lpa, pa, sizeof(lpa));

            while ((lane_mask && (lane_mask & 0xFF)) == 0) {
                   lane_mask >>= 8;
                   lpa.access.addr += 8;
            }
            lpa.access.lane_mask = lane_mask;
            pa = &lpa;
        }
    }
    reg_addr = symbol->addr;

    if (phy_iter->flags & PHY_SHELL_SYM_RESET) {
#if PHYMOD_CONFIG_INCLUDE_RESET_VALUES == 1
        if (phymod_phy_reg_write(pa, reg_addr, symbol->resetval) != 0) {
            cli_out("Error resetting %s\n", symbol->name);
            return -1;
        }
#endif
        return 0;
    }

    if (phy_iter->finfo) {
        if (phy_iter->finfo->num_fields) {
            and_mask = ~0;
            or_mask = 0;
            for (fdx = 0; fdx < phy_iter->finfo->num_fields; fdx++) {
                fld = &phy_iter->finfo->field[fdx];
                /* coverity[callee_ptr_arith] */
                rv = _phymod_encode_field(symbol, fnames,
                                          fld->name, fld->val,
                                          &and_mask, &or_mask);
                if (rv < 0) {
                    cli_out("Invalid field: %s\n", fld->name);
                    return -1;
                }
            }
        } else {
            and_mask = 0;
            or_mask = phy_iter->finfo->regval;
        }

        /* Read, update and write PHY register */
        if (phymod_phy_reg_read(pa, reg_addr, &data) != 0) {
            cli_out("Error reading %s\n", symbol->name);
            return -1;
        }
        data &= and_mask;
        data |= or_mask;
        if (phymod_phy_reg_write(pa, reg_addr, data) != 0) {
            cli_out("Error writing %s\n", symbol->name);
            return -1;
        }
    }
    else {
        /* Decode PHY register */
        if (phymod_phy_reg_read(pa, reg_addr, &data) != 0) {
            cli_out("Error reading %s\n", symbol->name);
            return -1;
        }
        if (data == 0 && (phy_iter->flags & PHY_SHELL_SYM_NZ)) {
            return 0;
        }
        lane_str[0] = 0;
        pIdx_str[0] = 0;
        if (phy_iter->lane >= 0) {
            sal_sprintf(lane_str, ".%d", phy_iter->lane);
            if (phy_iter->pll_idx >= 0) {
                sal_sprintf(pIdx_str, ".%d", phy_iter->pll_idx);
            }
        }
        /* wait for print buffer */
        sal_usleep(10000);
        /* Keep the watchdog timer alive */
        board_wdt_ping();
        cli_out("%s%s%s [0x%08"PRIx32"] = 0x%04"PRIx32"\n",
                symbol->name, lane_str, pIdx_str, reg_addr, data);
#if PHYMOD_CONFIG_INCLUDE_FIELD_INFO == 1
        if (phy_iter->flags & PHY_SHELL_SYM_RAW) {
            return 0;
        }
        if (symbol->fields) {
            /* coverity[callee_ptr_arith] */
            phymod_symbol_show_fields(symbol, fnames, &data,
                                      (phy_iter->flags & PHY_SHELL_SYM_NZ),
                                      _phymod_print_str,
                                      NULL, NULL);
        }
#endif
    }
    return 0;
}


/* Get PHYMod symbol table and access object from unit/port */
int
phymod_sym_info(int unit, int port, phymod_symbols_iter_t *iter, phymod_phy_access_t *pm_acc)
{
    phy_ctrl_t *pc;

    if (pm_acc == NULL) {
        return -1;
    }

    if (phy_port_info[unit] == NULL) {
        return -1;
    }

    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return -1;
    }

    if (pc->phymod_ctrl.phy[0] == NULL) {
        return -1;
    }

    sal_memcpy(pm_acc, &pc->phymod_ctrl.phy[0]->pm_phy, sizeof(*pm_acc));


    return 0;
}

/*
 * Function:
 *    phymod_symop_init
 * Purpose:
 *    Parse command line for symbolic PHY operation
 * Parameters:
 *    iter    - symbol table iterator structure
 *    a       - CLI argument list
 * Returns:
 *    BCMA_CLI_CMD_OK if no error
 */
int
phymod_symop_init(phymod_symbols_iter_t *iter, bcma_cli_args_t *a)
{
    phy_iter_t *phy_iter;
    phy_field_info_t *finfo = NULL;
    char *c;
    const char *name;
    uint32 flags;
    int fdx, lane, pll_idx;
    int val = 0;

    if (iter == NULL) {
        return BCMA_CLI_CMD_FAIL;
    }

    phy_iter = sal_alloc(sizeof(phy_iter_t), "phy_iter");
    if (phy_iter == NULL) {
        cli_out("%s: Unable to allocate PHY iterator\n",
                BCMA_CLI_ARG_CMD(a));
        return BCMA_CLI_CMD_FAIL;
    }

    sal_memset(iter, 0, sizeof(*iter));
    sal_memset(phy_iter, 0, sizeof(*phy_iter));
    iter->vptr = phy_iter;

    if ((c = BCMA_CLI_ARG_GET(a)) == NULL) {
        return BCMA_CLI_CMD_USAGE;
    }
    name = c;

    /* Check for output flags */
    flags = 0;
    do {
        if (sal_strcmp(c, "list") == 0) {
            flags |= PHY_SHELL_SYM_LIST;
        } else if (sal_strcmp(c, "raw") == 0) {
            flags |= PHY_SHELL_SYM_RAW;
        } else if (sal_strcmp(c, "nz") == 0) {
            flags |= PHY_SHELL_SYM_NZ;
        } else if (sal_strcmp(c, "reset") == 0) {
            flags |= PHY_SHELL_SYM_RESET;
        } else {
            name = c;
            break;
        }
    } while ((c = BCMA_CLI_ARG_GET(a)) != NULL);

    /* Check for lane-specification, i.e. <regname>.<lane> */
    lane = -1;
    pll_idx = -1;
    /* coverity[returned_pointer] */
    c = sal_strchr(name, '.');
    if (c != NULL) {
        sal_strncpy(phy_iter->tmpstr, name, sizeof(phy_iter->tmpstr));
        phy_iter->tmpstr[sizeof(phy_iter->tmpstr)-1] = 0;
        name = phy_iter->tmpstr;
        c = sal_strchr(name, '.');
        if (c != NULL) {
            *c++ = 0;
            lane = sal_ctoi(c, NULL);
            c = sal_strchr(c, '.');
            if (c != NULL) {
                *c++ = 0;
                pll_idx = sal_ctoi(c, NULL);
            }
        }
    }

    if ((c = BCMA_CLI_ARG_GET(a)) != NULL) {
        finfo = sal_alloc(sizeof(phy_field_info_t), "field_info");
        if (finfo == NULL) {
            cli_out("%s: Unable to allocate field info\n",
                    BCMA_CLI_ARG_CMD(a));
            return BCMA_CLI_CMD_FAIL;
        }
        sal_memset(finfo, 0, sizeof (*finfo));
        if (bcma_cli_parse_is_int(c)) {
            /* Raw register value */
            bcma_cli_parse_int(c, &val);
            finfo->regval = val;
            /* No further parameters expected */
            if (BCMA_CLI_ARG_CNT(a) > 0) {
                cli_out("%s: Unexpected argument %s\n",
                        BCMA_CLI_ARG_CMD(a), BCMA_CLI_ARG_CUR(a));
                sal_free(finfo);
                return BCMA_CLI_CMD_USAGE;
            }
        } else {
            /* Individual fields */
            fdx = 0;
            do {
                if (finfo->num_fields >= PHY_FIELDS_MAX) {
                    cli_out("%s: Too many fields\n",
                            BCMA_CLI_ARG_CMD(a));
                    sal_free(finfo);
                    return BCMA_CLI_CMD_FAIL;
                }
                sal_strncpy(finfo->field[fdx].name, c,
                            PHY_FIELD_NAME_MAX);
                c = sal_strchr(finfo->field[fdx].name, '=');
                if (c == NULL) {
                    cli_out("%s: Invalid field assignment: %s\n",
                            BCMA_CLI_ARG_CMD(a), finfo->field[fdx].name);
                    sal_free(finfo);
                    return BCMA_CLI_CMD_FAIL;
                }
                *c++ = 0;
                bcma_cli_parse_int(c, &val);
                finfo->field[fdx].val = val;
                fdx++;
            } while ((c = BCMA_CLI_ARG_GET(a)) != NULL);
            finfo->num_fields = fdx;
        }
    }

    iter->name = name;
    iter->matching_mode = PHYMOD_SYMBOLS_ITER_MODE_EXACT;
    if (sal_strcmp(iter->name, "*") != 0) {
        switch (iter->name[0]) {
        case '^':
            iter->matching_mode = PHYMOD_SYMBOLS_ITER_MODE_START;
            iter->name++;
            break;
        case '*':
            iter->matching_mode = PHYMOD_SYMBOLS_ITER_MODE_STRSTR;
            iter->name++;
            break;
        case '@':
            iter->matching_mode = PHYMOD_SYMBOLS_ITER_MODE_EXACT;
            iter->name++;
            break;
        default:
            if (flags & PHY_SHELL_SYM_LIST) {
                iter->matching_mode = PHYMOD_SYMBOLS_ITER_MODE_STRSTR;
            }
            break;
        }
    }
    phy_iter->lane = lane;
    phy_iter->pll_idx = pll_idx;
    phy_iter->flags = flags;
    phy_iter->finfo = finfo;

    /* Save for symbol lookup based on register address */
    phy_iter->name = iter->name;
    phy_iter->addr = sal_ctoi(phy_iter->name, NULL);

    return BCMA_CLI_CMD_OK;
}

/*
 * Function:
 *    phymod_symop_exec
 * Purpose:
 *    Execute operation on one or more symbols
 * Parameters:
 *    iter    - symbol table iterator structure
 *    symbols - symbol table
 *    pm_acc  - PHY access structure
 *    hdr     - header string show for register read operations
 * Returns:
 *    BCMA_CLI_CMD_OK if no error
 */
int
phymod_symop_exec(phymod_symbols_iter_t *iter, const phymod_symbols_t *symbols,
                  phymod_phy_access_t *pm_acc, char *hdr)
{
    phy_iter_t *phy_iter;

    if (iter == NULL || iter->vptr == NULL) {
        return BCMA_CLI_CMD_FAIL;
    }
    phy_iter = (phy_iter_t *)iter->vptr;

    iter->symbols = symbols;
    phy_iter->symbols = iter->symbols;
    phy_iter->phy_access = pm_acc;

    if (phy_iter->flags & PHY_SHELL_SYM_LIST) {
        /*
         * For symbol listings we force raw mode if multiple
         * matches are found.
         */
        phy_iter->count = 0;
        iter->function = _phymod_sym_iter_count;
        if (phymod_symbols_iter(iter) > 1) {
            phy_iter->flags |= PHY_SHELL_SYM_RAW;
        }
    }

    iter->function = _phymod_sym_iter_op;

    if (phy_iter->finfo == NULL && (phy_iter->flags & PHY_SHELL_SYM_RESET) == 0) {
        /* Show header only if not writing */
        cli_out("%s", hdr ? hdr : "");
    }
    /* If numerical value specified, then look up symbol name */
    if (phy_iter->name[0] >= '0' && phy_iter->name[0] <= '9') {
        iter->function = _phymod_sym_find_hex;
        iter->name = "*";
        phy_iter->found_name = NULL;
        /* Search for register address */
        if (phymod_symbols_iter(iter) == 0 || phy_iter->found_name == NULL) {
            cli_out("No matching address\n");
            return BCMA_CLI_CMD_OK;
        }
        iter->name = phy_iter->found_name;
        /* Restore iterator function */
        iter->function = _phymod_sym_iter_op;
    }
    /* Iterate over all symbols in symbol table */
    if (phymod_symbols_iter(iter) == 0) {
        cli_out("No matching symbols\n");
    }
    return BCMA_CLI_CMD_OK;
}

/*
 * Function:
 *    phymod_symop_cleanup
 * Purpose:
 *    Free resources allocated by phymod_symop_init
 * Parameters:
 *    iter    - symbol table iterator structure
 * Returns:
 *    BCMA_CLI_CMD_OK if no error
 */
int
phymod_symop_cleanup(phymod_symbols_iter_t *iter)
{
    phy_iter_t *phy_iter;

    if (iter == NULL || iter->vptr == NULL) {
        return BCMA_CLI_CMD_FAIL;
    }
    phy_iter = (phy_iter_t *)iter->vptr;

    if (phy_iter != NULL) {
        if (phy_iter->finfo != NULL) {
            sal_free(phy_iter->finfo);
        }
        sal_free(phy_iter);
    }
    sal_memset(iter, 0, sizeof(*iter));

    return BCMA_CLI_CMD_OK;
}

int phymod_symop_table_get(phymod_phy_access_t *phy, phymod_symbols_t **symbols)
{
    switch(phy->type){
#ifdef PHYMOD_QSGMIIE_SUPPORT
    case phymodDispatchTypeQsgmiie:
        *symbols = &bcmi_qsgmiie_serdes_symbols;
        break;
#endif /*PHYMOD_QSGMIIE_SUPPORT  */
#ifdef PHYMOD_TSCE_SUPPORT
    case phymodDispatchTypeTsce:
        *symbols = &bcmi_tsce_xgxs_symbols;
        break;
#endif /*PHYMOD_TSCE_SUPPORT  */
#ifdef PHYMOD_TSCF_SUPPORT
    case phymodDispatchTypeTscf:
    {
        /* first need to get the tscf model number */
      MAIN0_SERDESIDr_t serdesid;
      uint32_t model;

      /* next check serdes ID to see if gen2 or not */
      READ_MAIN0_SERDESIDr(&phy->access, &serdesid);
      model = MAIN0_SERDESIDr_MODEL_NUMBERf_GET(serdesid);

      /* check if tscf_gen2 version */
      if (model == 0x15)  {
        *symbols = &bcmi_tscf_xgxs_gen2_symbols;
      } else {
        *symbols = &bcmi_tscf_xgxs_symbols;
      }
        break;
    }
#endif /*PHYMOD_TSCF_SUPPORT  */
#if 0 /* #ifdef PHYMOD_PHY8806X_SUPPORT */
    case phymodDispatchTypePhy8806x:
        *symbols = &bcmi_phy8806x_xgxs_symbols;
        break;
#endif /*PHYMOD_PHY8806X_SUPPORT  */
#ifdef PHYMOD_EAGLE_SUPPORT
    case phymodDispatchTypeEagle:
        *symbols = &bcmi_eagle_xgxs_symbols;
        break;
#endif /*PHYMOD_EAGLE_SUPPORT  */
#ifdef PHYMOD_FALCON_SUPPORT
    case phymodDispatchTypeFalcon:
        *symbols = &bcmi_falcon_xgxs_symbols;
        break;
#endif /*PHYMOD_FALCON_SUPPORT  */
#ifdef PHYMOD_VIPER_SUPPORT
    case phymodDispatchTypeViper:
    {
        SERDESID0r_t serdes_id;
        uint32_t model = 0;

        READ_SERDESID0r(&phy->access, &serdes_id);
        model = SERDESID0r_MODEL_NUMBERf_GET(serdes_id);

        /* check if SGMIIPLUS2x4 core */
        if (model == 0xf) {
            *symbols = &bcmi_sgmiip2x4_serdes_symbols;
        } else {
            *symbols = &bcmi_viper_xgxs_symbols;
        }

        break;
    }
#endif /*PHYMOD_VIPER_SUPPORT  */
#ifdef PHYMOD_TSCF16_SUPPORT
    case phymodDispatchTypeTscf16:
        *symbols = &bcmi_tscf_16nm_xgxs_symbols;
        break;
#endif /*PHYMOD_TSCF16_SUPPORT  */
#ifdef PHYMOD_TSCE_DPLL_SUPPORT
    case phymodDispatchTypeTsce_dpll:
        *symbols = &bcmi_tsce_dpll_xgxs_symbols;
        break;
#endif
#ifdef PHYMOD_TSCE16_SUPPORT
    case phymodDispatchTypeTsce16:
        *symbols = &bcmi_tsce16_xgxs_symbols;
        break;
#endif
#ifdef PHYMOD_TSCF_GEN3_SUPPORT
    case phymodDispatchTypeTscf_gen3:
        *symbols = &bcmi_tscf_gen3_xgxs_symbols;
        break;
#endif /*PHYMOD_TSCF_GEN3_SUPPORT  */
#ifdef PHYMOD_BLACKHAWK_SUPPORT
    case phymodDispatchTypeBlackhawk:
        *symbols = &bcmi_blackhawk_xgxs_symbols;
        break;
#endif
#ifdef PHYMOD_TSCBH_SUPPORT
    case phymodDispatchTypeTscbh:
        *symbols = &bcmi_tscbh_xgxs_symbols;
        break;
#endif
#ifdef PHYMOD_TSCF16_GEN3_SUPPORT
    case phymodDispatchTypeTscf16_gen3:
        *symbols = &bcmi_tscf16_gen3_xgxs_symbols;
        break;
#endif /*PHYMOD_TSCF16_GEN3_SUPPORT  */
#ifdef PHYMOD_TSCBH_GEN2_SUPPORT
    case phymodDispatchTypeTscbh_gen2:
        *symbols = &bcmi_tscbh_gen2_xgxs_symbols;
        break;
#endif /*PHYMOD_TSCBH_GEN2_SUPPORT  */
#ifdef PHYMOD_TSCE7_SUPPORT
    case phymodDispatchTypeTsce7:
        *symbols = &bcmi_tsce7_xgxs_symbols;
        break;
#endif /*PHYMOD_TSCE7_SUPPORT  */
#ifdef PHYMOD_TSCBH_FLEXE_SUPPORT
    case phymodDispatchTypeTscbh_flexe:
        *symbols = &bcmi_tscbh_flexe_xgxs_symbols;
        break;
#endif /*PHYMOD_TSCBH_FLEXE_SUPPORT  */
#ifdef PHYMOD_TSCO_SUPPORT
    case phymodDispatchTypeTsco:
        *symbols = &bcmi_tsco_xgxs_symbols;
        break;
#endif /*PHYMOD_TSCO_SUPPORT  */
#ifdef PHYMOD_TSCBH_GEN3_SUPPORT
    case phymodDispatchTypeTscbh_gen3:
        *symbols = &bcmi_tscbh_gen3_xgxs_symbols;
        break;
#endif /*PHYMOD_TSCBH_GEN3_SUPPORT  */
#ifdef PHYMOD_BLACKHAWK7_V1L8P1_SUPPORT
        case phymodDispatchTypeBlackhawk7_v1l8p1:
            *symbols = &bcmi_blackhawk7_v1l8p1_xgxs_symbols;
            break;
#endif /*PHYMOD_BLACKHAWK7_V1L8P1_SUPPORT  */

    default:
        return PHYMOD_E_UNAVAIL;
    }
    return PHYMOD_E_NONE;
}


int
phymod_symop_access(int unit, bcma_cli_args_t *a, uint8 lport)
{
    phymod_symbols_iter_t iter;
    phymod_symbols_t *symbols;
    phymod_phy_access_t pm_acc;
    int rv = 0;
    char hdr[40];

    if (phymod_symop_init(&iter, a) < 0) {
        return rv;
    }

    if (phymod_sym_info(unit, lport, &iter, &pm_acc) < 0) {
        return rv;
    }

    if (phymod_symop_table_get(&pm_acc, &symbols) < 0) {
        return rv;
    }

    rv = sal_snprintf(hdr, sizeof(hdr), "Logical port %d  register value\n", lport);
    if (rv >= sizeof(hdr)) {
        return rv;
    }

    if (phymod_symop_exec(&iter, symbols, &pm_acc, hdr) < 0) {
        return rv;
    }

    return phymod_symop_cleanup(&iter);
}

#else
typedef int phymod_sym_access_not_empty; /* Make ISO compilers happy. */
#endif /* defined(PHYMOD_SUPPORT) */

#endif
