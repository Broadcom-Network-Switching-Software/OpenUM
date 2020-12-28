/*
 * $Id: flrxtx.c,v 1.1 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#include "utils/net.h"
#include "soc/pbsmh.h"

#if CFG_RXTX_SUPPORT_ENABLED

#define CPU_PORT_INDEX              (0)

/* Max number of packet buffers we can accept */
#define SOC_MAX_RX_BUF_POSTED       (1)

/* Max time for TX to be done */
#define TX_WAIT_TIMEOUT             (800000UL)     /* in us */

/* Time for RX error detection */
#define RX_CHECK_INTERVAL           (1000000UL)    /* in us */

/* Make use of flags for packet state */
#define RX_FLAG_STATE_MASK      (3U << 8)
#define RX_FLAG_STATE_DMA       (1U << 8)
#define RX_FLAG_STATE_RECEIVED  (3U << 8)
#define RX_PKT_RECEIVED(pkt)    \
            (((pkt)->flags & RX_FLAG_STATE_MASK) == RX_FLAG_STATE_RECEIVED)
#define RX_PKT_AVAILABLE(pkt)   \
            (((pkt)->flags & RX_FLAG_STATE_MASK) == 0)

#ifdef __LINUX__
#define PKTDMA_CMC_NUM    (0)
#else
#define PKTDMA_CMC_NUM    (1)
#endif
#define PKTDMA_TX_CH      (0)
#define PKTDMA_RX_CH      (1)

#define CMIC_PCIE_SO_OFFSET                     0x10000000

#define PKTDMA_ENABLE                           (0x00000002)
#define PKTDMA_ABORT                            (0x00000004)

#define PKTDMA_ADDR_DECODE_ERR                  (0x0000001C)

/* CMIC_CMC0_SHARED_IRQ_STAT0(x) */
#define DS_CMCx_CHy_DMA_DESC_DONE(y)            (0x00000001 << (y * 4))
#define DS_CMCx_CHy_DMA_CHAIN_DONE(y)           (0x00000002 << (y * 4))
#define DS_CMCx_CHy_COALESCING_INTR(y)          (0x00000004 << (y * 4))
#define DS_CMCx_CHy_DESC_CONTROLLED_INTR(y)     (0x00000008 << (y * 4))

/* CMIC_CMC0_SHARED_IRQ_STAT_CLR0(x) */
#define DS_CMCx_CHy_DMA_DESC_DONE_CLR(y)        (0x00000001 << (y * 4))
#define DS_CMCx_CHy_DMA_CHAIN_DONE_CLR(y)       (0x00000002 << (y * 4))
#define DS_CMCx_CHy_COALESCING_INTR_CLR(y)      (0x00000004 << (y * 4))
#define DS_CMCx_CHy_DESC_CONTROLLED_INTR_CLR(y) (0x00000008 << (y * 4))

/* TX/RX enabled */
static BOOL             rxtx_initialized;

/* RX variables */
static SOC_RX_HANDLER   rx_handler;
static BOOL             rx_handler_in_intr;

static soc_rx_packet_t  *rx_packets[SOC_MAX_RX_BUF_POSTED];
static BOOL             rx_pkt_valid[SOC_MAX_RX_BUF_POSTED];
static soc_rx_packet_t  *rx_pkt_in_dma = NULL;

/* TX variables */
static soc_tx_packet_t  *tx_pkt_in_dma;
static soc_tx_packet_t  *tx_pkts_list;

static tick_t           tx_timeout_ticks;
static tick_t           tx_start_time;

#define TX_CURRENT_TIME    sal_time_usecs()
#define TX_TIMEOUT_VAL     TX_WAIT_TIMEOUT
#define TX_TIMER_EXPIRED(start, interval) \
    (sal_time_usecs() - (start) >= (interval))

#if defined(__LINUX__)
/* Use bde->l2p and bde->p2l in sal_init.c */
extern unsigned int _PTR_TO_PCI(void * addr);
extern unsigned int _PCI_TO_PTR(void * addr);
#ifdef SYSTEM_NONCOHERENT_CACHE
/* Use bde->sinval and bde->sflush */
extern int dcache_invalidate(void* addr, int length);
extern int dcache_flush(void* addr, int length);
#define CACHE_DMA_INVAL(d, l) dcache_invalidate(d, l)
#define CACHE_DMA_SYNC(d, l)  dcache_flush(d, l)
#else
#define CACHE_DMA_INVAL(d, l)
#define CACHE_DMA_SYNC(d, l)
#endif
#define PCI_TO_PTR(a)   _PCI_TO_PTR(a)
#define PTR_TO_PCI(x)   _PTR_TO_PCI(x)
#else
/*
 ARM-R5 address map:
   ATCM : 0x0        - 0x1_FFFF
   BTCM : 0x4_0000   - 0x7_FFFF
   SRAM : 0x120_0000 - 0x12F_FFFF

 CMICX address map:
   mhost0 : 0x110_0000 - 0x115_FFFF
   mhost1 : 0x116_0000 - 0x11B_FFFF
   SRAM   : 0x120_0000 - 0x12F_FFFF
*/
#ifdef CFG_TCM_DMA_ENABLED
/* TCM region is marked as UNCACHABLE */
#define CACHE_DMA_INVAL(d, l)
#define CACHE_DMA_SYNC(d, l)

#define PCI_TO_PTR(a)   _PCI_TO_PTR(a)
#define PTR_TO_PCI(x)   _PTR_TO_PCI(x)

uint32 _PCI_TO_PTR(void * x) {
    if ((uint32)x >= 0x1120000) {
       return (uint32)((uint8 *)x - 0x1120000);
    } else {
       return (uint32)((uint8 *)x - 0x1100000);
    }
}

