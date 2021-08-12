/*! \file bcma_clicmd_coe.c
 *
 * CLI 'coe' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"

#ifdef CFG_SDKCLI_INCLUDED
#ifdef CFG_SDKCLI_COE_INCLUDED
#include "cli_porting.h"

#include <appl/sdkcli/bcma_cli.h>
#include <appl/sdkcli/bcma_cli_parse.h>

#include <appl/sdkcli/bcma_clicmd_coe.h>
#include <boardapi/coe.h>
#include <boardapi/port.h>

static int
chassis_mapping(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    char *src_list, *dest_list;
    SHR_BITDCL src_bitmap = 0, dest_bitmap = 0;
    int max_port_cnt = board_uport_count() + 1;
    int src_port_cnt = 0, dest_port_cnt = 0;
    uint8 src_port[2], dest_port[2];
    int uport;
    bcma_cli_parse_table_t pt;
    chassis_dir_t dir;
    char *cmd;
    int cid;
    int ret_val;
    int rv = SYS_OK;

    COMPILER_REFERENCE(cli);

    if (args->argc != 7) {
        cli_out("%s: Invalid paremeters: argc mismatch.\n",
                BCMA_CLI_ARG_CMD(args));
        return BCMA_CLI_CMD_FAIL;
    }

    if (!(cmd = BCMA_CLI_ARG_GET(args))) {
        return BCMA_CLI_CMD_USAGE;
    }

    if (bcma_cli_parse_cmp("Up", cmd, ' ')) {
        dir = CH_UPSTREAM;
    } else if (bcma_cli_parse_cmp("Down", cmd, ' ')) {
        dir = CH_DOWNSTREAM;
    } else {
        cli_out("%s: Invalid direction specifed; should be Up|Down.\n",
                BCMA_CLI_ARG_CMD(args));
        return BCMA_CLI_CMD_FAIL;
    }

    bcma_cli_parse_table_init(cli, &pt);
    bcma_cli_parse_table_add(&pt, "SourcePort", "str", &src_list, NULL);
    bcma_cli_parse_table_add(&pt, "DestPort", "str", &dest_list, NULL);
    bcma_cli_parse_table_add(&pt, "ChannelID", "int", &cid, NULL);
    ret_val = bcma_cli_parse_table_do_args(&pt, args);
    if (ret_val < 0) {
        cli_out("%s: Invalid option: %s\n",
                BCMA_CLI_ARG_CMD(args), BCMA_CLI_ARG_CUR(args));
        rv = BCMA_CLI_CMD_FAIL;
        goto cleanup;
    }

    if (src_list) {
        ret_val = bcma_cli_parse_bit_list(src_list, max_port_cnt, &src_bitmap);
        if (ret_val >= 0) {
            /* uport '0' is invalid port */
            if (src_bitmap & 0x1) {
                cli_out("%sInvalid source port(s).\n",
                                    BCMA_CLI_CONFIG_ERROR_STR);
                rv = BCMA_CLI_CMD_BAD_ARG;;
                goto cleanup;
            }

            SHR_BITCOUNT_RANGE(&src_bitmap, src_port_cnt, 0, max_port_cnt);
            if (dir == CH_UPSTREAM) {
                if (src_port_cnt > 1) {
                    cli_out("%sToo many source ports are specified.\n",
                                        BCMA_CLI_CONFIG_ERROR_STR);
                    rv = BCMA_CLI_CMD_BAD_ARG;;
                    goto cleanup;
                }
            } else {
                if (src_port_cnt > 2) {
                    cli_out("%sToo many source ports are specified.\n",
                                        BCMA_CLI_CONFIG_ERROR_STR);
                    rv = BCMA_CLI_CMD_BAD_ARG;;
                    goto cleanup;
                }
            }

            src_port_cnt = 0;
            SHR_BIT_ITER(&src_bitmap, max_port_cnt, uport) {
                src_port[src_port_cnt] = uport;
                src_port_cnt++;
            }

        } else if (ret_val == -2) {
            cli_out("%sInvalid source port(s).\n",
                                    BCMA_CLI_CONFIG_ERROR_STR);
            rv = BCMA_CLI_CMD_BAD_ARG;;
            goto cleanup;
        } else {
            rv = BCMA_CLI_CMD_USAGE;;
            goto cleanup;
        }
    } else {
        cli_out("%sNo source port specified\n",
                                    BCMA_CLI_CONFIG_ERROR_STR);
        rv = BCMA_CLI_CMD_BAD_ARG;;
        goto cleanup;
    }

    if (dest_list) {
        ret_val = bcma_cli_parse_bit_list(dest_list, max_port_cnt, &dest_bitmap);
        if (ret_val >= 0) {
            /* uport '0' is invalid port */
            if (dest_bitmap & 0x1) {
                cli_out("%sInvalid destination port(s).\n",
                                    BCMA_CLI_CONFIG_ERROR_STR);
                rv = BCMA_CLI_CMD_BAD_ARG;;
                goto cleanup;
            }

            SHR_BITCOUNT_RANGE(&dest_bitmap, dest_port_cnt, 0, max_port_cnt);
            if (dir == CH_UPSTREAM) {
                if (dest_port_cnt > 2) {
                    cli_out("%sToo many destination ports are specified.\n",
                                        BCMA_CLI_CONFIG_ERROR_STR);
                    rv = BCMA_CLI_CMD_BAD_ARG;;
                    goto cleanup;
                }
            } else {
                if (dest_port_cnt > 1) {
                    cli_out("%sToo many destination ports are specified.\n",
                                        BCMA_CLI_CONFIG_ERROR_STR);
                    rv = BCMA_CLI_CMD_BAD_ARG;;
                    goto cleanup;
                }
            }

            dest_port_cnt = 0;
            SHR_BIT_ITER(&dest_bitmap, max_port_cnt, uport) {
                dest_port[dest_port_cnt] = uport;
                dest_port_cnt++;
            }

        } else if (ret_val == -2) {
                cli_out("%sInvalid destination port(s).\n",
                                    BCMA_CLI_CONFIG_ERROR_STR);
            rv = BCMA_CLI_CMD_BAD_ARG;;
            goto cleanup;
        } else {
            rv = BCMA_CLI_CMD_USAGE;;
            goto cleanup;
        }
    } else {
        cli_out("%sNo destination port specified\n",
                                    BCMA_CLI_CONFIG_ERROR_STR);
        rv = BCMA_CLI_CMD_BAD_ARG;;
        goto cleanup;
    }

    if (src_bitmap & dest_bitmap) {
        cli_out("%sSource port cannot be the same as the destination port.\n",
                                    BCMA_CLI_CONFIG_ERROR_STR);
        rv = BCMA_CLI_CMD_BAD_ARG;;
        goto cleanup;
    }

    if (dir == CH_UPSTREAM) {
        if (dest_port_cnt == 1) {
            rv = board_coe_linecard_upstream_frontport_to_backplane_config(
                    dest_port[0], 0, src_port[0], cid);
        } else {
            rv = board_coe_linecard_upstream_frontport_to_backplane_config(
                    dest_port[0], dest_port[1], src_port[0], cid);
        }
    } else if (dir == CH_DOWNSTREAM){
        if (src_port_cnt == 1) {
            rv = board_coe_linecard_downstream_backplane_to_frontport_config(
                    src_port[0], dest_port[0], cid);
        } else {
            rv = board_coe_linecard_downstream_backplane_to_frontport_config(
                    src_port[0], dest_port[0], cid);
            if(rv == SYS_OK) {
                rv = board_coe_linecard_downstream_backplane_to_frontport_config(
                    src_port[1], dest_port[0], cid);
            }
        }
    } else {
        cli_out("%sNo upstream/downstream specifed.\n",
                                    BCMA_CLI_CONFIG_ERROR_STR);
        rv = BCMA_CLI_CMD_BAD_ARG;;
        goto cleanup;
    }

    if (rv != SYS_OK) {
        cli_out("Configure COE chassis mapping failed (%d)\n", rv);
        rv = BCMA_CLI_CMD_FAIL;;
        goto cleanup;
    }
    rv = BCMA_CLI_CMD_OK;;

