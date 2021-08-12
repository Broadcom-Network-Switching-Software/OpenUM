/*
 * $Id: main.c,v 1.43 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#include "appl/cli.h"
#include "appl/persistence.h"
#include "utils/system.h"
#include "utils/net.h"
#include <sal.h>
#include <appl/spi_mgmt.h>

extern char get_char(void);
extern int um_console_write(const char *buffer,int length);
extern void sal_console_init(int reset);

#ifdef CFG_SDKCLI_INCLUDED
#include "appl/sdkcli/bcma_cli.h"
#include "appl/sdkcli/bcma_cli_unit.h"
#include "appl/sdkcli/bcma_clicmd.h"
#include "appl/editline/bcma_readline.h"
#endif /* CFG_SDKCLI_INCLUDED */

#ifdef __C51__
#ifdef CODE_USERCLASS
#pragma userclass (code = init)
#endif /* CODE_USERCLASS */
#ifdef XDATA_USERCLASS
#pragma userclass (xdata = init)
#endif /* XDATA_USERCLASS */
#endif /* __C51__ */



/* Initialization routines called by main */
extern void board_early_init(void) REENTRANT;
extern sys_error_t board_init(void) REENTRANT;
extern void board_late_init(void) REENTRANT;
extern void sal_init(void) REENTRANT;
extern void background_init(void) REENTRANT;
extern void sys_timer_init(void) REENTRANT;
extern void sys_linkchange_init(void) REENTRANT;
extern void sys_rx_init(void) REENTRANT;
extern void sys_tx_init(void) REENTRANT;
extern void appl_init(void) REENTRANT;
#ifdef CFG_ENHANCED_POWER_SAVING
extern void power_management_init(void);
#endif /* CFG_ENHANCED_POWER_SAVING */
extern void ui_pcm_init(void);

/* Forwards */
#if defined(__ARM__) || defined(__LINUX__)
int main(void);
#else
void main(void);
#endif

#if (CFG_RXTX_SUPPORT_ENABLED && !defined(__BOOTLOADER__))
/* RX buffers (it's here so we can customize addr/size later in new code) */
#if defined(__MIPS__) && CFG_RAMAPP
STATIC uint8 rx_buffers[DEFAULT_RX_BUFFER_COUNT][DEFAULT_RX_BUFFER_SIZE] __attribute__ ((aligned (16)));
#elif defined(__LINUX__)
uint8* rx_buffers[DEFAULT_RX_BUFFER_COUNT];
#elif defined(__ARM__)
uint8 rx_buffers[DEFAULT_RX_BUFFER_COUNT][DEFAULT_RX_BUFFER_SIZE] __attribute__ ((section(".packet_buf"), aligned (32)));
#else
STATIC uint8 rx_buffers[DEFAULT_RX_BUFFER_COUNT][DEFAULT_RX_BUFFER_SIZE];
#endif
#endif /* CFG_RXTX_SUPPORT_ENABLED && !defined(__BOOTLOADER__) */

#ifdef CFG_SDKCLI_INCLUDED
static int
bcma_io_term_read(void *buf, int max)
{
    if (!buf) {
        return 0;
    } else {
        char *c = buf;
        *c = get_char();
        return 1;
    }
}

static int
bcma_io_term_write(const void *buf, int count)
{
    const char *c_buf = buf;
    return um_console_write(c_buf, count);
}

int
bcma_io_term_mode_set(int reset)
{
    sal_console_init(reset);
    return 0;
}

int
bcma_io_term_winsize_get(int *cols, int *rows)
{
    if (cols == NULL || rows == NULL) {
        return -1;
    }
    *cols = 80;
    *rows = 24;
    return 0;
}

static bcma_editline_io_cb_t el_io_cb = {
    bcma_io_term_read,
    bcma_io_term_write,
    bcma_io_term_mode_set,
    bcma_io_term_winsize_get,
    NULL,
    NULL
};

APISTATIC int
sdkcli_gets(struct bcma_cli_s *cli, const char *prompt, int max, char *buf)
{
    char *str = NULL;

    if (buf == NULL) {
        return BCMA_CLI_CMD_BAD_ARG;
    }

    str = readline(prompt); /* provided by editline */
    if (str == NULL) {
        return BCMA_CLI_CMD_EXIT;
    } else {
        int len = sal_strlen(str) + 1;
        sal_memcpy(buf, str, len > max ? max : len);
        buf[max - 1] = '\0';
        if (len > max) {
            sal_printf("WARNING: User inputs %d characters (limitation : %d)\n",
                        sal_strlen(str), max - 1);
        }
    }
    bcma_rl_free(str); /* free the memory that allocated by editline */
    return BCMA_CLI_CMD_OK;
}

APISTATIC void
sdkcli_history_add(int max, char *str)
{
    char *cmd_last = NULL;
    char *p = str;

    /* Remove the heading spaces if any. */
    while (p && sal_isspace(*p)) {
        p++;
    }

    /* Do no add empty string to history. */
    if (!p || !*p) {
        return;
    }

    /* Do not add the last duplicate command to history */
    cmd_last = bcma_editline_history_get(-1);
    if (cmd_last && sal_strcmp(p, cmd_last) == 0) {
        return;
    }

    /* Add command to history */
    add_history(p);
}

APISTATIC int
sdkcli_unit_max(void *cookie)
{
    return 0;
}

