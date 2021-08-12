/*! \file bcma_clicmd_setget.c
 *
 * CLI 'set' and 'get' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#include "error.h"

#ifdef CFG_SDKCLI_INCLUDED

#include <appl/sdkcli/bcma_cli.h>
#include <appl/sdkcli/bcma_cli_token.h>
#include <appl/sdkcli/bcma_cli_parse.h>
#include <utils/sym/util_symbol_blk.h>
#include <utils/sym/util_symbols_iter.h>
#include <utils/shr/shr_util.h>
#include <utils/ui.h>

/* basic access function for reg/mem/cmic */
static int
clicmd_reg_access(int unit, uint32_t block, uint32_t addr, uint32_t wsize,
                  uint32_t *rdata, uint32_t *wdata, uint32_t *wmask)
{
    soc_switch_t *soc = board_get_soc_by_unit(unit);
    int i;

    if (!soc || wsize > SYMBOL_MAX_WSIZE) {
        return BCMA_CLI_CMD_FAIL;
    }

    if (wsize > 1) {
        if (!soc->xgs_switch_reg64_set || !soc->xgs_switch_reg64_get) {
            return BCMA_CLI_CMD_FAIL;
        }

        if (wdata) {
            uint32_t *wdata_ptr = NULL;
            if (wmask) {
                uint32_t wdata_buf[SYMBOL_MAX_WSIZE] = {0};
                SOC_IF_ERROR_RETURN
                    (soc->xgs_switch_reg64_get(unit, block, addr,
                                               wdata_buf, wsize));
                for (i = 0; i < wsize; i++) {
                    wdata_buf[i] &= ~wmask[i];
                    wdata_buf[i] |= (wdata[i] & wmask[i]);
                }
                wdata_ptr = wdata_buf;
            } else {
                wdata_ptr = wdata;
            }
            SOC_IF_ERROR_RETURN
                (soc->xgs_switch_reg64_set(unit, block, addr, wdata_ptr, wsize));
        }

        if (rdata) {
            SOC_IF_ERROR_RETURN
                (soc->xgs_switch_reg64_get(unit, block, addr, rdata, wsize));
        }
    } else {
        if (!soc->xgs_switch_reg_set || !soc->xgs_switch_reg_get) {
            return BCMA_CLI_CMD_FAIL;
        }

        if (wdata) {
            uint32_t *wdata_ptr = NULL;
            if (wmask) {
                uint32_t wdata_buf = 0;
                SOC_IF_ERROR_RETURN
                    (soc->xgs_switch_reg_get(unit, block, addr, &wdata_buf));
                wdata_buf &= ~wmask[0];
                wdata_buf |= (wdata[0] & wmask[0]);
                wdata_ptr = &wdata_buf;
            } else {
                wdata_ptr = &wdata[0];
            }
            SOC_IF_ERROR_RETURN
                (soc->xgs_switch_reg_set(unit, block, addr, *wdata_ptr));
        }

        if (rdata) {
            SOC_IF_ERROR_RETURN
                (soc->xgs_switch_reg_get(unit, block, addr, rdata));
        }
    }
    return BCMA_CLI_CMD_OK;
}

static int
clicmd_mem_access(int unit, uint32_t block, uint32_t addr, uint32_t wsize,
                  uint32_t *rdata, uint32_t *wdata, uint32_t *wmask)
{
    soc_switch_t *soc = board_get_soc_by_unit(unit);
    int i;

    if (!soc || wsize > SYMBOL_MAX_WSIZE) {
        return BCMA_CLI_CMD_FAIL;
    }

    if (!soc->xgs_switch_mem_set || !soc->xgs_switch_mem_get) {
        return BCMA_CLI_CMD_FAIL;
    }

    if (wdata) {
        uint32_t *wdata_ptr = NULL;
        if (wmask) {
            uint32_t wdata_buf[SYMBOL_MAX_WSIZE] = {0};
            SOC_IF_ERROR_RETURN
                (soc->xgs_switch_mem_get(unit, block, addr, wdata_buf, wsize));
            for (i = 0; i < wsize; i++) {
                wdata_buf[i] &= ~wmask[i];
                wdata_buf[i] |= (wdata[i] & wmask[i]);
            }
            wdata_ptr = wdata_buf;
        } else {
            wdata_ptr = wdata;
        }
        SOC_IF_ERROR_RETURN
            (soc->xgs_switch_mem_set(unit, block, addr, wdata_ptr, wsize));
    }

    if (rdata) {
        SOC_IF_ERROR_RETURN
            (soc->xgs_switch_mem_get(unit, block, addr, rdata, wsize));
    }
    return BCMA_CLI_CMD_OK;
}

#ifdef CFG_CHIP_SYMBOLS_INCLUDED
#ifdef CFG_CHIP_SYMBOLS_FIELD_INCLUDED
static int
clicmd_tcam_access(int unit, uint32_t block, uint32_t addr, uint32_t wsize,
                   uint32 key_offset, uint32 mask_offest, uint32 key_len,
                   uint32_t *rdata, uint32_t *wdata, uint32_t *wmask)
{
    soc_switch_t *soc = board_get_soc_by_unit(unit);
    int i;

    if (!soc || wsize > SYMBOL_MAX_WSIZE) {
        return BCMA_CLI_CMD_FAIL;
    }

    if (!soc->xgs_switch_tcam_mem_set || !soc->xgs_switch_tcam_mem_get) {
        return BCMA_CLI_CMD_FAIL;
    }

    if (wdata) {
        uint32_t *wdata_ptr = NULL;
        if (wmask) {
            uint32_t wdata_buf[SYMBOL_MAX_WSIZE] = {0};
            SOC_IF_ERROR_RETURN
                (soc->xgs_switch_tcam_mem_get(unit, block, addr, wdata_buf, wsize,
                                              key_offset, mask_offest, key_len));
            for (i = 0; i < wsize; i++) {
                wdata_buf[i] &= ~wmask[i];
                wdata_buf[i] |= (wdata[i] & wmask[i]);
            }
            wdata_ptr = wdata_buf;
        } else {
            wdata_ptr = wdata;
        }
        SOC_IF_ERROR_RETURN
            (soc->xgs_switch_tcam_mem_set(unit, block, addr, wdata_ptr, wsize,
                                          key_offset, mask_offest, key_len));
    }

    if (rdata) {
        SOC_IF_ERROR_RETURN
            (soc->xgs_switch_tcam_mem_get(unit, block, addr, rdata, wsize,
                                          key_offset, mask_offest, key_len));
    }
    return BCMA_CLI_CMD_OK;
}
#endif /* CFG_CHIP_SYMBOLS_FIELD_INCLUDED */
#endif /* CFG_CHIP_SYMBOLS_INCLUDED */

