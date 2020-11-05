/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"

#define CFG_FLOW_CONTROL_ENABLED_PFC 0    /* enable PFC 3 by default if flow_control_type_pause_0_pfc_1 = 1 */
#define CFG_FLOW_CONTROL_ENABLED_COSQ 1   /* Based on priority <-> cosq mapping */

#define CFG_MMU_DEBUG           (0)

uint8 mmu_lossless = 1;

static sys_error_t soc_tdm_init(uint8 unit) {
    const uint32              *arr = NULL;
    int                 tdm_size;
    int i;
    uint32 val;

    IARB_TDM_CONTROLr_t iarb_tdm_control;
    IARB_TDM_TABLEm_t iarb_tdm_table;
    MMU_ARB_TDM_TABLEm_t mmu_arb_tdm_table;

    arr = sku_port_config->tdm_table;
    tdm_size = sku_port_config->tdm_table_size;

    /* DISABLE [Bit 0] = 1, TDM_WRAP_PTR = TDM_SIZE-1 */
    READ_IARB_TDM_CONTROLr(unit, iarb_tdm_control);
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_control, 1);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_control, 83);  
    WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_control);

    if (arr == NULL) {
        sal_printf("%s..:NO tdm table found..\n", __func__);
        return SYS_ERR_PARAMETER;
    }

    for (i = 0; i < tdm_size; i++) {
        IARB_TDM_TABLEm_CLR(iarb_tdm_table);
        IARB_TDM_TABLEm_PORT_NUMf_SET(iarb_tdm_table, arr[i]);
        WRITE_IARB_TDM_TABLEm(unit, i, iarb_tdm_table);

        /* TDM programmed in MMU is in Logical port domain */
        val = (arr[i] != 63) ? SOC_PORT_P2L_MAPPING(arr[i]) : 63;
        MMU_ARB_TDM_TABLEm_CLR(mmu_arb_tdm_table);
        MMU_ARB_TDM_TABLEm_PORT_NUMf_SET(mmu_arb_tdm_table, val);
        if (i == (tdm_size - 1)) {
            /* WRAP_EN = 1 */
            MMU_ARB_TDM_TABLEm_WRAP_ENf_SET(mmu_arb_tdm_table, 1);
        }
        WRITE_MMU_ARB_TDM_TABLEm(unit, i, mmu_arb_tdm_table);
    }

    /* DISABLE = 0, TDM_WRAP_PTR = TDM_SIZE-1 */
    READ_IARB_TDM_CONTROLr(unit, iarb_tdm_control);
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_control, 0);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_control, (tdm_size - 1));
    WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_control);    

    return SYS_OK;  
}


static int ceiling_func(uint32 numerators, uint32 denominator)
{
    uint32  result;
    if (denominator == 0) {
        return 0xFFFFFFFF;
    }
    result = numerators / denominator;
    if (numerators % denominator != 0) {
        result++;
    }
    return result;
}

