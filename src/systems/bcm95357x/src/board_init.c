/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
 
#include "system.h"
#include "brdimpl.h"
#include "bsp_config.h"
#include "brdimpl.h"
#include "utils/nvram.h"
#include "utils/system.h"
#include "ns16550.h"

/* __BOOT_UART_TRAP: For debug only !!! please define it as zero at final product
                                     In 50ms since power on,  If there is any character comes from uart,
                                     system will not perform normal booting flow and stay at bootcode recovery stage. */
#ifdef __BOOTLOADER__
#define __BOOTLOADER_UART_TRAP__ 0
#endif

#if !CFG_TIMER_USE_INTERRUPT
extern void sal_timer_task(void *param);
#endif /* !CFG_TIMER_USE_INTERRUPT */

extern void enable_arm_cyclecount(void);

#if CFG_CONSOLE_ENABLED
static const char *um_fwname = target;
#endif /* CFG_CONSOLE_ENABLED */

#ifdef CFG_RESET_BUTTON_INCLUDED
uint8 reset_button_enable = 0;
#endif /* CFG_RESET_BUTTON_INCLUDED */

#ifdef CFG_LED_MICROCODE_INCLUDED

/* Board Serial LED layout */
/* Ex: BCM956174R */
const uint8 BCM956174R_ledup0[] = 
{
    57, 56, 55, 54, 53, 52, 51, 50, /* QTC0.0 QTC0.1 */
    49, 48, 47, 46, 45, 44, 43, 42, /* QTC1.1 QTC1.1 */
    33, 32, 31, 30, 29, 28, 27, 26  /* QTC1.2 QTC1.3 */
};

const uint8 BCM956174R_ledup1[] = 
{
    86,              /* CLPORT0 */
    82,              /* XLPORT6 */ 
    77, 76, 75, 74,  /* XLPORT4 */
};

const ledup_ctrl_t BCM956174R_ledup_ctrl = {
   .pport_seq = { (uint8 *)BCM956174R_ledup0, (uint8 *)BCM956174R_ledup1 }, 
   .port_count = { sizeof(BCM956174R_ledup0), sizeof(BCM956174R_ledup1) },
   .leds_per_port = 2,
   .bits_per_led  = 1,   
   .num_of_color = 1,   
   .led_blink_period = 50,    /* 30 ms * 50 = 1.5s */
   .led_tx_rx_extension = 2,  /* 2 *30ms = 60ms */          
   .bits_color = {0, 0, 0, 0},
   .bits_color_off = 1,
   .fix_bits_total = {0, 0},
   .led_mode = {LED_MODE_LINK, LED_MODE_TXRX, 0},
};


void
board_ledup_init(const ledup_ctrl_t* p_ledup) {
    bcm5357x_ledup_init(p_ledup);
}

#endif
/* Access to shadowed registers at offset 0x1c */
#define REG_1C_SEL(_s)                  ((_s) << 10)
#define REG_1C_WR(_s,_v)                (REG_1C_SEL(_s) | (_v) | 0x8000)

/* Access expansion registers at offset 0x15 */
#define MII_EXP_MAP_REG(_r)             ((_r) | 0x0f00)
#define MII_EXP_UNMAP                   (0)

/*
 * Non-standard MII Registers
 */
#define MII_ECR_REG             0x10 /* MII Extended Control Register */
#define MII_EXP_REG             0x15 /* MII Expansion registers */
#define MII_EXP_SEL             0x17 /* MII Expansion register select */
#define MII_TEST1_REG           0x1e /* MII Test Register 1 */
#define RDB_LED_MATRIX          0x1f /* LED matrix mode */