static int
clicmd_cmic_access(int unit, uint32_t addr, uint32_t wsize,
                   uint32_t *rdata, uint32_t *wdata, uint32_t *wmask)
{
    soc_switch_t *soc = board_get_soc_by_unit(unit);
    int i;

    if (!soc || wsize > SYMBOL_MAX_WSIZE) {
        return BCMA_CLI_CMD_FAIL;
    }

    if (!soc->xgs_switch_write || !soc->xgs_switch_read) {
        return BCMA_CLI_CMD_FAIL;
    }

    if (addr % 4 != 0) {
        sal_printf(" Error, address 0x%08x is not word-alignment.\n", addr);
        return BCMA_CLI_CMD_FAIL;
    }

    if (wdata) {
        uint32_t *wdata_ptr = NULL;
        if (wmask) {
            uint32_t wdata_buf[SYMBOL_MAX_WSIZE] = {0};
            for (i = 0; i < wsize; i++) {
                SOC_IF_ERROR_RETURN
                    (soc->xgs_switch_read(unit, addr + (i * 4), &(wdata_buf[i])));
                wdata_buf[i] &= ~wmask[i];
                wdata_buf[i] |= (wdata[i] & wmask[i]);
            }
            wdata_ptr = wdata_buf;
        } else {
            wdata_ptr = wdata;
        }
        for (i = 0; i < wsize; i++) {
            SOC_IF_ERROR_RETURN
                (soc->xgs_switch_write(unit, addr + (i * 4), wdata_ptr[i]));
        }
    }

    if (rdata) {
        for (i = 0; i < wsize; i++) {
            SOC_IF_ERROR_RETURN
                (soc->xgs_switch_read(unit, addr + (i * 4), &(rdata[i])));
        }
    }
    return BCMA_CLI_CMD_OK;
}

/* parse one single uint32 value */
static int
clicmd_parse_uint32(bcma_cli_args_t *args, const char *desc, uint32_t *ret_val)
{
    char *str;

    if (!args || !ret_val) {
        return BCMA_CLI_CMD_FAIL;
    }

    str = BCMA_CLI_ARG_GET(args);
    if (bcma_cli_parse_uint32(str, ret_val) < 0) {
        return bcma_cli_parse_error(desc , str);
    }

    return BCMA_CLI_CMD_OK;
}

/* parse the array of uint32 value */
static int
clicmd_parse_uint32_arr(bcma_cli_args_t *args, const char *desc,
                        uint32_t *arr, uint32_t arr_size, uint32_t *ret_act_cnt)
{
    int cnt = 0, left_arg_num;
    char *str;

    if (!args || !arr) {
        return BCMA_CLI_CMD_FAIL;
    }

    left_arg_num = BCMA_CLI_ARG_CNT(args);
    for (cnt = 0; cnt < left_arg_num; cnt++) {
        if (cnt == arr_size) {
            break;
        }
        str = BCMA_CLI_ARG_GET(args);
        if (bcma_cli_parse_uint32(str, &arr[cnt]) < 0) {
            return bcma_cli_parse_error(desc, str);
        }
    }

    if (ret_act_cnt != NULL) {
        *ret_act_cnt = cnt;
    }
    return BCMA_CLI_CMD_OK;
}

#ifdef CFG_CHIP_SYMBOLS_INCLUDED
/* the range information of symbol */
typedef struct clicmd_sid_s {
    int index_start;
    int index_end;
    int block_start;
    int block_end;
    int port_start;
    int port_end;
} clicmd_sid_t;

/* the userdata in symbol iteration */
#define CLICMD_SYMBOL_ITER_FLAG_NONZERO     0x00000001
#ifdef CFG_CHIP_SYMBOLS_FIELD_INCLUDED
#define CLICMD_SYMBOL_ITER_FLAG_SINGLELINE  0x00000002
#define CLICMD_SYMBOL_ITER_FLAG_RAWDATA     0x00000004
#endif /* CFG_CHIP_SYMBOLS_FIELD_INCLUDED */

/* ctrl-c detection every 10 entries */
#define CLICMD_SYMBOL_ITER_CHECK_CTRLC      (10)

typedef struct clicmd_symbol_iter_cb_s {
    int unit;
    clicmd_sid_t sid;
    bcma_cli_args_t *write_data_args;
    uint32_t flags;
    uint32_t touched_entry;
} clicmd_symbol_iter_cb_t;

static void
clicmd_dump_symbol(int unit, const symbol_t *symbol, int i, int b, int p)
{
    int maxidx = SYMBOL_INDEX_MAX_GET(symbol->index);

    /* format: symbol_name[i].blktype_[b].[p] */
    sal_printf("%s", symbol->name);
    if (maxidx != 0) {
        sal_printf("[%d]", i);
    }
    if (b != -1) {
        sal_printf(".%s%d",
                   util_blktype_name_get(unit, symbol),
                   util_blktype_inst_get(unit, symbol, b));
    }
    if (p != -1) {
        sal_printf(".%d", p);
    }
}