static sys_error_t _soc_wolfhound2_mmu_init_helper_lossy(int unit)
{
    int port, phy_port, mport;
    int index;
    int cos;
    
    pbmp_t pbmp_cpu;
    pbmp_t pbmp_uplink;
    pbmp_t pbmp_uplink_1g;
    pbmp_t pbmp_uplink_2dot5g;
    pbmp_t pbmp_downlink;
    pbmp_t pbmp_downlink_1g;
    pbmp_t pbmp_downlink_2dot5g;
    pbmp_t pbmp_all;

    int standard_jumbo_frame;
    int cell_size;
    int ethernet_mtu_cell;
    int standard_jumbo_frame_cell;
    int total_physical_memory;
    int total_cell_memory_for_admission;
    
    int reserved_for_cfap;
    int skidmarker;
    int prefetch;
    int total_cell_memory;
    int cfapfullsetpoint;
    int total_advertised_cell_memory;
    int number_of_uplink_ports;
    int number_of_uplink_ports_1g;
    int number_of_uplink_ports_2dot5g;
    int number_of_downlink_ports;
    int number_of_downlink_ports_1g;
    int number_of_downlink_ports_2dot5g;
    int queue_port_limit_ratio;
    int egress_queue_min_reserve_uplink_ports_lossy;
    int egress_queue_min_reserve_downlink_ports_lossy;
    int egress_queue_min_reserve_uplink_ports_lossless;
    int egress_queue_min_reserve_downlink_ports_lossless;
    int egress_queue_min_reserve_cpu_ports;
    int egress_xq_min_reserve_lossy_ports;
    int egress_xq_min_reserve_lossless_uplink_ports;
    int egress_xq_min_reserve_lossless_downlink_ports;
    int num_active_pri_group_lossless;
    int num_lossy_queues;
    int mmu_xoff_pkt_threshold_uplink_ports;
    int mmu_xoff_pkt_threshold_downlink_ports;
    int mmu_xoff_cell_threshold_1g_port_downlink_ports;
    int mmu_xoff_cell_threshold_2dot5g_port_downlink_ports;
    int mmu_xoff_cell_threshold_all_uplink_ports;
    int num_cpu_queues;
    int num_cpu_ports;
    int numxqs_per_uplink_ports;
    int numxqs_per_downlink_ports_and_cpu_port;
    int headroom_for_1g_port;
    
    int xoff_cell_thresholds_per_port_1g_port_downlink_ports;
    int xoff_cell_thresholds_per_port_2dot5g_downlink_ports;
    int xoff_cell_threshold_all_uplink_ports;
    int xoff_packet_thresholds_per_port_uplink_port;
    int xoff_packet_thresholds_per_port_downlink_port;
    int discard_limit_per_port_pg_uplink_port;
    
    int discard_limit_per_port_pg_downlink_port;
    int total_reserved_cells_for_uplink_ports;
    int total_reserved_cells_for_downlink_ports;
    int total_reserved_cells_for_cpu_port;
    int total_reserved;
    int shared_space_cells;
    int reserved_xqs_per_uplink_port;
    int shared_xqs_per_uplink_port;
    int reserved_xqs_per_downlink_port;
    int shared_xqs_per_downlink_port;
    int cfapfullthreshold_cfapfullsetpoint_up;
    int gbllimitsetlimit_gblcellsetlimit_up;
    int totaldyncellsetlimit_totaldyncellsetlimit_up;
    int holcospktsetlimit0_pktsetlimit_up;
    int holcospktsetlimit3_pktsetlimit_up;
    int dynxqcntport_dynxqcntport_up;
    int lwmcoscellsetlimit0_cellsetlimit_up;
    int lwmcoscellsetlimit3_cellsetlimit_up;
    int holcoscellmaxlimit0_cellmaxlimit_up;
    int holcoscellmaxlimit3_cellmaxlimit_up;
    int dyncelllimit_dyncellsetlimit_up;
    int holcospktsetlimit0_pktsetlimit_down_1;
    int holcospktsetlimit3_pktsetlimit_down_1;
    int dynxqcntport_dynxqcntport_down_1;
    int lwmcoscellsetlimit0_cellsetlimit_down_1;
    int lwmcoscellsetlimit3_cellsetlimit_down_1;
    int holcoscellmaxlimit0_cellmaxlimit_down_1;
    int holcoscellmaxlimit3_cellmaxlimit_down_1;
    int dyncelllimit_dyncellsetlimit_down_1;
    int holcospktsetlimit0_pktsetlimit_down_2dot5;
    int holcospktsetlimit3_pktsetlimit_down_2dot5;
    int dynxqcntport_dynxqcntport_down_2dot5;
    int lwmcoscellsetlimit0_cellsetlimit_down_2dot5;
    int lwmcoscellsetlimit3_cellsetlimit_down_2dot5;
    int holcoscellmaxlimit0_cellmaxlimit_down_2dot5;
    int holcoscellmaxlimit3_cellmaxlimit_down_2dot5;
    int dyncelllimit_dyncellsetlimit_down_2dot5;


    int holcosminxqcnt0_holcosminxqcnt_cpu;
    int holcosminxqcnt3_holcosminxqcnt_cpu;
    int holcospktsetlimit0_pktsetlimit_cpu;
    int holcospktsetlimit3_pktsetlimit_cpu;

    int lwmcoscellsetlimit0_cellsetlimit_cpu;
    int lwmcoscellsetlimit3_cellsetlimit_cpu;
    int holcoscellmaxlimit0_cellmaxlimit_cpu;
    int holcoscellmaxlimit3_cellmaxlimit_cpu;

    int dynxqcntport_dynxqcntport_cpu;
    int dyncelllimit_dyncellsetlimit_cpu;

    CFAPFULLTHRESHOLDr_t cfapfullthreshold;
    GBLLIMITSETLIMITr_t gbllimitsetlimit;
    GBLLIMITRESETLIMITr_t gbllimitresetlimit;
    TOTALDYNCELLSETLIMITr_t totaldyncellsetlimit;
    TOTALDYNCELLRESETLIMITr_t totaldyncellresetlimit;
    
    MISCCONFIGr_t miscconfig;
    MMUPORTTXENABLEr_t  mmuporttxenable;
    PG_CTRL0r_t pg_ctrl0;
    PG_CTRL1r_t pg_ctrl1;
    PG2TCr_t pg2tc;    
    IBPPKTSETLIMITr_t ibppktsetlimit;
    MMU_FC_RX_ENr_t mmu_fc_rx_en;
    MMU_FC_TX_ENr_t mmu_fc_tx_en;
    PGCELLLIMITr_t pgcelllimit;
    PGDISCARDSETLIMITr_t pgdiscardsetlimit;
    HOLCOSMINXQCNTr_t holcosminxqcnt;
    HOLCOSPKTSETLIMITr_t holcospktsetlimit;
    HOLCOSPKTRESETLIMITr_t holcospktresetlimit;
    CNGCOSPKTLIMIT0r_t cngcospktlimit0;
    CNGCOSPKTLIMIT1r_t cngcospktlimit1;
    CNGPORTPKTLIMIT0r_t cngportpktlimit0;
    CNGPORTPKTLIMIT1r_t cngportpktlimit1;
    DYNXQCNTPORTr_t dynxqcntport;
    DYNRESETLIMPORTr_t dynresetlimport;
    LWMCOSCELLSETLIMITr_t lwmcoscellsetlimit;
    HOLCOSCELLMAXLIMITr_t holcoscellmaxlimit;
    DYNCELLLIMITr_t dyncelllimit;
    COLOR_DROP_ENr_t color_drop_en;
    SHARED_POOL_CTRLr_t shared_pool_ctrl;
    
    /* setup port bitmap according the port max speed for lossy
     */    
    num_cpu_ports = 0;
    number_of_uplink_ports = 0;
    number_of_uplink_ports_1g = 0;
    number_of_uplink_ports_2dot5g = 0;
    number_of_downlink_ports = 0;
    number_of_downlink_ports_1g = 0;
    number_of_downlink_ports_2dot5g = 0;
    

    PBMP_CLEAR(pbmp_cpu);
    PBMP_CLEAR(pbmp_uplink);
    PBMP_CLEAR(pbmp_uplink_1g);
    PBMP_CLEAR(pbmp_uplink_2dot5g);
    PBMP_CLEAR(pbmp_downlink);
    PBMP_CLEAR(pbmp_downlink_1g);
    PBMP_CLEAR(pbmp_downlink_2dot5g);
    PBMP_CLEAR(pbmp_all);
    
    PBMP_ASSIGN(pbmp_all, BCM5354X_ALL_PORTS_MASK);
    
    for (phy_port = 0; phy_port <= BCM5354X_PORT_MAX; phy_port++) {
        port = SOC_PORT_P2L_MAPPING(phy_port);
        if ((port == -1) || (SOC_PORT_L2P_MAPPING(port) == -1)) {
            continue;
        }
        
        if (IS_CPU_PORT(port)) {
            num_cpu_ports++;
            PBMP_PORT_ADD(pbmp_cpu, port);
        } else if(34 <= phy_port && phy_port <= 37){
            /* SGMII4P1 */
            number_of_uplink_ports++;
            PBMP_PORT_ADD(pbmp_uplink, port);
            
            if (SOC_PORT_SPEED_MAX(port) == 2500) {
                number_of_uplink_ports_2dot5g++;
                PBMP_PORT_ADD(pbmp_uplink_2dot5g, port);
            }else{
                number_of_uplink_ports_1g++;
                PBMP_PORT_ADD(pbmp_uplink_1g, port);
            }
        }else {
            number_of_downlink_ports++;
            PBMP_PORT_ADD(pbmp_downlink, port);
            
            if (SOC_PORT_SPEED_MAX(port) == 2500) {
                number_of_downlink_ports_2dot5g++;
                PBMP_PORT_ADD(pbmp_downlink_2dot5g, port);
            }else{
                number_of_downlink_ports_1g++;
                PBMP_PORT_ADD(pbmp_downlink_1g, port);
            }                        
        }            
    }
#if UM_DEBUG
    PBMP_ITER(pbmp_cpu, port){
        sal_printf("port=%d in pbmp_cpu\n", port);
    }
    PBMP_ITER(pbmp_downlink, port){
        sal_printf("port=%d in pbmp_downlink\n", port);
    }
    PBMP_ITER(pbmp_downlink_1g, port){
        sal_printf("port=%d in pbmp_downlink_1g\n", port);
    }
    PBMP_ITER(pbmp_downlink_2dot5g, port){
        sal_printf("port=%d in pbmp_downlink_2dot5g\n", port);
    }
    PBMP_ITER(pbmp_uplink, port){
        sal_printf("port=%d in pbmp_uplink\n", port);
    }
    PBMP_ITER(pbmp_uplink_1g, port){
        sal_printf("port=%d in pbmp_uplink_1g\n", port);
    }
    PBMP_ITER(pbmp_uplink_2dot5g, port){
        sal_printf("port=%d in pbmp_uplink_2dot5g\n", port);
    }
#endif

    standard_jumbo_frame = 9216;
    cell_size = 128;
    ethernet_mtu_cell = ceiling_func(15 * 1024 / 10, cell_size);
    standard_jumbo_frame_cell = ceiling_func(standard_jumbo_frame, cell_size);
    total_physical_memory = 4 * 1024;
    
    reserved_for_cfap = (31) * 2 + 7 + 29;
    skidmarker = 7;
    prefetch = 9;
    cfapfullsetpoint = total_physical_memory - reserved_for_cfap;
    total_cell_memory_for_admission = cfapfullsetpoint;
    total_cell_memory = total_cell_memory_for_admission;
    total_advertised_cell_memory = total_cell_memory;
    queue_port_limit_ratio = 4;
    egress_queue_min_reserve_uplink_ports_lossy = ethernet_mtu_cell;
    egress_queue_min_reserve_downlink_ports_lossy = ethernet_mtu_cell;
    egress_queue_min_reserve_uplink_ports_lossless = 0;
    egress_queue_min_reserve_downlink_ports_lossless = 0;
    egress_queue_min_reserve_cpu_ports = ethernet_mtu_cell;
    egress_xq_min_reserve_lossy_ports
          = ethernet_mtu_cell;
    egress_xq_min_reserve_lossless_uplink_ports = 0;
    egress_xq_min_reserve_lossless_downlink_ports = 0;
    num_active_pri_group_lossless = 0;
    num_lossy_queues = 4;
    mmu_xoff_pkt_threshold_uplink_ports = total_advertised_cell_memory;
    mmu_xoff_pkt_threshold_downlink_ports = total_advertised_cell_memory;
    mmu_xoff_cell_threshold_1g_port_downlink_ports
          = total_advertised_cell_memory;
    mmu_xoff_cell_threshold_2dot5g_port_downlink_ports
          = total_advertised_cell_memory;
    mmu_xoff_cell_threshold_all_uplink_ports = total_advertised_cell_memory;
    num_cpu_queues = 8;
    num_cpu_ports = 1;
    numxqs_per_uplink_ports = 512;
    numxqs_per_downlink_ports_and_cpu_port = 512;
    headroom_for_1g_port = 0;

    xoff_cell_thresholds_per_port_1g_port_downlink_ports
          = mmu_xoff_cell_threshold_1g_port_downlink_ports;
    xoff_cell_thresholds_per_port_2dot5g_downlink_ports
          = mmu_xoff_cell_threshold_2dot5g_port_downlink_ports;

    xoff_cell_threshold_all_uplink_ports
          = mmu_xoff_cell_threshold_all_uplink_ports;
    xoff_packet_thresholds_per_port_uplink_port
          = mmu_xoff_pkt_threshold_uplink_ports;
    xoff_packet_thresholds_per_port_downlink_port
          = mmu_xoff_pkt_threshold_downlink_ports;

    discard_limit_per_port_pg_uplink_port
        = xoff_cell_thresholds_per_port_1g_port_downlink_ports
          + headroom_for_1g_port;

    discard_limit_per_port_pg_downlink_port = total_advertised_cell_memory;

    total_reserved_cells_for_uplink_ports
        = egress_queue_min_reserve_uplink_ports_lossy
          * number_of_uplink_ports * num_lossy_queues
          + number_of_uplink_ports
          * egress_queue_min_reserve_uplink_ports_lossless
          * num_active_pri_group_lossless;
    total_reserved_cells_for_downlink_ports
        = number_of_downlink_ports
          * egress_queue_min_reserve_downlink_ports_lossy
          * (num_lossy_queues) + number_of_downlink_ports
          * egress_queue_min_reserve_downlink_ports_lossless
          * num_active_pri_group_lossless;
    total_reserved_cells_for_cpu_port
        = num_cpu_ports * egress_queue_min_reserve_cpu_ports
          * num_cpu_queues;
    total_reserved
        = total_reserved_cells_for_uplink_ports
          + total_reserved_cells_for_downlink_ports
          + total_reserved_cells_for_cpu_port;
    shared_space_cells = total_advertised_cell_memory - total_reserved;
    reserved_xqs_per_uplink_port
        = egress_xq_min_reserve_lossy_ports
          * num_lossy_queues + egress_xq_min_reserve_lossless_uplink_ports
          * num_active_pri_group_lossless;
    shared_xqs_per_uplink_port
          = numxqs_per_uplink_ports - reserved_xqs_per_uplink_port;
    reserved_xqs_per_downlink_port
        = egress_xq_min_reserve_lossy_ports
          * num_lossy_queues + egress_xq_min_reserve_lossless_downlink_ports
          * num_active_pri_group_lossless;
    shared_xqs_per_downlink_port
        = numxqs_per_downlink_ports_and_cpu_port
          - reserved_xqs_per_downlink_port;
    cfapfullthreshold_cfapfullsetpoint_up = cfapfullsetpoint;
    gbllimitsetlimit_gblcellsetlimit_up = total_cell_memory_for_admission;
    totaldyncellsetlimit_totaldyncellsetlimit_up = shared_space_cells;
    
    holcospktsetlimit0_pktsetlimit_up
        = shared_xqs_per_uplink_port
          + egress_xq_min_reserve_lossy_ports;

    holcospktsetlimit3_pktsetlimit_up
        = shared_xqs_per_uplink_port
          + egress_xq_min_reserve_lossy_ports;

    dynxqcntport_dynxqcntport_up
          = shared_xqs_per_uplink_port - skidmarker - prefetch;
    lwmcoscellsetlimit0_cellsetlimit_up
          = egress_queue_min_reserve_uplink_ports_lossy;

    lwmcoscellsetlimit3_cellsetlimit_up
          = egress_queue_min_reserve_uplink_ports_lossy;

    holcoscellmaxlimit0_cellmaxlimit_up
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit0_cellsetlimit_up;

    holcoscellmaxlimit3_cellmaxlimit_up
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit3_cellsetlimit_up;

    dyncelllimit_dyncellsetlimit_up = shared_space_cells;

    holcospktsetlimit0_pktsetlimit_down_1
        = shared_xqs_per_downlink_port
          + egress_xq_min_reserve_lossy_ports;

    holcospktsetlimit3_pktsetlimit_down_1
        = shared_xqs_per_downlink_port
          + egress_xq_min_reserve_lossy_ports;

    dynxqcntport_dynxqcntport_down_1
          = shared_xqs_per_downlink_port - skidmarker - prefetch;
    lwmcoscellsetlimit0_cellsetlimit_down_1 =
              egress_queue_min_reserve_downlink_ports_lossy;

    lwmcoscellsetlimit3_cellsetlimit_down_1 =
              egress_queue_min_reserve_downlink_ports_lossy;

    holcoscellmaxlimit0_cellmaxlimit_down_1
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit0_cellsetlimit_down_1;

    holcoscellmaxlimit3_cellmaxlimit_down_1
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit3_cellsetlimit_down_1;

    dyncelllimit_dyncellsetlimit_down_1 = shared_space_cells;
    
    holcospktsetlimit0_pktsetlimit_down_2dot5
        = shared_xqs_per_downlink_port
          + egress_xq_min_reserve_lossy_ports;

    holcospktsetlimit3_pktsetlimit_down_2dot5
        = shared_xqs_per_downlink_port
          + egress_xq_min_reserve_lossy_ports;

    dynxqcntport_dynxqcntport_down_2dot5
          = shared_xqs_per_downlink_port - skidmarker - prefetch;
    lwmcoscellsetlimit0_cellsetlimit_down_2dot5 =
              egress_queue_min_reserve_downlink_ports_lossy;

    lwmcoscellsetlimit3_cellsetlimit_down_2dot5 =
              egress_queue_min_reserve_downlink_ports_lossy;


    holcoscellmaxlimit0_cellmaxlimit_down_2dot5
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit0_cellsetlimit_down_2dot5;

    holcoscellmaxlimit3_cellmaxlimit_down_2dot5
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit3_cellsetlimit_down_2dot5;


    dyncelllimit_dyncellsetlimit_down_2dot5 = shared_space_cells;

    holcosminxqcnt0_holcosminxqcnt_cpu = egress_queue_min_reserve_cpu_ports;
    holcosminxqcnt3_holcosminxqcnt_cpu = egress_queue_min_reserve_cpu_ports;
    holcospktsetlimit0_pktsetlimit_cpu =
              shared_xqs_per_downlink_port + holcosminxqcnt0_holcosminxqcnt_cpu;
    holcospktsetlimit3_pktsetlimit_cpu =
              shared_xqs_per_downlink_port + holcosminxqcnt3_holcosminxqcnt_cpu;
    

    dynxqcntport_dynxqcntport_cpu =
              shared_xqs_per_downlink_port - skidmarker - prefetch;


    lwmcoscellsetlimit0_cellsetlimit_cpu = egress_queue_min_reserve_cpu_ports;
    lwmcoscellsetlimit3_cellsetlimit_cpu = egress_queue_min_reserve_cpu_ports;
    holcoscellmaxlimit0_cellmaxlimit_cpu =
              ceiling_func(shared_space_cells, queue_port_limit_ratio) +
              lwmcoscellsetlimit0_cellsetlimit_cpu;
    holcoscellmaxlimit3_cellmaxlimit_cpu =
              ceiling_func(shared_space_cells, queue_port_limit_ratio) +
              lwmcoscellsetlimit3_cellsetlimit_cpu;

    dyncelllimit_dyncellsetlimit_cpu = shared_space_cells;

    if ((shared_space_cells * cell_size)/1024 <= 100) {
        sal_printf("ERROR : Shared Pool Is Small, should be larger than 800 (value=%d)\n",
                 (shared_space_cells * cell_size)/1024);
        return SYS_ERR_PARAMETER;
    }

    /* system-based */
    READ_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);
    CFAPFULLTHRESHOLDr_CFAPFULLSETPOINTf_SET(cfapfullthreshold, cfapfullsetpoint);
    WRITE_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);

    CFAPFULLTHRESHOLDr_CFAPFULLRESETPOINTf_SET(cfapfullthreshold, 
                            cfapfullthreshold_cfapfullsetpoint_up -
                            (standard_jumbo_frame_cell));
    WRITE_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);

    READ_GBLLIMITSETLIMITr(unit, gbllimitsetlimit);
    GBLLIMITSETLIMITr_GBLCELLSETLIMITf_SET(gbllimitsetlimit, total_cell_memory_for_admission);
    WRITE_GBLLIMITSETLIMITr(unit, gbllimitsetlimit);

    READ_GBLLIMITRESETLIMITr(unit, gbllimitresetlimit);
    GBLLIMITRESETLIMITr_GBLCELLRESETLIMITf_SET(gbllimitresetlimit, gbllimitsetlimit_gblcellsetlimit_up);
    WRITE_GBLLIMITRESETLIMITr(unit, gbllimitresetlimit);

    READ_TOTALDYNCELLSETLIMITr(unit, totaldyncellsetlimit);
    TOTALDYNCELLSETLIMITr_TOTALDYNCELLSETLIMITf_SET(totaldyncellsetlimit, shared_space_cells);
    WRITE_TOTALDYNCELLSETLIMITr(unit, totaldyncellsetlimit);

    READ_TOTALDYNCELLRESETLIMITr(unit, totaldyncellresetlimit);
    TOTALDYNCELLRESETLIMITr_TOTALDYNCELLRESETLIMITf_SET(totaldyncellresetlimit, 
                            totaldyncellsetlimit_totaldyncellsetlimit_up -
                            (standard_jumbo_frame_cell * 2));
    WRITE_TOTALDYNCELLRESETLIMITr(unit, totaldyncellresetlimit);
    
    /* DYN_XQ_EN[Bit8] = 1, HOL_CELL_SOP_DROP_EN[Bit7] = 1, SKIDMARKER[Bit3:2] = 3 */
    READ_MISCCONFIGr(unit, miscconfig);
    MISCCONFIGr_MULTIPLE_ACCOUNTING_FIX_ENf_SET(miscconfig, 1);
    MISCCONFIGr_CNG_DROP_ENf_SET(miscconfig, 0);
    MISCCONFIGr_DYN_XQ_ENf_SET(miscconfig, 1);
    MISCCONFIGr_HOL_CELL_SOP_DROP_ENf_SET(miscconfig, 1);
    MISCCONFIGr_DYNAMIC_MEMORY_ENf_SET(miscconfig, 1);
    MISCCONFIGr_SKIDMARKERf_SET(miscconfig, 3);
    WRITE_MISCCONFIGr(unit, miscconfig);

    READ_MMUPORTTXENABLEr(unit, mmuporttxenable);
    MMUPORTTXENABLEr_MMUPORTTXENABLEf_SET(mmuporttxenable, 0xFFFFFFFD);
    WRITE_MMUPORTTXENABLEr(unit, mmuporttxenable);
    
    /* port-based : uplink */
    PBMP_ITER(pbmp_uplink, port) {
        mport = port; //mport == lport 
        
        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 1);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 2);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 3);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 4);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 5);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 6);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, 7);
        WRITE_PG_CTRL1r(unit, mport, pg_ctrl1);

        /* PG2TCr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            PG2TCr_PG_BMPf_SET(pg2tc, 0);
            WRITE_PG2TCr(unit, mport, index, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_packet_thresholds_per_port_uplink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr, index 0 */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_uplink_port);//discard_limit_per_port_pg_uplink_port for both 1g and 2.5g
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }        
        
        /* HOLCOSMINXQCNTr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            if ((cos >= 0) && (cos <= 2)) {
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossy_ports);
            } else if (cos == 3) {
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossy_ports);
            } else {
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, 0);
            }
            WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
        }
        
        /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            if ((cos >= 0) && (cos <= 2)) {
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                holcospktsetlimit0_pktsetlimit_up);
            } else if (cos == 3) {
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                holcospktsetlimit3_pktsetlimit_up);
            } else {
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 0);
            }
            WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
        }
        
        
        /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
            if ((cos >= 0) && (cos <= 2)) {
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                holcospktsetlimit0_pktsetlimit_up - 1);
            } else if (cos == 3) {
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                holcospktsetlimit3_pktsetlimit_up - 1);
            } else {
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 0);
            }
            WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
        }
        
        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
            CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0, 
                                numxqs_per_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
            CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1, 
                                numxqs_per_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0, 
                             numxqs_per_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1, 
                            numxqs_per_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, 
                            dynxqcntport_dynxqcntport_up);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, 
                            dynxqcntport_dynxqcntport_up - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            if ((cos >= 0) && (cos <= 2)) {
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_uplink_ports_lossy);
            } else if (cos == 3) {
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_uplink_ports_lossy);
            } else {
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 0);
            }
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            
        }
        
        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            if ((cos >= 0) && (cos <= 2)) {
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_uplink_ports_lossy);
            } else if (cos == 3) {
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_uplink_ports_lossy);
            } else {
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                0);
            }
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);      
        }

        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            if ((cos >= 0) && (cos <= 2)) {
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_up);
            } else if (cos == 3) {
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit3_cellmaxlimit_up);
            } else {
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                0);
            }
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            
        }
        
        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            if ((cos >= 0) && (cos <= 2)) {
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_up -
                                ethernet_mtu_cell);
            } else if (cos == 3) {
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit3_cellmaxlimit_up -
                                ethernet_mtu_cell);
            } else {
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                0);
            }
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, 
                            shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, 
                            dyncelllimit_dyncellsetlimit_up -
                            (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);
        
        /* COLOR_DROP_ENr, index 0 */
        READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
        COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 
                            0);
        WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);

        /* SHARED_POOL_CTRLr, index 0 */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 
                            0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        
    }

    /* port-based : downlink 1G */
    PBMP_ITER(pbmp_downlink_1g, port) {
        mport = port; //mport == lport 
        
        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 1);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 2);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 3);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 4);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 5);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 6);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, 7);
        WRITE_PG_CTRL1r(unit, mport, pg_ctrl1);

        /* PG2TCr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            PG2TCr_PG_BMPf_SET(pg2tc, 0);
            WRITE_PG2TCr(unit, mport, index, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_packet_thresholds_per_port_downlink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr, index 0 */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_cell_thresholds_per_port_1g_port_downlink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_cell_thresholds_per_port_1g_port_downlink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_downlink_port);
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }        
        
        /* HOLCOSMINXQCNTr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            if ((cos >= 0) && (cos <= 2)) {
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossy_ports);
            } else if (cos == 3) {
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossy_ports);
            } else {
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, 0);
            }
            WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
        }
        
        /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            if ((cos >= 0) && (cos <= 2)) {
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                holcospktsetlimit0_pktsetlimit_down_1);
            } else if (cos == 3) {
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                holcospktsetlimit3_pktsetlimit_down_1);
            } else {
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 0);
            }
            WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
        }
        
        
        /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
            if ((cos >= 0) && (cos <= 2)) {
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                holcospktsetlimit0_pktsetlimit_down_1 - 1);
            } else if (cos == 3) {
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                holcospktsetlimit3_pktsetlimit_down_1 - 1);
            } else {
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 0);
            }
            WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
        }
        
        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
            CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0, 
                                numxqs_per_downlink_ports_and_cpu_port - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
            CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1, 
                                numxqs_per_downlink_ports_and_cpu_port - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0, 
                             numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1, 
                            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, 
                            shared_xqs_per_downlink_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, 
                            dynxqcntport_dynxqcntport_down_1 - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            if ((cos >= 0) && (cos <= 2)) {
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_uplink_ports_lossy);
            } else if (cos == 3) {
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_uplink_ports_lossy);
            } else {
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 0);
            }
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            
        }
        
        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            if ((cos >= 0) && (cos <= 2)) {
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_uplink_ports_lossy);
            } else if (cos == 3) {
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_uplink_ports_lossy);
            } else {
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                0);
            }
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);      
        }
        
        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            if ((cos >= 0) && (cos <= 2)) {
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_down_1);
            } else if (cos == 3) {
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit3_cellmaxlimit_down_1);
            } else {
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                0);
            }
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            
        }
        
        
        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            if ((cos >= 0) && (cos <= 2)) {
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_down_1 -
                                ethernet_mtu_cell);
            } else if (cos == 3) {
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit3_cellmaxlimit_down_1 -
                                ethernet_mtu_cell);
            } else {
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                0);
            }
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, 
                            shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, 
                            dyncelllimit_dyncellsetlimit_down_1 -
                            (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);
        
        /* COLOR_DROP_ENr, index 0 */
        READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
        COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 
                            0);
        WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);

        /* SHARED_POOL_CTRLr, index 0 */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 
                            0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

    }

    /* port-based : downlink 2.5G */
    PBMP_ITER(pbmp_downlink_2dot5g, port) {
        mport = port; //mport == lport 
        
        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 1);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 2);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 3);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 4);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 5);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 6);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, 7);
        WRITE_PG_CTRL1r(unit, mport, pg_ctrl1);

        /* PG2TCr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            PG2TCr_PG_BMPf_SET(pg2tc, 0);
            WRITE_PG2TCr(unit, mport, index, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_packet_thresholds_per_port_uplink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr, index 0 */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_cell_thresholds_per_port_2dot5g_downlink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_cell_thresholds_per_port_2dot5g_downlink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_downlink_port);
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }        
        
        /* HOLCOSMINXQCNTr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            if ((cos >= 0) && (cos <= 2)) {
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossy_ports);
            } else if (cos == 3) {
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossy_ports);
            } else {
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, 0);
            }
            WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
        }
        
        /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            if ((cos >= 0) && (cos <= 2)) {
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                holcospktsetlimit0_pktsetlimit_down_2dot5);
            } else if (cos == 3) {
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                holcospktsetlimit3_pktsetlimit_down_2dot5);
            } else {
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 0);
            }
            WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
        }
        
        
        /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
            if ((cos >= 0) && (cos <= 2)) {
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                holcospktsetlimit0_pktsetlimit_down_2dot5 - 1);
            } else if (cos == 3) {
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                holcospktsetlimit3_pktsetlimit_down_2dot5 - 1);
            } else {
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 0);
            }
            WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
        }
        
        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
            CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0, 
                                numxqs_per_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
            CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1, 
                                numxqs_per_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0, 
                             numxqs_per_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1, 
                            numxqs_per_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, 
                            shared_xqs_per_uplink_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, 
                            dynxqcntport_dynxqcntport_down_2dot5 - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            if ((cos >= 0) && (cos <= 2)) {
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_uplink_ports_lossy);
            } else if (cos == 3) {
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_uplink_ports_lossy);
            } else {
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 0);
            }
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            
        }
        
        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            if ((cos >= 0) && (cos <= 2)) {
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_uplink_ports_lossy);
            } else if (cos == 3) {
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_uplink_ports_lossy);
            } else {
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                0);
            }
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);      
        }

        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            if ((cos >= 0) && (cos <= 2)) {
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_down_2dot5);
            } else if (cos == 3) {
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit3_cellmaxlimit_down_2dot5);
            } else {
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                0);
            }
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            
        }
        
        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            if ((cos >= 0) && (cos <= 2)) {
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_down_2dot5 -
                                ethernet_mtu_cell);
            } else if (cos == 3) {
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit3_cellmaxlimit_down_2dot5 -
                                ethernet_mtu_cell);
            } else {
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                0);
            }
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, 
                            shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, 
                            dyncelllimit_dyncellsetlimit_down_2dot5 -
                            (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);
        
        /* COLOR_DROP_ENr, index 0 */
        READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
        COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 
                            0);
        WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);

        /* SHARED_POOL_CTRLr, index 0 */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 
                            0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

    }

    /* port-based : cpu port (lport=pport=mport=0) */
    PBMP_ITER(pbmp_cpu, port) {
        mport = port; //mport == lport 
        
        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 1);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 2);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 3);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 4);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 5);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 6);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, 7);
        WRITE_PG_CTRL1r(unit, mport, pg_ctrl1);

        /* PG2TCr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            PG2TCr_PG_BMPf_SET(pg2tc, 0);
            WRITE_PG2TCr(unit, mport, index, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_packet_thresholds_per_port_uplink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr, index 0 */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_downlink_port);
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }        
        
        /* HOLCOSMINXQCNTr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            if ((cos >= 0) && (cos <= 2)) {
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossy_ports);
            } else if (cos == 3) {
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossy_ports);
            } else {
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, 0);
            }
            WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
        }
        
        /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            if ((cos >= 0) && (cos <= 2)) {
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                holcospktsetlimit0_pktsetlimit_cpu);
            } else if (cos == 3) {
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                holcospktsetlimit3_pktsetlimit_cpu);
            } else {
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 0);
            }
            WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
        }
        
        
        /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
            if ((cos >= 0) && (cos <= 2)) {
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                holcospktsetlimit0_pktsetlimit_cpu - 1);
            } else if (cos == 3) {
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                holcospktsetlimit3_pktsetlimit_cpu - 1);
            } else {
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 0);
            }
            WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
        }
        
        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
            CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0, 
                                numxqs_per_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
            CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1, 
                                numxqs_per_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0, 
                             numxqs_per_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1, 
                            numxqs_per_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, 
                            shared_xqs_per_uplink_port - skidmarker - prefetch);//shared_xqs_per_uplink_port instead of shared_xqs_per_cpu_port(no this one)
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, 
                            dynxqcntport_dynxqcntport_cpu - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            if ((cos >= 0) && (cos <= 2)) {
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_uplink_ports_lossy);
            } else if (cos == 3) {
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_uplink_ports_lossy);
            } else {
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 0);
            }
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            
        }
        
        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            if ((cos >= 0) && (cos <= 2)) {
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_uplink_ports_lossy);
            } else if (cos == 3) {
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_uplink_ports_lossy);
            } else {
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                0);
            }
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);      
        }

        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            if ((cos >= 0) && (cos <= 2)) {
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_cpu);
            } else if (cos == 3) {
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit3_cellmaxlimit_cpu);
            } else {
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                0);
            }
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            
        }
        
        
        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            if ((cos >= 0) && (cos <= 2)) {
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_cpu -
                                ethernet_mtu_cell);
            } else if (cos == 3) {
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit3_cellmaxlimit_cpu -
                                ethernet_mtu_cell);
            } else {
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                0);
            }
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, 
                            shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, 
                            dyncelllimit_dyncellsetlimit_cpu -
                            (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);
        
        /* COLOR_DROP_ENr, index 0 */
        READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
        COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 
                            0);
        WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);

        /* SHARED_POOL_CTRLr, index 0 */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 
                            0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
    }
    
    return SYS_OK;
}

