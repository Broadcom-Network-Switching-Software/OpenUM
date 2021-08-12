/*! \file bcma_bcmpktcmd_tx.c
 *
 * BCMPKT TX commands in CLI.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#ifdef CFG_SDKCLI_INCLUDED
#include "cli_porting.h"
#include "bcma_bcmpktcmd_internal.h"

#if CFG_RXTX_SUPPORT_ENABLED

#include <appl/sdkcli/bcma_cli.h>
#include <appl/sdkcli/bcma_cli_parse.h>

#include <appl/sdkcli/bcma_clicmd_tx.h>

#include "utils/ports.h"

#define BSL_LOG_MODULE      BSL_LS_APPL_TX

#define NAME_STR_LEN_MAX    128

#define DEF_VLAN            1
#define DEF_PKT_LEN         60  /* Does not include VLAN TAG size. */
#define DEF_PATTERN         0x01020304
#define DEF_PATTERN_INC     0x04040404

/* TX packet configurations */
typedef struct tx_async_s {
    uint32      count;
    uint32      current;
    BOOL        timer;
    BOOL        done;
    BOOL        verbose;
} tx_async_t;

typedef struct px_tx_s {
    int netif_id;
    int is_mcq;
    int int_pri;
    int len;
    int ethertype;
    int tpid;
    int untagged;
    int dei;
    int pcp;
    int vlan;
    int pattern;
    int pat_inc;
    int pat_random;
    int per_port_smac;
    int smac_inc;
    int dmac_inc;
} px_tx_t;

typedef struct bcma_bcmpkt_tx_cfg_s {
    int netif_id;
    char *data;   /*! Input raw data. */

    bcma_pbmp_t portlist;
    bool is_mcq;
    uint8_t int_pri;
    bcma_bcmpkt_pktgen_cfg_t gcfg;
} bcma_bcmpkt_tx_cfg_t;

/* Release exist buffers. */
static void
tx_cfg_buf_release(bcma_bcmpkt_tx_cfg_t *cfg, sys_pkt_t *packet)
{
    if (packet != NULL) {
        if (packet->cookie != NULL) {
            sal_free(packet->cookie);
        }
        if (packet->pkt_data != NULL) {
            sal_dma_free(packet->pkt_data);
        }
        sal_free(packet);
        packet = NULL;
    }

    if (cfg->data != NULL) {
        sal_free(cfg->data);
        cfg->data = NULL;
    }
}

static void
tx_cfg_api2px(bcma_bcmpkt_tx_cfg_t *cfg, px_tx_t *px_cfg)
{
    px_cfg->netif_id = cfg->netif_id;
    px_cfg->is_mcq = PX_NOT_CONFIGURED;
    px_cfg->int_pri = PX_NOT_CONFIGURED;
    px_cfg->len= (cfg->data == NULL) ?
                 (int)cfg->gcfg.len : PX_NOT_CONFIGURED;

    px_cfg->ethertype = cfg->gcfg.ethertype;
    px_cfg->tpid = cfg->gcfg.tpid;
    px_cfg->untagged = cfg->gcfg.untagged;
    px_cfg->dei = cfg->gcfg.dei;
    px_cfg->pcp = cfg->gcfg.pcp;
    px_cfg->vlan = cfg->gcfg.vlan;
    px_cfg->pattern = (int)cfg->gcfg.pattern;
    px_cfg->pat_inc = (int)cfg->gcfg.pat_inc;
    px_cfg->pat_random = cfg->gcfg.pat_random;
    px_cfg->per_port_smac = cfg->gcfg.per_port_smac;
    px_cfg->smac_inc = (int)cfg->gcfg.smac_inc;
    px_cfg->dmac_inc = (int)cfg->gcfg.dmac_inc;
}