APISTATIC int
sdkcli_unit_valid(void *cookie, int unit)
{
    if (unit == 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

APISTATIC bcma_cli_unit_cb_t sdkcli_unit_cb = {
    .unit_max = sdkcli_unit_max,
    .unit_valid = sdkcli_unit_valid,
};

APISTATIC void
APIFUNC(sdkcli)(void) REENTRANT

{
    bcma_cli_t *sdk_cli = NULL;

    /* Initialize SDK-version of readline */
    bcma_editline_init(&el_io_cb, NULL);

    sdk_cli = bcma_cli_create();
    bcma_cli_input_cb_set(sdk_cli, "CLI", sdkcli_gets, sdkcli_history_add);
    bcma_cli_unit_cb_set(sdk_cli, &sdkcli_unit_cb, NULL);
    bcma_clicmd_add_basic_cmds(sdk_cli);
    bcma_clicmd_add_switch_cmds(sdk_cli);
    bcma_clicmd_add_pktdma_cmds(sdk_cli);
    bcma_clicmd_add_coe_cmds(sdk_cli);
    bcma_cli_unit_set(sdk_cli, 0);
    bcma_cli_cmd_loop(sdk_cli);
    bcma_cli_destroy(sdk_cli);
    sdk_cli = NULL;
}
#endif /* CFG_SDKCLI_INCLUDED */

/* Function:
 *   main
 * Description:
 *   C startup function.
 * Parameters:
 *   None
 * Returns:
 *   None
 */
#if defined(__ARM__) || defined(__LINUX__)
int
main(void)
#else
void
main(void)
#endif
{
#if (CFG_RXTX_SUPPORT_ENABLED && !defined(__BOOTLOADER__))
    uint8 i;
#endif /* CFG_RXTX_SUPPORT_ENABLED && !defined(__BOOTLOADER__) */

#if defined(__LINUX__)
   if (sal_bde_init()) {
   	   sal_printf("bde module init fail\n");
   }
#endif
    board_early_init();

    sal_init();


    background_init();
#if CFG_TIMER_CALLBACK_SUPPORT
    sys_timer_init();
#endif /* CFG_TIMER_CALLBACK_SUPPORT */


#if CFG_LINKCHANGE_CALLBACK_SUPPORT
    sys_linkchange_init();
#endif /* CFG_LINKCHANGE_CALLBACK_SUPPORT */

    system_utils_init();

#if CFG_UIP_STACK_ENABLED
    net_utils_init();
#endif /* CFG_UIP_STACK_ENABLED */

    if (board_init() == SYS_OK) {
#if (CFG_RXTX_SUPPORT_ENABLED && !defined(__BOOTLOADER__))
#if defined(__LINUX__)
     for (i=0; i<DEFAULT_RX_BUFFER_COUNT; i++) {
	 	rx_buffers[i] = sal_dma_malloc(DEFAULT_RX_BUFFER_SIZE);
     }
#endif
#endif
#if (CFG_RXTX_SUPPORT_ENABLED && !defined(__BOOTLOADER__))
        sys_tx_init();
        sys_rx_init();
        for(i=0; i<DEFAULT_RX_BUFFER_COUNT; i++) {
            sys_rx_add_buffer(rx_buffers[i], DEFAULT_RX_BUFFER_SIZE);
        }
#endif /* CFG_RXTX_SUPPORT_ENABLED && !defined(__BOOTLOADER__) */

        appl_init();

#if CFG_ENHANCED_POWER_SAVING
        power_management_init();
#endif /* CFG_ENHANCED_POWER_SAVING */

#if CFG_PERSISTENCE_SUPPORT_ENABLED

        persistence_init();

        /*
         * Load current settings or factory defaults (if settings are invalid)
         */
        if (persistence_validate_current_settings()) {

            /*
             * If all current settins are valid, just load them.
             */
            persistence_load_all_current_settings();

        } else {

            /*
             * Part or all of data in current settings are not valid:
             * use factory defaults for the invalid items.
             * First we load factory default for all items.
             */
#if CFG_CONSOLE_ENABLED
            sal_printf("Some of current saved settings are invalid. "
                       "Loading factory defaults.....\n");
#endif /* CFG_CONSOLE_ENABLED */
            persistence_restore_factory_defaults();

            /*
             * Then load current settings for valid items.
             */
            persistence_load_all_current_settings();

            /*
             * Loading done; save to flash (current settings)
             */
            persistence_save_all_current_settings();
        }
#endif /* CFG_PERSISTENCE_SUPPORT_ENABLED */

#if defined(__UMDUMB__)
    board_vlan_type_set(VT_PORT_BASED);
    board_qos_type_set(QT_DOT1P_PRIORITY);
#endif

        board_late_init();
    } else {
        /*
         * FIXME:
         *
         * To remove this "else" block when device init is ready.
 */
        appl_init();
        board_late_init();
    }
#ifdef CFG_PCM_SUPPORT_INCLUDED
    ui_pcm_init();
#endif

#ifdef CFG_SPI_MGMT_INCLUDED
#if (!defined(__LINUX__) && !defined(__bootloader__))
    /* Enable SPI management */
    spi_mgmt_enable();
#endif
#endif /* CFG_SPI_MGMT_INCLUDED */

#if CFG_CLI_ENABLED || defined(CFG_SDKCLI_INCLUDED)
#ifdef CFG_SDKCLI_INCLUDED
    sdkcli();
#elif CFG_CLI_ENABLED
    cli();
#endif
#else
    for(;;) POLL();
#endif
#ifdef __ARM__
    return 0;
#endif
}

