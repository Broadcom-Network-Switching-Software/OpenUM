/*
 * $Id: flsnaketest.c,v 1.1 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
 
#include "system.h"


#define SNAKE_TEST_DURATION      (10)
#define SNAKE_TEST_PACKET_LEN    (68)

#define DA1_LAST_OCTET           0x01
#define DA2_LAST_OCTET           0x02

static const uint8 snake_test_da[6] = { 0x0, 0x0, 0x0, 0x0, 0x0, DA1_LAST_OCTET };
static const uint8 snake_test_sa[6] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x03 };
static int reverse_pkt_flag, rpkt_count;
static uint8 r_data[2][SNAKE_TEST_PACKET_LEN*2];
static uint8 r_len[2];

static void
snake_test_cbk(struct soc_tx_packet_s *pkt)
{
    if (!reverse_pkt_flag) {
        /* Reuse packet */
        reverse_pkt_flag = 1;
        return;
    }

    if (pkt) {
        if (pkt->buffer) {
            sal_dma_free(pkt->buffer);
        }
        sal_free(pkt);
    }
}

static sys_rx_t
snake_rx_handler(sys_pkt_t *pkt, void *cookie)
{
    /* Save data */
    if (rpkt_count < 2) {
        sal_memcpy(r_data[rpkt_count], pkt->pkt_data, pkt->pkt_len);
        r_len[rpkt_count] = pkt->pkt_len;
        rpkt_count++;
    }

    return SYS_RX_HANDLED;
}
/* Compared packets returned to CPU match tx packet */
int
snake_analysis(void)
{
    int i, j, k;

    if (rpkt_count != 2) {
        sal_printf("ERROR: Expected 2 packets received on CPU. Only %d received!\n", rpkt_count);
        return -1;
    }

    for (i = 0; i < 2; i++) {
        if (sal_memcmp(r_data[i], snake_test_da, 5)) {
            goto err;
        }

        if (r_data[i][5] != DA1_LAST_OCTET && r_data[i][5] != DA2_LAST_OCTET) {
            goto err;
        }

        if (sal_memcmp(&r_data[i][6], snake_test_sa, 6)) {
            goto err;
        }

        if (r_data[i][16] != 0x88 && r_data[i][17] != 0x00) {
            goto err;
        }

        for (j = 18, k = 0; k < SNAKE_TEST_PACKET_LEN - 14; j++, k++) {
            if (r_data[i][j] != (uint8)k) {
                goto err;
            }
        } 

    }
    sal_printf("Packet Data Check Successful: Expected 2 packets received on CPU. %d received!\n", rpkt_count);
    return 0;
err:
    sal_printf("\nERROR: Packet %d is not matched!\n", i+1);
    sal_printf("Data: ");
    for (j = 0; j < r_len[i]; j++) {
        sal_printf("%02x ", r_data[i][j]);
    }
    return -1;
}

#define RDBGC0(i) 0xa, 0x40003a00+(i)

/* Check error counters */
int
snake_stats(uint8 min_port, uint8 max_port)
{
    int uport;
    port_stat_t stat;

    SAL_UPORT_ITER(uport) {

         if (board_port_stat_get(uport, &stat)) {
             sal_printf("Get %d port stat error\n", uport);
         };

         sal_printf(" --------------------------------------------------------------------- \r\n");
         sal_printf("|                                                                     |\r\n");
         sal_printf("|                  Boardcom port %02d statistics                        |\r\n", uport);
         sal_printf("|                                                                     |\r\n");
         sal_printf(" --------------------------------------------------------------------- \r\n");
         sal_printf("                                                                       \r\n");
         sal_printf("  Tx bytes   %08x%08x                                         \r\n", stat.TxOctets_hi, stat.TxOctets_lo);
         sal_printf("  Tx packets %08x%08x                                         \r\n", stat.TxPkts_hi, stat.TxPkts_lo);
         sal_printf("  Tx unicast packets %08x%08x                                 \r\n", stat.TxUnicastPkts_hi, stat.TxUnicastPkts_lo);
         sal_printf("  Tx multicast packets %08x%08x                               \r\n", stat.TxMulticastPkts_hi, stat.TxMulticastPkts_lo);
         sal_printf("  Tx broadcast packets %08x%08x                               \r\n", stat.TxBroadcastPkts_hi, stat.TxBroadcastPkts_lo);
         sal_printf("  Tx pause frames    %08x%08x                                 \r\n", stat.TxPauseFramePkts_hi, stat.TxPauseFramePkts_lo);
         sal_printf("  Tx oversize frames %08x%08x                                 \r\n", stat.TxOversizePkts_hi, stat.TxOversizePkts_lo);
         sal_printf("  Tx LPI events   %08x                                        \r\n", stat.TxLPIPkts_lo);
         sal_printf("  Tx LPI duration %08x                                        \r\n", stat.TxLPIDuration_lo);
         sal_printf("  Rx bytes   %08x%08x                                         \r\n", stat.RxOctets_hi, stat.RxOctets_lo);
         sal_printf("  Rx packets %08x%08x                                         \r\n", stat.RxPkts_hi, stat.RxPkts_lo);
         sal_printf("  Rx unicast packets %08x%08x                                 \r\n", stat.RxUnicastPkts_hi, stat.RxUnicastPkts_lo);
         sal_printf("  Rx multicast packets %08x%08x                               \r\n", stat.RxMulticastPkts_hi, stat.RxMulticastPkts_lo);
         sal_printf("  Rx broadcast packets %08x%08x                               \r\n", stat.RxBroadcastPkts_hi, stat.RxBroadcastPkts_lo);
         sal_printf("  Rx pause frames    %08x%08x                                 \r\n", stat.RxPauseFramePkts_hi, stat.RxPauseFramePkts_lo);
         sal_printf("  Rx oversize frames %08x%08x                                 \r\n", stat.RxOversizePkts_hi, stat.RxOversizePkts_lo);
         sal_printf("  Rx LPI events   %08x                                        \r\n", stat.RxLPIPkts_lo);
         sal_printf("  Rx LPI duration %08x                                        \r\n", stat.RxLPIDuration_lo);
         sal_printf("  Rx FCS Error Frames %08x%08x                                \r\n", stat.CRCErrors_hi, stat.CRCErrors_lo);
         sal_printf("  Egress hold drop packets %08x                         \r\n", stat.EgressDropPkts_lo);
         sal_printf("                                                                      \r\n");
         sal_printf("                                                                      \r\n");
         sal_printf("                                                                      \r\n");
         sal_printf("                                                                      \r\n");         
    }
    
    return SYS_OK;
}

