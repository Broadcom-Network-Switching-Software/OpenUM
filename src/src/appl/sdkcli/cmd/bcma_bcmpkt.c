/*! \file bcma_bcmpkt.c
 *
 * General functions for Packet I/O command Lines.
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
#include <utils/shr/shr_pb_format.h>
#include <utils/shr/shr_types.h>
#include <utils/shr/shr_util.h>

#if CFG_RXTX_SUPPORT_ENABLED

#include <appl/sdkcli/bcma_cli.h>
#include <appl/sdkcli/bcma_cli_parse.h>

/*! Option of display packet on console. Enabled by default. */
static int px_watcher_console_display = 1;

/*! Convert two hex-characters to an integer */
static int
xbyte2i(int xch_hi, int xch_lo)
{
    if ((xch_hi = shr_util_xch2int(xch_hi)) < 0) {
        xch_hi = 0;
    }
    if ((xch_lo = shr_util_xch2int(xch_lo)) < 0) {
        xch_lo = 0;
    }
    return (xch_hi << 4) | xch_lo;
}

/*
 * Random pattern.
 */
static void
packet_payload_fill_random(uint8_t *buf, int size)
{
    int      pat_off = 24;
    uint32_t pat = sal_rand();

    while (size > 3) {
        *buf++ = pat >> 24;
        *buf++ = pat >> 16;
        *buf++ = pat >> 8;
        *buf++ = pat & 0xff;
        pat = sal_rand();
        size -= 4;
    }

    while (size > 0) {
        *buf++ = pat >> pat_off;
        pat_off -= 8;
        size--;
    }
}

/* Pattern increase by word. */
static uint32_t
packet_payload_fill_pattern(uint8_t *buf, int size, uint32_t pat, uint32_t inc)
{
    int pat_off = 24;

    while (size > 3) {
        *buf++ = pat >> 24;
        *buf++ = pat >> 16;
        *buf++ = pat >> 8;
        *buf++ = pat & 0xff;
        pat += inc;
        size-=4;
    }

    while (size > 0) {
        *buf++ = pat >> pat_off;
        pat_off -= 8;
        size--;
    }

    return pat;
}

void
bcma_bcmpkt_macaddr_inc(shr_mac_t macaddr, int amount)
{
    int i, v;
    for (i = 5; i >= 0; i--) {
        v = (int) macaddr[i] + amount;
        macaddr[i] = v;
        if (v >= 0 && v <= 255)
            break;
        amount = v >> 8;
    }
}

int
bcma_bcmpkt_packet_payload_fill(bcma_bcmpkt_pktgen_cfg_t *cfg,
                                sys_pkt_t *pkt)
{
    int payload_len;
    int offset = 0;
    uint8_t *filladdr;

    offset = ENET_MAC_SIZE * 2 + 2 /* Ethertype. */;

    /* Tagged. */
    if (!cfg->untagged) {
        offset += 4;
    }

    payload_len = pkt->pkt_len - offset;
    filladdr = pkt->pkt_data + offset;
    /* Fill pattern */
    if (cfg->pat_random) {
        packet_payload_fill_random(filladdr, payload_len);
    }
    else {
        cfg->pattern = packet_payload_fill_pattern(filladdr, payload_len,
                                                   cfg->pattern, cfg->pat_inc);
    }

    return 0;
}