void board_phy_led_init(void) {
    uint8 unit=0;
    int lport, pport;

#ifdef CFG_LED_MICROCODE_INCLUDED    
    if (sal_strcmp(board_name(), "BCM956174R") == 0) {
        board_ledup_init(&BCM956174R_ledup_ctrl);
    } else {
        board_ledup_init(NULL); /* To sort ledup scan out based on uport */
    }
#endif

    if(sal_strcmp(board_name(), "BCM953570K") == 0) 
    {
        for (lport = BCM5357X_LPORT_MIN; lport <= BCM5357X_LPORT_MAX ; lport++) {
             pport = SOC_PORT_L2P_MAPPING(lport);
             if (pport < 0) continue;
             if (pcm_phy_driver_name_get(unit, lport, 0) !=  NULL) {
                 /* Only need to set first port of phy about LED MATRIX mode*/
                 if (sal_strstr(pcm_phy_driver_name_get(unit, lport, 0), "54292")) {
                     if (((pport - 2) % 8) == 0) {

                         //extering RDB mode
                         pcm_phy_reg_set(unit, lport, 0, MII_EXP_SEL, 0xf7e);
                         pcm_phy_reg_set(unit, lport, 0, MII_EXP_REG, 0x0000);
                        
                         pcm_phy_reg_set(unit, lport, 0, MII_TEST1_REG, 0x85f);
                         pcm_phy_reg_set(unit, lport, 0, RDB_LED_MATRIX, 0x23);
                         pcm_phy_reg_set(unit, lport, 0, MII_TEST1_REG, 0x87);
                         pcm_phy_reg_set(unit, lport, 0, RDB_LED_MATRIX, 0x8000);
                         
                         //exit RDB mode
                         pcm_phy_reg_set(unit, lport, 0, 0x1E, 0x0087);
                         pcm_phy_reg_set(unit, lport, 0, 0x1F, 0x0000);
                                
                     }
                 } 
                 board_phy_led_mode_set(lport, BOARD_PHY_LED_NORMAL); 
            }
        }
    }
    
}

#if CFG_CONSOLE_ENABLED

#define UART_READREG(r)    SYS_REG_READ8((CFG_UART_BASE+(r)))
#define UART_WRITEREG(r,v) SYS_REG_WRITE8((CFG_UART_BASE+(r)), v)

void
board_console_init(uint32 baudrate, uint32 clk_hz)
{
    uint32 brtc;
    brtc = BRTC(clk_hz, baudrate);

    UART_WRITEREG(R_UART_IER, 0x0);

    UART_WRITEREG(R_UART_CFCR, CFCR_DLAB | CFCR_8BITS);
    UART_WRITEREG(R_UART_DATA, 0x0);
    UART_WRITEREG(R_UART_IER, 0x0);
    UART_WRITEREG(R_UART_CFCR, CFCR_8BITS);

    UART_WRITEREG(R_UART_MCR, MCR_DTR | MCR_RTS);

    UART_WRITEREG(R_UART_FIFO, FIFO_ENABLE | FIFO_RCV_RST | FIFO_XMT_RST);

    UART_WRITEREG(R_UART_CFCR, CFCR_DLAB | CFCR_8BITS);
    UART_WRITEREG(R_UART_DATA, brtc & 0xFF);
    UART_WRITEREG(R_UART_IER, brtc >> 8);
    UART_WRITEREG(R_UART_CFCR, CFCR_8BITS);
}
#endif /* CFG_CONSOLE_ENABLED */

/* Function:
 *   board_early_init
 * Description:
 *   Perform initialization of on-board devices that are required for SAL.
 *   This will be called from main startup task.
 * Parameters:
 *   None
 * Returns:
 *   None
 */
void
board_early_init(void)
{
   void (*funcptr)(void);

   /* Initialize timer using default clock */
   funcptr = (void (*)(void))enable_arm_cyclecount;

#if CFG_CONSOLE_ENABLED
   bcm5357x_chip_revision(0, &gh2_sw_info.devid, &gh2_sw_info.revid);
   if ((gh2_sw_info.devid & 0xF000) == 0xb000) { /* Hurricane3-MG */
       /* Initialize UART using default clock */
       board_console_init(CFG_UART_BAUDRATE, BOARD_CCA_UART_CLOCK);
       (*funcptr)();
       sal_timer_init(BOARD_CPU_CLOCK, TRUE);
   } else {                                      /* Quartz */  
       board_console_init(CFG_UART_BAUDRATE, BOARD_CCA_UART_CLOCK);
       (*funcptr)();
       sal_timer_init(BOARD_CPU_CLOCK, TRUE);
   }
#endif /* CFG_CONSOLE_ENABLED */



#if CFG_CONSOLE_ENABLED
    if ((gh2_sw_info.devid & 0xF000) == 0xb000) { /* Hurricane3-MG */
         sal_printf("\n\nHurricane3-MG %s-%d.%d.%d\n", um_fwname, CFE_VER_MAJOR, CFE_VER_MINOR, CFE_VER_BUILD);
    } else {
         sal_printf("\n\n\nQuartz %s-%d.%d.%d\n", um_fwname, CFE_VER_MAJOR, CFE_VER_MINOR, CFE_VER_BUILD);
    }
    sal_printf("Build Date: %s\n", __DATE__);
#endif /* CFG_CONSOLE_ENABLED */

#if CFG_FLASH_SUPPORT_ENABLED
   /* Flash driver init */
   flash_init(NULL);

#endif /* CFG_FLASH_SUPPORT_ENABLED */
}