static int
tx_cfg_px2api(bcma_cli_args_t *args, px_tx_t *px_cfg,
              char *fname, char *data, bcma_bcmpkt_tx_cfg_t *cfg)
{
    uint32_t psize_min, data_len;
    psize_min = BCMPKT_FRAME_SIZE_MIN;

    cfg->netif_id = px_cfg->netif_id;

    cfg->gcfg.untagged = (px_cfg->untagged) ? true : false;
    if (!cfg->gcfg.untagged) {
        psize_min += 4;
    }

    if (data != NULL) {
        if (cfg->data != NULL) {
            sal_free(cfg->data);
            cfg->data = NULL;
        }
        data_len = sal_strlen(data);
        if (data_len > 0) {
            data_len = (data_len + 1)/2;
            if (data_len > MAX_PACKET_LENGTH) {
                cli_out("Input packet length %d is too large\n", data_len);
                return SYS_ERR;
            }
            cfg->data = sal_strdup(data);
            if (!cfg->data) {
                cli_out("Strdup cfg->data failed\n");
                return SYS_ERR;
            }
            cfg->gcfg.len = data_len;
        } else {
            /* Restore pktgen len to default. */
            cfg->gcfg.len = DEF_PKT_LEN;
        }
        if (!cfg->gcfg.untagged) {
            cfg->gcfg.len += 4;
        }
    }
    else if (px_cfg->len != -1) {
        if (px_cfg->len < (int)psize_min) {
            cli_out("Input packet length %d is too small\n", px_cfg->len);
            return SYS_ERR;
        }

        cfg->gcfg.len = px_cfg->len;
        /* Release data. */
        if (cfg->data != NULL) {
            sal_free(cfg->data);
            cfg->data = NULL;
        }
    }

    if (px_cfg->int_pri != -1) {
        cfg->int_pri = px_cfg->int_pri & 0xf;
    }

    cfg->gcfg.ethertype = px_cfg->ethertype & 0xFFFF;
    cfg->gcfg.tpid = px_cfg->tpid & 0xFFFF;
    cfg->gcfg.dei = px_cfg->dei & 0x1;
    cfg->gcfg.pcp = px_cfg->pcp & 0x7;
    cfg->gcfg.vlan = px_cfg->vlan & 0xFFF;
    cfg->gcfg.pattern = (uint32_t)px_cfg->pattern;
    cfg->gcfg.pat_inc = (uint32_t)px_cfg->pat_inc;
    cfg->gcfg.pat_random = (px_cfg->pat_random) ? true : false;
    cfg->gcfg.per_port_smac = (px_cfg->per_port_smac) ? true : false;
    cfg->gcfg.smac_inc = (uint32_t)px_cfg->smac_inc;
    cfg->gcfg.dmac_inc = (uint32_t)px_cfg->dmac_inc;

    if (cfg->data == NULL) {
        if (cfg->gcfg.len < psize_min) {
            cli_out("Packet size %d is too small\n", cfg->gcfg.len);
            return SYS_ERR;
        }
    }
    return SYS_OK;
}