cleanup:
    bcma_cli_parse_table_done(&pt);
    return rv;
}

static int
passthru_mapping(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    char *src_list, *dest_list;
    SHR_BITDCL src_bitmap = 0, dest_bitmap = 0;
    int max_port_cnt = board_uport_count() + 1;
    int src_port_cnt = 0, dest_port_cnt = 0;
    uint8 src_port[2], dest_port[2];
    int uport;
    int ret_val;
    int rv = SYS_OK;

    COMPILER_REFERENCE(cli);

    if (args->argc != 5) {
        cli_out("%s: Invalid paremeters: %s\n",
                BCMA_CLI_ARG_CMD(args), BCMA_CLI_ARG_CUR(args));
        return BCMA_CLI_CMD_FAIL;
    }

    src_list = BCMA_CLI_ARG_GET(args);
    if (src_list) {
        ret_val = bcma_cli_parse_bit_list(src_list, max_port_cnt, &src_bitmap);
        if (ret_val >= 0) {
            /* uport '0' is invalid port */
            if (src_bitmap & 0x1) {
                cli_out("%sInvalid source port(s).\n",
                                    BCMA_CLI_CONFIG_ERROR_STR);
                return BCMA_CLI_CMD_BAD_ARG;
            }

            SHR_BITCOUNT_RANGE(&src_bitmap, src_port_cnt, 0, max_port_cnt);
            if (src_port_cnt > 2) {
                cli_out("%sToo many source ports are specified.\n",
                                    BCMA_CLI_CONFIG_ERROR_STR);
                return BCMA_CLI_CMD_BAD_ARG;
            } else {
                src_port_cnt = 0;
                SHR_BIT_ITER(&src_bitmap, max_port_cnt, uport) {
                    src_port[src_port_cnt] = uport;
                    src_port_cnt++;
                }
            }
        } else if (ret_val == -2) {
            cli_out("%sInvalid source port(s).\n",
                                    BCMA_CLI_CONFIG_ERROR_STR);
            return BCMA_CLI_CMD_BAD_ARG;
        } else {
            return BCMA_CLI_CMD_USAGE;
        }
    } else {
        cli_out("%sNo source port specified\n",
                                    BCMA_CLI_CONFIG_ERROR_STR);
        return BCMA_CLI_CMD_BAD_ARG;
    }

    dest_list = BCMA_CLI_ARG_GET(args);
    if (dest_list) {
        ret_val = bcma_cli_parse_bit_list(dest_list, max_port_cnt, &dest_bitmap);
        if (ret_val >= 0) {
            /* uport '0' is invalid port */
            if (dest_bitmap & 0x1) {
                cli_out("%sInvalid destination port(s).\n",
                                    BCMA_CLI_CONFIG_ERROR_STR);
                return BCMA_CLI_CMD_BAD_ARG;
            }

            SHR_BITCOUNT_RANGE(&dest_bitmap, dest_port_cnt, 0, max_port_cnt);
            if (dest_port_cnt > 2) {
                cli_out("%sToo many destination ports are specified.\n",
                                    BCMA_CLI_CONFIG_ERROR_STR);
                return BCMA_CLI_CMD_BAD_ARG;
            } else {
                dest_port_cnt = 0;
                SHR_BIT_ITER(&dest_bitmap, max_port_cnt, uport) {
                    dest_port[dest_port_cnt] = uport;
                    dest_port_cnt++;
                }
            }
        } else if (ret_val == -2) {
                cli_out("%sInvalid destination port(s).\n",
                                    BCMA_CLI_CONFIG_ERROR_STR);
            return BCMA_CLI_CMD_BAD_ARG;
        } else {
            return BCMA_CLI_CMD_USAGE;
        }
    } else {
        cli_out("%sNo destination port specified\n",
                                    BCMA_CLI_CONFIG_ERROR_STR);
        return BCMA_CLI_CMD_BAD_ARG;
    }

    if (src_bitmap & dest_bitmap) {
        cli_out("%sThe source port cannot be the same as the destination port.\n",
                                    BCMA_CLI_CONFIG_ERROR_STR);
        return BCMA_CLI_CMD_BAD_ARG;
    }
    if (src_port_cnt == 1) {
        if (dest_port_cnt == 1) {
            rv = board_coe_uplink_frontport_to_backplane_config(
                                dest_port[0], 0, src_port[0]);
        } else {
            rv = board_coe_uplink_frontport_to_backplane_config(
                                dest_port[0], dest_port[1], src_port[0]);
        }
    } else {
        if (dest_port_cnt == 1) {
            rv = board_coe_uplink_backplane_to_frontport_config(
                                src_port[0], src_port[1], dest_port[0]);
        } else {
            cli_out("%sToo many ports are specified.\n",
                                    BCMA_CLI_CONFIG_ERROR_STR);
            return BCMA_CLI_CMD_BAD_ARG;
        }
    }

    if (rv != SYS_OK) {
        cli_out("Configure COE passthru mapping failed (%d)\n", rv);
        return BCMA_CLI_CMD_FAIL;
    }
    return BCMA_CLI_CMD_OK;
}