uint32 _PTR_TO_PCI(void * x) {
    if ((uint32)x >= 0x40000) {
        return (uint32) ((uint8 *)x + 0x1120000);
    } else {
        return (uint32) ((uint8 *)x + 0x1100000);
    }
}
#else
/* Cached SRAM */
extern void dcache_invalidate(uint32 addr, uint32 len);
extern void dcache_flush(uint32 addr, uint32 len);

#define CACHE_DMA_INVAL(d, l)   \
        {   void (*funcptr)(uint32, uint32) = (void (*)(uint32, uint32))dcache_invalidate;\
            (*funcptr)((uint32)d, l); }
#define CACHE_DMA_SYNC(d, l)   \
        {   void (*funcptr)(uint32, uint32) = (void (*)(uint32, uint32))dcache_flush;\
            (*funcptr)((uint32)d, l); }
#define PCI_TO_PTR(a)   ((uint32)(a))
#define PTR_TO_PCI(x)   ((uint32)(x))
#endif /* CFG_TCM_DMA_ENABLED */
#endif /*__LINUX__*/

#define NUM_TX_DCB    2

static uint8    *tx_dcb;
static RX_DCB_t *rx_dcb;
static PBSMH_t  *pbhdr;


#ifdef CFG_DEBUGGING_INCLUDED

#define DP_FMT  "%sdata[%04x]: "        /* dump line start format */
#define DP_HDR  "%sheader[%04x]: "      /* dump header */
#define DP_BPL  16                      /* dumped bytes per line */

static void
soc_dma_ether_dump(char *pfx, uint8 *addr, int len, int offset)
{
    int         i = 0, j;

    if (addr == 0) {
        sal_printf("Bad packet ADDR!!\n");
        return;
    }

    if (len > DP_BPL && (DP_BPL & 1) == 0) {
        char    linebuf[128], *s;
        /* Show first line with MAC addresses in curly braces */
        s = linebuf;
        sal_sprintf(s, DP_FMT "{", pfx, i);
        while (*s != 0) s++;
        for (i = offset; i < offset + 6; i++) {
            sal_sprintf(s, "%02x", addr[i]);
            while (*s != 0) s++;
        }
        sal_sprintf(s, "} {");
        while (*s != 0) s++;
        for (; i < offset + 12; i++) {
            sal_sprintf(s, "%02x", addr[i]);
            while (*s != 0) s++;
        }
        sal_sprintf(s, "}");
        while (*s != 0) s++;
        for (; i < offset + DP_BPL; i += 2) {
            sal_sprintf(s, " %02x%02x", addr[i], addr[i + 1]);
            while (*s != 0) s++;
        }
        sal_printf("%s\n", linebuf);
    }

    for (; i < len; i += DP_BPL) {
        char    linebuf[128], *s;
        s = linebuf;
        sal_sprintf(s, DP_FMT, pfx, i);
        while (*s != 0) s++;
        for (j = i; j < i + DP_BPL && j < len; j++) {
            sal_sprintf(s, "%02x%s", addr[j], j & 1 ? " " : "");
            while (*s != 0) s++;
        }
        sal_printf("%s\n", linebuf);
    }
}

static void
soc_dma_dcb_dump(char *pfx, void *addr)
{
    int i;
    uint32 *p = (uint32 *)addr;

    sal_printf("%s: ", pfx);
    for (i = 0; i < 4; i++) {
        sal_printf("%08x ", *(p + i));
    }
    sal_printf("\n");
}
#endif /* CFG_DEBUGGING_INCLUDED */

static void
bcm5607x_rx_retrieve(soc_rx_packet_t *pkt)
{
    EP_TO_CPU_HEADER_t *hdr = (EP_TO_CPU_HEADER_t *)pkt->buffer;
    /* Gather information for the received packet */
    CACHE_DMA_INVAL(rx_dcb, sizeof(RX_DCB_t));
    CACHE_DMA_INVAL(pkt->buffer, pkt->buflen);

    pkt->pktlen = RX_DCB_BYTES_TRANSFERREDf_GET((*rx_dcb));
    pkt->flags = 0;

    pkt->lport = EP_TO_CPU_HEADER_SRC_PORT_NUMf_GET((*hdr));
    pkt->traffic_class = EP_TO_CPU_HEADER_OUTER_VIDf_GET((*hdr));
    pkt->timestamp = EP_TO_CPU_HEADER_TIMESTAMPf_GET((*hdr));
    pkt->timestamp_upper = EP_TO_CPU_HEADER_TIMESTAMP_UPPERf_GET((*hdr));

    pkt->buffer += EP_TO_CPU_HEADER_SIZE;
    pkt->pktlen -= EP_TO_CPU_HEADER_SIZE;

#ifdef CFG_DEBUGGING_INCLUDED
    soc_dma_dcb_dump("RX DCB", (void *)rx_dcb);
    sal_printf("pkt->lport = %d, pkt->traffic_class = %d\n", pkt->lport, pkt->traffic_class);
    soc_dma_ether_dump("EP_TO_CPU_HDR ", (uint8 *)hdr, EP_TO_CPU_HEADER_SIZE, 0);
    soc_dma_ether_dump("RX ", pkt->buffer, pkt->pktlen, 0);
#endif
}