#ifdef CFG_DEBUGGING_INCLUDED
static void
tx_cfg_dump(shr_pb_t *pb, bcma_bcmpkt_tx_cfg_t *cfg)
{
    shr_pb_printf(pb, "\nTX Configuration:\n");
    shr_pb_printf(pb, "\tNetifID: %d\n", cfg->netif_id);
    if (cfg->data)
        shr_pb_printf(pb, "\tDATA: %s\n", cfg->data);
    shr_pb_printf(pb, "\tMcQType: %d\n", cfg->is_mcq);
    shr_pb_printf(pb, "\tIntPrio: %d\n", cfg->int_pri);
    shr_pb_printf(pb, "\tLENgth: %d\n", cfg->gcfg.len);
    shr_pb_printf(pb, "\tEtherType: %04x\n", cfg->gcfg.ethertype);
    shr_pb_printf(pb, "\tTPID: %04x\n", cfg->gcfg.tpid);
    shr_pb_printf(pb, "\tUnTagged: %d\n", cfg->gcfg.untagged);
    shr_pb_printf(pb, "\tDEI: %d\n", cfg->gcfg.dei);
    shr_pb_printf(pb, "\tPCP: %04x\n", cfg->gcfg.pcp);
    shr_pb_printf(pb, "\tVlanID: %d\n", cfg->gcfg.vlan);
    shr_pb_printf(pb, "\tPATtern: 0x%08x\n", cfg->gcfg.pattern);
    shr_pb_printf(pb, "\tPATternInc: 0x%08x\n", cfg->gcfg.pat_inc);
    shr_pb_printf(pb, "\tPATternRandom: %d\n", cfg->gcfg.pat_random);
    shr_pb_printf(pb, "\tPerPortSrcMac: %d\n", cfg->gcfg.per_port_smac);
    shr_pb_printf(pb, "\tDestMac: %02x:%02x:%02x:%02x:%02x:%02x\n",
                  cfg->gcfg.dmac[0], cfg->gcfg.dmac[1], cfg->gcfg.dmac[2],
                  cfg->gcfg.dmac[3], cfg->gcfg.dmac[4], cfg->gcfg.dmac[5]);
    shr_pb_printf(pb, "\tDestMacInc: %04x\n", cfg->gcfg.dmac_inc);
    shr_pb_printf(pb, "\tSourceMac: %02x:%02x:%02x:%02x:%02x:%02x\n",
                  cfg->gcfg.smac[0], cfg->gcfg.smac[1], cfg->gcfg.smac[2],
                  cfg->gcfg.smac[3], cfg->gcfg.smac[4], cfg->gcfg.smac[5]);
    shr_pb_printf(pb, "\tSourceMacInc: %04x\n", cfg->gcfg.smac_inc);
}
#endif
static int
tx_parser(bcma_cli_t *cli, bcma_cli_args_t *args, bcma_bcmpkt_tx_cfg_t *cfg)
{
    int rv;
#ifdef CFG_DEBUGGING_INCLUDED
    shr_pb_t *pb;
#endif
    px_tx_t px_cfg;
    char *data = NULL;
    int port_max_cnt = board_uport_count() + 1;
    bcma_cli_parse_table_t pt;

    bcma_cli_parse_table_init(cli, &pt);
    bcma_cli_parse_data_add_net();
    bcma_bcmpkt_parse_data_add();
    tx_cfg_api2px(cfg, &px_cfg);
    bcma_cli_parse_table_add(&pt, "LENgth", "int",
                             &px_cfg.len, NULL);
    bcma_cli_parse_table_add(&pt, "IntPrio", "int",
                             &px_cfg.int_pri, NULL);
    bcma_cli_parse_table_add(&pt, "UnTagged", "bool",
                             &px_cfg.untagged, NULL);
    bcma_cli_parse_table_add(&pt, "EtherType", "hex",
                             &px_cfg.ethertype, NULL);
    bcma_cli_parse_table_add(&pt, "TPID", "hex",
                             &px_cfg.tpid, NULL);
    bcma_cli_parse_table_add(&pt, "DEI", "hex",
                             &px_cfg.dei, NULL);
    bcma_cli_parse_table_add(&pt, "VlanID", "int",
                             &px_cfg.vlan, NULL);
    bcma_cli_parse_table_add(&pt, "PCP", "int",
                             &px_cfg.pcp, NULL);
    bcma_cli_parse_table_add(&pt, "PATtern", "hex",
                             &px_cfg.pattern, NULL);
    bcma_cli_parse_table_add(&pt, "PATternInc", "hex",
                             &px_cfg.pat_inc, NULL);
    bcma_cli_parse_table_add(&pt, "PATternRandom", "bool",
                             &px_cfg.pat_random, NULL);
    bcma_cli_parse_table_add(&pt, "PerPortSrcMac", "bool",
                             &px_cfg.per_port_smac, NULL);
    bcma_cli_parse_table_add(&pt, "SourceMacInc", "hex",
                             &px_cfg.smac_inc, NULL);
    bcma_cli_parse_table_add(&pt, "DestMacInc", "hex",
                             &px_cfg.dmac_inc, NULL);
    bcma_cli_parse_table_add(&pt, "PortList", "bmp",
                             &cfg->portlist.pbits, &port_max_cnt);
    bcma_cli_parse_table_add(&pt, "DATA", "str",
                             &data, NULL);
    bcma_cli_parse_table_add(&pt, "SourceMac", "mac",
                             cfg->gcfg.smac, NULL);
    bcma_cli_parse_table_add(&pt, "DestMac", "mac",
                             cfg->gcfg.dmac, NULL);

    rv = bcma_cli_parse_table_do_args(&pt, args);
    bcma_cli_parse_table_done(&pt);
    if (rv < 0) {
        cli_out("%s: Invalid option: %s\n",
                BCMA_CLI_ARG_CMD(args), BCMA_CLI_ARG_CUR(args));
        return BCMA_CLI_CMD_USAGE;
    }

    /* Save options into TX configuration. */
    rv = tx_cfg_px2api(args, &px_cfg, NULL, data, cfg);
    if (rv < 0) {
        return BCMA_CLI_CMD_FAIL;
    }

#ifdef CFG_DEBUGGING_INCLUDED
    pb = shr_pb_create();
    tx_cfg_dump(pb, cfg);
    um_console_print(shr_pb_str(pb));
    shr_pb_destroy(pb);
#endif
    return BCMA_CLI_CMD_OK;
}