#if CFG_RXTX_SUPPORT_ENABLED
static void
bcm5357x_fp_init(void)
{
    /* 
     * Slice 0: Trap Layer 4 protocol packets.
     * SLICE0_F2 = 0 (L4_DST and L4_SRC)
     *
     * Slice 1: 
     * Ingress rate limit (SLICE1_F3 = 2)
     * Match system_mac (SLICE1_F2 = 5)
     *
     * Slice 2: 
     *  (1)Port-based QoS  (SLICE2_F3 = 2)
     *  (2)Make 1P priority have precedence over DSCP(SLICE2_F1 = 6)
     *  Note (1) and (2) are mutually exclusive.
     *
     * Slice 3: Loop detect
     * Slice3_F3 = 2, Slice3_F2 = 8, Slice3_F1 = 4;
 */

#if (CFG_UIP_STACK_ENABLED)
    FP_TCAMm_t fp_tcam;
    FP_GLOBAL_MASK_TCAMm_t fp_global_mask_tcam;
    FP_POLICY_TABLEm_t fp_policy_table;
    uint32 mac_addr32[2], ones[2];
#endif
    FP_PORT_FIELD_SELm_t fp_port_field_sel;
    FP_SLICE_ENABLEr_t fp_slice_enable;
    FP_SLICE_MAPm_t fp_slice_map;
    PORTm_t port_entry;
    int pport, lport;
    
    
#if (CFG_UIP_STACK_ENABLED)
    IFP_REDIRECTION_PROFILEm_t ifp_redirection_profile;    
#endif /* CFG_UIP_STACK_ENABLED */

#if CFG_UIP_STACK_ENABLED
    uint8 mac_addr[6];
#endif /* CFG_UIP_STACK_ENABLED */

    /* Enable FILTER_ENABLE[bit 0] for all ports, include CPU. */
    for (pport = 0; pport <= BCM5357X_PORT_MAX; pport++) {
        if (1 == pport) {
            continue;
        }
        READ_PORTm(0, pport, port_entry);
        PORTm_FILTER_ENABLEf_SET(port_entry, 1);
        WRITE_PORTm(0, pport, port_entry);
    }
  
    FP_SLICE_ENABLEr_FP_SLICE_ENABLE_ALLf_SET(fp_slice_enable, 0xFF);
    FP_SLICE_ENABLEr_FP_LOOKUP_ENABLE_ALLf_SET(fp_slice_enable, 0xFF);
    WRITE_FP_SLICE_ENABLEr(0, fp_slice_enable);

    for (lport = BCM5357X_LPORT_MIN; lport <= BCM5357X_LPORT_MAX; lport++) {
        /* Slice 0: F2 = 0 L4DstPort;  F3=11 SrcPort for MDNS*/
        READ_FP_PORT_FIELD_SELm(0, lport , fp_port_field_sel);
        FP_PORT_FIELD_SELm_SLICE0_F2f_SET(fp_port_field_sel, 0);
        FP_PORT_FIELD_SELm_SLICE0_F3f_SET(fp_port_field_sel, 0xb);
        WRITE_FP_PORT_FIELD_SELm(0, lport, fp_port_field_sel);
    }

    FP_SLICE_MAPm_CLR(fp_slice_map);
    FP_SLICE_MAPm_VIRTUAL_SLICE_0_PHYSICAL_SLICE_NUMBER_ENTRY_0f_SET(fp_slice_map, 0);
    FP_SLICE_MAPm_VIRTUAL_SLICE_0_VIRTUAL_SLICE_GROUP_ENTRY_0f_SET(fp_slice_map, 0);
    FP_SLICE_MAPm_VIRTUAL_SLICE_1_PHYSICAL_SLICE_NUMBER_ENTRY_0f_SET(fp_slice_map, 1);
    FP_SLICE_MAPm_VIRTUAL_SLICE_1_VIRTUAL_SLICE_GROUP_ENTRY_0f_SET(fp_slice_map, 1);
    FP_SLICE_MAPm_VIRTUAL_SLICE_2_PHYSICAL_SLICE_NUMBER_ENTRY_0f_SET(fp_slice_map, 2);
    FP_SLICE_MAPm_VIRTUAL_SLICE_2_VIRTUAL_SLICE_GROUP_ENTRY_0f_SET(fp_slice_map, 2);
    FP_SLICE_MAPm_VIRTUAL_SLICE_3_PHYSICAL_SLICE_NUMBER_ENTRY_0f_SET(fp_slice_map, 3);
    FP_SLICE_MAPm_VIRTUAL_SLICE_3_VIRTUAL_SLICE_GROUP_ENTRY_0f_SET(fp_slice_map, 3);
    FP_SLICE_MAPm_VIRTUAL_SLICE_4_PHYSICAL_SLICE_NUMBER_ENTRY_0f_SET(fp_slice_map, 4);
    FP_SLICE_MAPm_VIRTUAL_SLICE_4_VIRTUAL_SLICE_GROUP_ENTRY_0f_SET(fp_slice_map, 4);
    FP_SLICE_MAPm_VIRTUAL_SLICE_5_PHYSICAL_SLICE_NUMBER_ENTRY_0f_SET(fp_slice_map, 5);
    FP_SLICE_MAPm_VIRTUAL_SLICE_5_VIRTUAL_SLICE_GROUP_ENTRY_0f_SET(fp_slice_map, 5);
    FP_SLICE_MAPm_VIRTUAL_SLICE_6_PHYSICAL_SLICE_NUMBER_ENTRY_0f_SET(fp_slice_map, 6);
    FP_SLICE_MAPm_VIRTUAL_SLICE_6_VIRTUAL_SLICE_GROUP_ENTRY_0f_SET(fp_slice_map, 6);
    FP_SLICE_MAPm_VIRTUAL_SLICE_7_PHYSICAL_SLICE_NUMBER_ENTRY_0f_SET(fp_slice_map, 7);
    FP_SLICE_MAPm_VIRTUAL_SLICE_7_VIRTUAL_SLICE_GROUP_ENTRY_0f_SET(fp_slice_map, 7);
    WRITE_FP_SLICE_MAPm(0, fp_slice_map);

#if CFG_UIP_STACK_ENABLED
    /* Program for DUT'S MAC  : entry 0 of slice 1 */
    get_system_mac(mac_addr);

    ones[0] = 0xFFFFFFFF;
    ones[1] = 0x0000FFFF;

    mac_addr32[0] = (mac_addr[2] << 24) | (mac_addr[3] << 16) | (mac_addr[4] << 8) | (mac_addr[5]);
    mac_addr32[1] = (mac_addr[0] << 8) | (mac_addr[1]);

        
    FP_TCAMm_CLR(fp_tcam);	
    FP_TCAMm_F2_5_DAf_SET(fp_tcam, mac_addr32);
    FP_TCAMm_F2_5_DA_MASKf_SET(fp_tcam, ones);
    FP_TCAMm_VALIDf_SET(fp_tcam, 3);
    WRITE_FP_TCAMm(0, SYS_MAC_IDX, fp_tcam);
 
    FP_GLOBAL_MASK_TCAMm_CLR(fp_global_mask_tcam);
    FP_GLOBAL_MASK_TCAMm_VALIDf_SET(fp_global_mask_tcam, 1);
    WRITE_FP_GLOBAL_MASK_TCAMm(0, SYS_MAC_IDX, fp_global_mask_tcam);

    FP_POLICY_TABLEm_CLR(fp_policy_table);
    FP_POLICY_TABLEm_METER_SHARING_MODE_MODIFIERf_SET(fp_policy_table, 1);
    FP_POLICY_TABLEm_METER_PAIR_MODE_MODIFIERf_SET(fp_policy_table, 1);
    FP_POLICY_TABLEm_R_COPY_TO_CPUf_SET(fp_policy_table, 1);
    FP_POLICY_TABLEm_Y_COPY_TO_CPUf_SET(fp_policy_table, 1);
    FP_POLICY_TABLEm_G_COPY_TO_CPUf_SET(fp_policy_table, 1);
    FP_POLICY_TABLEm_GREEN_TO_PIDf_SET(fp_policy_table, 1);
    WRITE_FP_POLICY_TABLEm(0, SYS_MAC_IDX, fp_policy_table);

    for (lport = BCM5357X_LPORT_MIN; lport <= BCM5357X_LPORT_MAX; lport++) {
        READ_FP_PORT_FIELD_SELm(0, lport , fp_port_field_sel);
        /* Slice 1: F2 = 5 MACDA;  F3=11 SrcPort*/
        FP_PORT_FIELD_SELm_SLICE1_F3f_SET(fp_port_field_sel, 0xb);
        FP_PORT_FIELD_SELm_SLICE1_F2f_SET(fp_port_field_sel, 0x5);
        FP_PORT_FIELD_SELm_SLICE1_F1f_SET(fp_port_field_sel, 0x0);
        FP_PORT_FIELD_SELm_SLICE1_DOUBLE_WIDE_KEY_SELECTf_SET(fp_port_field_sel, 0x0);
        WRITE_FP_PORT_FIELD_SELm(0, lport, fp_port_field_sel);
    }
#endif /* CFG_UIP_STACK_ENABLED */

#if CFG_UIP_IPV6_ENABLED
    /* Program IPV6 all notes multicast MAC  : entry 1 of slice 1
    * FP_TCAM :
    * DA is start from bit 106
    * MASK of DA is start from bit 316
    */
    mac_addr[0] = 0x33;
    mac_addr[1] = 0x33;
    mac_addr[2] = 0x00;
    mac_addr[3] = 0x00;
    mac_addr[4] = 0x00;
    mac_addr[5] = 0x01;
    mac_addr32[0] = (mac_addr[2] << 24) | (mac_addr[3] << 16) | (mac_addr[4] << 8) | (mac_addr[5]);
    mac_addr32[1] = (mac_addr[0] << 8) | (mac_addr[1]);
    
    FP_TCAMm_CLR(fp_tcam);
    FP_TCAMm_F2_5_DAf_SET(fp_tcam, mac_addr32);
    FP_TCAMm_F2_5_DA_MASKf_SET(fp_tcam, ones);
    FP_TCAMm_VALIDf_SET(fp_tcam, 3);
    WRITE_FP_TCAMm(0, (SYS_MAC_IDX + 1), fp_tcam);
    
    
    FP_POLICY_TABLEm_CLR(fp_policy_table);
    FP_POLICY_TABLEm_METER_SHARING_MODE_MODIFIERf_SET(fp_policy_table, 1);
    FP_POLICY_TABLEm_METER_PAIR_MODE_MODIFIERf_SET(fp_policy_table, 1);
    FP_POLICY_TABLEm_R_COPY_TO_CPUf_SET(fp_policy_table, 1);
    FP_POLICY_TABLEm_Y_COPY_TO_CPUf_SET(fp_policy_table, 1);
    FP_POLICY_TABLEm_G_COPY_TO_CPUf_SET(fp_policy_table, 1);
    FP_POLICY_TABLEm_GREEN_TO_PIDf_SET(fp_policy_table, 1);
    WRITE_FP_POLICY_TABLEm(0, (SYS_MAC_IDX + 1), fp_policy_table);

    FP_GLOBAL_MASK_TCAMm_CLR(fp_global_mask_tcam);
    FP_GLOBAL_MASK_TCAMm_VALIDf_SET(fp_global_mask_tcam, 1);
    WRITE_FP_GLOBAL_MASK_TCAMm(0, (SYS_MAC_IDX + 1), fp_global_mask_tcam);

    /* Program for IPV6 multicast MAC  : entry 2 of slice 1
    * FP_TCAM :
    * DA is start from bit 106
    * MASK of DA is start from bit 316
    */
    get_system_mac(mac_addr);
    mac_addr[0] = 0x33;
    mac_addr[1] = 0x33;
    mac_addr[2] = 0xff;
    mac_addr32[0] = (mac_addr[2] << 24) | (mac_addr[3] << 16) | (mac_addr[4] << 8) | (mac_addr[5]);
    mac_addr32[1] = (mac_addr[0] << 8) | (mac_addr[1]);
  
    FP_TCAMm_CLR(fp_tcam);
    FP_TCAMm_F2_5_DAf_SET(fp_tcam, mac_addr32);
    FP_TCAMm_F2_5_DA_MASKf_SET(fp_tcam, ones);
    FP_TCAMm_VALIDf_SET(fp_tcam, 3);
    WRITE_FP_TCAMm(0, (SYS_MAC_IDX + 2), fp_tcam);
 
    FP_POLICY_TABLEm_CLR(fp_policy_table);
    FP_POLICY_TABLEm_METER_SHARING_MODE_MODIFIERf_SET(fp_policy_table, 1);
    FP_POLICY_TABLEm_METER_PAIR_MODE_MODIFIERf_SET(fp_policy_table, 1);
    FP_POLICY_TABLEm_R_COPY_TO_CPUf_SET(fp_policy_table, 1);
    FP_POLICY_TABLEm_Y_COPY_TO_CPUf_SET(fp_policy_table, 1);
    FP_POLICY_TABLEm_G_COPY_TO_CPUf_SET(fp_policy_table, 1);
    FP_POLICY_TABLEm_GREEN_TO_PIDf_SET(fp_policy_table, 1);
    WRITE_FP_POLICY_TABLEm(0, (SYS_MAC_IDX + 2), fp_policy_table);
 
    FP_GLOBAL_MASK_TCAMm_CLR(fp_global_mask_tcam);
    FP_GLOBAL_MASK_TCAMm_VALIDf_SET(fp_global_mask_tcam, 1);
    WRITE_FP_GLOBAL_MASK_TCAMm(0, (SYS_MAC_IDX + 2), fp_global_mask_tcam);

#endif /* CFG_UIP_IPV6_ENABLED */

#if (CFG_UIP_STACK_ENABLED)
    /* Set each redireciton entry per each port
     * No need to exclude source port nor trunk ports but due to pvlan need to use
     * cpu and each front port needs each entry.
     */
    IFP_REDIRECTION_PROFILEm_CLR(ifp_redirection_profile);
    IFP_REDIRECTION_PROFILEm_BITMAPf_SET(ifp_redirection_profile, SOC_PBMP(BCM5357X_ALL_PORTS_MASK));
    for (lport = 0; lport <= BCM5357X_LPORT_MAX; lport ++) {
        WRITE_IFP_REDIRECTION_PROFILEm(0, lport, ifp_redirection_profile);
    }
#endif /* (CFG_UIP_STACK_ENABLED) */
}
#endif /* CFG_RXTX_SUPPORT_ENABLED */