APISTATIC void
bcm5607x_rx_refill(void)
{
    uint8 i;
    uint8 unit = 0;
    int cmc = PKTDMA_CMC_NUM;
    int ch = PKTDMA_RX_CH;
    CMIC_CMC_PKTDMA_CH_CTRLr_t ctrl;
    CMIC_CMC_PKTDMA_CH_DESC_ADDR_LOr_t desc_addr_lo;
#ifdef __LINUX__
    CMIC_CMC_PKTDMA_CH_DESC_ADDR_HIr_t desc_addr_hi;
#endif

    for(i = 0; i < SOC_MAX_RX_BUF_POSTED; i++) {
        if (rx_pkt_valid[i]) {
            soc_rx_packet_t *pkt = rx_packets[i];
            if (RX_PKT_AVAILABLE(pkt)) {
                pkt->flags |= RX_FLAG_STATE_DMA;
                pkt->buffer = pkt->alloc_ptr;
                rx_pkt_in_dma = pkt;
                RX_DCB_CLR((*rx_dcb));
                RX_DCB_ADDR_LOf_SET(*rx_dcb, PTR_TO_PCI(pkt->buffer));
#ifdef __LINUX__
                RX_DCB_ADDR_HIf_SET(*rx_dcb, CMIC_PCIE_SO_OFFSET); /* Upper 32-bit address */
#endif
                RX_DCB_BYTE_COUNTf_SET((*rx_dcb), pkt->buflen);

                CACHE_DMA_SYNC(rx_dcb, sizeof(RX_DCB_t));
                CMIC_CMC_PKTDMA_CH_DESC_ADDR_LOr_ADDRf_SET(desc_addr_lo, PTR_TO_PCI(rx_dcb));
                WRITE_CMIC_CMC_PKTDMA_CH_DESC_ADDR_LOr(unit, cmc, ch, desc_addr_lo);
#ifdef __LINUX__
                CMIC_CMC_PKTDMA_CH_DESC_ADDR_HIr_ADDRf_SET(desc_addr_hi, CMIC_PCIE_SO_OFFSET);
                WRITE_CMIC_CMC_PKTDMA_CH_DESC_ADDR_HIr(unit, cmc, ch, desc_addr_hi);
#endif
                READ_CMIC_CMC_PKTDMA_CH_CTRLr(unit, cmc, ch, ctrl);
                CMIC_CMC_PKTDMA_CH_CTRLr_DMA_ENf_SET(ctrl, 0);
                WRITE_CMIC_CMC_PKTDMA_CH_CTRLr(unit, cmc, ch, ctrl);
                CMIC_CMC_PKTDMA_CH_CTRLr_DMA_ENf_SET(ctrl, 1);
                WRITE_CMIC_CMC_PKTDMA_CH_CTRLr(unit, cmc, ch, ctrl);
                return;
            }
        }
    }

    rx_pkt_in_dma = NULL;
}

static void
bcm5607x_rx_direct(void)
{
    soc_rx_packet_t *pkt = rx_pkt_in_dma;

    if (rx_pkt_in_dma != NULL) {
        if (rxtx_initialized == FALSE) {
            /* DMA may has been stopped */
            rx_pkt_in_dma = NULL;
            return;
        }

        /* Gather packet information */
        bcm5607x_rx_retrieve(pkt);

        /* Call packet handler */
        pkt->flags &= ~RX_FLAG_STATE_MASK;

        /* Call packet handler */
        if (rx_handler != NULL) {
            /*
             * Assume we use only one buffer here. Need to change the way to
             * handle it if more than one buffer used.
             */
            rx_pkt_valid[0] = FALSE;
            rx_pkt_in_dma = NULL;
            (*rx_handler)(pkt);
        } else {
            /* Try to restart DMA */
            bcm5607x_rx_refill();
        }
    }
}

APISTATIC void
cmicx_dma_chan_abort(int _cmc, int _chan)
{
    CMIC_CMC_PKTDMA_CH_CTRLr_t ctrl;
    CMIC_CMC_SHARED_IRQ_STAT_CLR0r_t irq_stat_clr;
    CMIC_CMC_PKTDMA_CH_STATr_t dma_stat;

    READ_CMIC_CMC_PKTDMA_CH_STATr(0, _cmc, _chan, dma_stat);
    if (CMIC_CMC_PKTDMA_CH_STATr_DMA_ACTIVEf_GET(dma_stat)) {
        int i;
        READ_CMIC_CMC_PKTDMA_CH_CTRLr(0, _cmc, _chan, ctrl);
        CMIC_CMC_PKTDMA_CH_CTRLr_DMA_ENf_SET(ctrl, 1);
        WRITE_CMIC_CMC_PKTDMA_CH_CTRLr(0, _cmc, _chan, ctrl);
        CMIC_CMC_PKTDMA_CH_CTRLr_ABORT_DMAf_SET(ctrl, 1);
        WRITE_CMIC_CMC_PKTDMA_CH_CTRLr(0, _cmc, _chan, ctrl);

        /* Check active */
        for (i = 1000; i > 0; i--) {
            READ_CMIC_CMC_PKTDMA_CH_STATr(0, _cmc, _chan, dma_stat);
            if (CMIC_CMC_PKTDMA_CH_STATr_DMA_ACTIVEf_GET(dma_stat)) {
                sal_usleep(10);
            } else {
                break;
            }
        }
    }

    /* Clearing enable will also clear CHAIN_DONE */
    READ_CMIC_CMC_PKTDMA_CH_CTRLr(0, _cmc, _chan, ctrl);
    CMIC_CMC_PKTDMA_CH_CTRLr_DMA_ENf_SET(ctrl, 0);
    CMIC_CMC_PKTDMA_CH_CTRLr_ABORT_DMAf_SET(ctrl, 0);
    WRITE_CMIC_CMC_PKTDMA_CH_CTRLr(0, _cmc, _chan, ctrl);

    READ_CMIC_CMC_SHARED_IRQ_STAT_CLR0r(0, _cmc, irq_stat_clr);
    irq_stat_clr.v[0] |= DS_CMCx_CHy_DMA_CHAIN_DONE_CLR(_chan) |
                         DS_CMCx_CHy_DMA_DESC_DONE_CLR(_chan);
    WRITE_CMIC_CMC_SHARED_IRQ_STAT_CLR0r(0, _cmc, irq_stat_clr);
}