static sys_error_t _soc_wolfhound2_mmu_init_helper_lossless(int unit)
{
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
    sys_error_t rv = SYS_OK;  
#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */
    int port, phy_port, mport;
    int index;
    int cos;

    pbmp_t pbmp_cpu;
    pbmp_t pbmp_uplink;
    pbmp_t pbmp_uplink_1g;
    pbmp_t pbmp_uplink_2dot5g;
    pbmp_t pbmp_downlink;
    pbmp_t pbmp_downlink_1g;
    pbmp_t pbmp_downlink_2dot5g;
    pbmp_t pbmp_all;
    pbmp_t pbmp_lossless_downlink_ports;
    pbmp_t lpbmp;
    
    int flow_control_type_pause_0_pfc_1 = 0;
    int standard_jumbo_frame;
    int cell_size;
    int ethernet_mtu_cell;
    int standard_jumbo_frame_cell;
    int total_physical_memory;
    int total_cell_memory_for_admission;
    
    int reserved_for_cfap;
    int skidmarker;
    int prefetch;
    int total_cell_memory;
    int cfapfullsetpoint;
    int total_advertised_cell_memory;
    int number_of_uplink_ports;
    int number_of_uplink_ports_1g;
    int number_of_uplink_ports_2dot5g;
    int number_of_downlink_ports;
    int number_of_downlink_ports_1g;
    int number_of_downlink_ports_2dot5g;
    int queue_port_limit_ratio;
    int egress_queue_min_reserve_uplink_ports_lossy;
    int egress_queue_min_reserve_downlink_ports_lossy;
    int egress_queue_min_reserve_uplink_ports_lossless;
    int egress_queue_min_reserve_downlink_ports_lossless;
    int egress_queue_min_reserve_cpu_ports;
    int egress_xq_min_reserve_lossy_ports;
    int egress_xq_min_reserve_lossless_uplink_ports;
    int egress_xq_min_reserve_lossless_downlink_ports;
    int num_active_pri_group_lossless;
    int num_lossy_queues;
    int mmu_xoff_pkt_threshold_uplink_ports;
    int mmu_xoff_pkt_threshold_downlink_ports;
    
    int mmu_xoff_cell_threshold_downlink_lossy_ports;
    int mmu_xoff_cell_threshold_downlink_lossless_ports;
    
    int mmu_xoff_cell_threshold_1g_uplink_ports;
    int num_cpu_queues;
    int num_cpu_ports;
    int numxqs_per_uplink_ports;
    int numxqs_per_downlink_ports_and_cpu_port;
    int headroom_for_lossless_downlink_port;
    int xoff_cell_thresholds_per_port_downlink_lossless_ports;
    int xoff_packet_thresholds_per_port_uplink_port;
    int xoff_packet_thresholds_per_port_downlink_port;
    int discard_limit_per_port_pg_lossy_downlink_port;
    int discard_limit_per_port_pg_downlink_2dot5g_port;
    int discard_limit_per_port_pg_uplink_port;

    int total_reserved_cells_for_uplink_ports;
    int total_reserved_cells_for_downlink_ports;
    int total_reserved_cells_for_cpu_port;
    int total_reserved;
    int shared_space_cells;
    int reserved_xqs_per_uplink_port;
    int shared_xqs_per_uplink_port;
    int reserved_xqs_per_downlink_port;
    int shared_xqs_per_downlink_port;
    int cfapfullthreshold_cfapfullsetpoint_up;
    int gbllimitsetlimit_gblcellsetlimit_up;
    int totaldyncellsetlimit_totaldyncellsetlimit_up;
    int holcospktsetlimit0_pktsetlimit_up;
    int holcospktsetlimit3_pktsetlimit_up;
    int dynxqcntport_dynxqcntport_up;
    int lwmcoscellsetlimit0_cellsetlimit_up;
    int holcoscellmaxlimit0_cellmaxlimit_up;
    int dyncelllimit_dyncellsetlimit_up;
    int holcospktsetlimit0_pktsetlimit_down_lossy;
    int holcospktsetlimit3_pktsetlimit_down_lossy;
    int dynxqcntport_dynxqcntport_down_lossy;
    int lwmcoscellsetlimit0_cellsetlimit_down_lossy;
    int holcoscellmaxlimit0_cellmaxlimit_down_lossy;
    int dyncelllimit_dyncellsetlimit_down_lossy;
    int holcospktsetlimit0_pktsetlimit_down_lossless;
    int holcospktsetlimit3_pktsetlimit_down_lossless;
    int lwmcoscellsetlimit0_cellsetlimit_down_lossless;
    int holcoscellmaxlimit0_cellmaxlimit_down_lossless;

    int holcosminxqcnt0_holcosminxqcnt_cpu;
    int holcosminxqcnt3_holcosminxqcnt_cpu;
    int holcospktsetlimit0_pktsetlimit_cpu;
    int holcospktsetlimit3_pktsetlimit_cpu;

    int lwmcoscellsetlimit0_cellsetlimit_cpu;
    int holcoscellmaxlimit0_cellmaxlimit_cpu;
    int dyncelllimit_dyncellsetlimit_cpu;
    int num_lossless_downlink_ports=0;
    int lport;

    CFAPFULLTHRESHOLDr_t cfapfullthreshold;
    GBLLIMITSETLIMITr_t gbllimitsetlimit;
    GBLLIMITRESETLIMITr_t gbllimitresetlimit;
    TOTALDYNCELLSETLIMITr_t totaldyncellsetlimit;
    TOTALDYNCELLRESETLIMITr_t totaldyncellresetlimit;
    
    MISCCONFIGr_t miscconfig;
    MMUPORTTXENABLEr_t  mmuporttxenable;
    PG_CTRL0r_t pg_ctrl0;
    PG_CTRL1r_t pg_ctrl1;
    PG2TCr_t pg2tc;    
    IBPPKTSETLIMITr_t ibppktsetlimit;
    MMU_FC_RX_ENr_t mmu_fc_rx_en;
    MMU_FC_TX_ENr_t mmu_fc_tx_en;
    PGCELLLIMITr_t pgcelllimit;
    PGDISCARDSETLIMITr_t pgdiscardsetlimit;
    HOLCOSMINXQCNTr_t holcosminxqcnt;
    HOLCOSPKTSETLIMITr_t holcospktsetlimit;
    HOLCOSPKTRESETLIMITr_t holcospktresetlimit;
    CNGCOSPKTLIMIT0r_t cngcospktlimit0;
    CNGCOSPKTLIMIT1r_t cngcospktlimit1;
    CNGPORTPKTLIMIT0r_t cngportpktlimit0;
    CNGPORTPKTLIMIT1r_t cngportpktlimit1;
    DYNXQCNTPORTr_t dynxqcntport;
    DYNRESETLIMPORTr_t dynresetlimport;
    LWMCOSCELLSETLIMITr_t lwmcoscellsetlimit;
    HOLCOSCELLMAXLIMITr_t holcoscellmaxlimit;
    DYNCELLLIMITr_t dyncelllimit;
    COLOR_DROP_ENr_t color_drop_en;
    SHARED_POOL_CTRLr_t shared_pool_ctrl;
    
    /* setup port bitmap according the port max speed for lossless
     */    
    num_cpu_ports = 0;
    number_of_uplink_ports = 0;
    number_of_uplink_ports_1g = 0;
    number_of_uplink_ports_2dot5g = 0;
    number_of_downlink_ports = 0;
    number_of_downlink_ports_1g = 0;
    number_of_downlink_ports_2dot5g = 0;
    

    PBMP_CLEAR(pbmp_cpu);
    PBMP_CLEAR(pbmp_uplink);
    PBMP_CLEAR(pbmp_uplink_1g);
    PBMP_CLEAR(pbmp_uplink_2dot5g);
    PBMP_CLEAR(pbmp_downlink);
    PBMP_CLEAR(pbmp_downlink_1g);
    PBMP_CLEAR(pbmp_downlink_2dot5g);
    PBMP_CLEAR(pbmp_all);
    PBMP_CLEAR(lpbmp);
    PBMP_CLEAR(pbmp_lossless_downlink_ports);    

#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
    rv = sal_config_uint8_get(SAL_CONFIG_MMU_LOSSLESS, &mmu_lossless);
    if ((rv == SYS_OK) && mmu_lossless) {
        //sal_printf("Vendor Config : Change to enable the lossless MMU setting.\n");
    
        PBMP_CLEAR(lpbmp);
        if (sal_config_pbmp_get(SAL_CONFIG_MMU_LOSSLESS_PORTS, &lpbmp) == SYS_OK) {
            sal_printf("Vendor Config : mmu_lossless_ports pbmp %s\n", SOC_PBMP_FORMAT(lpbmp));
            SOC_LPORT_ITER(lport) {
                if (PBMP_MEMBER(lpbmp, lport) ) {
                    sal_printf("Vendor Config : lport(%d) set to lossless\n", lport);
                    num_lossless_downlink_ports++;
                    PBMP_PORT_ADD(pbmp_lossless_downlink_ports, lport);
                    
                    if(num_lossless_downlink_ports == 4){
                        /* supports up to 4 lossless downlink ports  */
                        break;
                    }
                }
            }
        }         
    }
#endif
    
    PBMP_ASSIGN(pbmp_all, BCM5354X_ALL_PORTS_MASK);
    
    for (phy_port = 0; phy_port <= BCM5354X_PORT_MAX; phy_port++) {
        port = SOC_PORT_P2L_MAPPING(phy_port);
        if ((port == -1) || (SOC_PORT_L2P_MAPPING(port) == -1)) {
            continue;
        }
        
        if (IS_CPU_PORT(port)) {
            num_cpu_ports++;
            PBMP_PORT_ADD(pbmp_cpu, port);
        } else if(34 <= phy_port && phy_port <= 37){
            /* SGMII4P1 */
            number_of_uplink_ports++;
            PBMP_PORT_ADD(pbmp_uplink, port);
            
            if (SOC_PORT_SPEED_MAX(port) == 2500) {
                number_of_uplink_ports_2dot5g++;
                PBMP_PORT_ADD(pbmp_uplink_2dot5g, port);
            }else{
                number_of_uplink_ports_1g++;
                PBMP_PORT_ADD(pbmp_uplink_1g, port);
            }
        }else {
            number_of_downlink_ports++;
            PBMP_PORT_ADD(pbmp_downlink, port);
            
            if (SOC_PORT_SPEED_MAX(port) == 2500) {
                number_of_downlink_ports_2dot5g++;
                PBMP_PORT_ADD(pbmp_downlink_2dot5g, port);
            }else{
                number_of_downlink_ports_1g++;
                PBMP_PORT_ADD(pbmp_downlink_1g, port);
            }                        
        }            
    }
    
#if UM_DEBUG
    PBMP_ITER(pbmp_cpu, port){
        sal_printf("port=%d in pbmp_cpu\n", port);
    }
    PBMP_ITER(pbmp_downlink, port){
        sal_printf("port=%d in pbmp_downlink\n", port);
    }
    PBMP_ITER(pbmp_downlink_1g, port){
        sal_printf("port=%d in pbmp_downlink_1g\n", port);
    }
    PBMP_ITER(pbmp_downlink_2dot5g, port){
        sal_printf("port=%d in pbmp_downlink_2dot5g\n", port);
    }
    PBMP_ITER(pbmp_uplink, port){
        sal_printf("port=%d in pbmp_uplink\n", port);
    }
    PBMP_ITER(pbmp_uplink_1g, port){
        sal_printf("port=%d in pbmp_uplink_1g\n", port);
    }
    PBMP_ITER(pbmp_uplink_2dot5g, port){
        sal_printf("port=%d in pbmp_uplink_2dot5g\n", port);
    }
    PBMP_ITER(pbmp_lossless_downlink_ports, port){
        sal_printf("port=%d in pbmp_lossless_downlink_ports\n", port);
    }
#endif

    standard_jumbo_frame = 9216;
    cell_size = 128;
    ethernet_mtu_cell = ceiling_func(15 * 1024 / 10, cell_size);
    standard_jumbo_frame_cell = ceiling_func(standard_jumbo_frame, cell_size);
    total_physical_memory = 4 * 1024;
    
    reserved_for_cfap = (31) * 2 + 7 + 29;
    skidmarker = 7;
    prefetch = 9;
    cfapfullsetpoint = total_physical_memory - reserved_for_cfap;
    total_cell_memory_for_admission = cfapfullsetpoint;
    total_cell_memory = total_cell_memory_for_admission;
    total_advertised_cell_memory = total_cell_memory;
    queue_port_limit_ratio = 4;
    
    num_active_pri_group_lossless = 1;
    num_lossy_queues = 3;
    
    mmu_xoff_pkt_threshold_uplink_ports = total_advertised_cell_memory;
    mmu_xoff_pkt_threshold_downlink_ports = total_advertised_cell_memory;
    mmu_xoff_cell_threshold_downlink_lossy_ports
          = total_advertised_cell_memory;
    mmu_xoff_cell_threshold_downlink_lossless_ports
          = 36;
    mmu_xoff_cell_threshold_1g_uplink_ports = total_advertised_cell_memory;
    
    num_cpu_queues = 8;
    num_cpu_ports = 1;
    numxqs_per_uplink_ports = 512;
    numxqs_per_downlink_ports_and_cpu_port = 512;
    headroom_for_lossless_downlink_port = mmu_xoff_cell_threshold_downlink_lossless_ports;
    
    xoff_cell_thresholds_per_port_downlink_lossless_ports
          = mmu_xoff_cell_threshold_downlink_lossless_ports;
    
    xoff_packet_thresholds_per_port_uplink_port
          = mmu_xoff_pkt_threshold_uplink_ports;
    xoff_packet_thresholds_per_port_downlink_port
          = mmu_xoff_pkt_threshold_downlink_ports;

    egress_xq_min_reserve_lossless_uplink_ports = 
        (xoff_cell_thresholds_per_port_downlink_lossless_ports + mmu_xoff_cell_threshold_downlink_lossless_ports) * num_lossless_downlink_ports;
    egress_xq_min_reserve_lossless_downlink_ports = 0;
    egress_xq_min_reserve_lossy_ports
          = ethernet_mtu_cell;
    
    egress_queue_min_reserve_uplink_ports_lossy = ethernet_mtu_cell;
    egress_queue_min_reserve_downlink_ports_lossy = ethernet_mtu_cell;
    egress_queue_min_reserve_uplink_ports_lossless = egress_xq_min_reserve_lossless_uplink_ports;
    egress_queue_min_reserve_downlink_ports_lossless = 0;
    egress_queue_min_reserve_cpu_ports = ethernet_mtu_cell;
    
    discard_limit_per_port_pg_lossy_downlink_port = total_advertised_cell_memory;
    discard_limit_per_port_pg_downlink_2dot5g_port = headroom_for_lossless_downlink_port + xoff_cell_thresholds_per_port_downlink_lossless_ports;
    discard_limit_per_port_pg_uplink_port = total_advertised_cell_memory;


    total_reserved_cells_for_uplink_ports
        = egress_queue_min_reserve_uplink_ports_lossy
          * number_of_uplink_ports * num_lossy_queues
          + number_of_uplink_ports
          * egress_queue_min_reserve_uplink_ports_lossless
          * num_active_pri_group_lossless;
    total_reserved_cells_for_downlink_ports
        = number_of_downlink_ports
          * egress_queue_min_reserve_downlink_ports_lossy
          * (num_lossy_queues) + number_of_downlink_ports
          * egress_queue_min_reserve_downlink_ports_lossless
          * num_active_pri_group_lossless;
    total_reserved_cells_for_cpu_port
        = num_cpu_ports * egress_queue_min_reserve_cpu_ports
          * num_cpu_queues;
    total_reserved
        = total_reserved_cells_for_uplink_ports
          + total_reserved_cells_for_downlink_ports
          + total_reserved_cells_for_cpu_port;
    shared_space_cells = total_advertised_cell_memory - total_reserved;
    reserved_xqs_per_uplink_port
        = egress_xq_min_reserve_lossy_ports
          * num_lossy_queues + egress_xq_min_reserve_lossless_uplink_ports
          * num_active_pri_group_lossless;
    shared_xqs_per_uplink_port
          = numxqs_per_uplink_ports - reserved_xqs_per_uplink_port;
    reserved_xqs_per_downlink_port
        = egress_xq_min_reserve_lossy_ports
          * num_lossy_queues + egress_xq_min_reserve_lossless_downlink_ports
          * num_active_pri_group_lossless;
    shared_xqs_per_downlink_port
        = numxqs_per_downlink_ports_and_cpu_port
          - reserved_xqs_per_downlink_port;
    cfapfullthreshold_cfapfullsetpoint_up = cfapfullsetpoint;
    gbllimitsetlimit_gblcellsetlimit_up = total_cell_memory_for_admission;
    totaldyncellsetlimit_totaldyncellsetlimit_up = shared_space_cells;
    
    holcospktsetlimit0_pktsetlimit_up
        = shared_xqs_per_uplink_port
          + egress_xq_min_reserve_lossy_ports;

    holcospktsetlimit3_pktsetlimit_up
        = shared_xqs_per_uplink_port
          + egress_xq_min_reserve_lossless_uplink_ports;

    dynxqcntport_dynxqcntport_up
          = shared_xqs_per_uplink_port - skidmarker - prefetch;
    lwmcoscellsetlimit0_cellsetlimit_up
          = egress_queue_min_reserve_uplink_ports_lossy;

    holcoscellmaxlimit0_cellmaxlimit_up
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit0_cellsetlimit_up;
                                
    dyncelllimit_dyncellsetlimit_up = shared_space_cells;

    holcospktsetlimit0_pktsetlimit_down_lossy
        = shared_xqs_per_downlink_port
          + egress_xq_min_reserve_lossy_ports;

    holcospktsetlimit3_pktsetlimit_down_lossy
        = shared_xqs_per_downlink_port
          + egress_xq_min_reserve_lossless_downlink_ports;

    dynxqcntport_dynxqcntport_down_lossy
          = shared_xqs_per_downlink_port - skidmarker - prefetch;
    lwmcoscellsetlimit0_cellsetlimit_down_lossy =
              egress_queue_min_reserve_downlink_ports_lossy;

    holcoscellmaxlimit0_cellmaxlimit_down_lossy
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit0_cellsetlimit_down_lossy;

    dyncelllimit_dyncellsetlimit_down_lossy = shared_space_cells;
    
    holcospktsetlimit0_pktsetlimit_down_lossless
        = shared_xqs_per_downlink_port
          + egress_xq_min_reserve_lossy_ports;

    holcospktsetlimit3_pktsetlimit_down_lossless
        = shared_xqs_per_downlink_port
          + egress_xq_min_reserve_lossless_downlink_ports;

	
    lwmcoscellsetlimit0_cellsetlimit_down_lossless =
              egress_queue_min_reserve_downlink_ports_lossy;

    holcoscellmaxlimit0_cellmaxlimit_down_lossless
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit0_cellsetlimit_down_lossless;

    holcosminxqcnt0_holcosminxqcnt_cpu = egress_queue_min_reserve_cpu_ports;
    holcosminxqcnt3_holcosminxqcnt_cpu = 0;
    holcospktsetlimit0_pktsetlimit_cpu =
              shared_xqs_per_downlink_port + holcosminxqcnt0_holcosminxqcnt_cpu;
    holcospktsetlimit3_pktsetlimit_cpu =
              shared_xqs_per_downlink_port + holcosminxqcnt3_holcosminxqcnt_cpu;
    
	lwmcoscellsetlimit0_cellsetlimit_cpu = egress_queue_min_reserve_cpu_ports;
    holcoscellmaxlimit0_cellmaxlimit_cpu =
              ceiling_func(shared_space_cells, queue_port_limit_ratio) +
              lwmcoscellsetlimit0_cellsetlimit_cpu;
    
    dyncelllimit_dyncellsetlimit_cpu = shared_space_cells;

    if ((shared_space_cells * cell_size)/1024 <= 100) {
        sal_printf("ERROR : Shared Pool Is Small, should be larger than 800 (value=%d)\n",
                 (shared_space_cells * cell_size)/1024);
        return SYS_ERR_PARAMETER;
    }
    if(CFG_FLOW_CONTROL_ENABLED_COSQ >=4){
        sal_printf("ERROR : CFG_FLOW_CONTROL_ENABLED_COSQ=%d is out of range(0~3)", CFG_FLOW_CONTROL_ENABLED_COSQ);
        return SYS_ERR_PARAMETER;
    }
    
    /* system-based */
    READ_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);
    CFAPFULLTHRESHOLDr_CFAPFULLSETPOINTf_SET(cfapfullthreshold, cfapfullsetpoint);
    WRITE_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);

    CFAPFULLTHRESHOLDr_CFAPFULLRESETPOINTf_SET(cfapfullthreshold, 
                            cfapfullthreshold_cfapfullsetpoint_up -
                            (standard_jumbo_frame_cell));
    WRITE_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);

    READ_GBLLIMITSETLIMITr(unit, gbllimitsetlimit);
    GBLLIMITSETLIMITr_GBLCELLSETLIMITf_SET(gbllimitsetlimit, total_cell_memory_for_admission);
    WRITE_GBLLIMITSETLIMITr(unit, gbllimitsetlimit);

    READ_GBLLIMITRESETLIMITr(unit, gbllimitresetlimit);
    GBLLIMITRESETLIMITr_GBLCELLRESETLIMITf_SET(gbllimitresetlimit, gbllimitsetlimit_gblcellsetlimit_up);
    WRITE_GBLLIMITRESETLIMITr(unit, gbllimitresetlimit);

    READ_TOTALDYNCELLSETLIMITr(unit, totaldyncellsetlimit);
    TOTALDYNCELLSETLIMITr_TOTALDYNCELLSETLIMITf_SET(totaldyncellsetlimit, shared_space_cells);
    WRITE_TOTALDYNCELLSETLIMITr(unit, totaldyncellsetlimit);

    READ_TOTALDYNCELLRESETLIMITr(unit, totaldyncellresetlimit);
    TOTALDYNCELLRESETLIMITr_TOTALDYNCELLRESETLIMITf_SET(totaldyncellresetlimit, 
                            totaldyncellsetlimit_totaldyncellsetlimit_up -
                            (standard_jumbo_frame_cell * 2));
    WRITE_TOTALDYNCELLRESETLIMITr(unit, totaldyncellresetlimit);
    
    /* DYN_XQ_EN[Bit8] = 1, HOL_CELL_SOP_DROP_EN[Bit7] = 1, SKIDMARKER[Bit3:2] = 3 */
    READ_MISCCONFIGr(unit, miscconfig);
    MISCCONFIGr_MULTIPLE_ACCOUNTING_FIX_ENf_SET(miscconfig, 1);
    MISCCONFIGr_CNG_DROP_ENf_SET(miscconfig, 0);
    MISCCONFIGr_DYN_XQ_ENf_SET(miscconfig, 1);
    MISCCONFIGr_HOL_CELL_SOP_DROP_ENf_SET(miscconfig, 1);
    MISCCONFIGr_DYNAMIC_MEMORY_ENf_SET(miscconfig, 1);
    MISCCONFIGr_SKIDMARKERf_SET(miscconfig, 3);
    WRITE_MISCCONFIGr(unit, miscconfig);

    READ_MMUPORTTXENABLEr(unit, mmuporttxenable);
    MMUPORTTXENABLEr_MMUPORTTXENABLEf_SET(mmuporttxenable, 0xFFFFFFFD);
    WRITE_MMUPORTTXENABLEr(unit, mmuporttxenable);
    
    /* port-based : uplink */
    PBMP_ITER(pbmp_uplink, port) {
        mport = port; //mport == lport 
        
        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?0:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?1:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?2:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?3:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?4:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?5:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?6:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        WRITE_PG_CTRL1r(unit, mport, pg_ctrl1);

        /* PG2TCr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            PG2TCr_PG_BMPf_SET(pg2tc, 0);
            WRITE_PG2TCr(unit, mport, index, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_packet_thresholds_per_port_uplink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr, index 0 */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, mmu_xoff_cell_threshold_1g_uplink_ports); // mmu_xoff_cell_threshold_1g_uplink_ports for both 1g and 2.5g uplink ports
                }else{
                    PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, mmu_xoff_cell_threshold_1g_uplink_ports); // mmu_xoff_cell_threshold_1g_uplink_ports for both 1g and 2.5g uplink ports
                }
            }else{//cos==4~7
                PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, 0); 
                
            }
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;

            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, mmu_xoff_cell_threshold_1g_uplink_ports); // mmu_xoff_cell_threshold_1g_uplink_ports for both 1g and 2.5g uplink ports
                }else{
                    PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, mmu_xoff_cell_threshold_1g_uplink_ports); // mmu_xoff_cell_threshold_1g_uplink_ports for both 1g and 2.5g uplink ports
                }
            }else{//cos==4~7
                PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, 0); 
                
            }
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;

            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_uplink_port);//discard_limit_per_port_pg_uplink_port for both 1g and 2.5g
                }else{
                    PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_uplink_port);//discard_limit_per_port_pg_uplink_port for both 1g and 2.5g
                }
            }else{//cos==4~7
                PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, 0); 
                
            }
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }        
        
        /* HOLCOSMINXQCNTr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;

            READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossless_uplink_ports);
                }else{
                    HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossy_ports); //bot both lossy downlink and uplink ports
                }
            }else{//cos==4~7
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, 0);
                
            }
            WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
        }
        
        /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                holcospktsetlimit3_pktsetlimit_up);
                }else{
                    HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                holcospktsetlimit0_pktsetlimit_up);
                }
            }else{//cos==4~7
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 0);
            }
            WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
        }
                
        /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                holcospktsetlimit3_pktsetlimit_up - 1);
                }else{
                    HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                holcospktsetlimit0_pktsetlimit_up - 1);
                }
            }else{//cos==4~7
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 0);
            }
            WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
        }
        
        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
            CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0, 
                                numxqs_per_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
            CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1, 
                                numxqs_per_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0, 
                             numxqs_per_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1, 
                            numxqs_per_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, 
                            dynxqcntport_dynxqcntport_up);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, 
                            dynxqcntport_dynxqcntport_up - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_uplink_ports_lossless);
                }else{
                    LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_uplink_ports_lossy);
                }
            }else{//cos==4~7
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 0);
            }
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
        }
        
        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_uplink_ports_lossless);
                }else{
                    LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_uplink_ports_lossy);
                }
            }else{//cos==4~7
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 0);
            }
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
        }

        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_up);
                }else{
                    HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_up);
                }
            }else{//cos==4~7
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                0);
            }
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
        }
        
        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_up -
                                ethernet_mtu_cell);
                }else{
                    HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_up -
                                ethernet_mtu_cell);
                }
            }else{//cos==4~7
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                0);
            }
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, 
                            shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, 
                            dyncelllimit_dyncellsetlimit_up -
                            (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);
        
        /* COLOR_DROP_ENr, index 0 */
        READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
        COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 
                            0);
        WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);

        /* SHARED_POOL_CTRLr, index 0 */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 
                            0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        
    }

    /* port-based : downlink 1G */
    PBMP_ITER(pbmp_downlink, port) {
        mport = port; //mport == lport 
        
        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, (0x1 << CFG_FLOW_CONTROL_ENABLED_PFC));
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?0:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?1:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?2:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?3:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?4:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?5:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?6:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        WRITE_PG_CTRL1r(unit, mport, pg_ctrl1);

        /* PG2TCr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            PG2TCr_PG_BMPf_SET(pg2tc, 0);
            WRITE_PG2TCr(unit, mport, index, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_packet_thresholds_per_port_downlink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr, index 0 */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, mmu_xoff_cell_threshold_downlink_lossy_ports); 
                }else{
                    PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, mmu_xoff_cell_threshold_downlink_lossy_ports); 
                }
            }else{//cos==4~7
                PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, 0); 
            }
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;

            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, mmu_xoff_cell_threshold_downlink_lossy_ports); 
                }else{
                    PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, mmu_xoff_cell_threshold_downlink_lossy_ports); 
                }
            }else{//cos==4~7
                PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, 0); 
            }
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;

            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_lossy_downlink_port);
                }else{
                    PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_lossy_downlink_port);
                }
            }else{//cos==4~7
                PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, 0); 
                
            }
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }        
        
        /* HOLCOSMINXQCNTr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;

            READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossless_downlink_ports);
                }else{
                    HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossy_ports); //bot both lossy downlink and uplink ports
                }
            }else{//cos==4~7
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, 0);
                
            }
            WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
        }
        
        /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                holcospktsetlimit3_pktsetlimit_down_lossy);
                }else{
                    HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                holcospktsetlimit0_pktsetlimit_down_lossy);
                }
            }else{//cos==4~7
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 0);
            }
            WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
        }
                
        /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                holcospktsetlimit3_pktsetlimit_down_lossy - 1);
                }else{
                    HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                holcospktsetlimit0_pktsetlimit_down_lossy - 1);
                }
            }else{//cos==4~7
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 0);
            }
            WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
        }
        
        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
            CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0, 
                                numxqs_per_downlink_ports_and_cpu_port - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
            CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1, 
                                numxqs_per_downlink_ports_and_cpu_port - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0, 
                             numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1, 
                            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, 
                            dynxqcntport_dynxqcntport_down_lossy);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, 
                            dynxqcntport_dynxqcntport_down_lossy - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_downlink_ports_lossless);
                }else{
                    LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_downlink_ports_lossy);
                }
            }else{//cos==4~7
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 0);
            }
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
        }
        
        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_downlink_ports_lossless);
                }else{
                    LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_downlink_ports_lossy);
                }
            }else{//cos==4~7
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 0);
            }
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
        }

        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_down_lossy);
                }else{
                    HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_down_lossy);
                }
            }else{//cos==4~7
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                0);
            }
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
        }
        
        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_down_lossy -
                                ethernet_mtu_cell);
                }else{
                    HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_down_lossy -
                                ethernet_mtu_cell);
                }
            }else{//cos==4~7
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                0);
            }
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, 
                            shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, 
                            dyncelllimit_dyncellsetlimit_down_lossy -
                            (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);
        
        /* COLOR_DROP_ENr, index 0 */
        READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
        COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 
                            0);
        WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);

        /* SHARED_POOL_CTRLr, index 0 */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 
                            0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        
    }

    /* port-based : pbmp_lossless_downlink_ports */
    PBMP_ITER(pbmp_lossless_downlink_ports, port) {
        mport = port; //mport == lport 
        
        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, (0x1 << CFG_FLOW_CONTROL_ENABLED_PFC));
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?0:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?1:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?2:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?3:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?4:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?5:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?6:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:CFG_FLOW_CONTROL_ENABLED_PFC);
        WRITE_PG_CTRL1r(unit, mport, pg_ctrl1);

        /* PG2TCr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            PG2TCr_PG_BMPf_SET(pg2tc, 0);
                    
            if(index == CFG_FLOW_CONTROL_ENABLED_PFC)
                PG2TCr_PG_BMPf_SET(pg2tc, flow_control_type_pause_0_pfc_1?(0x1 << CFG_FLOW_CONTROL_ENABLED_PFC):255);
                
            WRITE_PG2TCr(unit, mport, index, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_packet_thresholds_per_port_downlink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, (0x1 << CFG_FLOW_CONTROL_ENABLED_PFC));
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr, index 0 */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, (0x1 << CFG_FLOW_CONTROL_ENABLED_PFC));
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_PFC){
                    PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, mmu_xoff_cell_threshold_downlink_lossless_ports); 
                }else{
                    PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, mmu_xoff_cell_threshold_downlink_lossy_ports); 
                }
            }else{//cos==4~7
                PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, 0); 
            }
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;

            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_PFC){
                    PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, mmu_xoff_cell_threshold_downlink_lossless_ports); 
                }else{
                    PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, mmu_xoff_cell_threshold_downlink_lossy_ports); 
                }
            }else{//cos==4~7
                PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, 0); 
            }
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;

            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_PFC){
                    PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_downlink_2dot5g_port);
                }else{
                    PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_lossy_downlink_port);
                }
            }else{//cos==4~7
                PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, 0); 
                
            }
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }        
        
        /* HOLCOSMINXQCNTr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;

            READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossless_downlink_ports);
                }else{
                    HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossy_ports); //bot both lossy downlink and uplink ports
                }
            }else{//cos==4~7
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, 0);
                
            }
            WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
        }
        
        /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                holcospktsetlimit3_pktsetlimit_down_lossless);
                }else{
                    HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                holcospktsetlimit0_pktsetlimit_down_lossless);
                }
            }else{//cos==4~7
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 0);
            }
            WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
        }
                
        /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                holcospktsetlimit3_pktsetlimit_down_lossless - 1);
                }else{
                    HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                holcospktsetlimit0_pktsetlimit_down_lossless - 1);
                }
            }else{//cos==4~7
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 0);
            }
            WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
        }
        
        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
            CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0, 
                                numxqs_per_downlink_ports_and_cpu_port - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
            CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1, 
                                numxqs_per_downlink_ports_and_cpu_port - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0, 
                             numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1, 
                            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, 
                            dynxqcntport_dynxqcntport_down_lossy);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, 
                            dynxqcntport_dynxqcntport_down_lossy - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_downlink_ports_lossless);
                }else{
                    LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_downlink_ports_lossy);
                }
            }else{//cos==4~7
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 0);
            }
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
        }
        
        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_downlink_ports_lossless);
                }else{
                    LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_downlink_ports_lossy);
                }
            }else{//cos==4~7
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 0);
            }
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
        }

        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_down_lossless);
                }else{
                    HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_down_lossless);
                }
            }else{//cos==4~7
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                0);
            }
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
        }
        
        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_down_lossless -
                                ethernet_mtu_cell);
                }else{
                    HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_down_lossless -
                                ethernet_mtu_cell);
                }
            }else{//cos==4~7
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                0);
            }
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, 
                            shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, 
                            dyncelllimit_dyncellsetlimit_down_lossy -
                            (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);
        
        /* COLOR_DROP_ENr, index 0 */
        READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
        COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 
                            0);
        WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);

        /* SHARED_POOL_CTRLr, index 0 */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 
                            0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        
    }

    /* port-based : cpu port (lport=pport=mport=0) */
    PBMP_ITER(pbmp_cpu, port) {
        mport = port; //mport == lport 
        
        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 1);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 2);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 3);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 4);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 5);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 6);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, 7);
        WRITE_PG_CTRL1r(unit, mport, pg_ctrl1);

        /* PG2TCr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            PG2TCr_PG_BMPf_SET(pg2tc, 0);
            WRITE_PG2TCr(unit, mport, index, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_packet_thresholds_per_port_downlink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr, index 0 */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, mmu_xoff_cell_threshold_downlink_lossy_ports); 
                }else{
                    PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, mmu_xoff_cell_threshold_downlink_lossy_ports); 
                }
            }else{//cos==4~7
                PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, 0); 
            }
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;

            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, mmu_xoff_cell_threshold_downlink_lossy_ports); 
                }else{
                    PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, mmu_xoff_cell_threshold_downlink_lossy_ports); 
                }
            }else{//cos==4~7
                PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, 0); 
            }
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;

            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_lossy_downlink_port);
                }else{
                    PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_lossy_downlink_port);
                }
            }else{//cos==4~7
                PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, 0); 
                
            }
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }        
        
        /* HOLCOSMINXQCNTr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;

            READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, 0);
                }else{
                    HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_queue_min_reserve_cpu_ports); 
                }
            }else{//cos==4~7
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, 0);
                
            }
            WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
        }
        
        /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                holcospktsetlimit3_pktsetlimit_cpu);
                }else{
                    HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                holcospktsetlimit0_pktsetlimit_cpu );
                }
            }else{//cos==4~7
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 0);
            }
            WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
        }
                
        /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                holcospktsetlimit3_pktsetlimit_cpu - 1);
                }else{
                    HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                holcospktsetlimit0_pktsetlimit_cpu - 1);
                }
            }else{//cos==4~7
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 0);
            }
            WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
        }
        
        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
            CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0, 
                                numxqs_per_downlink_ports_and_cpu_port - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
            CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1, 
                                numxqs_per_downlink_ports_and_cpu_port - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0, 
                             numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1, 
                            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, 
                            dynxqcntport_dynxqcntport_down_lossy);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, 
                            dynxqcntport_dynxqcntport_down_lossy - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_cpu_ports);
                }else{
                    LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_cpu_ports);
                }
            }else{//cos==4~7
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 0);
            }
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
        }
        
        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_cpu_ports);
                }else{
                    LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_cpu_ports);
                }
            }else{//cos==4~7
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 0);
            }
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
        }

        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_cpu);
                }else{
                    HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_cpu);
                }
            }else{//cos==4~7
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                0);
            }
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
        }
        
        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            cos = index;
            
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            if(cos < 4){
                if (cos == CFG_FLOW_CONTROL_ENABLED_COSQ){
                    HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_cpu -
                                ethernet_mtu_cell);
                }else{
                    HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit0_cellmaxlimit_cpu -
                                ethernet_mtu_cell);
                }
            }else{//cos==4~7
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                0);
            }
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, 
                            shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, 
                            dyncelllimit_dyncellsetlimit_cpu -
                            (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);
        
        /* COLOR_DROP_ENr, index 0 */
        READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
        COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 
                            0);
        WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);

        /* SHARED_POOL_CTRLr, index 0 */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 
                            0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        
    }
    
    return SYS_OK;
}

