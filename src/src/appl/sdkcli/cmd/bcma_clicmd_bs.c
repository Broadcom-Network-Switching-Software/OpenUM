/*! \file bcma_clicmd_bs.c
 *
 * CLI 'BroadSync' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#include "appl/bs.h"

#if defined(CFG_SDKCLI_INCLUDED) && defined(CFG_BROADSYNC_INCLUDED)

#include "cli_porting.h"
#include <appl/sdkcli/bcma_cli_parse.h>


int
bcma_clicmd_bs(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    int rv, val;
    const char *str, *cmd;

    if (!(cmd = BCMA_CLI_ARG_GET(args))) {
        return BCMA_CLI_CMD_USAGE;
    }

    if (bcma_cli_parse_cmp("VERsion", cmd, 0)) {
        uint32_t ver_buf_ptr = *((uint32_t *) BS_VER_PTR_ADDR);
        char *ver_str = (char *) ver_buf_ptr;
        cli_out("BS %s\n", ver_str);
        return BCMA_CLI_CMD_OK;
    }

    if (bcma_cli_parse_cmp("DeBuG", cmd, 0)) {
        str = BCMA_CLI_ARG_GET(args);
        if (str == NULL) {
            return BCMA_CLI_CMD_USAGE;
        }
        if (bcma_cli_parse_cmp("PRinT", str, 0)) {
            volatile char *printbuf = (volatile char *) BS_PRINT_BUFFER_ADDR;
            cli_out("%s\n", printbuf + BS_PRINT_BUFF_CMD_RSP_PRINT_OFFSET);
            cli_out("%s", printbuf);
            cli_out("%s\n", printbuf + BS_PRINT_BUFF_PLL_PRINT_OFFSET);
            return BCMA_CLI_CMD_OK;
        }
        if (bcma_cli_parse_cmp("CoreDump", str, 0)) {
            rv = board_broadsync_core_dump_print();
            if (rv != SYS_OK) {
                cli_out("Error %d\n", rv);
                return BCMA_CLI_CMD_FAIL;
            }
            return BCMA_CLI_CMD_OK;
        }
        if (bcma_cli_parse_int(str, &val) < 0) {
            cli_out("Invalid Debug paramter: %s\n", str);
            return BCMA_CLI_CMD_BAD_ARG;
        }
        rv = board_broadsync_log_configure(val, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        if (rv != SYS_OK) {
            cli_out("Error %d\n", rv);
            return BCMA_CLI_CMD_FAIL;
        }
        return BCMA_CLI_CMD_OK;
    }

    if (bcma_cli_parse_cmp("TimeStamps", cmd, 0))
    {
        int i;
        int min_event_id = 0;
        int max_event_id = BS_MAX_TS_EVENTS-1;
        uint64_t diff;
        volatile bs_shared_ts_data_t ts_data;

        /* Copy a snapshot of timestamps */
        ts_data = *(volatile bs_shared_ts_data_t *)BS_DEBUG_TIMESTAMP_ADDR;

        cli_out(" #  Last Timestamp    Prev Timestamp    <difference>  Equivalent TS1     Equivalent Full Time\n");
        for (i = min_event_id; i <= max_event_id; ++i) {
            diff = (((uint64_t)ts_data.event_timestamp[i].ts_hi << 32) | ((uint64_t)ts_data.event_timestamp[i].ts_lo)) -
                    (((uint64_t)ts_data.prev_event_timestamp[i].ts_hi << 32) | ((uint64_t)ts_data.prev_event_timestamp[i].ts_lo));

            cli_out("%2d: %08x:%08x %08x:%08x <%10u>  %08x:%08x  %u.%09u\n",
                    i,
                    (unsigned)ts_data.event_timestamp[i].ts_hi, (unsigned)ts_data.event_timestamp[i].ts_lo,
                    (unsigned)ts_data.prev_event_timestamp[i].ts_hi, (unsigned)ts_data.prev_event_timestamp[i].ts_lo,
                    (unsigned)diff,
                    (unsigned)ts_data.ts1_timestamp.ts_hi, (unsigned)ts_data.ts1_timestamp.ts_lo,
                    ts_data.full_time_seconds_low, (unsigned)ts_data.full_time_nanoseconds);
        }
        return BCMA_CLI_CMD_OK;
    }

    if (bcma_cli_parse_cmp("ConFiG", cmd, 0)) {
        int mode, bitclk, hb;
        /* Get Mode. */
        str = BCMA_CLI_ARG_GET(args);
        if (str == NULL) {
            return BCMA_CLI_CMD_USAGE;
        }
        if (bcma_cli_parse_cmp("Master", str, 0)) {
            mode = 1;
        } else if (bcma_cli_parse_cmp("Slave", str, 0)) {
            mode = 0;
        } else {
            return BCMA_CLI_CMD_USAGE;
        }
        /* get bitclk */
        str = BCMA_CLI_ARG_GET(args);
        if ((str == NULL) || (bcma_cli_parse_int(str, &bitclk) < 0)) {
            return BCMA_CLI_CMD_USAGE;
        }
        /* get hb clk */
        str = BCMA_CLI_ARG_GET(args);
        if ((str == NULL) || (bcma_cli_parse_int(str, &hb) < 0)) {
            return BCMA_CLI_CMD_USAGE;
        }
        rv = board_broadsync_config_set(mode, bitclk, hb);
        if (rv != SYS_OK) {
            cli_out("Error %d\n", rv);
            return BCMA_CLI_CMD_FAIL;
        }
        return BCMA_CLI_CMD_OK;
    }

    if (bcma_cli_parse_cmp("FreqOffset", cmd, 0)) {
        /* get offset value */
        str = BCMA_CLI_ARG_GET(args);
        if ((str == NULL) || (bcma_cli_parse_int(str, &val) < 0)) {
            cli_out("Invalid Offset Value: %s\n", str);
            return BCMA_CLI_CMD_BAD_ARG;
        }
        rv = board_broadsync_freq_offset_set(val);
        if (rv != SYS_OK) {
            cli_out("Error %d\n", rv);
            return BCMA_CLI_CMD_FAIL;
        }
        return BCMA_CLI_CMD_OK;
    }

    if (bcma_cli_parse_cmp("PhaseOffset", cmd, 0)) {
        int sec, nsec, neg;
        /* get sec */
        str = BCMA_CLI_ARG_GET(args);
        if ((str == NULL) || (bcma_cli_parse_int(str, &sec) < 0)) {
            return BCMA_CLI_CMD_USAGE;
        }
        /* get nsec */
        str = BCMA_CLI_ARG_GET(args);
        if ((str == NULL) || (bcma_cli_parse_int(str, &nsec) < 0)) {
            return BCMA_CLI_CMD_USAGE;
        }
        /* Is Negative */
        str = BCMA_CLI_ARG_GET(args);
        if (str == NULL) {
            return BCMA_CLI_CMD_USAGE;
        }
        if (bcma_cli_parse_cmp("True", str, 0)) {
            neg = 1;
        } else if (bcma_cli_parse_cmp("False", str, 0)) {
            neg = 0;
        } else {
            return BCMA_CLI_CMD_USAGE;
        }
        rv = board_broadsync_phase_offset_set(sec, nsec, neg);
        if (rv != SYS_OK) {
            cli_out("Error %d\n", rv);
            return BCMA_CLI_CMD_FAIL;
        }
        return BCMA_CLI_CMD_OK;
    }

    if (bcma_cli_parse_cmp("NtpOffset", cmd, 0)) {
        int sec, nsec, neg;
        /* get sec */
        str = BCMA_CLI_ARG_GET(args);
        if ((str == NULL) || (bcma_cli_parse_int(str, &sec) < 0)) {
            return BCMA_CLI_CMD_USAGE;
        }
        /* get nsec */
        str = BCMA_CLI_ARG_GET(args);
        if ((str == NULL) || (bcma_cli_parse_int(str, &nsec) < 0)) {
            return BCMA_CLI_CMD_USAGE;
        }
        /* Is Negative */
        str = BCMA_CLI_ARG_GET(args);
        if (str == NULL) {
            return BCMA_CLI_CMD_USAGE;
        }
        if (bcma_cli_parse_cmp("True", str, 0)) {
            neg = 1;
        } else if (bcma_cli_parse_cmp("False", str, 0)) {
            neg = 0;
        } else {
            return BCMA_CLI_CMD_USAGE;
        }
        rv = board_broadsync_ntp_offset_set(sec, nsec, neg);
        if (rv != SYS_OK) {
            cli_out("Error %d\n", rv);
            return BCMA_CLI_CMD_FAIL;
        }
        return BCMA_CLI_CMD_OK;
    }

    if (bcma_cli_parse_cmp("1pps", cmd, 0)) {
        str = BCMA_CLI_ARG_GET(args);
        if (str == NULL) {
            return BCMA_CLI_CMD_USAGE;
        }
        if (bcma_cli_parse_cmp("oN", str, 0)) {
            val = 1;
        } else if (bcma_cli_parse_cmp("oFf", str, 0)) {
            val = 0;
        } else {
            return BCMA_CLI_CMD_USAGE;
        }
        rv = board_broadsync_debug_1pps_set(val);
        if (rv != SYS_OK) {
            cli_out("Error %d\n", rv);
            return BCMA_CLI_CMD_FAIL;
        }
        return BCMA_CLI_CMD_OK;
    }

    if (bcma_cli_parse_cmp("ReInit", cmd, 0)) {
        rv = board_broadsync_reinit();
        if (rv != SYS_OK) {
            cli_out("Error %d\n", rv);
            return BCMA_CLI_CMD_FAIL;
        }
        return BCMA_CLI_CMD_OK;
    }

    cli_out("%sUnknown BroadSync Command: %s\n", BCMA_CLI_CONFIG_ERROR_STR, cmd);

    return BCMA_CLI_CMD_FAIL;
}

#endif /* CFG_SDKCLI_INCLUDED */