APISTATIC sys_error_t
bcm5607x_tx_internal(soc_tx_packet_t *pkt)
{
    int i, sobmh_hdr = 0;
    uint32 port;
    enet_hdr_t *th; /* Tagged header pointers */
    int cmc = PKTDMA_CMC_NUM;
    CMIC_CMC_PKTDMA_CH_CTRLr_t ctrl;
    CMIC_CMC_PKTDMA_CH_DESC_ADDR_LOr_t desc_addr_lo;
#ifdef __LINUX__
    CMIC_CMC_PKTDMA_CH_DESC_ADDR_HIr_t desc_addr_hi;
#endif
    TX_DCB_t *dcb = (TX_DCB_t *)tx_dcb;
    TX_DCB_t *dcb0 = (TX_DCB_t *)tx_dcb;
    TX_DCB_t *dcb1 = (TX_DCB_t *)(tx_dcb + TX_DCB_SIZE);

    TX_DCB_CLR(*dcb0);
    TX_DCB_CLR(*dcb1);
    PBSMH_CLR(*pbhdr);

    
    if (PBMP_NOT_NULL(pkt->port_bitmap)) {
        /* Use SOBMH mode. */
        for (port = BCM5607X_LPORT_MIN; port <= BCM5607X_LPORT_MAX; port++) {
            if (PBMP_MEMBER(pkt->port_bitmap, port)) {
                break;
            }
        }

        if (port > BCM5607X_PORT_MAX) {
            return SYS_ERR_PARAMETER;
        }

        sobmh_hdr = 1;
        PBS_MH_V7_W0_START_SET((soc_pbsmh_v7_hdr_t *)pbhdr);
        soc_pbsmh_v7_field_set(0, (soc_pbsmh_v7_hdr_t *)pbhdr,
                       PBSMH_dst_port, port);
        soc_pbsmh_v7_field_set(0, (soc_pbsmh_v7_hdr_t *)pbhdr,
                       PBSMH_unicast, 1);

        /* remove tag for untag members */
        if (PBMP_NOT_NULL(pkt->untag_bitmap)) {
            th = (enet_hdr_t *)pkt->buffer;
            if (ENET_TAGGED(th)) {
                pkt->pktlen -= 4;
                for (i = 12 ; i < pkt->pktlen ; i++) {
                    pkt->buffer[i] = pkt->buffer[i + 4];
                }
                sal_memset(&(pkt->buffer[pkt->pktlen]), 0, 4);
            }
        }
    }

    if (sobmh_hdr) {
        TX_DCB_ADDR_LOf_SET(*dcb0, PTR_TO_PCI(pbhdr));
#ifdef __LINUX__
        TX_DCB_ADDR_HIf_SET(*dcb0, CMIC_PCIE_SO_OFFSET); /* Upper 32-bit address */
#endif
        TX_DCB_BYTE_COUNTf_SET(*dcb0, PBSMH_SIZE);
        TX_DCB_HGf_SET(*dcb0, 1);
        TX_DCB_CHAINf_SET(*dcb0, 1);
        TX_DCB_SGf_SET(*dcb0, 1);

        TX_DCB_HGf_SET(*dcb1, 1);
    }

    TX_DCB_ADDR_LOf_SET(*dcb1, PTR_TO_PCI(pkt->buffer));
#ifdef __LINUX__
    TX_DCB_ADDR_HIf_SET(*dcb1, CMIC_PCIE_SO_OFFSET); /* Upper 32-bit address */
#endif
    TX_DCB_BYTE_COUNTf_SET(*dcb1, pkt->pktlen);

#ifdef CFG_DEBUGGING_INCLUDED
    for (i = 0; i < NUM_TX_DCB; i++) {
        soc_dma_dcb_dump("TX DCB", (i == 0)? (void *)dcb0: (void *)dcb1);
        soc_dma_ether_dump("TX ", (i == 0)? (uint8 *)pbhdr: pkt->buffer,
                                   (i == 0)? PBSMH_SIZE: pkt->pktlen, 0);
    }
    sal_printf("\n");
#endif /* CFG_DEBUGGING_INCLUDED */

    CACHE_DMA_SYNC(pkt->buffer, pkt->pktlen);
    CACHE_DMA_SYNC(tx_dcb[0], sizeof(TX_DCB_t) * 2);
    CACHE_DMA_SYNC(pbhdr, PBSMH_SIZE);

    if (!sobmh_hdr) {
        dcb = dcb1;
    }

#ifdef CFG_DEBUGGING_INCLUDED
    sal_printf("%s:PTR_TO_PCI(dcb) is 0x%x\n", __func__, PTR_TO_PCI(dcb));
#endif

    CMIC_CMC_PKTDMA_CH_DESC_ADDR_LOr_ADDRf_SET(desc_addr_lo, PTR_TO_PCI(dcb));
    WRITE_CMIC_CMC_PKTDMA_CH_DESC_ADDR_LOr(0, cmc, PKTDMA_TX_CH, desc_addr_lo);
#ifdef __LINUX__
    CMIC_CMC_PKTDMA_CH_DESC_ADDR_HIr_ADDRf_SET(desc_addr_hi, CMIC_PCIE_SO_OFFSET);
    WRITE_CMIC_CMC_PKTDMA_CH_DESC_ADDR_HIr(0, cmc, PKTDMA_TX_CH, desc_addr_hi);
#endif

    READ_CMIC_CMC_PKTDMA_CH_CTRLr(0, cmc, PKTDMA_TX_CH, ctrl);
    CMIC_CMC_PKTDMA_CH_CTRLr_DMA_ENf_SET(ctrl, 1);
    WRITE_CMIC_CMC_PKTDMA_CH_CTRLr(0, cmc, PKTDMA_TX_CH, ctrl);

    tx_start_time = TX_CURRENT_TIME;
    tx_pkt_in_dma = pkt;

    return SYS_OK;
}

