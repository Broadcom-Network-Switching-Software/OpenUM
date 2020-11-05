/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#include "utils/net.h"

#define DEBUG 0

#if DEBUG 
#define LOCAL_DEBUG_PRINTF(args ...)  do { sal_printf("%s %d:", __FUNCTION__, __LINE__); sal_printf(args); } while(0)
#else
#define LOCAL_DEBUG_PRINTF(args ...)  
#endif

#if CFG_RXTX_SUPPORT_ENABLED

#define RXTX_DEBUG                  (0)

#define CPU_PORT_INDEX              (0)

/* Max number of packet buffers we can accept */
#define SOC_MAX_RX_BUF_POSTED       (1)

/* Max time for TX to be done */
#if (CFG_UM_BCMSIM==1) || (CONFIG_EMULATION==1)
#define TX_WAIT_TIMEOUT             (2000000UL)     /* in us */
#else
#define TX_WAIT_TIMEOUT             (800000UL)     /* in us */
#endif

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

#if defined(__LINUX__)
extern int dcache_invalidate(void* addr, int length);
extern int dcache_flush(void* addr, int length);
extern unsigned int _PTR_TO_PCI(void * addr);
extern unsigned int _PCI_TO_PTR(void * addr);
#define CACHE_DMA_INVAL(d, l) dcache_invalidate(d, l)
#define CACHE_DMA_SYNC(d, l)  dcache_flush(d, l)
#define PCI_TO_PTR(a)   _PCI_TO_PTR(a)
#define PTR_TO_PCI(x)   _PTR_TO_PCI(x)

#else
#define CFG_TCM_DMA_ENABLED 1
#ifdef CFG_TCM_DMA_ENABLED
#define CACHE_DMA_INVAL(d, l)
#define CACHE_DMA_SYNC(d, l)

/* 
   CMICD map ATCM at 0x100_0000 and BTCM at 0x108_0000 
   whereas ATCM is based at 0x0000 and BTCM is based at 0x4_0000 
*/
#define PCI_TO_PTR(a)   _PCI_TO_PTR(a)
#define PTR_TO_PCI(x)   _PTR_TO_PCI(x)
uint32 _PCI_TO_PTR(void * x) {       
    return (uint32) x;
}