#if CFG_MMU_DEBUG
static void _soc_wolfhound2_mmu_init_debug(int unit)
{
    int port, mport;
    int index;
    pbmp_t pbmp_all;
    
    CFAPFULLTHRESHOLDr_t cfapfullthreshold;
    GBLLIMITSETLIMITr_t gbllimitsetlimit;
    GBLLIMITRESETLIMITr_t gbllimitresetlimit;
    TOTALDYNCELLSETLIMITr_t totaldyncellsetlimit;
    TOTALDYNCELLRESETLIMITr_t totaldyncellresetlimit;
    TWO_LAYER_SCH_MODEr_t two_layer_sch_mode;
    MISCCONFIGr_t miscconfig;
    MMUPORTTXENABLE_0r_t mmuporttxenable_0;
    MMUPORTTXENABLE_1r_t mmuporttxenable_1;
    MMUPORTTXENABLE_2r_t mmuporttxenable_2;
    PG_CTRL0r_t pg_ctrl0;
    PG_CTRL1r_t pg_ctrl1;
    PG2TCr_t pg2tc;    
    IBPPKTSETLIMITr_t ibppktsetlimit;
    MMU_FC_RX_ENr_t mmu_fc_rx_en;
    MMU_FC_TX_ENr_t mmu_fc_tx_en;
    PGCELLLIMITr_t pgcelllimit;
    PGDISCARDSETLIMITr_t pgdiscardsetlimit;
    HOLCOSMINXQCNTr_t holcosminxqcnt;
    HOLCOSMINXQCNT_QLAYERr_t holcosminxqcnt_qlayer;
    HOLCOSPKTSETLIMITr_t holcospktsetlimit;
    HOLCOSPKTSETLIMIT_QLAYERr_t holcospktsetlimit_qlayer;
    HOLCOSPKTRESETLIMITr_t holcospktresetlimit;
    HOLCOSPKTRESETLIMIT_QLAYERr_t holcospktresetlimit_qlayer;
    CNGCOSPKTLIMIT0r_t cngcospktlimit0;
    CNGCOSPKTLIMIT1r_t cngcospktlimit1;
    CNGCOSPKTLIMIT0_QLAYERr_t cngcospktlimit0_qlayer;
    CNGCOSPKTLIMIT1_QLAYERr_t cngcospktlimit1_qlayer;
    CNGPORTPKTLIMIT0r_t cngportpktlimit0;
    CNGPORTPKTLIMIT1r_t cngportpktlimit1;
    DYNXQCNTPORTr_t dynxqcntport;
    DYNRESETLIMPORTr_t dynresetlimport;
    LWMCOSCELLSETLIMITr_t lwmcoscellsetlimit;
    LWMCOSCELLSETLIMIT_QLAYERr_t lwmcoscellsetlimit_qlayer;
    HOLCOSCELLMAXLIMITr_t holcoscellmaxlimit;
    HOLCOSCELLMAXLIMIT_QLAYERr_t holcoscellmaxlimit_qlayer;
    DYNCELLLIMITr_t dyncelllimit;
    COLOR_DROP_ENr_t color_drop_en;
    COLOR_DROP_EN_QLAYERr_t color_drop_en_qlayer;
    HOLCOSPKTSETLIMIT_QGROUPr_t holcospktsetlimit_qgroup;
    HOLCOSPKTRESETLIMIT_QGROUPr_t holcospktresetlimit_qgroup;
    CNGCOSPKTLIMIT0_QGROUPr_t cngcospktlimit0_qgroup;
    CNGCOSPKTLIMIT1_QGROUPr_t cngcospktlimit1_qgroup;
    HOLCOSCELLMAXLIMIT_QGROUPr_t holcoscellmaxlimit_qgroup;
    COLOR_DROP_EN_QGROUPr_t color_drop_en_qgroup;
    SHARED_POOL_CTRLr_t shared_pool_ctrl;
    SHARED_POOL_CTRL_EXT1r_t shared_pool_ctrl_ext1;
    SHARED_POOL_CTRL_EXT2r_t shared_pool_ctrl_ext2;

    PBMP_ASSIGN(pbmp_all, BCM5354X_ALL_PORTS_MASK);

    sal_printf("\n########################### MMU SETTING #############################\n");

    /* system-based */
    sal_printf("\nSystem-Based ==>\n");
    READ_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);
    sal_printf("CFAPFULLTHRESHOLD.CFAPFULLSETPOINT 0x%x\n", CFAPFULLTHRESHOLDr_CFAPFULLSETPOINTf_GET(cfapfullthreshold));
    sal_printf("CFAPFULLTHRESHOLD.CFAPFULLRESETPOINT 0x%x\nx", CFAPFULLTHRESHOLDr_CFAPFULLRESETPOINTf_GET(cfapfullthreshold));
    READ_GBLLIMITSETLIMITr(unit, gbllimitsetlimit);
    sal_printf("GBLLIMITSETLIMIT.GBLCELLSETLIMIT 0x%x\n", GBLLIMITSETLIMITr_GBLCELLSETLIMITf_GET(gbllimitsetlimit));
    READ_GBLLIMITRESETLIMITr(unit, gbllimitresetlimit);
    sal_printf("GBLLIMITRESETLIMIT.GBLCELLRESETLIMIT 0x%x\n", GBLLIMITRESETLIMITr_GBLCELLRESETLIMITf_GET(gbllimitresetlimit));
    READ_TOTALDYNCELLSETLIMITr(unit, totaldyncellsetlimit);
    sal_printf("TOTALDYNCELLSETLIMIT.TOTALDYNCELLSETLIMIT 0x%x\n", TOTALDYNCELLSETLIMITr_TOTALDYNCELLSETLIMITf_GET(totaldyncellsetlimit));
    READ_TOTALDYNCELLRESETLIMITr(unit, totaldyncellresetlimit);
    sal_printf("TOTALDYNCELLRESETLIMIT.TOTALDYNCELLRESETLIMIT 0x%x\n", TOTALDYNCELLRESETLIMITr_TOTALDYNCELLRESETLIMITf_GET(totaldyncellresetlimit));

    PBMP_ITER(pbmp_all, port) {
        if (SOC_PORT_BLOCK_INDEX(port) != 0x0) {
            continue;
        }
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
        if (mport >= MMU_64Q_PPORT_BASE) {
            mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
            READ_TWO_LAYER_SCH_MODEr(unit, mport, two_layer_sch_mode);
            sal_printf("port %d mport %d TWO_LAYER_SCH_MODE.SCH_MODE 0x%x\n", port, mport, TWO_LAYER_SCH_MODEr_SCH_MODEf_GET(two_layer_sch_mode));
        }
    }

    READ_MISCCONFIGr(unit, miscconfig);
    sal_printf("MISCCONFIG.MULTIPLE_ACCOUNTING_FIX_EN 0x%x\n", MISCCONFIGr_MULTIPLE_ACCOUNTING_FIX_ENf_GET(miscconfig));
    sal_printf("MISCCONFIG.CNG_DROP_EN 0x%x\n", MISCCONFIGr_CNG_DROP_ENf_GET(miscconfig));
    sal_printf("MISCCONFIG.DYN_XQ_EN 0x%x\n", MISCCONFIGr_DYN_XQ_ENf_GET(miscconfig));
    sal_printf("MISCCONFIG.HOL_CELL_SOP_DROP_EN 0x%x\n", MISCCONFIGr_HOL_CELL_SOP_DROP_ENf_GET(miscconfig));
    sal_printf("MISCCONFIG.DYNAMIC_MEMORY_EN 0x%x\n", MISCCONFIGr_DYNAMIC_MEMORY_ENf_GET(miscconfig));
    sal_printf("MISCCONFIG.SKIDMARKER 0x%x\n", MISCCONFIGr_SKIDMARKERf_GET(miscconfig));

    READ_MMUPORTTXENABLE_0r(unit, mmuporttxenable_0);
    sal_printf("MMUPORTTXENABLE_0.MMUPORTTXENABLE 0x%x\n", MMUPORTTXENABLE_0r_MMUPORTTXENABLEf_GET(mmuporttxenable_0));

    READ_MMUPORTTXENABLE_1r(unit, mmuporttxenable_1);
    sal_printf("MMUPORTTXENABLE_1.MMUPORTTXENABLE 0x%x\n", MMUPORTTXENABLE_1r_MMUPORTTXENABLEf_GET(mmuporttxenable_1));

    READ_MMUPORTTXENABLE_2r(unit, mmuporttxenable_2);
    sal_printf("MMUPORTTXENABLE_2.MMUPORTTXENABLE 0x%x\n", MMUPORTTXENABLE_2r_MMUPORTTXENABLEf_GET(mmuporttxenable_2));
    sal_printf("\n");

    PBMP_PORT_ADD(pbmp_all, BCM5354X_PORT_CMIC);
    PBMP_ITER(pbmp_all, port) {
        if (IS_CPU_PORT(port)) {
            mport = 0;
            sal_printf("\nPort-Based (cpu port) ==>\n");
        } else {
            if (SOC_PORT_BLOCK_INDEX(port) != 0x0) {
                continue;
            }

            mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
            sal_printf("\nPort-Based (port %d mport %d speed %d) ==>\n", port, mport, SOC_PORT_SPEED_MAX(port));
        }

        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        sal_printf("PG_CTRL0.PPFC_PG_EN 0x%x\n", PG_CTRL0r_PPFC_PG_ENf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI0_GRP 0x%x\n", PG_CTRL0r_PRI0_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI1_GRP 0x%x\n", PG_CTRL0r_PRI1_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI2_GRP 0x%x\n", PG_CTRL0r_PRI2_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI3_GRP 0x%x\n", PG_CTRL0r_PRI3_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI4_GRP 0x%x\n", PG_CTRL0r_PRI4_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI5_GRP 0x%x\n", PG_CTRL0r_PRI5_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI6_GRP 0x%x\n", PG_CTRL0r_PRI6_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI7_GRP 0x%x\n", PG_CTRL0r_PRI7_GRPf_GET(pg_ctrl0));

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, pg_ctrl1);
        sal_printf("PG_CTRL1.PRI8_GRP 0x%x\n", PG_CTRL1r_PRI8_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI9_GRP 0x%x\n", PG_CTRL1r_PRI9_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI10_GRP 0x%x\n", PG_CTRL1r_PRI10_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI11_GRP 0x%x\n", PG_CTRL1r_PRI11_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI12_GRP 0x%x\n", PG_CTRL1r_PRI12_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI13_GRP 0x%x\n", PG_CTRL1r_PRI13_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI14_GRP 0x%x\n", PG_CTRL1r_PRI14_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI15_GRP 0x%x\n", PG_CTRL1r_PRI15_GRPf_GET(pg_ctrl1));

        /* PG2TCr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            sal_printf("COSQ %d PG2TC.PG_BMP 0x%x\n", index, PG2TCr_PG_BMPf_GET(pg2tc));
        }

        /* IBPPKTSETLIMITr, index 0 */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        sal_printf("IBPPKTSETLIMIT.PKTSETLIMIT 0x%x\n", IBPPKTSETLIMITr_PKTSETLIMITf_GET(ibppktsetlimit));
        sal_printf("IBPPKTSETLIMIT.RESETLIMITSEL 0x%x\n", IBPPKTSETLIMITr_RESETLIMITSELf_GET(ibppktsetlimit));

        /* MMU_FC_RX_ENr, index 0 */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        sal_printf("MMU_FC_RX_EN.MMU_FC_RX_ENABLE 0x%x\n", MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_GET(mmu_fc_rx_en));

        /* MMU_FC_TX_ENr, index 0 */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        sal_printf("MMU_FC_TX_EN.MMU_FC_TX_ENABLE 0x%x\n", MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_GET(mmu_fc_tx_en));

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            sal_printf("COSQ %d PGCELLLIMIT.CELLSETLIMIT 0x%x\n", index, PGCELLLIMITr_CELLSETLIMITf_GET(pgcelllimit));
        }

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            sal_printf("COSQ %d PGCELLLIMIT.CELLRESETLIMIT 0x%x\n", index, PGCELLLIMITr_CELLRESETLIMITf_GET(pgcelllimit));
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            sal_printf("COSQ %d PGDISCARDSETLIMIT.DISCARDSETLIMIT 0x%x\n", index, PGDISCARDSETLIMITr_DISCARDSETLIMITf_GET(pgdiscardsetlimit));
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSMINXQCNTr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
                sal_printf("COSQ %d HOLCOSMINXQCNT.HOLCOSMINXQCNT 0x%x\n", index, HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_GET(holcosminxqcnt));
            }
        } else {
            /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 8 */
            for (index = 0; index <= 8; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
                sal_printf("COSQ %d HOLCOSMINXQCNT_QLAYER.HOLCOSMINXQCNT 0x%x\n", index, HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_GET(holcosminxqcnt_qlayer));
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
                sal_printf("COSQ %d HOLCOSPKTSETLIMIT.PKTSETLIMIT 0x%x\n", index, HOLCOSPKTSETLIMITr_PKTSETLIMITf_GET(holcospktsetlimit));
            }
        } else {
            /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 8 */
            for (index = 0; index <= 8; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
                sal_printf("COSQ %d HOLCOSPKTSETLIMIT_QLAYER.PKTSETLIMIT 0x%x\n", index, HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_GET(holcospktsetlimit_qlayer));
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
                sal_printf("COSQ %d HOLCOSPKTRESETLIMIT.PKTRESETLIMIT 0x%x\n", index, HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_GET(holcospktresetlimit));
            }
        } else {
            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 8 */
            for (index = 0; index <= 8; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
                sal_printf("COSQ %d HOLCOSPKTRESETLIMIT_QLAYER.PKTRESETLIMIT 0x%x\n", index, HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_GET(holcospktresetlimit_qlayer));
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
                sal_printf("COSQ %d CNGCOSPKTLIMIT0.CNGPKTSETLIMIT0 0x%x\n", index, CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_GET(cngcospktlimit0));
            }

            /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
                sal_printf("COSQ %d CNGCOSPKTLIMIT1.CNGPKTSETLIMIT1 0x%x\n", index, CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_GET(cngcospktlimit1));
            }
        } else {
            /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 8 */
            for (index = 0; index <= 8; index++) {
                READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index, cngcospktlimit0_qlayer);
                sal_printf("COSQ %d CNGCOSPKTLIMIT0_QLAYER.CNGPKTSETLIMIT0 0x%x\n", index, CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_GET(cngcospktlimit0_qlayer));
            }

            /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 8 */
            for (index = 0; index <=8 ; index++) {
                READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index, cngcospktlimit1_qlayer);
                sal_printf("COSQ %d CNGCOSPKTLIMIT1_QLAYER.CNGPKTSETLIMIT1 0x%x\n", index, CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_GET(cngcospktlimit1_qlayer));
            }
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        sal_printf("CNGPORTPKTLIMIT0.CNGPORTPKTLIMIT0 0x%x\n", CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_GET(cngportpktlimit0));

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        sal_printf("CNGPORTPKTLIMIT1.CNGPORTPKTLIMIT1 0x%x\n", CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_GET(cngportpktlimit1));

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        sal_printf("DYNXQCNTPORT.DYNXQCNTPORT 0x%x\n", DYNXQCNTPORTr_DYNXQCNTPORTf_GET(dynxqcntport));

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        sal_printf("DYNRESETLIMPORT.DYNRESETLIMPORT 0x%x\n", DYNRESETLIMPORTr_DYNRESETLIMPORTf_GET(dynresetlimport));

        if (mport < MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
                sal_printf("COSQ %d LWMCOSCELLSETLIMIT.CELLSETLIMIT 0x%x\n", index, LWMCOSCELLSETLIMITr_CELLSETLIMITf_GET(lwmcoscellsetlimit));
            }
        } else {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 8 */
            for (index = 0; index <= 8; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);            
                sal_printf("COSQ %d LWMCOSCELLSETLIMIT_QLAYER.CELLSETLIMIT 0x%x\n", index, LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_GET(lwmcoscellsetlimit_qlayer));
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
                sal_printf("COSQ %d LWMCOSCELLSETLIMIT.CELLRESETLIMIT 0x%x\n", index, LWMCOSCELLSETLIMITr_CELLRESETLIMITf_GET(lwmcoscellsetlimit));
            }
        } else {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 8 */
            for (index = 0; index <= 8; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                sal_printf("COSQ %d LWMCOSCELLSETLIMIT_QLAYER.CELLRESETLIMIT 0x%x\n", index, LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_GET(lwmcoscellsetlimit_qlayer));
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
                sal_printf("COSQ %d HOLCOSCELLMAXLIMIT.CELLMAXLIMIT 0x%x\n", index, HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_GET(holcoscellmaxlimit));
            }
        } else {
            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 8 */
            for (index = 0; index <= 8; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                sal_printf("COSQ %d HOLCOSCELLMAXLIMIT_QLAYER.CELLMAXLIMIT 0x%x\n", index, HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_GET(holcoscellmaxlimit_qlayer));
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
                sal_printf("COSQ %d HOLCOSCELLMAXLIMIT.CELLMAXRESUMELIMIT 0x%x\n", index, HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_GET(holcoscellmaxlimit));
            }
        } else {
            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 8 */
            for (index = 0; index <= 8; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                sal_printf("COSQ %d HOLCOSCELLMAXLIMIT_QLAYER.CELLMAXRESUMELIMIT 0x%x\n", index, HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_GET(holcoscellmaxlimit_qlayer));
            }
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        sal_printf("DYNCELLLIMIT.DYNCELLSETLIMIT 0x%x\n", DYNCELLLIMITr_DYNCELLSETLIMITf_GET(dyncelllimit));
        sal_printf("DYNCELLLIMIT.DYNCELLRESETLIMIT 0x%x\n", DYNCELLLIMITr_DYNCELLRESETLIMITf_GET(dyncelllimit));

        if (mport < MMU_64Q_PPORT_BASE) {
            /* COLOR_DROP_ENr, index 0 */
            READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
            sal_printf("COLOR_DROP_EN.COLOR_DROP_EN 0x%x\n", COLOR_DROP_ENr_COLOR_DROP_ENf_GET(color_drop_en));
        } else {
            /* COLOR_DROP_EN_QLAYERr, index 0 */
            READ_COLOR_DROP_EN_QLAYERr(unit, mport, 0, color_drop_en_qlayer);
            sal_printf("COLOR_DROP_EN_QLAYER.COLOR_DROP_EN 0x%x\n", COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_GET(color_drop_en_qlayer));

            /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index, holcospktsetlimit_qgroup);
                sal_printf("COSQ %d HOLCOSPKTSETLIMIT_QGROUP.PKTSETLIMIT 0x%x\n", index, HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_GET(holcospktsetlimit_qgroup));
            }

            /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index, holcospktresetlimit_qgroup);
                sal_printf("COSQ %d HOLCOSPKTRESETLIMIT_QGROUP.PKTRESETLIMIT 0x%x\n", index, HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_GET(holcospktresetlimit_qgroup));
            }

            /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index, cngcospktlimit0_qgroup);
                sal_printf("COSQ %d CNGCOSPKTLIMIT0_QGROUP.CNGPKTSETLIMIT0 0x%x\n", index, CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_GET(cngcospktlimit0_qgroup));
            }

            /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index, cngcospktlimit1_qgroup);
                sal_printf("COSQ %d CNGCOSPKTLIMIT1_QGROUP.NGPKTSETLIMIT1 0x%x\n", index, CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_GET(cngcospktlimit1_qgroup));
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
                sal_printf("COSQ %d HOLCOSCELLMAXLIMIT_QGROUP.CELLMAXLIMIT 0x%x\n", index, HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_GET(holcoscellmaxlimit_qgroup));
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
                sal_printf("COSQ %d HOLCOSCELLMAXLIMIT_QGROUP.CELLMAXRESUMELIMIT 0x%x\n", index, HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_GET(holcoscellmaxlimit_qgroup));
            }

            /* COLOR_DROP_EN_QGROUPr, index 0 */
            READ_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
            sal_printf("COLOR_DROP_EN_QGROUP.COLOR_DROP_EN 0x%x\n", COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_GET(color_drop_en_qgroup));
        }

        /* SHARED_POOL_CTRLr, index 0 */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        sal_printf("SHARED_POOL_CTRL.DYNAMIC_COS_DROP_EN 0x%x\n", SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_GET(shared_pool_ctrl));
        sal_printf("SHARED_POOL_CTRL.SHARED_POOL_DISCARD_EN 0x%x\n", SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_GET(shared_pool_ctrl));
        sal_printf("SHARED_POOL_CTRL.SHARED_POOL_XOFF_EN 0x%x\n", SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_GET(shared_pool_ctrl));

        if (mport >= MMU_64Q_PPORT_BASE) {
            /* SHARED_POOL_CTRL_EXT1r, index 0 */
            READ_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);
            sal_printf("SHARED_POOL_CTRL_EXT1.DYNAMIC_COS_DROP_EN 0x%x\n", SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_GET(shared_pool_ctrl_ext1));

            /* SHARED_POOL_CTRL_EXT2r, index 0 */
            READ_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
            sal_printf("SHARED_POOL_CTRL_EXT2.DYNAMIC_COS_DROP_EN 0x%x\n", SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_GET(shared_pool_ctrl_ext2));
        }
    }
}
#endif /* CFG_MMU_DEBUG */