APISTATIC bool
soc_dma_desc_done(uint8 unit, int cmc, int ch)
{
    CMIC_CMC_PKTDMA_CH_CTRLr_t ctrl;
    CMIC_CMC_SHARED_IRQ_STAT0r_t irq_stat;
    CMIC_CMC_SHARED_IRQ_STAT_CLR0r_t irq_stat_clr;
    CMIC_CMC_PKTDMA_CH_STATr_t dma_stat;
    CMIC_CMC_PKTDMA_CH_DESC_ADDR_LOr_t desc_addr_lo;

    READ_CMIC_CMC_SHARED_IRQ_STAT0r(unit, cmc, irq_stat);

    if (irq_stat.v[0] & DS_CMCx_CHy_DMA_CHAIN_DONE(ch)) {
        READ_CMIC_CMC_PKTDMA_CH_STATr(unit, cmc, ch, dma_stat);
        if (dma_stat.v[0] & PKTDMA_ADDR_DECODE_ERR) {
            sal_printf("ERROR %s: ch %d irq_stat = 0x%x, dma_stat = 0x%x\n", __FUNCTION__, ch, irq_stat.v[0], dma_stat.v[0]);
            READ_CMIC_CMC_PKTDMA_CH_DESC_ADDR_LOr(unit, cmc, ch, desc_addr_lo);
            sal_printf("%s: desc_addr_lo = 0x%x\n", __FUNCTION__, desc_addr_lo.v[0]);
            cmicx_dma_chan_abort(cmc, ch);
            return false;
        }
#ifdef CFG_DEBUGGING_INCLUDED
        sal_printf("%s: ch %d stat = 0x%x, dma_stat = 0x%x\n", __FUNCTION__, ch, irq_stat.v[0], dma_stat.v[0]);
#endif
        /* Clear bits. */
        CMIC_CMC_SHARED_IRQ_STAT_CLR0r_CLR(irq_stat_clr);
        irq_stat_clr.v[0] |= (DS_CMCx_CHy_DMA_DESC_DONE_CLR(ch) |
                              DS_CMCx_CHy_DMA_CHAIN_DONE(ch));
        WRITE_CMIC_CMC_SHARED_IRQ_STAT_CLR0r(unit, cmc, irq_stat_clr);

        READ_CMIC_CMC_PKTDMA_CH_CTRLr(unit, cmc, ch, ctrl);
        CMIC_CMC_PKTDMA_CH_CTRLr_DMA_ENf_SET(ctrl, 0);
        WRITE_CMIC_CMC_PKTDMA_CH_CTRLr(unit, cmc, ch, ctrl);

        return true;
    }
    return false;
}

APISTATIC void
bcm5607x_rxtx_task(void* data)
{
    int cmc = PKTDMA_CMC_NUM;

    /* RX polling mode */
    if (rx_pkt_in_dma != NULL) {
        if (soc_dma_desc_done(0, cmc, PKTDMA_RX_CH)) {
            bcm5607x_rx_direct();
        }
    }

    /* TX polling mode */
    if (tx_pkt_in_dma != NULL) {
        soc_tx_packet_t *pkt;
        
        if (soc_dma_desc_done(0, cmc, PKTDMA_TX_CH)) {
            tx_pkt_in_dma->status = SYS_OK;
        }
        else if (TX_TIMER_EXPIRED(tx_start_time, tx_timeout_ticks)) {
            sal_printf("Error %s: tx time out!\n", __func__);
            /* Timeout, should abort this. */
            tx_pkt_in_dma->status = SYS_ERR_TIMEOUT;
            cmicx_dma_chan_abort(cmc, PKTDMA_TX_CH);
        }
        else {
            /* Haven't completed */
            return;
        }

        /* Send next in queue (if any) */
        pkt = tx_pkt_in_dma;
        tx_pkt_in_dma = NULL;
        if (tx_pkts_list && rxtx_initialized) {
            soc_tx_packet_t *ppkt = tx_pkts_list;
            tx_pkts_list = ppkt->next;
            bcm5607x_tx_internal(ppkt);
        }

        /* Notify tx caller */
        (*(pkt->callback))(pkt);
    }
}

void
bcm5607x_rxtx_stop(void)
{
    int cmc = PKTDMA_CMC_NUM;
    pbmp_t pbmp;
    EPC_LINK_BMAP_LO_64r_t epc_link_bmap_lo_64;
    CMIC_TOP_EPINTF_RELEASE_ALL_CREDITSr_t ep_release_all_credits;

    rxtx_initialized = FALSE;

    /* make CPU epc link clear */
    READ_EPC_LINK_BMAP_LO_64r(0, epc_link_bmap_lo_64);
    EPC_LINK_BMAP_LO_64r_PORT_BITMAPf_GET(epc_link_bmap_lo_64, &PBMP_WORD_GET(pbmp, 0));
    PBMP_PORT_REMOVE(pbmp, 0);
    EPC_LINK_BMAP_LO_64r_PORT_BITMAPf_SET(epc_link_bmap_lo_64, &PBMP_WORD_GET(pbmp, 0));
    WRITE_EPC_LINK_BMAP_LO_64r(0, epc_link_bmap_lo_64);

    CMIC_TOP_EPINTF_RELEASE_ALL_CREDITSr_SET(ep_release_all_credits, 0);
    WRITE_CMIC_TOP_EPINTF_RELEASE_ALL_CREDITSr(0, ep_release_all_credits);

    /* Abort TX DMA */
    cmicx_dma_chan_abort(cmc, PKTDMA_TX_CH);
    /* Abort RX DMA */
    cmicx_dma_chan_abort(cmc, PKTDMA_RX_CH);
}

sys_error_t
bcm5607x_tx(uint8 unit, soc_tx_packet_t *pkt)
{
    SAL_ASSERT(pkt != NULL && pkt->callback != NULL && pkt->buffer != NULL);
    if (pkt == NULL || pkt->callback == NULL || pkt->buffer == NULL) {
        return SYS_ERR_PARAMETER;
    }
    if (pkt->pktlen == 0 || unit > 0) {
        return SYS_ERR_PARAMETER;
    }

    pkt->unit = 0;

    if (tx_pkt_in_dma == NULL) {
        bcm5607x_tx_internal(pkt);
    } else {
        /* DMA in progress, queue this packet */
        pkt->next = tx_pkts_list;
        tx_pkts_list = pkt;
    }
    return SYS_OK;
}