#ifdef CFG_CHIP_SYMBOLS_FIELD_INCLUDED
static int
clicmd_symbol_get_field(chip_info_t *chip_info, const symbol_t *symbol,
                        const char *field_name, symbol_field_info_t *ret_finfo)
{
    const char *pch = NULL;
    const char *fname = NULL;

    if (!chip_info || !symbol || !ret_finfo || !field_name) {
        return BCMA_CLI_CMD_FAIL;
    }

    SYMBOL_FIELDS_ITER_BEGIN(symbol->fields, *ret_finfo,
                             chip_info->symbols->field_names) {
        if (ret_finfo->name == NULL) {
            continue;
        }
        /*
         * strip leading hidden information in the string
         *  ex. {:KEY_TYPE:0}L2:VLAN_ID
         */
        pch = sal_strchr(ret_finfo->name, '}');
        if (pch == NULL) {
            fname = ret_finfo->name;
        } else {
            fname = pch + 1;
        }
        if (sal_strcmp(fname, field_name) == 0) {
            return BCMA_CLI_CMD_OK;
        }
    } SYMBOL_FIELDS_ITER_END();

    sal_printf("ERROR: cannot find the field %s in symbol %s\n",
               field_name, symbol->name);
    return BCMA_CLI_CMD_FAIL;
}

static int
clicmd_symbol_dump_fields(chip_info_t *chip_info, const symbol_t *symbol,
                          uint32_t flags, uint32_t *rdata)
{
    symbol_field_info_t finfo;
    symbol_field_info_t finfo_keytype;
    int idx = 0;
    char *indent;
    char *delimiter;
    uint32_t val[SYMBOL_MAX_WSIZE] = {0};
    int val_wsize, i;
    const char *display_name = NULL;
    bool first_field = true;

    if (!chip_info || !symbol || !rdata) {
        return BCMA_CLI_CMD_FAIL;
    }

    sal_memset(&finfo, 0, sizeof(symbol_field_info_t));
    sal_memset(&finfo_keytype, 0, sizeof(symbol_field_info_t));

    /* Indentation before each field */
    indent = (flags & CLICMD_SYMBOL_ITER_FLAG_SINGLELINE) ? "" : "    ";

    /* Delimiter between each field */
    delimiter = (flags & CLICMD_SYMBOL_ITER_FLAG_SINGLELINE) ? ", " : "\n";

    /* Iterate each field */
    SYMBOL_FIELDS_ITER_BEGIN(symbol->fields, finfo,
                             chip_info->symbols->field_names) {

        /* default field name */
        display_name = finfo.name;

        /* skip if all zero */
        SOC_IF_ERROR_RETURN
            (symbol_field_value_get(rdata, &finfo, val, SYMBOL_MAX_WSIZE));

        val_wsize = ((finfo.maxbit - finfo.minbit) / 32) + 1;

        if (flags & CLICMD_SYMBOL_ITER_FLAG_NONZERO) {
            bool all_zero = true;
            for (i = 0; i < val_wsize; i++) {
                if (val[i] != 0) {
                    all_zero = false;
                }
            }
            if (all_zero == true) {
                continue;
            }
        }

        /* skip inactive view */
        if (finfo.name && sal_strstr(finfo.name, "{:KEY_TYPE:") == finfo.name) {

            uint32_t key_type_act;
            const char *pos0, *pos1;
            char strbuf[10] = {'\0'};
            int strlen;
            bcma_cli_tokens_t tokens;
            bool match = false;

            /* get the actual keytype in rdata */
            if (finfo_keytype.name == NULL) {
                SOC_IF_ERROR_RETURN
                    (clicmd_symbol_get_field(chip_info, symbol, "KEY_TYPE",
                                             &finfo_keytype));
            }
            SOC_IF_ERROR_RETURN
                (symbol_field_value_get32(rdata, &finfo_keytype, &key_type_act));

            /*
             * get keytype in this field
             *   ex.
             *     {:KEY_TYPE:0}L2:VLAN_ID
             *     {:KEY_TYPE:1|2}VLAN:CLASS_ID
             */
            pos0 = finfo.name + sal_strlen("{:KEY_TYPE:");
            pos1 = sal_strchr(finfo.name, '}') - 1;
            strlen = pos1 - pos0 + 1;
            if (strlen < 1 || strlen >= sizeof(strbuf)) {
                return BCMA_CLI_CMD_FAIL;
            }
            sal_strncpy(strbuf, pos0, strlen);
            strbuf[strlen] = '\0';

            SOC_IF_ERROR_RETURN
                (bcma_cli_tokenize(strbuf, &tokens, "|"));

            for (i = 0; i < tokens.argc; i++) {
                uint32_t key_type_fld;

                SOC_IF_ERROR_RETURN
                    (bcma_cli_parse_uint32(tokens.argv[i], &key_type_fld));

                if (key_type_fld == key_type_act) {
                    match = true;
                    break;
                }
            }

            if (match == false) {
                continue;
            } else {
                /* strip leading KEY_TYPE info */
                display_name = sal_strchr(finfo.name, '}') + 1;
            }
        }

        /* Add the delimiter between this and the previous field */
        if (first_field == true) {
            first_field = false;
        } else {
            sal_printf("%s", delimiter);
        }

        /* dump field name */
        if (display_name) {
            sal_printf("%s%s", indent, display_name);
        } else {
            sal_printf("%sfield%d", indent, idx++);
        }

        /*
         * Values less thans 10 in decimal only,
         * anything else is shown as a hex-string.
         */
        if (val_wsize == 1 && val[0] < 10) {
            sal_printf("=%d", val[0]);
        } else {
            for (i = val_wsize - 1; i >= 0; i--) {
                if (i == val_wsize - 1) {
                    sal_printf("=0x%x", val[i]);
                } else {
                    sal_printf("_%08x", val[i]);
                }
            }
        }
    } SYMBOL_FIELDS_ITER_END();
    return BCMA_CLI_CMD_OK;
}
#endif /* CFG_CHIP_SYMBOLS_FIELD_INCLUDED */