/* Function:
 *   board_init
 * Description:
 *   Perform board (chip and devices) initialization.
 *   This will be called from main startup task.
 * Parameters:
 *   None
 * Returns:
 *   SYS_OK or SYS_XXX
 */
sys_error_t
board_init(void)
{
    sys_error_t rv = SYS_OK;  

#ifdef __BOOTLOADER__
#if __BOOTLOADER_UART_TRAP__
        sal_usleep(500000);
        if (sal_char_avail() == FALSE)
#endif /* __BOOTLODER_UART_TRAP__ */

        {
            hsaddr_t loadaddr;
            if (board_loader_mode_get(NULL, FALSE) != LM_UPGRADE_FIRMWARE) {
#ifdef CFG_DUAL_IMAGE_INCLUDED
                /* Determine which image to boot */
                if (board_select_boot_image(&loadaddr)) {
#else
                /* Validate firmware image if not requested to upgrade firmware */
                if (board_check_image(BOARD_FIRMWARE_ADDR, &loadaddr)) {
#endif /* CFG_DUAL_IMAGE_INCLUDED */
                    /* launch firmware */
#if CFG_CONSOLE_ENABLED
                    sal_printf("Load program at 0x%08lX...\n", loadaddr);
#endif /* CFG_CONSOLE_ENABLED */
                    board_load_program(loadaddr);
                }
                /* Stay in loader in case of invalid firmware */
            }
        }

#endif /* __BOOTLOADER__ */


#if CFG_FLASH_SUPPORT_ENABLED
#if defined(CFG_NVRAM_SUPPORT_INCLUDED) || defined(CFG_VENDOR_CONFIG_SUPPORT_INCLUDED)
   nvram_init();
#endif /* defined(CFG_NVRAM_SUPPORT_INCLUDED) || defined(CFG_VENDOR_CONFIG_SUPPORT_INCLUDED) */
#endif /* CFG_FLASH_SUPPORT_ENABLED */

#if CONFIG_EMULATION
    sal_printf("EMULATION usage only!\n");
#endif

#if !CFG_TIMER_USE_INTERRUPT
    task_add(sal_timer_task, (void *)NULL);
#endif /* CFG_TIMER_USE_INTERRUPT */

    rv = bcm5357x_sw_init();

    if (rv != SYS_OK) {
        return rv;
    }
    uint16 uport;
    board_lport_to_uport(0, 2, &uport);

#ifdef CFG_SWITCH_VLAN_INCLUDED
    _brdimpl_vlan_init();
#endif /* CFG_SWITCH_VLAN_INCLUDED */

#ifdef CFG_SWITCH_QOS_INCLUDED
    bcm5357x_qos_init();
#endif /* CFG_SWITCH_QOS_INCLUDED */

#ifdef CFG_SWITCH_RATE_INCLUDED
    bcm5357x_rate_init();
#endif /* CFG_SWITCH_RATE_INCLUDED */

#ifdef CFG_SWITCH_LAG_INCLUDED
    {
        uint32 val32;

        /* Enables factoring src/dest MAC or src/dest IP into non-unicast trunk block mask hashing */
        bcm5357x_reg_get(0, R_HASH_CONTROL, &val32);
        val32 |= 0x6;
        bcm5357x_reg_set(0, R_HASH_CONTROL, val32);
        sys_register_linkchange(board_lag_linkchange, NULL);
    }
#endif /* CFG_SWITCH_LAG_INCLUDED */

#if defined(CFG_SWITCH_EEE_INCLUDED)
    /* Init EEE for HK_EEE */
    {
        bcm5357x_eee_init();
    }
#endif /* CFG_SWITCH_EEE_INCLUDED */

#if CFG_RXTX_SUPPORT_ENABLED
    bcm5357x_rxtx_init();
    brdimpl_rxtx_init();
    bcm5357x_fp_init();
#endif /* CFG_RXTX_SUPPORT_ENABLED */

#ifdef CFG_RESET_BUTTON_INCLUDED
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
    rv = sal_config_uint8_get(SAL_CONFIG_RESET_BUTTON_ENABLE, &reset_button_enable);
    if ((rv == SYS_OK) && reset_button_enable) {
        sal_printf("Vendor Config : Change to enable the reset buton feature by configuration.\n");
    }

    rv = sal_config_uint8_get(SAL_CONFIG_RESET_BUTTON_GPIO_BIT, &reset_button_gpio_bit);
    if (rv == SYS_OK) {
        sal_printf("Vendor Config : Change to use GPIO bit %d for reset button by configuration.\n", reset_button_gpio_bit);
    }

    rv = sal_config_uint8_get(SAL_CONFIG_RESET_BUTTON_POLARITY, &reset_button_active_high);
    if (rv == SYS_OK) {
        sal_printf("Vendor Config : Change the reset button polarity to %d\n", reset_button_active_high);
        sal_printf("                (0:active low/1:active high) by configuration.\n");
    }
#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */
    if (reset_button_enable) {
        /* Detect reset button per 1 second */
        timer_add(brdimpl_reset_button_detect, NULL, (1 * 1000000));
    }
#endif /* CFG_RESET_BUTTON_INCLUDED */

    return SYS_OK;
}


/* Function:
 *   board_late_init
 * Description:
 *   Perform final stage of platform dependent initialization
 *   This will be called from main startup task.
 * Parameters:
 *   None
 * Returns:
 *   None
 */
void
board_late_init(void) {
#if !(defined(__BOOTLOADER__) ||  !defined(CFG_SWITCH_STAT_INCLUDED))
    board_port_stat_clear_all();
#endif
    
    board_phy_led_init();
}