sys_error_t
bcm5607x_rx_set_handler(uint8 unit, SOC_RX_HANDLER fn, BOOL intr)
{
    if (fn == NULL || unit > 0) {
        /* XXX: should allow to remove current handler (and do RX reset) */
        return SYS_ERR_PARAMETER;
    }
    if (rxtx_initialized == FALSE) {
        return SYS_ERR_STATE;
    }

    /* XXX: currently it can only be set once (and can't change) */
    SAL_ASSERT(rx_handler == NULL);
    if (rx_handler != NULL) {
        return SYS_ERR_STATE;
    }

    rx_handler = fn;
    rx_handler_in_intr = intr;

    return SYS_OK;
}

sys_error_t
bcm5607x_rx_fill_buffer(uint8 unit, soc_rx_packet_t *pkt)
{
    uint8 i;
    int cmc = PKTDMA_CMC_NUM;
    int ch = PKTDMA_RX_CH;
    CMIC_CMC_PKTDMA_CH_CTRLr_t ctrl;
    CMIC_CMC_PKTDMA_CH_DESC_ADDR_LOr_t desc_addr_lo;
#ifdef __LINUX__
    CMIC_CMC_PKTDMA_CH_DESC_ADDR_HIr_t desc_addr_hi;
#endif

    if (pkt == NULL || pkt->buffer == NULL || pkt->buflen == 0 || unit > 0) {
        return SYS_ERR_PARAMETER;
    }
    if (rxtx_initialized == FALSE) {
        return SYS_ERR_STATE;
    }

    if (rx_handler == NULL) {
        return SYS_ERR_STATE;
    }

    for(i=0; i<SOC_MAX_RX_BUF_POSTED; i++) {
        if (rx_pkt_valid[i] == FALSE) {
            pkt->unit = 0;
            pkt->flags = 0;
            rx_packets[i] = pkt;
            rx_pkt_valid[i] = TRUE;
            if (rx_pkt_in_dma == NULL) {
                pkt->flags |= RX_FLAG_STATE_DMA;
                rx_pkt_in_dma = pkt;
                RX_DCB_CLR((*rx_dcb));
                RX_DCB_ADDR_LOf_SET(*rx_dcb, PTR_TO_PCI(pkt->buffer));
#ifdef __LINUX__
                RX_DCB_ADDR_HIf_SET(*rx_dcb, CMIC_PCIE_SO_OFFSET); /* Upper 32-bit address */
#endif
                RX_DCB_BYTE_COUNTf_SET((*rx_dcb), pkt->buflen);

#ifdef CFG_DEBUGGING_INCLUDED
                soc_dma_dcb_dump("RX DCB", (void *)rx_dcb);
#endif
                CACHE_DMA_SYNC(rx_dcb, sizeof(RX_DCB_t));
                CMIC_CMC_PKTDMA_CH_DESC_ADDR_LOr_ADDRf_SET(desc_addr_lo, PTR_TO_PCI(rx_dcb));
                WRITE_CMIC_CMC_PKTDMA_CH_DESC_ADDR_LOr(unit, cmc, ch, desc_addr_lo);
#ifdef __LINUX__
                CMIC_CMC_PKTDMA_CH_DESC_ADDR_HIr_ADDRf_SET(desc_addr_hi, CMIC_PCIE_SO_OFFSET);
                WRITE_CMIC_CMC_PKTDMA_CH_DESC_ADDR_HIr(unit, cmc, ch, desc_addr_hi);
#endif
                READ_CMIC_CMC_PKTDMA_CH_CTRLr(unit, cmc, ch, ctrl);
                CMIC_CMC_PKTDMA_CH_CTRLr_DMA_ENf_SET(ctrl, 1);
                WRITE_CMIC_CMC_PKTDMA_CH_CTRLr(unit, cmc, ch, ctrl);
            }
            return SYS_OK;
        }
    }

    return SYS_ERR_FULL;
}