static int
passthru_show(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    int rv = SYS_OK;
    coe_rule_table_t coe_fp_qualify;
    int idx = 0, rule = 1;

    cli_out("COE configuration for pass-through mode:\n");
    while(1) {
        rv = board_coe_uplink_get(idx++, &coe_fp_qualify);
        if (rv != SYS_OK || !coe_fp_qualify.valid) {
            break;
        }

        if (coe_fp_qualify.default_drop) {
            cli_out("  %d: Enable default drop. \n", rule++);
        } else {
            if (coe_fp_qualify.dir == CH_DOWNSTREAM) {
                if (coe_fp_qualify.bp_port2) {
                    cli_out("  %d: Source port %d redirects to destination ports %d, %d\n", rule++,
                            coe_fp_qualify.front_port,
                            coe_fp_qualify.bp_port1,
                            coe_fp_qualify.bp_port2);
                } else {
                    cli_out("  %d: Source port %d redirects to destination ports %d\n", rule++,
                            coe_fp_qualify.front_port,
                            coe_fp_qualify.bp_port1);
                }
            } else {
                if (coe_fp_qualify.bp_port2) {
                    if (coe_fp_qualify.active_class_id >= 0) {
                        uint32 class_id1, class_id2;
                        int active_port = 0;

                        board_port_class_get(coe_fp_qualify.bp_port1, PortClassFieldIngress, &class_id1);
                        board_port_class_get(coe_fp_qualify.bp_port2, PortClassFieldIngress, &class_id2);
                        if (coe_fp_qualify.active_class_id == class_id1) {
                            active_port = coe_fp_qualify.bp_port1;
                        } else if (coe_fp_qualify.active_class_id == class_id2) {
                            active_port = coe_fp_qualify.bp_port2;
                        }

                        cli_out("  %d: Source port %d, %d redirects to destination ports %d. Active port %d (class ID: 0x%x)\n", rule++,
                                coe_fp_qualify.bp_port1,
                                coe_fp_qualify.bp_port2,
                                coe_fp_qualify.front_port,
                                active_port,
                                coe_fp_qualify.active_class_id);
                    } else {
                        cli_out("  %d: Source port %d %d redirects to destination ports %d\n", rule++,
                                coe_fp_qualify.bp_port1,
                                coe_fp_qualify.bp_port2,
                                coe_fp_qualify.front_port);
                    }
                } else {
                    cli_out("  %d: Source port %d redirects to destination ports: %d\n", rule++,
                            coe_fp_qualify.bp_port1,
                            coe_fp_qualify.front_port);
                }
            }
        }
    }

    return BCMA_CLI_CMD_OK;
}