static int
tx_packet_content_fill(bcma_bcmpkt_tx_cfg_t *cfg, sys_pkt_t *packet)
{
    int rv;
    uint32_t len;

    if (cfg->data != NULL) {
        uint32_t buf_size = packet->buf_len;
        rv = bcma_bcmpkt_load_data_from_istr(cfg->data, packet->alloc_ptr,
                                             buf_size, &len);
        if (rv < 0) {
            cli_out("Load data: %s failed (%d)\n", cfg->data, rv);
            return SYS_ERR;
        }
        if (len < ENET_MIN_PKT_SIZE) {
            cli_out("Invalid packet size: %d\n", packet->pkt_len);
            return SYS_ERR;
        }
        packet->pkt_len = len + 4;
    }
    else {
        rv = bcma_bcmpkt_packet_generate(&cfg->gcfg, packet);
        if (rv < 0) {
            cli_out("Generate packet failed (%d)\n", rv);
            return SYS_ERR;
        }
    }

    return BCMA_CLI_CMD_OK;

}

static int
tx_send(int unit, bcma_bcmpkt_tx_cfg_t *cfg, sys_pkt_t *packet,
        int hg_hdr_size, int count)
{
    int c, rv;

    for(c = 0; c < count; c++) {
        rv = sys_tx(packet, NULL);
        if (rv != SYS_OK) {
            sal_printf("ERROR(%d)\n", rv);
            return -1;
        }

        /* Update payload and MAC for next packet. */
        if (!cfg->data) {
            rv = bcma_bcmpkt_packet_payload_fill(&cfg->gcfg, packet);
            if (rv < 0) {
                cli_out("Update payload failed (%d)\n", rv);
                return -1;
            }
        }
        if (cfg->gcfg.smac_inc > 0) {
            bcma_bcmpkt_macaddr_inc(cfg->gcfg.smac, cfg->gcfg.smac_inc);
            sal_memcpy(packet->pkt_data + hg_hdr_size + ENET_MAC_SIZE,
                       cfg->gcfg.smac, ENET_MAC_SIZE);
        }
        if (cfg->gcfg.dmac_inc > 0) {
            bcma_bcmpkt_macaddr_inc(cfg->gcfg.dmac, cfg->gcfg.dmac_inc);
            sal_memcpy(packet->pkt_data + hg_hdr_size, cfg->gcfg.dmac,
                       ENET_MAC_SIZE);
        }
    }

    return SYS_OK;
}

