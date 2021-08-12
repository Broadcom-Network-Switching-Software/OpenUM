/*
 * $Id: app_init.c,v 1.19 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"

#ifdef CFG_BROADSYNC_INCLUDED

#include "appl/bs.h"
#include <binfs.h>
#include <boardapi/mcs.h>

/* Hard Addresses. */
#define BS_OUT_MSG_ADDR                     0x01180000
#define BS_IN_MSG_ADDR                      0x01180100

/* BS Messaging States.*/
#define BS_MSG_STATE_IDLE   0
#define BS_MSG_STATE_BUSY   1
#define BS_MSG_STATE_VALID  2

/* BS Out Messaging Commands.*/
#define BS_MSG_COMMAND_DBG_SET              1
#define BS_MSG_COMMAND_RECONFIG             2
#define BS_MSG_COMMAND_FREQ_OFFSET_SET      3
#define BS_MSG_COMMAND_PHASE_OFFSET_SET     4
#define BS_MSG_COMMAND_NTP_OFFSET_SET       5
#define BS_MSG_COMMAND_1PPS_SET             6

/* BS Messaging structure.*/
typedef struct {
    uint32_t state;
    uint32_t command;
    uint32_t data[62];
} bs_msg_t;

volatile bs_msg_t *bs_out_msg = (volatile bs_msg_t *) BS_OUT_MSG_ADDR;
volatile bs_msg_t *bs_in_msg = (volatile bs_msg_t *) BS_IN_MSG_ADDR;

bs_exe_core_dump_t *bs_core_dump = (bs_exe_core_dump_t *)BS_EXEC_DEBUG_SRAM_START;
/*!
 * \brief Configure BroadSync Debug Logging.
 *
 * \param [in] debug_mask mask that sets log level
 *
 * Remaining parameters are for UDP Log feature which is
 * currently unsupported.
 *
 * \retval SYS_OK No errors.
 */
int board_broadsync_log_configure(uint32 debug_mask,
                                  uint64 udp_log_mask,
                                  shr_mac_t src_mac,
                                  shr_mac_t dest_mac,
                                  uint16 tpid,
                                  uint16 vid,
                                  uint8 ttl,
                                  shr_ip_t src_addr,
                                  shr_ip_t dest_addr,
                                  uint16 udp_port)
{
    if (bs_out_msg->state != BS_MSG_STATE_IDLE) {
        return SYS_ERR_TIMEOUT;
    }

    bs_out_msg->command = BS_MSG_COMMAND_DBG_SET;
    bs_out_msg->data[0] = debug_mask;
    bs_out_msg->state = BS_MSG_STATE_VALID;

    return SYS_OK;
}

/*!
 * \brief Re-Configure BroadSync Interface.
 *
 * \param [in] mode 1=Master 0=Slave
 * \param [in] bitclk_hz Bit Clock in Hz
 * \param [in] hb_hz HB Clock in Hz
 *
 * \retval SYS_OK No errors.
 */
int board_broadsync_config_set(int mode, int bitclk_hz, int hb_hz)
{
    if (bs_out_msg->state != BS_MSG_STATE_IDLE) {
        return SYS_ERR_TIMEOUT;
    }

    bs_out_msg->command = BS_MSG_COMMAND_RECONFIG;
    bs_out_msg->data[0] = mode;
    bs_out_msg->data[1] = bitclk_hz;
    bs_out_msg->data[2] = hb_hz;
    bs_out_msg->state = BS_MSG_STATE_VALID;

    return SYS_OK;
}

/*!
 * \brief Configure a frequency offset to Broadsync clock.
 *
 * \param [in] offset_ppb Offset Value
 *
 * \retval SYS_OK No errors.
 */
int board_broadsync_freq_offset_set(int offset_ppb)
{
    if (bs_out_msg->state != BS_MSG_STATE_IDLE) {
        return SYS_ERR_TIMEOUT;
    }

    bs_out_msg->command = BS_MSG_COMMAND_FREQ_OFFSET_SET;
    bs_out_msg->data[0] = offset_ppb;
    bs_out_msg->state = BS_MSG_STATE_VALID;

    return SYS_OK;
}

/*!
 * \brief Configure a phase offset to Broadsync time.
 *
 * \param [in] offset_sec Seconds Offset Value
 * \param [in] offset_nsec NanoSeconds Offset Value
 * \param [in] is_negative 0=Positive 1=Negative
 *
 * \retval SYS_OK No errors.
 */
int board_broadsync_phase_offset_set(int offset_sec, int offset_nsec,
                                     int is_negative)
{
    if (bs_out_msg->state != BS_MSG_STATE_IDLE) {
        return SYS_ERR_TIMEOUT;
    }

    bs_out_msg->command = BS_MSG_COMMAND_PHASE_OFFSET_SET;
    bs_out_msg->data[0] = offset_sec;
    bs_out_msg->data[1] = offset_nsec;
    bs_out_msg->data[3] = is_negative;
    bs_out_msg->state = BS_MSG_STATE_VALID;

    return SYS_OK;
}