static void
snake_test_txrx(uint8 min_port, uint8 max_port)
{
    int i, j, duration;
    tick_t start;
    soc_tx_packet_t *spkt;
    l2x_entry_t l2x_entry;
    uint8 unit = 0, lport;
    rpkt_count = 0;

    sys_rx_register(
                snake_rx_handler, 
                CFG_CLI_RX_MON_PRIORITY, 
                NULL,
                SYS_RX_REGISTER_FLAG_ACCEPT_PKT_ERRORS | 
                SYS_RX_REGISTER_FLAG_ACCEPT_TRUNCATED_PKT
                );

    spkt = (soc_tx_packet_t *)sal_malloc(sizeof(soc_tx_packet_t));
    if (spkt == NULL) {
        sal_printf("snake_test_txrx: malloc failed!\n");
        return;
    }

    sal_memset(spkt, 0, sizeof(soc_tx_packet_t));
    board_uport_to_lport(min_port, &unit, &lport);
    spkt->port_bitmap = (1 << lport);
    spkt->buffer = (uint8 *)sal_dma_malloc(SNAKE_TEST_PACKET_LEN+4);
    spkt->pktlen = SNAKE_TEST_PACKET_LEN+4;
    spkt->callback = snake_test_cbk;

    /* DA and SA */    
    sal_memcpy(spkt->buffer, snake_test_da, 6);
    sal_memcpy(spkt->buffer + 6, snake_test_sa, 6);

    /* Untagged packet, insert 0x8800 to invalidate 802.3 length field for XLMAC */
    *(spkt->buffer + 12) = 0x88;
    *(spkt->buffer + 13) = 0x00;
    for (i = 14, j = 0; i < SNAKE_TEST_PACKET_LEN; i++, j++) {
        *(spkt->buffer + i) = (uint8)j;
    }

    reverse_pkt_flag = 0;

    board_port_stat_clear_all();

    bcm5607x_tx(0, spkt);

    while(!reverse_pkt_flag) {
        sal_sleep(100);
    }
    board_uport_to_lport(max_port, &unit, &lport);

    spkt->port_bitmap = (1 << lport);
    *(spkt->buffer+5) = DA2_LAST_OCTET;

    bcm5607x_tx(0, spkt);

    sal_sleep(1000);

    start = sal_get_ticks();

    sal_printf("\nSnake Test: circular test on all ports for 60 seconds\n");

    duration = 0;

    while (duration < SNAKE_TEST_DURATION) {
        while(!SAL_TIME_EXPIRED(start, 10000)) {
            POLL();
        }
        duration += 10;
        sal_printf("\nTime elapsed:  %d seconds\n", duration);
        start = sal_get_ticks();
    }

    /* Stop traffic */
    sal_memcpy(l2x_entry.mac_addr, snake_test_da , 6);
    board_uport_to_lport(max_port, &unit, &lport);
    l2x_entry.vlan_id = lport;
    l2x_entry.port = 0;
    l2x_entry.is_static = TRUE;
    bcm5607x_l2_op(0, &l2x_entry,  SC_OP_L2_INS_CMD, NULL);

    board_uport_to_lport(min_port, &unit, &lport);
    l2x_entry.mac_addr[5] = DA2_LAST_OCTET;
    l2x_entry.vlan_id = lport;
    l2x_entry.port = 0;
    l2x_entry.is_static = TRUE;
    bcm5607x_l2_op(0, &l2x_entry,  SC_OP_L2_INS_CMD, NULL);

    /* Receive packets */
    sal_sleep(1000);

    snake_analysis();

}