void
bcm5607x_rxtx_init(void)
{
    uint8 unit = 0;
    int i, cmc = PKTDMA_CMC_NUM;
    pbmp_t pbmp;
    CMIC_CMC_PKTDMA_CH_CTRLr_t ctrl;
    CMIC_CMC_PKTDMA_CH_COS_CTRL_RX_0r_t cos_ctrl_rx_0;
    EPC_LINK_BMAP_LO_64r_t epc_link_bmap_lo_64;
    CMIC_TOP_EPINTF_MAX_INTERFACE_CREDITSr_t ep_max_interface_credits;
    CMIC_TOP_EPINTF_RELEASE_ALL_CREDITSr_t ep_release_all_credits;
    IP_TO_CMICM_CREDIT_TRANSFERr_t ip_to_cmicm_credit_transfer;
    EGR_ENABLEm_t egr_enable;
    CMIC_TOP_CONFIGr_t cmic_top_config;

    /* Lazy initialization */
    if (rxtx_initialized) {
        return;
    }

    bcm5607x_rxtx_stop();

    rxtx_initialized = TRUE;

#ifdef __LINUX__
#if 0 
    /* Enable Hot Swap manager to handle CPU hot swap or Warmboot case */
    SOC_IF_ERROR_RETURN(READ_PAXB_0_PAXB_HOTSWAP_CTRLr(unit, &rval));
    soc_reg_field_set(unit, PAXB_0_PAXB_HOTSWAP_CTRLr, &rval, ENABLEf, 1);
    SOC_IF_ERROR_RETURN(WRITE_PAXB_0_PAXB_HOTSWAP_CTRLr(unit, rval));

    /* Disable iProc reset on PCie link down event */
    SOC_IF_ERROR_RETURN(WRITE_PAXB_0_RESET_ENABLE_IN_PCIE_LINK_DOWNr(unit, 0x0));
#endif
#endif

    READ_CMIC_CMC_PKTDMA_CH_CTRLr(unit, cmc, PKTDMA_TX_CH, ctrl);
    CMIC_CMC_PKTDMA_CH_CTRLr_PKTDMA_ENDIANESSf_SET(ctrl, CFG_LITTLE_ENDIAN ? 0 : 1);
    CMIC_CMC_PKTDMA_CH_CTRLr_DIRECTIONf_SET(ctrl, 1);
    WRITE_CMIC_CMC_PKTDMA_CH_CTRLr(unit, cmc, PKTDMA_TX_CH, ctrl);
    CMIC_CMC_PKTDMA_CH_CTRLr_DIRECTIONf_SET(ctrl, 0);
    WRITE_CMIC_CMC_PKTDMA_CH_CTRLr(unit, cmc, PKTDMA_RX_CH, ctrl);

    /*
     * This has to be done only once after both CMIC and EP blocks
     * are out of reset.
     */

    /* Enable CMIC to release available credits to EP */
    READ_CMIC_TOP_EPINTF_MAX_INTERFACE_CREDITSr(unit, ep_max_interface_credits);
    CMIC_TOP_EPINTF_MAX_INTERFACE_CREDITSr_MAX_CREDITSf_SET(ep_max_interface_credits, 0x20);
    WRITE_CMIC_TOP_EPINTF_MAX_INTERFACE_CREDITSr(unit, ep_max_interface_credits);
    CMIC_TOP_EPINTF_MAX_INTERFACE_CREDITSr_WR_EP_INTF_CREDITSf_SET(ep_max_interface_credits, 1);
    WRITE_CMIC_TOP_EPINTF_MAX_INTERFACE_CREDITSr(unit, ep_max_interface_credits);

    CMIC_TOP_EPINTF_RELEASE_ALL_CREDITSr_SET(ep_release_all_credits, 0);
    WRITE_CMIC_TOP_EPINTF_RELEASE_ALL_CREDITSr(unit, ep_release_all_credits);

    CMIC_TOP_EPINTF_RELEASE_ALL_CREDITSr_SET(ep_release_all_credits, 1);
    WRITE_CMIC_TOP_EPINTF_RELEASE_ALL_CREDITSr(unit, ep_release_all_credits);

    /* Map all COS queues to default Rx DMA channel (1) */
    CMIC_CMC_PKTDMA_CH_COS_CTRL_RX_0r_COS_BMPf_SET(cos_ctrl_rx_0, 0xFF);
    WRITE_CMIC_CMC_PKTDMA_CH_COS_CTRL_RX_0r(unit, cmc, PKTDMA_RX_CH, cos_ctrl_rx_0);

    /* Enable CPU port to receive packets. */
    READ_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64);
    EPC_LINK_BMAP_LO_64r_PORT_BITMAPf_GET(epc_link_bmap_lo_64, &PBMP_WORD_GET(pbmp, 0));
    PBMP_PORT_ADD(pbmp, CPU_PORT_INDEX);
    EPC_LINK_BMAP_LO_64r_PORT_BITMAPf_SET(epc_link_bmap_lo_64, &PBMP_WORD_GET(pbmp, 0));
    WRITE_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64);

    READ_EGR_ENABLEm(unit, CPU_PORT_INDEX, egr_enable);
    EGR_ENABLEm_PRT_ENABLEf_SET(egr_enable, 1);
    WRITE_EGR_ENABLEm(unit, CPU_PORT_INDEX, egr_enable);

    tx_dcb = sal_dma_malloc(2*sizeof(TX_DCB_t));
    rx_dcb = sal_dma_malloc(sizeof(RX_DCB_t));
    pbhdr  = sal_dma_malloc(sizeof(PBSMH_t));

    /*
     * Enable IP to CMICM credit transfer, should be done in MMU init?
     */
    IP_TO_CMICM_CREDIT_TRANSFERr_CLR(ip_to_cmicm_credit_transfer);
    IP_TO_CMICM_CREDIT_TRANSFERr_NUM_OF_CREDITSf_SET(ip_to_cmicm_credit_transfer, 32);
    IP_TO_CMICM_CREDIT_TRANSFERr_TRANSFER_ENABLEf_SET(ip_to_cmicm_credit_transfer, 1);
    WRITE_IP_TO_CMICM_CREDIT_TRANSFERr(unit, ip_to_cmicm_credit_transfer);

    READ_CMIC_TOP_CONFIGr(unit, cmic_top_config);
    CMIC_TOP_CONFIGr_IP_INTERFACE_HEADER_ENDIANESSf_SET(cmic_top_config, 1);
    WRITE_CMIC_TOP_CONFIGr(unit, cmic_top_config);

#if CFG_CONSOLE_ENABLED
    sal_printf("TX/RX support enabled.\n");
#endif /* CFG_CONSOLE_ENABLED */

    /* Initialize RX global variables */
    rx_handler = NULL;
    rx_handler_in_intr = FALSE;
    for(i=0; i<SOC_MAX_RX_BUF_POSTED; i++) {
        rx_pkt_valid[i] = FALSE;
    }
    rx_pkt_in_dma = NULL;

    /* Register a background task for RX handling */
    task_add(bcm5607x_rxtx_task, (void *)NULL);

    /* Initialize TX global variables */
    tx_pkt_in_dma = NULL;
    tx_pkts_list = NULL;

    tx_timeout_ticks = TX_TIMEOUT_VAL;
}