/* Load data from input string. */
int
bcma_bcmpkt_load_data_from_istr(char *istr, uint8_t *buf, uint32_t buf_size,
                                uint32_t *data_size)
{
    uint32_t byte, chr, len;

    len = sal_strlen(istr);
    byte = 0;
    for (chr = 0; chr < (len - 1); chr += 2) {
        if (chr == 0 && istr[chr] == '0' &&
            (istr[chr + 1] == 'x' || istr[chr + 1] == 'X')) {
            chr += 2;
            if (chr > (len - 2)) {
                break;
            }
        } else if (istr[chr] == '_') {
            chr++;
            if (chr > (len - 2)) {
                break;
            }
        }
        if (!sal_isxdigit(istr[chr]) || !sal_isxdigit(istr[chr + 1])) {
            cli_out("input data error %c%c\n", istr[chr], istr[chr + 1]);
            return (SYS_ERR_PARAMETER);
        }
        buf[byte++] = xbyte2i(istr[chr], istr[chr + 1]);

        if (byte == buf_size) {
            chr += 2;
            break;
        }
    }
    if (len != chr) {
        cli_out("Input data error at character %d\n", chr + 1);
        return (SYS_ERR_PARAMETER);
    }

    *data_size = byte;
    return 0;
}

int
bcma_bcmpkt_packet_generate(bcma_bcmpkt_pktgen_cfg_t *cfg,
                            sys_pkt_t *pkt)
{
    uint8_t *filladdr;

    if (pkt->pkt_len < cfg->len) {
        cli_out("Configure length is too big\n");
        return SYS_ERR_PARAMETER;
    }

    sal_memset(pkt->pkt_data, 0, cfg->len);
    filladdr = pkt->pkt_data;
    sal_memcpy(filladdr, cfg->dmac, ENET_MAC_SIZE);
    filladdr += ENET_MAC_SIZE;
    sal_memcpy(filladdr, cfg->smac, ENET_MAC_SIZE);
    filladdr += ENET_MAC_SIZE;

    /* Tagged. */
    if (!cfg->untagged) {
        *filladdr++ = cfg->tpid >> 8;
        *filladdr++ = cfg->tpid & 0xff;
        *filladdr++ = (cfg->dei << 4) | (cfg->pcp << 5) | (cfg->vlan >> 8);
        *filladdr++ = cfg->vlan & 0xff;
    }
    *filladdr++ = cfg->ethertype >> 8;
    *filladdr++ = cfg->ethertype & 0xff;

    bcma_bcmpkt_packet_payload_fill(cfg, pkt);

    return SYS_OK;
}

int
bcma_bcmpkt_lmatch_check(const char *dst, const char *src, int size)
{
    int len;
    char *name = NULL;

    len = sal_strlen(dst);
    if (len >= size) {
        sal_alloc(size + 1, name);
        if (name != NULL) {
            sal_memcpy(name, dst, size);
            name[size] = '\0';
            if (!sal_strcasecmp(name, src)) {
                goto exit;
            }
        }
    }
    return SYS_ERR;
exit:
    sal_free(name);
    return SYS_OK;
}

void
bcma_bcmpkt_data_dump(shr_pb_t *pb, const uint8_t *data, int size)
{
    int idx;

    if (data == NULL) {
        cli_out("%s: NULL pointer\n", __func__);
        return;
    }

    if (size > 256) {
        size = 256;
    }

    for (idx = 0; idx < size; idx++) {
        if ((idx & 0xf) == 0) {
            shr_pb_printf(pb, "%04x: ", idx);
        }
        if ((idx & 0xf) == 8) {
            shr_pb_printf(pb, "- ");
        }
        shr_pb_printf(pb, "%02x ", data[idx]);
        if ((idx & 0xf) == 0xf) {
            shr_pb_printf(pb, "\n");
        }
    }
    if ((idx & 0xf) != 0) {
        shr_pb_printf(pb, "\n");
    }
}

