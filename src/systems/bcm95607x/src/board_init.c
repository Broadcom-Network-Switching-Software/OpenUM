/*
 * $Id: board_init.c,v 1.15 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#include "brdimpl.h"
#include "bsp_config.h"
#include "brdimpl.h"
#include "utils/nvram.h"
#include "utils/system.h"
#include "utils/shr/shr_debug.h"
#include "ns16550.h"
#include <boardapi/mcs.h>
#include <led.h>
#include <cmicx_m0ssq_internal.h>
#include <cmicx_led_internal.h>
#include <binfs.h>

/*! Common information for LED FW and UM. */
#include <cmicx_customer_led_common.h>

/* Hardware linkscan driver. */
#include <lm_drv_internal.h>

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

#if defined(CFG_BROADSYNC_INCLUDED) && defined(CFG_MCS_INCLUDED)
extern void bs_exception_detect(void *param);
#endif /* defined(CFG_BROADSYNC_INCLUDED) && defined(CFG_MCS_INCLUDED) */

#if CFG_CONSOLE_ENABLED
static const char *um_fwname = target;
#endif /* CFG_CONSOLE_ENABLED */

#ifdef CFG_RESET_BUTTON_INCLUDED
uint8 reset_button_enable = 0;
#endif /* CFG_RESET_BUTTON_INCLUDED */

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

#ifdef __LINUX__
#ifdef CFG_WDT_INCLUDED
    {
        /* Always disable watchdog timer when running external CPU mode
         * if it has been triggered by R5
         */
        IPROCGENRES_WDT0_WDT_WDOGCONTROLr_t iproc_wdt0_wdogcontrol;
        int unit = 0;

        IPROCGENRES_WDT0_WDT_WDOGCONTROLr_CLR(iproc_wdt0_wdogcontrol);
        WRITE_IPROCGENRES_WDT0_WDT_WDOGCONTROLr(unit, iproc_wdt0_wdogcontrol);
    }
#endif
    
    MHOST_CR5_RST_CTRLr_t cr5_ctrl;
    READ_MHOST_CR5_RST_CTRLr(0, 0, cr5_ctrl);
    MHOST_CR5_RST_CTRLr_CPU_HALT_Nf_SET(cr5_ctrl, 0);
    WRITE_MHOST_CR5_RST_CTRLr(0, 0, cr5_ctrl);
    MHOST_CR5_RST_CTRLr_RESET_Nf_SET(cr5_ctrl, 0);
    WRITE_MHOST_CR5_RST_CTRLr(0, 0, cr5_ctrl);
#endif

    /* Initialize timer using default clock */
    funcptr = (void (*)(void))enable_arm_cyclecount;
    (*funcptr)();
    sal_timer_init(BOARD_CPU_CLOCK, TRUE);

#if CFG_CONSOLE_ENABLED
    board_console_init(CFG_UART_BAUDRATE, BOARD_CCA_UART_CLOCK);
    sal_printf("\n\nFirelight %s-%d.%d.%d\n",
            um_fwname, CFE_VER_MAJOR, CFE_VER_MINOR, CFE_VER_BUILD);
    sal_printf("Build Date: %s. Build Time: %s\n", __DATE__, __TIME__);
#endif /* CFG_CONSOLE_ENABLED */

#if CFG_FLASH_SUPPORT_ENABLED
    /* Flash driver init */
    flash_init(NULL);
#endif /* CFG_FLASH_SUPPORT_ENABLED */
}

#if CFG_RXTX_SUPPORT_ENABLED
static void
bcm5607x_fp_init(void)
{
#ifdef CFG_COE_INCLUDED
    fl_coe_ifp_init();
#endif

#ifdef CFG_SWITCH_QOS_INCLUDED
    FP_PORT_FIELD_SELm_t fp_port_field_sel;
    int i;

    for (i = BCM5607X_LPORT_MIN; i <= BCM5607X_LPORT_MAX; i++) {
        READ_FP_PORT_FIELD_SELm(0, i, fp_port_field_sel);
        /* for 802.1p, Set SLICE4_F3 = 3(Vlan qualifier) */
        FP_PORT_FIELD_SELm_SLICE4_F1f_SET(fp_port_field_sel, 0x4);
        FP_PORT_FIELD_SELm_SLICE4_F2f_SET(fp_port_field_sel, 0x8);
        FP_PORT_FIELD_SELm_SLICE4_F3f_SET(fp_port_field_sel, 0x3);
        FP_PORT_FIELD_SELm_SLICE4_DOUBLE_WIDE_MODEf_SET(fp_port_field_sel, 0);
        /* for MPLS_EXP, Set F2f = 5(ETHERTYPE), F1f=9(DATA) */
        FP_PORT_FIELD_SELm_SLICE5_F1f_SET(fp_port_field_sel, 0x4);
        FP_PORT_FIELD_SELm_SLICE5_F2f_SET(fp_port_field_sel, 0x8);
        FP_PORT_FIELD_SELm_SLICE5_F3f_SET(fp_port_field_sel, 0x3);
        FP_PORT_FIELD_SELm_SLICE5_DOUBLE_WIDE_MODEf_SET(fp_port_field_sel, 0);
        WRITE_FP_PORT_FIELD_SELm(0, i, fp_port_field_sel);
    }
#endif /* CFG_SWITCH_QOS_INCLUDED */
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

    rv = bcm5607x_sw_init();

    if (rv != SYS_OK) {
        return rv;
    }

#ifdef CFG_SWITCH_VLAN_INCLUDED
    _brdimpl_vlan_init();
#endif /* CFG_SWITCH_VLAN_INCLUDED */

#ifdef CFG_SWITCH_QOS_INCLUDED
    bcm5607x_qos_init();
#endif /* CFG_SWITCH_QOS_INCLUDED */

#ifdef CFG_SWITCH_RATE_INCLUDED
    bcm5607x_rate_init();
#endif /* CFG_SWITCH_RATE_INCLUDED */

#ifdef CFG_SWITCH_LAG_INCLUDED
    {
        uint32 val32;

        /* Enables factoring src/dest MAC or src/dest IP into non-unicast trunk block mask hashing */
        bcm5607x_reg_get(0, R_HASH_CONTROL, &val32);
        val32 |= 0x6;
        bcm5607x_reg_set(0, R_HASH_CONTROL, val32);
        sys_register_linkchange(board_lag_linkchange, NULL);
    }
#endif /* CFG_SWITCH_LAG_INCLUDED */

#if defined(CFG_SWITCH_EEE_INCLUDED)
    /* Init EEE for HK_EEE */
    {
        bcm5607x_eee_init();
    }
#endif /* CFG_SWITCH_EEE_INCLUDED */

#if CFG_RXTX_SUPPORT_ENABLED
    bcm5607x_rxtx_init();
    brdimpl_rxtx_init();
    bcm5607x_fp_init();
#endif /* CFG_RXTX_SUPPORT_ENABLED */

#ifdef CFG_COE_INCLUDED
    rv = fl_coe_config_init();
    if (rv != SYS_OK) {
        sal_printf("fl_coe_config_init return %d\n", rv);
        return rv;
    }
#endif

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

#ifdef CFG_WDT_INCLUDED
#ifndef __LINUX__
    if (board_wdt_reset_triggered())
       sal_printf("========== WDT triggered reboot ==========\n");
    board_wdt_enable();
#endif
#endif /* CFG_WDT_INCLUDED */

    return SYS_OK;
}