void
bcm5607x_ep_to_cpu_hdr_dump(uint8 unit, char *prefix, void *addr)
{
    EP_TO_CPU_HEADER_t *hdr = (EP_TO_CPU_HEADER_t *)(addr - EP_TO_CPU_HEADER_SIZE);

    sal_printf("%s  %sdo_not_change_ttl %sbpdu %sl3routed %schg_tos %semirror %simirror\n",
        prefix,
        EP_TO_CPU_HEADER_DO_NOT_CHANGE_TTLf_GET(*hdr) ? "" : "!",
        EP_TO_CPU_HEADER_BPDUf_GET(*hdr) ? "" : "!",
        EP_TO_CPU_HEADER_IP_ROUTEDf_GET(*hdr) ? "" : "!",
        EP_TO_CPU_HEADER_CHANGE_DSCPf_GET(*hdr) ? "" : "!",
        EP_TO_CPU_HEADER_EMIRRORf_GET(*hdr) ? "" : "!",
        EP_TO_CPU_HEADER_IMIRRORf_GET(*hdr) ? "" : "!"
        );
    sal_printf("%s  %sreplicated %sl3only %soam_pkt %ssrc_hg\n",
        prefix,
        EP_TO_CPU_HEADER_REPLICATIONf_GET(*hdr) ? "" : "!",
        EP_TO_CPU_HEADER_L3ONLYf_GET(*hdr) ? "" : "!",
        EP_TO_CPU_HEADER_SPECIAL_PACKET_INDICATORf_GET(*hdr) ? "" : "!",
        EP_TO_CPU_HEADER_SRC_HIGIGf_GET(*hdr) ? "" : "!"
        );
    sal_printf("%s  %sswitch_pkt %sregen_crc  %sservice_tag %sing_untagged\n",
        prefix,
        EP_TO_CPU_HEADER_SWITCHf_GET(*hdr) ? "" : "!",
        EP_TO_CPU_HEADER_REGEN_CRCf_GET(*hdr) ? "" : "!",
        EP_TO_CPU_HEADER_SD_TAG_PRESENTf_GET(*hdr) ? "" : "!",
        (EP_TO_CPU_HEADER_INCOMING_TAG_STATUSf_GET(*hdr) == 0) ? "" : "!"
        );
    sal_printf("%s  %svfi_valid %schg_ecn ecn=%d\n",
        prefix,
        EP_TO_CPU_HEADER_VFI_VALIDf_GET(*hdr) ? "" : "!",
        EP_TO_CPU_HEADER_CHANGE_ECNf_GET(*hdr) ? "" : "!",
        EP_TO_CPU_HEADER_ECNf_GET(*hdr)
        );
    sal_printf("%s  cpu_cos=%d cos=%d im_mtp_index=%d reason=%08x_%08x\n",
        prefix,
        EP_TO_CPU_HEADER_CPU_COSf_GET(*hdr),
        EP_TO_CPU_HEADER_COSf_GET(*hdr),
        EP_TO_CPU_HEADER_IM_MTP_INDEXf_GET(*hdr),
        hdr->ep_to_cpu_header[4],
        hdr->ep_to_cpu_header[5]
        );
    sal_printf("%s  reason_type=%d match_rule=%d hg_type=%d em_mtp_index=%d\n",
        prefix,
        EP_TO_CPU_HEADER_CPU_OPCODE_TYPEf_GET(*hdr),
        EP_TO_CPU_HEADER_MATCHED_RULEf_GET(*hdr),
        EP_TO_CPU_HEADER_SRC_HIGIG_TYPEf_GET(*hdr),
        EP_TO_CPU_HEADER_EM_MTP_INDEXf_GET(*hdr)
        );
    sal_printf("%s  srcport=%d dscp=%d outer_pri=%d outer_cfi=%d outer_vid=%d\n",
        prefix,
        EP_TO_CPU_HEADER_SRC_PORT_NUMf_GET(*hdr),
        EP_TO_CPU_HEADER_DSCPf_GET(*hdr),
        EP_TO_CPU_HEADER_OUTER_PRIf_GET(*hdr),
        EP_TO_CPU_HEADER_OUTER_CFIf_GET(*hdr),
        EP_TO_CPU_HEADER_OUTER_VIDf_GET(*hdr)
        );
    sal_printf("%s  inner_pri=%d inner_cfi=%d inner_vid=%d\n",
        prefix,
        EP_TO_CPU_HEADER_INNER_PRIf_GET(*hdr),
        EP_TO_CPU_HEADER_INNER_CFIf_GET(*hdr),
        EP_TO_CPU_HEADER_INNER_VIDf_GET(*hdr)
        );
    sal_printf("%s  hgi=%d itag_status=%d otag_action=%d itag_action=%d\n",
        prefix,
        EP_TO_CPU_HEADER_HGIf_GET(*hdr),
        EP_TO_CPU_HEADER_INCOMING_TAG_STATUSf_GET(*hdr),
        EP_TO_CPU_HEADER_ING_OTAG_ACTIONf_GET(*hdr),
        EP_TO_CPU_HEADER_ING_ITAG_ACTIONf_GET(*hdr)
        );
    sal_printf("%s  repl_nhi=%05x ts_type=%d timestamp=%08x_%08x\n",
        prefix,
        EP_TO_CPU_HEADER_REPLICATION_OR_NHOP_INDEXf_GET(*hdr),
        EP_TO_CPU_HEADER_TIMESTAMP_TYPEf_GET(*hdr),
        EP_TO_CPU_HEADER_TIMESTAMP_UPPERf_GET(*hdr),
        EP_TO_CPU_HEADER_TIMESTAMPf_GET(*hdr)
        );
    sal_printf("%s  vfi=%d vntag_action=%d\n",
        prefix,
        EP_TO_CPU_HEADER_VFI_4_0f_GET(*hdr) +
        (EP_TO_CPU_HEADER_VFI_9_5f_GET(*hdr) << 5) +
        (EP_TO_CPU_HEADER_VFI_11_10f_GET(*hdr) << 10),
        EP_TO_CPU_HEADER_VNTAG_ACTIONf_GET(*hdr)
        );
}
#endif /* CFG_RXTX_SUPPORT_ENABLED */