static void
snake_test_init(uint8 mode, uint8 min_port, uint8 max_port)
{
    int uport;
    int lb_mode;

    l2x_entry_t l2x_entry;
    uint8 unit = 0, lport;
    
    PORTm_t port_entry;   

    SAL_UPORT_ITER(uport) {        
        /* Program Port table to change PVID */
        board_uport_to_lport(uport, &unit, &lport);
        READ_PORTm(unit, lport, port_entry);
        PORTm_PORT_VIDf_SET(port_entry, lport);
        WRITE_PORTm(unit, lport, port_entry);
    }
#if defined(CFG_SWITCH_STAT_INCLUDED)
    board_port_stat_clear_all();
#endif

    if (mode == 2) {
        /* Snake test with external cable */
        sal_memset(l2x_entry.mac_addr, 0x0 , 5);
        l2x_entry.mac_addr[5] = DA1_LAST_OCTET;

        for (uport = (min_port + 1); uport <= max_port; uport+=2) {
            if (uport == max_port) {
                board_uport_to_lport(min_port, &unit, &lport);            
                l2x_entry.port = lport;
            } else {
                board_uport_to_lport((uport+1), &unit, &lport);            
                l2x_entry.port = lport;
            }
            board_uport_to_lport((uport), &unit, &lport);            
            l2x_entry.vlan_id = lport;
            l2x_entry.is_static = TRUE;
            bcm5607x_l2_op(0, &l2x_entry,  SC_OP_L2_INS_CMD, NULL);
        }

        l2x_entry.mac_addr[5] = DA2_LAST_OCTET;
        for (uport = (max_port - 1); uport >= min_port; uport-=2) {
            if (uport == min_port) {
                board_uport_to_lport(max_port, &unit, &lport);            
                l2x_entry.port = lport;
            } else {
                board_uport_to_lport((uport-1), &unit, &lport);            
                l2x_entry.port = lport;
            }
            board_uport_to_lport((uport), &unit, &lport);            
            l2x_entry.vlan_id = lport;
            l2x_entry.is_static = TRUE;
            bcm5607x_l2_op(0, &l2x_entry,  SC_OP_L2_INS_CMD, NULL);
        }
    } else if (mode == 0 || mode == 1) {
        /* Snake test with MAC/PHY loopback */
        lb_mode = (mode == 0) ? PORT_LOOPBACK_MAC : PORT_LOOPBACK_PHY;
 
        sal_memcpy(l2x_entry.mac_addr, snake_test_da , 6);

        for (uport = min_port; uport <= max_port; uport++) {
            if (uport == max_port) {
                board_uport_to_lport(min_port, &unit, &lport);
                l2x_entry.port = lport;
            } else {
                 board_uport_to_lport(uport+1, &unit, &lport);
                l2x_entry.port = lport;
            }
            board_uport_to_lport(uport, &unit, &lport);
            l2x_entry.vlan_id = lport;
            l2x_entry.is_static = TRUE;
            bcm5607x_l2_op(0, &l2x_entry,  SC_OP_L2_INS_CMD, NULL);
        }

        l2x_entry.mac_addr[5] = DA2_LAST_OCTET;
        for (uport = max_port; uport >= min_port; uport--) {
            if (uport == min_port) {
                board_uport_to_lport(max_port, &unit, &lport);
                l2x_entry.port = lport;
            } else {
                board_uport_to_lport(uport-1, &unit, &lport);
                l2x_entry.port = lport;
            }
            board_uport_to_lport(uport, &unit, &lport);
            l2x_entry.vlan_id = lport;
            l2x_entry.is_static = TRUE;
            bcm5607x_l2_op(0, &l2x_entry,  SC_OP_L2_INS_CMD, NULL);
        }

        for (uport = min_port; uport <= max_port; uport++) {
            board_uport_to_lport(uport, &unit, &lport);
            bcm5607x_loopback_enable(0, lport, lb_mode);
        }

        sal_sleep(2000);
    }
}

void
snaketest(uint8 mode, uint8 min_port, uint8 max_port)
{
    snake_test_init(mode, min_port, max_port);
    snake_test_txrx(min_port, max_port);

    if (snake_stats(min_port, max_port) == 0) {
        sal_printf("\nSnake Test Completed successfully\n");
    }
}