/*!
 * \brief Configure a phase offset to NTP time.relative to BS time.
 *
 * \param [in] offset_sec Seconds Offset Value
 * \param [in] offset_nsec NanoSeconds Offset Value
 * \param [in] is_negative 0=Positive 1=Negative
 *
 * \retval SYS_OK No errors.
 */
int board_broadsync_ntp_offset_set(int offset_sec, int offset_nsec,
                                   int is_negative)
{
    if (bs_out_msg->state != BS_MSG_STATE_IDLE) {
        return SYS_ERR_TIMEOUT;
    }

    bs_out_msg->command = BS_MSG_COMMAND_NTP_OFFSET_SET;
    bs_out_msg->data[0] = offset_sec;
    bs_out_msg->data[1] = offset_nsec;
    bs_out_msg->data[3] = is_negative;
    bs_out_msg->state = BS_MSG_STATE_VALID;

    return SYS_OK;
}

/*!
 * \brief Enable/Disable 1PPS signal.
 *
 * \param [in] enableOutput 1=Enable 0-Disable
 *
 * \retval SYS_OK No errors.
 */
int board_broadsync_debug_1pps_set(uint8 enableOutput)
{
    if (bs_out_msg->state != BS_MSG_STATE_IDLE) {
        return SYS_ERR_TIMEOUT;
    }

    bs_out_msg->command = BS_MSG_COMMAND_1PPS_SET;
    bs_out_msg->data[0] = enableOutput;
    bs_out_msg->state = BS_MSG_STATE_VALID;

    return SYS_OK;
}

int board_broadsync_reinit(void)
{
    int rv = SYS_ERR_UNAVAIL;
    const uint8 *pbuf = NULL;
    int len;

    rv = board_mcs_uc_reset(1);
    if(rv != SYS_OK){
        return rv;
    }

    /* Load and Run BS firmware. */
    if (binfs_file_data_get
                    ("broadsync/BCM56070_1_um_bs.bin", &pbuf, &len))
    {
        sal_printf("Load and run Broadsync FW on core-1.\n");

        rv = board_mcs_uc_fw_load(1, 0, pbuf, len);
        if (rv != SYS_OK)
        {
            goto exit;
        }

        rv = board_mcs_uc_start(1);
        if (rv != SYS_OK)
        {
            goto exit;
        }
        bs_core_dump->length = BS_EXEC_CRASH_INFO_SRAM_LEN;
        bs_core_dump->core_status = 0;
    } else {
        sal_printf("Can't get file broadsync/BCM56070_1_um_bs.bin.\n");
        rv = SYS_ERR_NOT_FOUND;
    }

exit:
    return rv;
}

int board_broadsync_core_dump_print(void)
{
    sal_printf("uC-%d current status: %s\n", 1, (bs_core_dump->core_status == 0)?"Running":"Exception");
    /* Exception Type :
     * 0 - Reset Reentry
     * 1 - Undefined Instruction
     * 2 - SWI
     * 3 - Prefetch Abort
     * 4 - Data Abort
     */
    sal_printf("Last available crash info:\n");
    sal_printf("Type : 0x%08x\n", bs_core_dump->type);
    /* Current Program Status Register */
    sal_printf("CPSR : 0x%08x\n", bs_core_dump->cpsr);
    /* Data Fault Status Register */
    sal_printf("DFSR : 0x%08x\n", bs_core_dump->dfsr);
    /* Data Fault Address Register */
    sal_printf("DFAR : 0x%08x\n", bs_core_dump->dfar);
    /* Instruction Fault Status Register */
    sal_printf("IFSR : 0x%08x\n", bs_core_dump->ifsr);
    /* Instruction Fault Address Register */

    sal_printf("IFAR : 0x%08x\n", bs_core_dump->ifar);
    return SYS_OK;
}

void bs_exception_detect(void *param)
{
    if ((*(uint32_t *)BS_EXEC_CPSR_PTR_ADDR != 0) && (bs_core_dump->core_status == 0)){
        bs_core_dump->core_status = 1;
        bs_core_dump->type = *(uint32_t *)BS_EXEC_TYPE_PTR_ADDR;
        bs_core_dump->cpsr = *(uint32_t *)BS_EXEC_CPSR_PTR_ADDR;
        bs_core_dump->dfsr = *(uint32_t *)BS_EXEC_DFSR_PTR_ADDR;
        bs_core_dump->dfar = *(uint32_t *)BS_EXEC_DFAR_PTR_ADDR;
        bs_core_dump->ifsr = *(uint32_t *)BS_EXEC_IFSR_PTR_ADDR;
        bs_core_dump->ifar = *(uint32_t *)BS_EXEC_IFAR_PTR_ADDR;
        bs_core_dump->length = BS_EXEC_CRASH_INFO_SRAM_LEN;
        board_broadsync_core_dump_print();
    }
}
#endif /* CFG_BROADSYNC_INCLUDED */