static int
coe_activeportclass_cmd(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    char *cmd;
    int portclass;

    if (args->argc > 3) {
        cli_out("%s: Invalid paremeters: argc mismatch.\n",
                BCMA_CLI_ARG_CMD(args));
        return BCMA_CLI_CMD_FAIL;
    }
    if (args->argc == 2) {
        cli_out("Active port class is %d\n", coe_permitted_class_id);
        return BCMA_CLI_CMD_OK;
    }
    cmd = BCMA_CLI_ARG_GET(args);
    if ((cmd == NULL) || (bcma_cli_parse_int(cmd, &portclass) < 0)) {
        return BCMA_CLI_CMD_USAGE;
    }

    return(board_coe_active_port_class(portclass));
}

static int
coe_show_cmd(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    int i;
#ifdef CFG_COE_SCENARIO_INCLUDED
    uint32 coe_scenario = 0;
#endif

    if (args->argc != 2) {
        cli_out("%s: Invalid paremeters: argc mismatch.\n",
                BCMA_CLI_ARG_CMD(args));
        return BCMA_CLI_CMD_FAIL;
    }
#ifdef CFG_COE_SCENARIO_INCLUDED
    if (sal_config_uint32_get(
            SAL_CONFIG_COE_SCENARIO, &coe_scenario) == SYS_OK) {
        if (coe_scenario != 0) {
            cli_out("coe_scenario=%d\n", coe_scenario);
        }
    }
#endif
    cli_out("COE configuration for chassis mode:\n");
    for (i = 0; i < MAX_COE_RULES_NUMBER; i++) {
        if (coe_rule_table[i].valid && coe_rule_table[i].default_drop) {
            cli_out("\nRule %d: Default drop. \n", (i + 1));
            cli_out("fp_counter_table_index=%d\n",
                coe_rule_table[i].fp_counter_table_index);
        } else if (coe_rule_table[i].valid &&
                    (coe_rule_table[i].dir == CH_UPSTREAM)) {
            cli_out("\nRule %d: Upstream \n", (i + 1));
            if (coe_rule_table[i].bp_port2 != 0) {
                cli_out("port: %d redirects to port %d, %d \n",
                    coe_rule_table[i].front_port,
                    coe_rule_table[i].bp_port1,
                    coe_rule_table[i].bp_port2);
            } else {
                cli_out("port: %d redirects to port %d\n",
                    coe_rule_table[i].front_port,
                    coe_rule_table[i].bp_port1);
            }
            cli_out("custom_header: 0x%08x fp_counter_table_index=%d\n",
                coe_rule_table[i].custom_header,
                coe_rule_table[i].fp_counter_table_index);
        } else if (coe_rule_table[i].valid &&
                    (coe_rule_table[i].dir == CH_DOWNSTREAM)) {
            cli_out("\nRule %d: Downstream \n", (i + 1));
            cli_out("port: %d redirects to port %d\n",
                coe_rule_table[i].bp_port1,
                coe_rule_table[i].front_port);
            cli_out("custom_header: 0x%08x fp_counter_table_index=%d\n",
                coe_rule_table[i].custom_header,
                coe_rule_table[i].fp_counter_table_index);
        }
    }

    return BCMA_CLI_CMD_OK;
}