static int
tx_packet_alloc(int unit, bcma_bcmpkt_tx_cfg_t *cfg, sys_pkt_t **packet)
{
    sys_pkt_t *pkt = NULL;

    pkt = (sys_pkt_t *)sal_malloc(sizeof(sys_pkt_t));
    if (pkt == NULL) {
        sal_printf("Out of memory!\n");
        return -1;
    }
    sal_memset(pkt, 0, sizeof(sys_pkt_t));
    pkt->pkt_data = (uint8 *)sal_dma_malloc(cfg->gcfg.len + 4);
    if (pkt->pkt_data == NULL) {
        sal_printf("Out of memory!\n");
        return -1;
    }
    pkt->alloc_ptr = pkt->pkt_data;
    pkt->cookie = (void *)sal_malloc(sizeof(tx_async_t));
    if (pkt->cookie == NULL) {
        sal_printf("Out of memory!\n");
        return -1;
    }

    pkt->cos = cfg->int_pri;
    pkt->buf_len = cfg->gcfg.len + 4;
    pkt->pkt_len = cfg->gcfg.len;
    *packet = pkt;
    return 0;
}

int
bcma_bcmpktcmd_tx(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    int rv = BCMA_CLI_CMD_OK;
    int unit = 0;
    char *cmd;
    int count = 0;
    int port_cnt = 0;
    uint32_t hg_hdr_size = 0;
    sys_pkt_t *packet = NULL;
    bcma_bcmpkt_tx_cfg_t cfg;
    bcma_bcmpkt_pktgen_cfg_t *gcfg;
    int port_max_cnt = board_uport_count();

    sal_memset(&cfg, 0, sizeof(bcma_bcmpkt_tx_cfg_t));
    gcfg = &cfg.gcfg;

    cfg.netif_id = NETIF_DEFID;
    gcfg->pkttype = ENET_TYPE_NONE;
    gcfg->ethertype = ENET_ETHERTYPE;
    gcfg->tpid = ENET_TPID;
    gcfg->len = DEF_PKT_LEN + 4;
    gcfg->vlan = DEF_VLAN;
    gcfg->pattern = DEF_PATTERN;
    gcfg->pat_inc = DEF_PATTERN_INC;
    /* Default source MAC address 00:bc:20:00:00:00 */
    gcfg->smac[1] = 0xbc;
    gcfg->smac[2] = 0x20;
    /* Default destination MAC address 00:bc:10:00:00:00 */
    gcfg->dmac[1] = 0xbc;
    gcfg->dmac[2] = 0x10;

    if (!(cmd = BCMA_CLI_ARG_GET(args))) {
        return BCMA_CLI_CMD_USAGE;
    }

    count = sal_strtol(cmd, &cmd, 0);
    if (*cmd != 0 || count <= 0) {
        tx_cfg_buf_release(&cfg, packet);
        return rv;
    }

    rv = tx_parser(cli, args, &cfg);
    if (rv < 0) {
        tx_cfg_buf_release(&cfg, packet);
        return rv;
    }

    if (tx_packet_alloc(unit, &cfg, &packet)) {
        tx_cfg_buf_release(&cfg, packet);
        return BCMA_CLI_CMD_FAIL;
    }
    rv = tx_packet_content_fill(&cfg, packet);
    if (rv < 0) {
        tx_cfg_buf_release(&cfg, packet);
        return rv;
    }

    SHR_BITCOUNT_RANGE(cfg.portlist.pbits, port_cnt, 1, port_max_cnt);

    if (port_cnt == 0) { /* No port configure. */
        rv = tx_send(unit, &cfg, packet, hg_hdr_size, count);
        if (rv < 0) {
            tx_cfg_buf_release(&cfg, packet);
            return rv;
        }
    } else { /* Have port configure. */
        int port;
        SHR_BIT_ITER(cfg.portlist.pbits, port_max_cnt + 1, port) {
            if (port == 0) {
                continue;
            }
            uplist_clear(packet->tx_uplist);
            uplist_port_add(packet->tx_uplist, port);
            rv = tx_send(unit, &cfg, packet, hg_hdr_size, count);
            if (rv < 0) {
                tx_cfg_buf_release(&cfg, packet);
                return rv;
            }
        }
    }

    tx_cfg_buf_release(&cfg, packet);
    return BCMA_CLI_CMD_OK;
}
#endif
#endif