#ifdef CFG_LED_MICROCODE_INCLUDED
int
board_ledup_init(void)
{
    uint8 led_option = 1;
    uint8 ext_led_program[CMICX_CUSTOMER_LED_FW_SIZE_MAX];
    const uint8 *led_program = NULL;
    int byte_count;

    SHR_FUNC_ENTER(0);

    SHR_IF_ERR_EXIT
        (bcm5607x_led_drv_init(0));

#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
    if (sal_config_uint8_get(SAL_CONFIG_LED_OPTION, &led_option) == SYS_OK) {
        sal_printf("Vendor Config : Overwrite serial led option with value %d.\n", led_option);
    }
#endif
    if (led_option == 1 || led_option == 2) {
        SHR_IF_ERR_EXIT
            (binfs_file_data_get("cmicfw/cmicx_customer_led.bin", &led_program, &byte_count));

        SHR_IF_ERR_EXIT
            (board_led_control_data_write(0, LED_OPTION, &led_option, 1));

        SHR_IF_ERR_EXIT
            (board_led_fw_load(0, led_program, byte_count));

        SHR_IF_ERR_EXIT
            (board_led_fw_start_set(0, 1));

#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
    } else if (led_option == 3) {
        byte_count = sal_config_bytes_get(SAL_CONFIG_LED_PROGRAM,
                                          ext_led_program,
                                          sizeof(ext_led_program));
        sal_printf("Vendor Config : Load customer LED0 ucdoe for with length %d.\n", byte_count);
        if (byte_count > 0) {
            board_led_fw_load(0, ext_led_program, byte_count);
            board_led_fw_start_set(0, 1);
        }
#endif
    }

exit:
    SHR_FUNC_EXIT();
}
#endif

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

#if defined(CFG_BROADSYNC_INCLUDED) && defined(CFG_MCS_INCLUDED)
    const uint8 *pbuf = NULL;
    int len;
#endif

    SHR_FUNC_ENTER(0);

#if !(defined(__BOOTLOADER__) ||  !defined(CFG_SWITCH_STAT_INCLUDED))
    board_port_stat_clear_all();
#endif

#ifdef CFG_M0SSQ_INCLUDED
    SHR_IF_ERR_EXIT
        (bcm5607x_m0ssq_drv_init(0));
#endif

#ifdef CFG_LED_MICROCODE_INCLUDED
    SHR_IF_ERR_EXIT
        (board_ledup_init());
#endif

#ifdef CFG_MCS_INCLUDED
    /* MCS driver init. */
    SHR_IF_ERR_EXIT
        (bcm5607x_mcs_drv_init(0));
#endif

#if defined(CFG_BROADSYNC_INCLUDED) && defined(CFG_MCS_INCLUDED)

    /* Load and Run BS firmware. */
    if (SHR_SUCCESS(binfs_file_data_get
                    ("broadsync/BCM56070_1_um_bs.bin", &pbuf, &len)))
    {
        sal_printf("Load and run Broadsync FW on core-1.\n");

        SHR_IF_ERR_EXIT
            (board_mcs_uc_fw_load(1, 0, pbuf, len));

        SHR_IF_ERR_EXIT
            (board_mcs_uc_start(1));
        task_add(bs_exception_detect, NULL);
    } else {
        sal_printf("Can't get file broadsync/BCM56070_1_um_bs.bin.\n");
    }
#endif

#ifdef CFG_HARDWARE_LINKSCAN_INCLUDED

    /* Hardware linkscan init. */
    SHR_IF_ERR_EXIT
        (bcm5607x_lm_drv_init(0));

    SHR_IF_ERR_EXIT
        (lm_hw_init(0));

#endif

    SHR_EXIT();
exit:
    return;
}