static int
clicmd_symbol_iter_cb(const symbol_t *symbol, void *vptr)
{
    clicmd_symbol_iter_cb_t *symbol_cb = (clicmd_symbol_iter_cb_t *)vptr;
    chip_info_t *chip_info;
    int unit, maxidx, i, b, p;
    bool is_cmic_iproc;
    clicmd_sid_t sid;
    const char *blktype_name;
    bool is_write = false; /* true  : write operation,
                            * false : read operation */
    bool is_write_w_mask = false; /* true  : write data with mask,
                                   * false : write raw data */
    uint32_t wdata[SYMBOL_MAX_WSIZE] = {0};
    uint32_t wmask[SYMBOL_MAX_WSIZE] = {0};

    if (symbol == NULL || symbol_cb == NULL) {
        return BCMA_CLI_CMD_FAIL;
    }

    unit = symbol_cb->unit;
    sid = symbol_cb->sid;

    chip_info = board_get_chipinfo_by_unit(unit);
    if (!chip_info) {
        return BCMA_CLI_CMD_FAIL;
    }

    /* Is it cmic or iproc */
    blktype_name = util_blktype_name_get(unit, symbol);
    if (blktype_name == NULL) {
        sal_printf("ERROR: cannot find block name for symbol %s\n", symbol->name);
        return BCMA_CLI_CMD_FAIL;
    }
    is_cmic_iproc = ((sal_strcmp(blktype_name, "cmic") == 0) ||
                     (sal_strcmp(blktype_name, "iproc") == 0))? true: false;

    /* No port/block identifiers on cmic/iproc registers */
    if (is_cmic_iproc == true) {
        if (sid.block_start != -1 || sid.block_end != -1 ||
            sid.port_start != -1 || sid.port_end != -1) {
            sal_printf("ERROR: This symbol %s does not support block [%d, %d]"
                       "and port [%d, %d] identifiers\n",
                       symbol->name, sid.block_start, sid.block_end,
                       sid.port_start, sid.port_end);
            return BCMA_CLI_CMD_FAIL;
        }
    }

    /* No port identifiers without SYMBOL_FLAG_PORT */
    if (!(symbol->flags & SYMBOL_FLAG_PORT)) {
        if (sid.port_start != -1 || sid.port_end != -1) {
            sal_printf("ERROR: This symbol %s does not support port [%d, %d] identifier.\n",
                       symbol->name, sid.port_start, sid.port_end);
            return BCMA_CLI_CMD_FAIL;
        }
    }

    /* extend sid if the user does not specify */
    maxidx = SYMBOL_INDEX_MAX_GET(symbol->index);
    if (sid.index_start < 0 || sid.index_end < 0) {
        sid.index_start = 0;
        sid.index_end = maxidx;
    } else if (sid.index_start < 0) {
        sal_printf("ERROR: invalid index_start %d, min is 0\n", sid.index_start);
        return BCMA_CLI_CMD_FAIL;
    } else if (sid.index_end > maxidx) {
        sal_printf("ERROR: invalid index_end %d, max is %d\n", sid.index_end, maxidx);
        return BCMA_CLI_CMD_FAIL;
    }

    if (is_cmic_iproc == false && (sid.block_start < 0 || sid.block_end < 0)) {
        sid.block_start = -1;
        sid.block_end = -1;

        /* get the range of the same blocktype */
        SOC_IF_ERROR_RETURN
            (util_blk_range_get(unit, blktype_name, -1,
                                &sid.block_start, &sid.block_end));
    }

    if ((symbol->flags & SYMBOL_FLAG_PORT) && (is_cmic_iproc == false) &&
        (sid.port_start < 0 || sid.port_end < 0)) {
        sid.port_start = 0;
        sid.port_end = SOC_MAX_NUM_PORTS - 1;
    }

    /* parse data for write operation */
    if (symbol_cb->write_data_args != NULL) {
        bcma_cli_args_t *args = symbol_cb->write_data_args;
        char *str = BCMA_CLI_ARG_CUR(args);
        int cnt;

        if (str == NULL) {
            return bcma_cli_parse_error("data", NULL);
        }

        is_write = true;
        if (sal_strstr(str, "all=") == str) {
            /* All 32-bit data words are assigned with the same value */
            uint32_t val;
            str += 4;
            if (bcma_cli_parse_uint32(str, &val) < 0) {
                return bcma_cli_parse_error("data", str);
            }
            for (cnt = 0; cnt < SYMBOL_MAX_WSIZE; cnt++) {
                wdata[cnt] = val;
            }
            BCMA_CLI_ARG_NEXT(args);
        } else if (bcma_cli_parse_is_int(str) == TRUE) {
            /* All tokens are treated as 32-bit data words */
            SOC_IF_ERROR_RETURN
                (clicmd_parse_uint32_arr(args, "data", wdata,
                                         SYMBOL_MAX_WSIZE, NULL));
#ifdef CFG_CHIP_SYMBOLS_FIELD_INCLUDED
        } else {
            /* All tokens are treated as field=value */
            uint32_t val_allone[SYMBOL_MAX_WSIZE];

            sal_memset(val_allone, 0xff, sizeof(uint32_t) * SYMBOL_MAX_WSIZE);
            is_write_w_mask = true;

            while ((str = BCMA_CLI_ARG_GET(args)) != NULL) {
                bcma_cli_tokens_t tokens;
                symbol_field_info_t finfo;
                uint32_t fval[SYMBOL_MAX_WSIZE];
                int fval_wsize;

                SOC_IF_ERROR_RETURN
                    (bcma_cli_tokenize(str, &tokens, "="));

                if (tokens.argc != 2) {
                    return bcma_cli_parse_error("field data", str);
                }

                SOC_IF_ERROR_RETURN
                    (clicmd_symbol_get_field(chip_info, symbol,
                                             tokens.argv[0], &finfo));

                fval_wsize = bcma_cli_parse_array_uint32
                                (tokens.argv[1], SYMBOL_MAX_WSIZE, fval);
                if (fval_wsize < 0 || fval_wsize > SYMBOL_MAX_WSIZE) {
                    return bcma_cli_parse_error("field data", tokens.argv[1]);
                }

                SOC_IF_ERROR_RETURN
                    (symbol_field_value_set(wdata, &finfo,
                                            fval, SYMBOL_MAX_WSIZE));
                SOC_IF_ERROR_RETURN
                    (symbol_field_value_set(wmask, &finfo,
                                            val_allone, SYMBOL_MAX_WSIZE));
            }
        }
#else
        } else {
            /* unrecognized input */
            return bcma_cli_parse_error("data", str);
        }
#endif /* CFG_CHIP_SYMBOLS_FIELD_INCLUDED */
    }

    /* start to traverse each sid in the range */
    for (i = sid.index_start; i <= sid.index_end; i++) {
        for (b = sid.block_start; b <= sid.block_end; b++) {

            const chip_block_t *block = NULL;

            /* skip invalid block id */
            if (b != -1) {
                block = util_blk_get(unit, b);
                if (block == NULL) {
                    continue;
                }
                if (!((1 << block->type) &
                      SYMBOL_FLAG_BLKTYPES(symbol->flags))) {
                    continue;
                }
            }

            for (p = sid.port_start; p <= sid.port_end; p++) {

                uint32_t addr;
                uint32_t wsize;
                uint32_t rdata[SYMBOL_MAX_WSIZE] = {0};
                uint32_t rdata_idx;
                int step = SYMBOL_INDEX_STEP_GET(symbol->index);

                if (p != -1) {
                    /*
                     * find the first physical port in the port block,
                     * skip the invalid lane index (started from zero in the block)
                     */
                    int p_first = -1, p_iter;
                    PBMP_ITER(block->pbmps, p_iter) {
                        p_first = p_iter;
                        break;
                    }
                    if (p_first == -1) {
                        sal_printf("ERROR: pbmp is empty in block %d\n", b);
                        return BCMA_CLI_CMD_FAIL;
                    }
                    if (p_first + p >= SOC_MAX_NUM_PORTS ||
                        !PBMP_MEMBER(block->pbmps, p_first + p)) {
                        continue;
                    }
                }

                /*
                 *  finally, we got a valid "i, b, p" what we want here,
                 *  then calculate the address
                 */
                if (is_cmic_iproc == true) {
                    if (step == 1) {
                        addr = symbol->addr + (i * 4);
                    } else {
                        addr = symbol->addr + (i * step);
                    }
                }else {
                    addr = chip_info->block_port_addr(b, p, symbol->addr, i * step);
                }

                /* access symbol with block, addr */
                if (is_cmic_iproc == true) {
                    wsize = 1;
                    SOC_IF_ERROR_RETURN
                        (clicmd_cmic_access(unit, addr, wsize,
                                            is_write == false? rdata: NULL,
                                            is_write == true? wdata: NULL,
                                            is_write_w_mask == true? wmask: NULL));
                } else if (symbol->flags & SYMBOL_FLAG_REGISTER) {
                    wsize = (symbol->flags & SYMBOL_FLAG_R64)? 2 : 1;
                    SOC_IF_ERROR_RETURN
                        (clicmd_reg_access(unit, b, addr, wsize,
                                           is_write == false? rdata: NULL,
                                           is_write == true? wdata: NULL,
                                           is_write_w_mask == true? wmask: NULL));
                } else if (symbol->flags & SYMBOL_FLAG_MEMORY) {
                    wsize = SHR_BYTES2WORDS
                                (SYMBOL_INDEX_SIZE_GET(symbol->index));
                    if (wsize == 0 || wsize > SYMBOL_MAX_WSIZE) {
                        sal_printf("ERROR: invalid entry size (%d words), max is %d\n",
                                   wsize, SYMBOL_MAX_WSIZE);
                        return BCMA_CLI_CMD_FAIL;
                    }
                    #ifdef CFG_CHIP_SYMBOLS_FIELD_INCLUDED
                    if (symbol->flags & SYMBOL_FLAG_TCAM) {
                        uint32 key_offset, mask_offest, key_len;
                        symbol_field_info_t finfo_key, finfo_mask;
                        SOC_IF_ERROR_RETURN
                            (clicmd_symbol_get_field(chip_info, symbol,
                                                     "KEY", &finfo_key));
                        SOC_IF_ERROR_RETURN
                            (clicmd_symbol_get_field(chip_info, symbol,
                                                     "MASK", &finfo_mask));
                        key_offset = finfo_key.minbit;
                        mask_offest = finfo_mask.minbit;
                        key_len = finfo_key.maxbit - finfo_key.minbit + 1;
                        SOC_IF_ERROR_RETURN
                            (clicmd_tcam_access(unit, b, addr, wsize,
                                                key_offset, mask_offest, key_len,
                                                is_write == false? rdata: NULL,
                                                is_write == true? wdata: NULL,
                                                is_write_w_mask == true? wmask: NULL));
                    } else
                    #endif /* CFG_CHIP_SYMBOLS_FIELD_INCLUDED */
                    {
                        SOC_IF_ERROR_RETURN
                            (clicmd_mem_access(unit, b, addr, wsize,
                                               is_write == false? rdata: NULL,
                                               is_write == true? wdata: NULL,
                                               is_write_w_mask == true? wmask: NULL));
                    }
                } else {
                    sal_printf("ERROR: no any handling functions\n", b);
                    return BCMA_CLI_CMD_FAIL;
                }

                /* Keep the watchdog timer alive */
                board_wdt_ping();


                /* skip dump this symbol if write operation */
                if (is_write == true) {
                    continue;
                }

                /* skip dump this symbol if all zero */
                if (symbol_cb->flags & CLICMD_SYMBOL_ITER_FLAG_NONZERO) {
                    bool all_zero = true;
                    for (rdata_idx = 0; rdata_idx < wsize; rdata_idx++) {
                        if (rdata[rdata_idx] !=0 ) {
                            all_zero = false;
                        }
                    }
                    if (all_zero == true) {
                        continue;
                    }
                }

                /* dump */
                sal_printf("symbol ");
                clicmd_dump_symbol(unit, symbol, i, b, p);
                if (is_cmic_iproc == true) {
                    sal_printf(" (%s, ", blktype_name);
                } else {
                    sal_printf(" (Block %2d, ", b);
                }
                sal_printf("0x%08x)", addr);
                if (p != -1) {
                    sal_printf("{port %d}", p);
                }
                sal_printf(" =");
                for (rdata_idx = 0; rdata_idx < wsize; rdata_idx++) {
                    sal_printf(" 0x%08x", rdata[rdata_idx]);
                }
#ifdef CFG_CHIP_SYMBOLS_FIELD_INCLUDED
                if (!(symbol_cb->flags & CLICMD_SYMBOL_ITER_FLAG_RAWDATA)) {
                    if (symbol_cb->flags & CLICMD_SYMBOL_ITER_FLAG_SINGLELINE) {
                        sal_printf(", ");
                    } else {
                        sal_printf("\n");
                    }
                    SOC_IF_ERROR_RETURN
                        (clicmd_symbol_dump_fields(chip_info, symbol,
                                                   symbol_cb->flags, rdata));
                }
#endif /* CFG_CHIP_SYMBOLS_FIELD_INCLUDED */
                sal_printf("\n");

                /* ctrl-c detection */
                symbol_cb->touched_entry++;
                if (symbol_cb->touched_entry == CLICMD_SYMBOL_ITER_CHECK_CTRLC) {
                    char input_char;

                    symbol_cb->touched_entry = 0;
                    if (sal_getchar_nonblock(&input_char) == 0 &&
                        input_char == UI_KB_CTRL_C) {
                        return BCMA_CLI_CMD_INTR;
                    }
                }
                sal_usleep(10000);
            }
        }
    }
    return BCMA_CLI_CMD_OK;
}