static int
watcher_info_show(sys_pkt_t *packet,
                  watcher_data_t *wdata)
{
    shr_pb_t *pb = NULL;

    if (!(wdata->debug_mode & (WATCHER_DEBUG_SHOW_PACKET_DATA |
                                WATCHER_DEBUG_SHOW_META_DATA |
                                WATCHER_DEBUG_SHOW_RX_REASON))) {
        return SYS_OK;
    }

    pb = shr_pb_create();
    sal_printf("\nWatch information:\n");

    if (wdata->debug_mode & WATCHER_DEBUG_SHOW_META_DATA) {
        sal_printf("[RX metadata information]\n");
#ifdef CFG_CMICX_SUPPORT
        cmicx_ep_to_cpu_hdr_dump(0, "", packet->pkt_data);
#endif
    }

    if (wdata->debug_mode & WATCHER_DEBUG_SHOW_RX_REASON) {
    }

    if (wdata->debug_mode & WATCHER_DEBUG_SHOW_PACKET_DATA) {
        shr_pb_printf(pb, "Packet raw data (%d):\n", packet->pkt_len);
        bcma_bcmpkt_data_dump(pb, packet->pkt_data, packet->pkt_len);
    }

    if (px_watcher_console_display) {
        um_console_print(shr_pb_str(pb));
    }

    shr_pb_destroy(pb);
    return SYS_OK;
}

static int
watcher_packet_lpbk_tx(sys_pkt_t *packet)
{
    return SYS_OK;
}

static void
watcher_rx_rate_show(int netif_id, watcher_data_t *wdata)
{
    if (!(wdata->debug_mode & WATCHER_DEBUG_SHOW_RX_RATE)) {
        return;
    }

    wdata->rx_packets++;
    if (wdata->rx_packets == 1) {
        wdata->start_time = sal_time_usecs();
    }
    if (wdata->rx_packets == 100000) {
        uint32_t delt_ms = (sal_time_usecs() - wdata->start_time) / 1000;
         /* Show rate when speed no less than 1k pps. */
        if (delt_ms < 100000) {
            cli_out("Network interface %d receive rate is %u PPS \n",
                    netif_id, 100000000/delt_ms);
        }
        wdata->rx_packets = 0;
    }
}

static int
watcher_packet_terminate(sys_pkt_t *packet,
                         watcher_data_t *wdata, sys_rx_t *result)
{
    return SYS_OK;
}

sys_rx_t
bcma_bcmpkt_watcher(sys_pkt_t *packet, void *cookie)
{
    int rv;
    sys_rx_t rx_result = SYS_RX_NOT_HANDLED;
    watcher_data_t *wdata = (watcher_data_t *)cookie;

    if (wdata == NULL) {
        cli_out("Application contex check failed.\n");
        return rx_result;
    }

    /* Display packet Info. */
    rv = watcher_info_show(packet, wdata);
    if (rv < 0) {
        cli_out("Dump packet info failed (%d).\n", rv);
        return rx_result;
    }

    /* Loopback packet Data to TX. */
    if (wdata->debug_mode & WATCHER_DEBUG_LPBK_PACKET) {
        rx_result = SYS_RX_HANDLED;
        rv = watcher_packet_lpbk_tx(packet);
        if (rv < 0) {
            cli_out("Loopback packet to TX failed (%d).\n", rv);
            return rx_result;
        }
        return SYS_RX_HANDLED;
    }

    /* Display RX rate for every 100k packets. */
    watcher_rx_rate_show(-1, wdata);

    rv = watcher_packet_terminate(packet, wdata, &rx_result);
    if (rv < 0) {
        cli_out("Terminate packet failed (%d).\n", rv);
        return rx_result;
    }

    return rx_result;
}

void
bcma_bcmpkt_flag_set(uint32_t *flags, uint32_t this_flag, int do_set)
{
    *flags &= ~this_flag;
    if (do_set) {
        *flags |= this_flag;
    }
}

bool
bcma_bcmpkt_flag_status_get(const uint32_t flags, uint32_t this_flag)
{
    return ((flags & this_flag) != 0);
}

int
bcma_bcmpkt_chan_qmap_set(int unit, int chan_id, SHR_BITDCL *queue_bmp,
                          uint32_t num_queues)
{
    return SYS_OK;
}

int
bcma_bcmpkt_rx_watch_output_enable(int file_en, int console_en)
{
    px_watcher_console_display = console_en;
    return SYS_OK;
}
#endif
#endif