void
soc_mmu_init(uint8 unit)
{
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
    sys_error_t rv = SYS_OK;  
#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */
    MMUPORTENABLEr_t mmuportenable;
    IP_TO_CMICM_CREDIT_TRANSFERr_t ip_to_cmicm_credit_transfer;
    pbmp_t lpbmp;
   
    soc_tdm_init(unit);
    
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
    rv = sal_config_uint8_get(SAL_CONFIG_MMU_LOSSLESS, &mmu_lossless);
#endif

    if ((rv == SYS_OK) && mmu_lossless) {
        rv = _soc_wolfhound2_mmu_init_helper_lossless(unit);
    } else {
        rv = _soc_wolfhound2_mmu_init_helper_lossy(unit);
    }

#if CFG_MMU_DEBUG
    _soc_wolfhound2_mmu_init_debug(unit);
#endif /* CFG_MMU_DEBUG */

    
    /* 
     * Enable IP to CMIC credit transfer: 
     * TRANSFER_ENABLE =1, NUM_OF_CREDITS = 32 
     */
    IP_TO_CMICM_CREDIT_TRANSFERr_CLR(ip_to_cmicm_credit_transfer);
    IP_TO_CMICM_CREDIT_TRANSFERr_NUM_OF_CREDITSf_SET(ip_to_cmicm_credit_transfer, 32);
    IP_TO_CMICM_CREDIT_TRANSFERr_TRANSFER_ENABLEf_SET(ip_to_cmicm_credit_transfer, 1);
    WRITE_IP_TO_CMICM_CREDIT_TRANSFERr(unit, ip_to_cmicm_credit_transfer);
    

    /*
     * Port enable
     */
    PBMP_ASSIGN(lpbmp, BCM5354X_ALL_PORTS_MASK);
    /* Port enable */
#if CFG_RXTX_SUPPORT_ENABLED
    /* Add CPU port */
    PBMP_PORT_ADD(lpbmp, 0);
#endif /* CFG_RXTX_SUPPORT_ENABLED */
    MMUPORTENABLEr_CLR(mmuportenable);
    MMUPORTENABLEr_MMUPORTENABLEf_SET(mmuportenable, PBMP_WORD_GET(lpbmp, 0));
    WRITE_MMUPORTENABLEr(unit, mmuportenable);
    
}