/* format : symbol_name[idx].blktype_instid.port */
static int
clicmd_parse_sid(int unit, const char *str,
                 char *ret_name, clicmd_sid_t *ret_sid)
{
    char *pos0, *pos1, *pos2;
    bcma_cli_tokens_t tokens;

    if (str == NULL || ret_name ==NULL || ret_sid == NULL) {
        return BCMA_CLI_CMD_FAIL;
    }

    ret_sid->index_start = -1;
    ret_sid->index_end = -1;
    ret_sid->block_start = -1;
    ret_sid->block_end = -1;
    ret_sid->port_start = -1;
    ret_sid->port_end = -1;

    if (bcma_cli_tokenize(str, &tokens, ".") < 0) {
        return BCMA_CLI_CMD_FAIL;
    }

    if (tokens.argc < 1 || tokens.argc > 3) {
        return bcma_cli_parse_error("sid", str);
    }

    /*
     * tokens.argv[0]
     *   symbol_name
     *   symbol_name[idx]
     */
    pos0 = tokens.argv[0];
    pos1 = sal_strchr(pos0, '[');
    pos2 = sal_strchr(pos0, ']');
    if (pos1) {
        bcma_cli_tokens_t sub_tokens;
        uint32_t index;
        if (!pos2) {
            return bcma_cli_parse_error("symbol", tokens.argv[0]);
        }
        if ((pos2 - pos0) < (pos1 - pos0)) {
            return bcma_cli_parse_error("symbol", tokens.argv[0]);
        }
        if (bcma_cli_tokenize(pos0, &sub_tokens, "[]") < 0) {
            return bcma_cli_parse_error("symbol", tokens.argv[0]);
        }
        if (sub_tokens.argc != 2) {
            return bcma_cli_parse_error("symbol", tokens.argv[0]);
        }
        if (bcma_cli_parse_uint32(sub_tokens.argv[1], &index)) {
            return bcma_cli_parse_error("symbol", tokens.argv[0]);
        }
        ret_sid->index_start = index;
        ret_sid->index_end = index;
        sal_strlcpy(ret_name, sub_tokens.argv[0], SYMBOL_NAME_MAXLEN);
    } else {
        sal_strlcpy(ret_name, tokens.argv[0], SYMBOL_NAME_MAXLEN);
    }

    if (tokens.argc >= 2) {
        pos0 = tokens.argv[1];
        if (sal_strcmp(pos0, "uport") == 0) {
            /*
             * tokens.argv[1] & tokens.argv[2]
             *   uport.port (ex. uport.30)
             */
            uint32_t uport;
            uint8 lport, port_unit;
            if (tokens.argc != 3) {
                return bcma_cli_parse_error("port", NULL);
            }
            if (bcma_cli_parse_uint32(tokens.argv[2], &uport) < 0) {
                return bcma_cli_parse_error("port", tokens.argv[2]);
            }
            if (board_uport_to_lport((uint16)uport,
                                     &port_unit, &lport) < 0) {
                return bcma_cli_parse_error("port", tokens.argv[2]);
            }
            ret_sid->block_start = SOC_PORT_BLOCK(lport);
            ret_sid->block_end = SOC_PORT_BLOCK(lport);
            ret_sid->port_start = SOC_PORT_BLOCK_INDEX(lport);
            ret_sid->port_end = SOC_PORT_BLOCK_INDEX(lport);
        } else {
            /*
             * tokens.argv[1]
             *   blktype        (ex. xlport)
             *   blktype+instid (ex. xlport1)
             */
            pos1 = pos0; /* find the first digit in the string */
            while (*pos1) {
                if (sal_isdigit(*pos1)) {
                    break;
                }
                pos1++;
            }
            if (*pos1 == '\0') {
                /* no block insid */
                if (util_blk_range_get(unit, pos0, -1,
                                       &ret_sid->block_start,
                                       &ret_sid->block_end) < 0) {
                    return bcma_cli_parse_error("block", tokens.argv[1]);
                }
            } else {
                int blktype_strlen = pos1 - pos0;
                uint32_t blkinst;
                char blktype_str[BCMA_CLI_CONFIG_ARGS_CNT_MAX];

                if (blktype_strlen >= BCMA_CLI_CONFIG_ARGS_CNT_MAX) {
                    return bcma_cli_parse_error("block", tokens.argv[1]);
                }
                sal_strncpy(blktype_str, pos0, blktype_strlen);
                blktype_str[blktype_strlen] = '\0';
                if (bcma_cli_parse_uint32(pos1, &blkinst) < 0) {
                    return bcma_cli_parse_error("block", tokens.argv[1]);
                }
                if (util_blk_range_get(unit, blktype_str, blkinst,
                                       &ret_sid->block_start, &ret_sid->block_end) < 0) {
                    return bcma_cli_parse_error("block", tokens.argv[1]);
                }
            }
            /*
             * tokens.argv[2]
             *    port
             */
            if (tokens.argc >= 3) {
                uint32_t port;
                if (bcma_cli_parse_uint32(tokens.argv[2], &port) < 0) {
                    return bcma_cli_parse_error("port", tokens.argv[2]);
                }
                ret_sid->port_start = port;
                ret_sid->port_end = port;
            }
        }
    }



    return BCMA_CLI_CMD_OK;
}
#endif /* CFG_CHIP_SYMBOLS_INCLUDED */