uint32 _PTR_TO_PCI(void * x) {
    return (uint32) x;
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


static TX_DCB_t *tx_dcb = NULL;
static RX_DCB_t *rx_dcb = NULL; 

/* Forwards */
static void bcm5354x_rx_direct(void);
static void bcm5354x_rx_refill(void);
static void bcm5354x_rx_retrieve(soc_rx_packet_t *pkt);
static void bcm5354x_rxtx_task(void *data);
static sys_error_t bcm5354x_tx_internal(soc_tx_packet_t *pkt);


static void
bcm5354x_rx_direct(void)
{
    soc_rx_packet_t *pkt = rx_pkt_in_dma;

    if (rx_pkt_in_dma != NULL) {
        if (rxtx_initialized == FALSE) {
            /* DMA may has been stopped */
            rx_pkt_in_dma = NULL;
            return;
        }

        /* Gather packet information */
        bcm5354x_rx_retrieve(pkt);

        /* Call packet handler */
        pkt->flags &= ~RX_FLAG_STATE_MASK;

        /* Call packet handler */
        if (rx_handler != NULL) {
            /* 
             * Assuem we use only one buffer here. Need to change the way to
             * handle it if more than one buffer used.
             */
            rx_pkt_valid[0] = FALSE;
            rx_pkt_in_dma = NULL;
            (*rx_handler)(pkt);
        } else {
            /* Try to restart DMA */
            bcm5354x_rx_refill();
        }
    }
}

APISTATIC void
bcm5354x_rx_refill(void)
{
    uint8 i;
    uint8 unit = 0;
    CMIC_CMC0_DMA_DESCr_t cmic_cmc1_dma_desc;
    CMIC_CMC0_CHX_DMA_CTRLr_t cmic_cmc1_chx_dma_ctrl;    
    for(i=0; i<SOC_MAX_RX_BUF_POSTED; i++) {
        if (rx_pkt_valid[i]) {
            soc_rx_packet_t *pkt = rx_packets[i];
            if (RX_PKT_AVAILABLE(pkt)) {
                pkt->flags |= RX_FLAG_STATE_DMA;
                rx_pkt_in_dma = pkt;
                RX_DCB_CLR((*rx_dcb));
                RX_DCB_MEM_ADDRf_SET((*rx_dcb), PTR_TO_PCI(pkt->buffer));
                RX_DCB_BYTE_COUNTf_SET((*rx_dcb), pkt->buflen);
#if RXTX_DEBUG
                sal_printf("dcb:buf addr = 0x%08x:0x%08x\n", rx_dcb, PTR_TO_PCI(pkt->buffer));
#endif
                CACHE_DMA_SYNC(rx_dcb, sizeof(RX_DCB_t));
                CMIC_CMC0_DMA_DESCr_CLR(cmic_cmc1_dma_desc);
                CMIC_CMC0_DMA_DESCr_ADDRf_SET(cmic_cmc1_dma_desc, PTR_TO_PCI(rx_dcb));
                WRITE_CMIC_CMC0_DMA_DESCr(unit, RX_CH1, cmic_cmc1_dma_desc);

                READ_CMIC_CMC0_CHX_DMA_CTRLr(unit, RX_CH1, cmic_cmc1_chx_dma_ctrl);
                CMIC_CMC0_CHX_DMA_CTRLr_DMA_ENf_SET(cmic_cmc1_chx_dma_ctrl, 1);
                WRITE_CMIC_CMC0_CHX_DMA_CTRLr(unit, RX_CH1, cmic_cmc1_chx_dma_ctrl);
                return;
            }
        }
    }
    
    rx_pkt_in_dma = NULL;
}

APISTATIC void
bcm5354x_rx_retrieve(soc_rx_packet_t *pkt)
{
    /* Gather information for the received packet */
    CACHE_DMA_INVAL(rx_dcb, sizeof(RX_DCB_t));
    CACHE_DMA_INVAL(pkt->buffer, pkt->buflen);

    pkt->pktlen = RX_DCB_BYTES_TRANSFERREDf_GET((*rx_dcb));
    pkt->flags = 0;
#if RXTX_DEBUG
    sal_printf("\n0x%08x-%08x-%08x-%08x\n", rx_dcb->v[5], rx_dcb->v[6], rx_dcb->v[7], rx_dcb->v[8]);
#endif
    pkt->lport = RX_DCB_EP_TO_CPU_HDR_SRC_PORT_NUMf_GET((*rx_dcb));
    pkt->traffic_class = RX_DCB_EP_TO_CPU_HDR_OUTER_VIDf_GET((*rx_dcb)); 
    pkt->timestamp = RX_DCB_EP_TO_CPU_HDR_TIMESTAMPf_GET((*rx_dcb));
}

APISTATIC void
bcm5354x_rxtx_task(void* data)
{


    CMIC_CMC0_DMA_STATr_t cmic_cmc1_dma_stat;
    CMIC_CMC0_CHX_DMA_CTRLr_t cmic_cmc1_chx_dma_ctrl;
    CMIC_CMC0_DMA_STAT_CLRr_t cmic_cmc1_dma_stat_clr;

    READ_CMIC_CMC0_DMA_STATr(0, cmic_cmc1_dma_stat);
    /* RX polling mode */
    if (rx_pkt_in_dma != NULL) {
        if (CMIC_CMC0_DMA_STATr_CH1_CHAIN_DONEf_GET(cmic_cmc1_dma_stat)) {

            /* Clear bits. */
            READ_CMIC_CMC0_CHX_DMA_CTRLr(0, RX_CH1, cmic_cmc1_chx_dma_ctrl);
            CMIC_CMC0_CHX_DMA_CTRLr_DMA_ENf_SET(cmic_cmc1_chx_dma_ctrl, 0);
            WRITE_CMIC_CMC0_CHX_DMA_CTRLr(0, RX_CH1, cmic_cmc1_chx_dma_ctrl);

            READ_CMIC_CMC0_DMA_STAT_CLRr(0,  cmic_cmc1_dma_stat_clr);
            CMIC_CMC0_DMA_STAT_CLRr_CH1_DESCRD_CMPLT_CLRf_SET(cmic_cmc1_dma_stat_clr, 1);
            WRITE_CMIC_CMC0_DMA_STAT_CLRr(0,  cmic_cmc1_dma_stat_clr);
            
            bcm5354x_rx_direct();
        }
    }

    /* TX */
    if (tx_pkt_in_dma != NULL) {
        soc_tx_packet_t *pkt;
        if (CMIC_CMC0_DMA_STATr_CH0_CHAIN_DONEf_GET(cmic_cmc1_dma_stat)) {
            /* Clear bits. */
            READ_CMIC_CMC0_CHX_DMA_CTRLr(0, TX_CH, cmic_cmc1_chx_dma_ctrl);
            CMIC_CMC0_CHX_DMA_CTRLr_DMA_ENf_SET(cmic_cmc1_chx_dma_ctrl, 0);
            WRITE_CMIC_CMC0_CHX_DMA_CTRLr(0, TX_CH, cmic_cmc1_chx_dma_ctrl);
            
            READ_CMIC_CMC0_DMA_STAT_CLRr(0,  cmic_cmc1_dma_stat_clr);
            CMIC_CMC0_DMA_STAT_CLRr_CH0_DESCRD_CMPLT_CLRf_SET(cmic_cmc1_dma_stat_clr, 1);
            WRITE_CMIC_CMC0_DMA_STAT_CLRr(0,  cmic_cmc1_dma_stat_clr);

            tx_pkt_in_dma->status = SYS_OK;
        } else if (SAL_TIME_EXPIRED(tx_start_time, tx_timeout_ticks)) {
            LOCAL_DEBUG_PRINTF("time out %x %x %x\n", tx_start_time , tx_start_time + tx_timeout_ticks, sal_get_ticks());
            /* Timeout, should abort this. */
            tx_pkt_in_dma->status = SYS_ERR_TIMEOUT;
            tx_pkt_in_dma->status = SYS_ERR;

            if (CMIC_CMC0_DMA_STATr_CH0_DMA_ACTIVEf_GET(cmic_cmc1_dma_stat)) {            
                /* Abort TX DMA since it has something wrong */
                int i;
                READ_CMIC_CMC0_CHX_DMA_CTRLr(0, TX_CH, cmic_cmc1_chx_dma_ctrl);
                CMIC_CMC0_CHX_DMA_CTRLr_DMA_ENf_SET(cmic_cmc1_chx_dma_ctrl, 1);
                CMIC_CMC0_CHX_DMA_CTRLr_ABORT_DMAf_SET(cmic_cmc1_chx_dma_ctrl, 1);                
                WRITE_CMIC_CMC0_CHX_DMA_CTRLr(0, TX_CH, cmic_cmc1_chx_dma_ctrl);

                /* Check active */
                for (i = 10; i > 0; i--) {
                     READ_CMIC_CMC0_DMA_STATr(0, cmic_cmc1_dma_stat);                                    
                     if (CMIC_CMC0_DMA_STATr_CH0_DMA_ACTIVEf_GET(cmic_cmc1_dma_stat)) {
                         sal_usleep(10);
                     } else {
                         break;
                     }
                }
                
                /* Restore value */
                CMIC_CMC0_CHX_DMA_CTRLr_DMA_ENf_SET(cmic_cmc1_chx_dma_ctrl, 0);
                CMIC_CMC0_CHX_DMA_CTRLr_ABORT_DMAf_SET(cmic_cmc1_chx_dma_ctrl, 0);                
                WRITE_CMIC_CMC0_CHX_DMA_CTRLr(0, TX_CH, cmic_cmc1_chx_dma_ctrl);
                
                READ_CMIC_CMC0_DMA_STAT_CLRr(0,cmic_cmc1_dma_stat_clr);
                CMIC_CMC0_DMA_STAT_CLRr_CH0_DESCRD_CMPLT_CLRf_SET(cmic_cmc1_dma_stat_clr, 1);
                WRITE_CMIC_CMC0_DMA_STAT_CLRr(0,cmic_cmc1_dma_stat_clr);
            }
        } else {
            /* Haven't completed */
            return;
        } 

        /* Send next in queue (if any) */
        pkt = tx_pkt_in_dma;
        tx_pkt_in_dma = NULL;
        if (tx_pkts_list && rxtx_initialized) {
            soc_tx_packet_t *ppkt = tx_pkts_list;
            tx_pkts_list = ppkt->next;
            bcm5354x_tx_internal(ppkt);
        }

        /* Notify tx caller */
        (*(pkt->callback))(pkt);

    }
}

void
bcm5354x_rxtx_stop(void)
{

    int j;
    pbmp_t pbmp;
    EPC_LINK_BMAP_64r_t epc_link_bmap_64;
    CMIC_RXBUF_EPINTF_RELEASE_ALL_CREDITSr_t cmic_rxbuf_epintf_release_all_credits;
    CMIC_CMC0_DMA_STATr_t cmic_cmc1_dma_stat;
    CMIC_CMC0_CHX_DMA_CTRLr_t cmic_cmc1_chx_dma_ctrl;
    CMIC_CMC0_DMA_STAT_CLRr_t cmic_cmc1_dma_stat_clr;
    
    rxtx_initialized = FALSE;
    
    READ_EPC_LINK_BMAP_64r(0, epc_link_bmap_64);
    PBMP_WORD_SET(pbmp, 0, EPC_LINK_BMAP_64r_PORT_BITMAP_LOf_GET(epc_link_bmap_64));
    PBMP_PORT_REMOVE(pbmp, 0);
    EPC_LINK_BMAP_64r_PORT_BITMAPf_SET(epc_link_bmap_64, PBMP_WORD_GET(pbmp, 0));     
    WRITE_EPC_LINK_BMAP_64r(0, epc_link_bmap_64);
    
    
    CMIC_RXBUF_EPINTF_RELEASE_ALL_CREDITSr_CLR(cmic_rxbuf_epintf_release_all_credits);
    WRITE_CMIC_RXBUF_EPINTF_RELEASE_ALL_CREDITSr(0, cmic_rxbuf_epintf_release_all_credits);
    
    /* Poll for the channel 0-1 to become inactive */
    READ_CMIC_CMC0_DMA_STATr(0, cmic_cmc1_dma_stat);
    
    
    /* Abort TX DMA */
    READ_CMIC_CMC0_CHX_DMA_CTRLr(0, TX_CH, cmic_cmc1_chx_dma_ctrl);
    if (CMIC_CMC0_DMA_STATr_CH0_DMA_ACTIVEf_GET(cmic_cmc1_dma_stat)) {
        CMIC_CMC0_CHX_DMA_CTRLr_ABORT_DMAf_SET(cmic_cmc1_chx_dma_ctrl, 1);
        WRITE_CMIC_CMC0_CHX_DMA_CTRLr(0, TX_CH, cmic_cmc1_chx_dma_ctrl);
    }
    /* Abort RX DMA */
    READ_CMIC_CMC0_CHX_DMA_CTRLr(0, RX_CH1, cmic_cmc1_chx_dma_ctrl);
    if (CMIC_CMC0_DMA_STATr_CH1_DMA_ACTIVEf_GET(cmic_cmc1_dma_stat)) {
        CMIC_CMC0_CHX_DMA_CTRLr_ABORT_DMAf_SET(cmic_cmc1_chx_dma_ctrl, 1);
        WRITE_CMIC_CMC0_CHX_DMA_CTRLr(0, RX_CH1, cmic_cmc1_chx_dma_ctrl);
    }
    
    
    /* Check active */
    for (j = 10; j > 0; j--) {
            READ_CMIC_CMC0_DMA_STATr(0, cmic_cmc1_dma_stat);
            if (CMIC_CMC0_DMA_STATr_CH0_DMA_ACTIVEf_GET(cmic_cmc1_dma_stat) || 
                CMIC_CMC0_DMA_STATr_CH1_DMA_ACTIVEf_GET(cmic_cmc1_dma_stat)) {
                sal_usleep(10);
            } else {
                break;
            }
    }
    
    READ_CMIC_CMC0_CHX_DMA_CTRLr(0, TX_CH, cmic_cmc1_chx_dma_ctrl);
    CMIC_CMC0_CHX_DMA_CTRLr_DMA_ENf_SET(cmic_cmc1_chx_dma_ctrl, 0);
    WRITE_CMIC_CMC0_CHX_DMA_CTRLr(0, TX_CH, cmic_cmc1_chx_dma_ctrl);
    
    READ_CMIC_CMC0_CHX_DMA_CTRLr(0, RX_CH1, cmic_cmc1_chx_dma_ctrl);
    CMIC_CMC0_CHX_DMA_CTRLr_DMA_ENf_SET(cmic_cmc1_chx_dma_ctrl, 0);
    WRITE_CMIC_CMC0_CHX_DMA_CTRLr(0, RX_CH1, cmic_cmc1_chx_dma_ctrl);
    
    
    READ_CMIC_CMC0_DMA_STAT_CLRr(0, cmic_cmc1_dma_stat_clr);
    CMIC_CMC0_DMA_STAT_CLRr_CH0_DESCRD_CMPLT_CLRf_SET(cmic_cmc1_dma_stat_clr, 1);
    CMIC_CMC0_DMA_STAT_CLRr_CH1_DESCRD_CMPLT_CLRf_SET(cmic_cmc1_dma_stat_clr, 1);
    WRITE_CMIC_CMC0_DMA_STAT_CLRr(0, cmic_cmc1_dma_stat_clr);


}

void
bcm5354x_rxtx_init(void)
{
    uint8 unit = 0;
    int i;
    pbmp_t pbmp;
#ifdef __LINUX__
    CMIC_PCIE_USERIF_PURGE_CONTROLr_t cmic_pcie_userif_purge_control;
    CMIC_CMC0_HOSTMEM_ADDR_REMAP_0r_t cmic_cmc1_hostmem_addr_remap_0;
    CMIC_CMC0_HOSTMEM_ADDR_REMAP_1r_t cmic_cmc1_hostmem_addr_remap_1;
    CMIC_CMC0_HOSTMEM_ADDR_REMAP_2r_t cmic_cmc1_hostmem_addr_remap_2;    
    CMIC_CMC0_HOSTMEM_ADDR_REMAP_3r_t cmic_cmc1_hostmem_addr_remap_3;        
#endif    
    CMIC_CMC0_CHX_DMA_CTRLr_t cmic_cmc1_chx_dma_ctrl;
    CMIC_RXBUF_EPINTF_RELEASE_ALL_CREDITSr_t cmic_rxbuf_epintf_release_all_credits;    
    CMIC_CMC0_CH1_COS_CTRL_RX_0r_t cmic_cmc1_ch1_cos_ctrl_rx_0;
    EPC_LINK_BMAP_64r_t epc_link_bmap_64;
    EGR_ENABLEm_t egr_enable;
    IP_TO_CMICM_CREDIT_TRANSFERr_t ip_to_cmicm_credit_transfer;
    CMICD_M0_IDM_IO_CONTROL_DIRECTr_t cmicd_m0_idm_io_control_direct;
	CMICD_S0_IDM_IO_CONTROL_DIRECTr_t cmicd_s0_idm_io_control_direct;

    
    /* Lazy initialization */
    if (rxtx_initialized) {
        return;
    }

    bcm5354x_rxtx_stop();

    rxtx_initialized = TRUE;

#ifdef __LINUX__
    /* Known good state, CH0 for tx and CH1 for rx. */
    CMIC_PCIE_USERIF_PURGE_CONTROLr_CLR(cmic_pcie_userif_purge_control);
    CMIC_PCIE_USERIF_PURGE_CONTROLr_ENABLE_PURGE_IF_USERIF_RESETf_SET(cmic_pcie_userif_purge_control, 1);
    CMIC_PCIE_USERIF_PURGE_CONTROLr_ENABLE_PURGE_IF_USERIF_TIMESOUTf_SET(cmic_pcie_userif_purge_control, 1);
    CMIC_PCIE_USERIF_PURGE_CONTROLr_ENABLE_PIO_PURGE_IF_USERIF_RESETf_SET(cmic_pcie_userif_purge_control, 1);
    WRITE_CMIC_PCIE_USERIF_PURGE_CONTROLr(unit, cmic_pcie_userif_purge_control);
    
    CMIC_CMC0_HOSTMEM_ADDR_REMAP_0r_SET(cmic_cmc1_hostmem_addr_remap_0, 0x144d2450);
    WRITE_CMIC_CMC0_HOSTMEM_ADDR_REMAP_0r(unit, cmic_cmc1_hostmem_addr_remap_0);

    CMIC_CMC0_HOSTMEM_ADDR_REMAP_1r_SET(cmic_cmc1_hostmem_addr_remap_1, 0x19617595);
    WRITE_CMIC_CMC0_HOSTMEM_ADDR_REMAP_1r(unit, cmic_cmc1_hostmem_addr_remap_1);

    CMIC_CMC0_HOSTMEM_ADDR_REMAP_2r_SET(cmic_cmc1_hostmem_addr_remap_2, 0x1e75c6da);
    WRITE_CMIC_CMC0_HOSTMEM_ADDR_REMAP_2r(unit, cmic_cmc1_hostmem_addr_remap_2);

    CMIC_CMC0_HOSTMEM_ADDR_REMAP_3r_SET(cmic_cmc1_hostmem_addr_remap_3, 0x1f);
    WRITE_CMIC_CMC0_HOSTMEM_ADDR_REMAP_3r(unit, cmic_cmc1_hostmem_addr_remap_3);
#endif

    CMIC_CMC0_CHX_DMA_CTRLr_CLR(cmic_cmc1_chx_dma_ctrl);
    CMIC_CMC0_CHX_DMA_CTRLr_PKTDMA_ENDIANESSf_SET(cmic_cmc1_chx_dma_ctrl, CFG_LITTLE_ENDIAN ? 0 : 1);
    CMIC_CMC0_CHX_DMA_CTRLr_DIRECTIONf_SET(cmic_cmc1_chx_dma_ctrl, 1);    
    WRITE_CMIC_CMC0_CHX_DMA_CTRLr(unit, TX_CH, cmic_cmc1_chx_dma_ctrl);
    
    CMIC_CMC0_CHX_DMA_CTRLr_DIRECTIONf_SET(cmic_cmc1_chx_dma_ctrl, 0);    
    WRITE_CMIC_CMC0_CHX_DMA_CTRLr(unit, RX_CH1, cmic_cmc1_chx_dma_ctrl);    


    /* Enable CMIC to release available credits to EP */
    CMIC_RXBUF_EPINTF_RELEASE_ALL_CREDITSr_CLR(cmic_rxbuf_epintf_release_all_credits);
    CMIC_RXBUF_EPINTF_RELEASE_ALL_CREDITSr_RELEASE_ALL_CREDITSf_SET(cmic_rxbuf_epintf_release_all_credits, 0);
    WRITE_CMIC_RXBUF_EPINTF_RELEASE_ALL_CREDITSr(unit, cmic_rxbuf_epintf_release_all_credits);
    CMIC_RXBUF_EPINTF_RELEASE_ALL_CREDITSr_RELEASE_ALL_CREDITSf_SET(cmic_rxbuf_epintf_release_all_credits, 1);
    WRITE_CMIC_RXBUF_EPINTF_RELEASE_ALL_CREDITSr(unit, cmic_rxbuf_epintf_release_all_credits);


    /* Map all COS queues to default Rx DMA channel (1) */
    CMIC_CMC0_CH1_COS_CTRL_RX_0r_CLR(cmic_cmc1_ch1_cos_ctrl_rx_0);
    CMIC_CMC0_CH1_COS_CTRL_RX_0r_COS_BMPf_SET(cmic_cmc1_ch1_cos_ctrl_rx_0, 0xFF);
    WRITE_CMIC_CMC0_CH1_COS_CTRL_RX_0r(unit, cmic_cmc1_ch1_cos_ctrl_rx_0);


    /* Enable CPU port to receive packets. */
    READ_EPC_LINK_BMAP_64r(unit, epc_link_bmap_64);
    PBMP_WORD_SET(pbmp, 0, EPC_LINK_BMAP_64r_PORT_BITMAPf_GET(epc_link_bmap_64));
    PBMP_PORT_ADD(pbmp, CPU_PORT_INDEX);
    EPC_LINK_BMAP_64r_PORT_BITMAPf_SET(epc_link_bmap_64, PBMP_WORD_GET(pbmp, 0));     
    WRITE_EPC_LINK_BMAP_64r(unit, epc_link_bmap_64);

    READ_EGR_ENABLEm(unit, CPU_PORT_INDEX, egr_enable);
    EGR_ENABLEm_PRT_ENABLEf_SET(egr_enable, 1);
    WRITE_EGR_ENABLEm(unit, CPU_PORT_INDEX, egr_enable);
    
    tx_dcb = sal_dma_malloc(sizeof(TX_DCB_t));
    rx_dcb = sal_dma_malloc(sizeof(RX_DCB_t));
    
    /* 
     * Enable IP to CMIC credit transfer: 
     * TRANSFER_ENABLE =1, NUM_OF_CREDITS = 32 
     */
    IP_TO_CMICM_CREDIT_TRANSFERr_CLR(ip_to_cmicm_credit_transfer);
    IP_TO_CMICM_CREDIT_TRANSFERr_NUM_OF_CREDITSf_SET(ip_to_cmicm_credit_transfer, 32);
    IP_TO_CMICM_CREDIT_TRANSFERr_TRANSFER_ENABLEf_SET(ip_to_cmicm_credit_transfer, 1);
    WRITE_IP_TO_CMICM_CREDIT_TRANSFERr(unit, ip_to_cmicm_credit_transfer);

    /*
      Enable DMA between A9 and CMIC
    */
    READ_CMICD_S0_IDM_IO_CONTROL_DIRECTr(unit, cmicd_s0_idm_io_control_direct);
    CMICD_S0_IDM_IO_CONTROL_DIRECTr_SET(cmicd_s0_idm_io_control_direct, cmicd_s0_idm_io_control_direct.v[0] | 0x3);
    WRITE_CMICD_S0_IDM_IO_CONTROL_DIRECTr(unit, cmicd_s0_idm_io_control_direct);


    READ_CMICD_M0_IDM_IO_CONTROL_DIRECTr(unit, cmicd_m0_idm_io_control_direct);
    CMICD_M0_IDM_IO_CONTROL_DIRECTr_AWCACHEf_SET(cmicd_m0_idm_io_control_direct, 0x7);
    CMICD_M0_IDM_IO_CONTROL_DIRECTr_ARCACHEf_SET(cmicd_m0_idm_io_control_direct, 0xB);
    CMICD_M0_IDM_IO_CONTROL_DIRECTr_BYPASS_CTf_SET(cmicd_m0_idm_io_control_direct, 0x1);
    CMICD_M0_IDM_IO_CONTROL_DIRECTr_CTf_SET(cmicd_m0_idm_io_control_direct, 0x1);    
    WRITE_CMICD_M0_IDM_IO_CONTROL_DIRECTr(unit, cmicd_m0_idm_io_control_direct);

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
    task_add(bcm5354x_rxtx_task, (void *)NULL);
    
    /* Initialize TX global variables */
    tx_pkt_in_dma = NULL;
    tx_pkts_list = NULL;
    tx_timeout_ticks = SAL_USEC_TO_TICKS(TX_WAIT_TIMEOUT);
}

APISTATIC sys_error_t
bcm5354x_tx_internal(soc_tx_packet_t *pkt)
{
    int i;
    uint32 port;
    enet_hdr_t  *th; /* Tagged header pointers */
    CMIC_CMC0_CHX_DMA_CTRLr_t cmic_cmc1_chx_dma_ctrl;
    CMIC_CMC0_DMA_DESCr_t cmic_cmc1_dma_desc;
    
    TX_DCB_CLR((*tx_dcb));

    
    if (PBMP_NOT_NULL(pkt->port_bitmap)) {
        TX_DCB_HIGIG_PKTf_SET((*tx_dcb), 1);
        /* Use SOBMH mode. */
        for (port = BCM5354X_LPORT_MIN; port <= BCM5354X_LPORT_MAX; port++) {
            if (PBMP_MEMBER(pkt->port_bitmap, port)) {
                break;
            }
        }

        if (port > BCM5354X_PORT_MAX) {
            return SYS_ERR_PARAMETER;
        }
        
        /* SOBMH_FROM_CPU */    
        TX_DCB_SOBMH_HEADER_TYPEf_SET((*tx_dcb), 1);
        /* INTERNAL ONLY */
        TX_DCB_SOBMH_STARTf_SET((*tx_dcb), 2);
        /* Assign LOCAL_DEST_PORT */
        TX_DCB_SOBMH_LOCAL_DEST_PORTf_SET((*tx_dcb), port);

        /* Use cosq 3 and enable UNICAST */
        TX_DCB_SOBMH_UNICASTf_SET((*tx_dcb), 1);
        TX_DCB_SOBMH_COSf_SET((*tx_dcb), 0x3);

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

    TX_DCB_MEM_ADDRf_SET((*tx_dcb), PTR_TO_PCI(pkt->buffer));
    TX_DCB_BYTE_COUNTf_SET((*tx_dcb),pkt->pktlen);
    TX_DCB_STAT_UPDATEf_SET((*tx_dcb), 1);

#if RXTX_DEBUG
    {
        int j;
        uint32 *p = (uint32 *)tx_dcb;
        sal_printf("\nTX DCB: ");
        for (j = 0; j < 16; j++) {
            sal_printf("0x%08x ", *(p + j));
        }
        sal_printf("\n");
    }
    sal_printf("dcb:buf addr = 0x%08x:0x%08x\n", tx_dcb, PTR_TO_PCI(pkt->buffer));
#endif /* RXTX_DEBUG */

    CACHE_DMA_SYNC(pkt->buffer, pkt->pktlen);
    CACHE_DMA_SYNC(tx_dcb, sizeof(TX_DCB_t));

    CMIC_CMC0_DMA_DESCr_CLR(cmic_cmc1_dma_desc);
    CMIC_CMC0_DMA_DESCr_ADDRf_SET(cmic_cmc1_dma_desc, PTR_TO_PCI(tx_dcb));
    WRITE_CMIC_CMC0_DMA_DESCr(0, TX_CH, cmic_cmc1_dma_desc);
    
    READ_CMIC_CMC0_CHX_DMA_CTRLr(0, TX_CH, cmic_cmc1_chx_dma_ctrl);
    CMIC_CMC0_CHX_DMA_CTRLr_DMA_ENf_SET(cmic_cmc1_chx_dma_ctrl, 1);
    WRITE_CMIC_CMC0_CHX_DMA_CTRLr(0, TX_CH, cmic_cmc1_chx_dma_ctrl);


    tx_start_time = sal_get_ticks();
    tx_pkt_in_dma = pkt;

    return SYS_OK;
}

sys_error_t
bcm5354x_tx(uint8 unit, soc_tx_packet_t *pkt)
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
        bcm5354x_tx_internal(pkt);
    } else {
        /* DMA in progress, queue this packet */
        pkt->next = tx_pkts_list;
        tx_pkts_list = pkt;
    }
    return SYS_OK;
}