static int
coe_chassis_cmd(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    char* cmd;

    if (!(cmd = BCMA_CLI_ARG_GET(args))) {
        return BCMA_CLI_CMD_USAGE;
    }

    if (bcma_cli_parse_cmp("Mapping", cmd, ' ')) {
        return chassis_mapping(cli, args);
    }

    cli_out("%sUnknown COE chassis command: %s\n", BCMA_CLI_CONFIG_ERROR_STR, cmd);
    return BCMA_CLI_CMD_FAIL;
}

static int
coe_passthru_cmd(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    char* cmd;

    if (!(cmd = BCMA_CLI_ARG_GET(args))) {
        return BCMA_CLI_CMD_USAGE;
    }

    if (bcma_cli_parse_cmp("Mapping", cmd, ' ')) {
        return passthru_mapping(cli, args);
    }

    if (bcma_cli_parse_cmp("Show", cmd, ' ')) {
        return passthru_show(cli, args);
    }

    cli_out("%sUnknown coe command: %s\n", BCMA_CLI_CONFIG_ERROR_STR, cmd);
    return BCMA_CLI_CMD_FAIL;
}

int
bcma_clicmd_coe(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    char* cmd;
    int ret;

    if (!(cmd = BCMA_CLI_ARG_GET(args))) {
        return BCMA_CLI_CMD_USAGE;
    }

    if (bcma_cli_parse_cmp("ChaSsis", cmd, ' ')) {
        if (coe_mode == COE_MODE_PASSTHRU) {
            cli_out("%sCurrent COE mode is pass-through mode.\n", BCMA_CLI_CONFIG_ERROR_STR);
            return BCMA_CLI_CMD_FAIL;
        }

        if ((ret = coe_chassis_cmd(cli, args)) == BCMA_CLI_CMD_OK) {
            return ret;
        }
    }

    if (bcma_cli_parse_cmp("ActivePortClass", cmd,  ' ')) {
        return coe_activeportclass_cmd(cli, args);
    }

    if (bcma_cli_parse_cmp("Show", cmd,  ' ')) {
        if (coe_mode == COE_MODE_CHASSIS) {
            return coe_show_cmd(cli, args);
        } else if (coe_mode == COE_MODE_PASSTHRU) {
            return passthru_show(cli, args);
        } else {
            cli_out("No COE Chassis or Passthru APIs have been configured.\n");
            return BCMA_CLI_CMD_FAIL;
        }
    }

    if (bcma_cli_parse_cmp("PassThru", cmd,  ' ')) {
        if (coe_mode == COE_MODE_CHASSIS) {
            cli_out("%sCurrent COE mode is chassis mode.\n", BCMA_CLI_CONFIG_ERROR_STR);
            return BCMA_CLI_CMD_FAIL;
        }

        if ((ret = coe_passthru_cmd(cli, args)) == BCMA_CLI_CMD_OK) {
            return ret;
        }
     }

    cli_out("%sUnknown coe command: %s\n", BCMA_CLI_CONFIG_ERROR_STR, cmd);

    return BCMA_CLI_CMD_FAIL;
}
#endif /* CFG_SDKCLI_COE_INCLUDED */
#endif /* CFG_SDKCLI_INCLUDED */