static int
clicmd_get_raw(int unit, const char* str, bcma_cli_args_t *args)
{
    uint32_t block, addr, wsize = 1, rdata_idx;
    uint32_t rdata[SYMBOL_MAX_WSIZE] = {0};

    if (!args) {
        return BCMA_CLI_CMD_FAIL;
    }

    if (sal_strcmp(str, "reg") == 0 || sal_strcmp(str, "mem") == 0) {
        SOC_IF_ERROR_RETURN
            (clicmd_parse_uint32(args, "block", &block));
        SOC_IF_ERROR_RETURN
            (clicmd_parse_uint32(args, "addr", &addr));
    } else if (bcma_cli_parse_is_int(str) == TRUE) {
        if (bcma_cli_parse_uint32(str, &addr) < 0) {
            return bcma_cli_parse_error("addr", str);
        }
    } else {
        return BCMA_CLI_CMD_FAIL;
    }

    if (BCMA_CLI_ARG_CUR(args) != NULL) {
        SOC_IF_ERROR_RETURN
            (clicmd_parse_uint32(args, "wsize", &wsize));

    }

    if (bcma_cli_parse_is_int(str) == TRUE) {
        int i;
        for (i = 0; i < wsize; i++) {
            if (i % 4 == 0) {
                sal_printf("    [addr 0x%08x] =", addr);
            }
            SOC_IF_ERROR_RETURN
                (clicmd_cmic_access(unit, addr, 1, rdata, NULL, NULL));
            sal_printf(" 0x%08x", rdata[0]);
            if (i % 4 == 3 || i == wsize - 1) {
                sal_printf("\n");
            }
            addr += 4;

            /* Keep the watchdog timer alive */
            board_wdt_ping();
        }
    } else {
        if (wsize == 0 || wsize > SYMBOL_MAX_WSIZE) {
            sal_printf("ERROR: invalid wsize %d, min is %d, max is %d\n",
                       wsize, 0, SYMBOL_MAX_WSIZE);
            return BCMA_CLI_CMD_FAIL;
        }
        if (sal_strcmp(str, "reg") == 0) {
            SOC_IF_ERROR_RETURN
                (clicmd_reg_access(unit, block, addr, wsize, rdata, NULL, NULL));
        } else if (sal_strcmp(str, "mem") == 0) {
            SOC_IF_ERROR_RETURN
                (clicmd_mem_access(unit, block, addr, wsize, rdata, NULL, NULL));
        } else {
            return BCMA_CLI_CMD_FAIL;
        }
        sal_printf("    %s [block %2d, addr 0x%08x] =", str, block, addr);
        for (rdata_idx = 0; rdata_idx < wsize; rdata_idx++) {
            sal_printf(" 0x%08x", rdata[rdata_idx]);
        }
        sal_printf("\n");
    }

    return BCMA_CLI_CMD_OK;
}