sys_error_t
bcm5354x_rx_set_handler(uint8 unit, SOC_RX_HANDLER fn, BOOL intr)
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
bcm5354x_rx_fill_buffer(uint8 unit, soc_rx_packet_t *pkt)
{
    uint8 i;

    CMIC_CMC0_DMA_DESCr_t cmic_cmc1_dma_desc;
    CMIC_CMC0_CHX_DMA_CTRLr_t cmic_cmc1_chx_dma_ctrl;

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
                RX_DCB_MEM_ADDRf_SET((*rx_dcb), PTR_TO_PCI(pkt->buffer));
                RX_DCB_BYTE_COUNTf_SET((*rx_dcb), pkt->buflen);                
#if RXTX_DEBUG
                sal_printf("dcb:buf addr = 0x%08x:0x%08x\n", rx_dcb, PTR_TO_PCI(pkt->buffer));
#endif
                CACHE_DMA_SYNC(rx_dcb, sizeof(RX_DCB_t));
                CMIC_CMC0_DMA_DESCr_CLR(cmic_cmc1_dma_desc);
                CMIC_CMC0_DMA_DESCr_ADDRf_SET(cmic_cmc1_dma_desc, PTR_TO_PCI(rx_dcb));
                WRITE_CMIC_CMC0_DMA_DESCr(0, RX_CH1, cmic_cmc1_dma_desc);

                READ_CMIC_CMC0_CHX_DMA_CTRLr(0, RX_CH1, cmic_cmc1_chx_dma_ctrl);
                CMIC_CMC0_CHX_DMA_CTRLr_DMA_ENf_SET(cmic_cmc1_chx_dma_ctrl, 1);
                WRITE_CMIC_CMC0_CHX_DMA_CTRLr(0, RX_CH1, cmic_cmc1_chx_dma_ctrl);

            }
            return SYS_OK;
        }
    }
    
    return SYS_ERR_FULL;
}

#endif /* CFG_RXTX_SUPPORT_ENABLED */