static int
clicmd_set_raw(int unit, const char* str, bcma_cli_args_t *args)
{
    uint32_t block, addr, wsize;
    uint32_t wdata[SYMBOL_MAX_WSIZE] = {0};

    if (!args) {
        return BCMA_CLI_CMD_FAIL;
    }

    if (sal_strcmp(str, "reg") == 0 || sal_strcmp(str, "mem") == 0) {
        SOC_IF_ERROR_RETURN
            (clicmd_parse_uint32(args, "block", &block));
        SOC_IF_ERROR_RETURN
            (clicmd_parse_uint32(args, "addr", &addr));
    } else if (bcma_cli_parse_is_int(str) == TRUE) {
        if (bcma_cli_parse_uint32(str, &addr) < 0) {
            return bcma_cli_parse_error("addr", str);
        }
    } else {
        return BCMA_CLI_CMD_FAIL;
    }

    SOC_IF_ERROR_RETURN
        (clicmd_parse_uint32_arr(args, "value", wdata,
                                 SYMBOL_MAX_WSIZE, &wsize));

    if (wsize == 0) {
        return bcma_cli_parse_error("value" , NULL);
    }

    if (bcma_cli_parse_is_int(str) == TRUE) {
        SOC_IF_ERROR_RETURN
            (clicmd_cmic_access(unit, addr, wsize, NULL, wdata, NULL));
    } else {
        if (sal_strcmp(str, "reg") == 0) {
            SOC_IF_ERROR_RETURN
                (clicmd_reg_access(unit, block, addr, wsize, NULL, wdata, NULL));
        } else if (sal_strcmp(str, "mem") == 0) {
            SOC_IF_ERROR_RETURN
                (clicmd_mem_access(unit, block, addr, wsize, NULL, wdata, NULL));
        } else {
            return BCMA_CLI_CMD_FAIL;
        }
    }

    return BCMA_CLI_CMD_OK;
}

int
bcma_clicmd_get(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    char *str;
#ifdef CFG_CHIP_SYMBOLS_INCLUDED
    chip_info_t *chip_info;
    util_symbols_iter_t iter;
    clicmd_symbol_iter_cb_t iter_cb;
    char parse_name[SYMBOL_NAME_MAXLEN] = "";
    clicmd_sid_t parse_sid;
    uint32_t parse_flags = 0;
    int rc;
#endif /* CFG_CHIP_SYMBOLS_INCLUDED */

    /* parse the first arg */
    if (BCMA_CLI_ARG_CNT(args) == 0) {
        return BCMA_CLI_CMD_USAGE;
    }

    /* check if raw access */
    str = BCMA_CLI_ARG_GET(args);
    if (sal_strcmp(str, "reg") == 0 || sal_strcmp(str, "mem") == 0 ||
        bcma_cli_parse_is_int(str) == TRUE) {
        return clicmd_get_raw(cli->cur_unit, str, args);
    }

#ifndef CFG_CHIP_SYMBOLS_INCLUDED
    return BCMA_CLI_CMD_NO_SYM;
#else

    /* if not raw access, check options first */
    while (str != NULL) {
        if (bcma_cli_parse_cmp("NonZero", str, 0) == TRUE) {
            parse_flags |= CLICMD_SYMBOL_ITER_FLAG_NONZERO;
            str = BCMA_CLI_ARG_GET(args);
#ifdef CFG_CHIP_SYMBOLS_FIELD_INCLUDED
        } else if (bcma_cli_parse_cmp("CompactFormat", str, 0) == TRUE) {
            parse_flags |= CLICMD_SYMBOL_ITER_FLAG_SINGLELINE;
            str = BCMA_CLI_ARG_GET(args);
        } else if (bcma_cli_parse_cmp("Raw", str, 0) == TRUE) {
            parse_flags |= CLICMD_SYMBOL_ITER_FLAG_RAWDATA;
            str = BCMA_CLI_ARG_GET(args);
#endif /* CFG_CHIP_SYMBOLS_FIELD_INCLUDED */
        } else {
            break;
        }
    }

    /* then treat the next arg as symbol name */
    if (str == NULL) {
        return bcma_cli_parse_error("symbol name", NULL);
    }
    if (sal_strstr(str, "*") != NULL) {
        sal_strlcpy(parse_name, str, SYMBOL_NAME_MAXLEN);
        parse_sid.index_start = -1;
        parse_sid.index_end = -1;
        parse_sid.block_start = -1;
        parse_sid.block_end = -1;
        parse_sid.port_start = -1;
        parse_sid.port_end = -1;
    } else {
        SOC_IF_ERROR_RETURN
            (clicmd_parse_sid(cli->cur_unit, str, parse_name, &parse_sid));
    }

    /* ready to traverse symbol table */
    chip_info = board_get_chipinfo_by_unit(cli->cur_unit);
    if (!chip_info || !chip_info->symbols) {
        return BCMA_CLI_CMD_NO_SYM;
    }

    sal_memset(&iter_cb, 0, sizeof(clicmd_symbol_iter_cb_t));
    iter_cb.unit = cli->cur_unit;
    iter_cb.sid = parse_sid;
    iter_cb.flags = parse_flags;

    sal_memset(&iter, 0, sizeof(iter));
    iter.name = parse_name;
    iter.matching_mode = UTIL_SYMBOLS_ITER_MODE_EXACT;
    iter.symbols = chip_info->symbols;
    iter.function = clicmd_symbol_iter_cb;
    iter.vptr = &iter_cb;

    rc = util_symbols_iter(&iter);
    if (rc == BCMA_CLI_CMD_INTR) {
        sal_printf("Ctrl-C detected, abort the iteration\n");
    } else if (rc < 0) {
        sal_printf("ERROR: something wrong in symbols iteration\n");
        return BCMA_CLI_CMD_FAIL;
    } else if (rc == 0) {
        sal_printf("ERROR: cannot find symbol %s\n", parse_name);
    }
    return BCMA_CLI_CMD_OK;
#endif /* CFG_CHIP_SYMBOLS_INCLUDED */
}

int
bcma_clicmd_set(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    char *str;
#ifdef CFG_CHIP_SYMBOLS_INCLUDED
    chip_info_t *chip_info;
    util_symbols_iter_t iter;
    clicmd_symbol_iter_cb_t iter_cb;
    char parse_name[SYMBOL_NAME_MAXLEN] = "";
    clicmd_sid_t parse_sid;
    int rc;
#endif /* CFG_CHIP_SYMBOLS_INCLUDED */

    /* parse the first arg */
    if (BCMA_CLI_ARG_CNT(args) == 0) {
        return BCMA_CLI_CMD_USAGE;
    }

    /* check if raw access */
    str = BCMA_CLI_ARG_GET(args);
    if (sal_strcmp(str, "reg") == 0 || sal_strcmp(str, "mem") == 0 ||
        bcma_cli_parse_is_int(str) == TRUE) {
        return clicmd_set_raw(cli->cur_unit, str, args);
    }

#ifndef CFG_CHIP_SYMBOLS_INCLUDED
    return BCMA_CLI_CMD_NO_SYM;
#else

    /* if not raw access, treat the first arg as symbol name */
    SOC_IF_ERROR_RETURN
        (clicmd_parse_sid(cli->cur_unit, str, parse_name, &parse_sid));

    /* ready to traverse symbol table */
    chip_info = board_get_chipinfo_by_unit(cli->cur_unit);
    if (!chip_info || !chip_info->symbols) {
        return BCMA_CLI_CMD_NO_SYM;
    }

    sal_memset(&iter_cb, 0, sizeof(clicmd_symbol_iter_cb_t));
    iter_cb.unit = cli->cur_unit;
    iter_cb.sid = parse_sid;
    iter_cb.write_data_args = args;

    sal_memset(&iter, 0, sizeof(iter));
    iter.name = parse_name;
    iter.matching_mode = UTIL_SYMBOLS_ITER_MODE_EXACT;
    iter.symbols = chip_info->symbols;
    iter.function = clicmd_symbol_iter_cb;
    iter.vptr = &iter_cb;

    rc = util_symbols_iter(&iter);
    if (rc == BCMA_CLI_CMD_INTR) {
        sal_printf("Ctrl-C detected, abort the iteration\n");
    } else if (rc < 0) {
        sal_printf("ERROR: some thing wrong in symbols iteration\n");
        return BCMA_CLI_CMD_FAIL;
    } else if (rc == 0) {
        sal_printf("ERROR: cannot find symbol %s\n", parse_name);
    }
    return BCMA_CLI_CMD_OK;
#endif /* CFG_CHIP_SYMBOLS_INCLUDED */
}
#endif /* CFG_SDKCLI_INCLUDED */
