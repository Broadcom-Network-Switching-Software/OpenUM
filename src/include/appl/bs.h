/*
 * $Id: bs.h,v 1.2 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _BS_H_
#define _BS_H_

#include <utils/shr/shr_types.h>

/* Hard Addresses. */
#define BS_DEBUG_STRUCTURE_ADDR             0x01180200
#define BS_DEBUG_TIMESTAMP_ADDR             0x01180800
#define BS_PRINT_BUFFER_ADDR                0x01181000

#define BS_PRINT_BUFF_DEFAULT_PRINT_OFFSET  (0)
#define BS_PRINT_BUFF_CMD_RSP_PRINT_OFFSET  (0x200)
#define BS_PRINT_BUFF_PLL_PRINT_OFFSET      (0x300)

#define BS_EXEC_CPSR_PTR_ADDR               0x01160060
#define BS_EXEC_TYPE_PTR_ADDR               0x01160064
#define BS_EXEC_DFSR_PTR_ADDR               0x01160068
#define BS_EXEC_DFAR_PTR_ADDR               0x0116006c
#define BS_EXEC_IFSR_PTR_ADDR               0x01160070
#define BS_EXEC_IFAR_PTR_ADDR               0x01160074

#define BS_VER_PTR_ADDR                     0x011600b4

#define BS_EXEC_DEBUG_SRAM_START            0x012FF000

#define BS_EXEC_CRASH_INFO_SRAM_LEN         (24)

#define BS_MAX_TS_EVENTS                    (19)

/* Structure for containing 32/48/64-bit hardware timestamp */
typedef struct {
    uint32_t ts_hi;
    uint32_t ts_lo;
} bs_shared_hw_ts_t;

typedef struct {
    uint32_t capture_mask;
    uint32_t full_time_seconds_hi;
    uint32_t full_time_seconds_low;
    uint32_t full_time_nanoseconds;

    bs_shared_hw_ts_t ts0_timestamp;
    bs_shared_hw_ts_t ts1_timestamp;

    bs_shared_hw_ts_t event_timestamp[BS_MAX_TS_EVENTS];
    bs_shared_hw_ts_t prev_event_timestamp[BS_MAX_TS_EVENTS];
} bs_shared_ts_data_t;

typedef struct {
    uint32_t core_status; /* 0 - Running, 1 - Exception*/
    uint32_t length; /* Length of the crash info in bytes */
    uint32_t type;
    uint32_t cpsr;
    uint32_t dfsr;
    uint32_t dfar;
    uint32_t ifsr;
    uint32_t ifar;
} bs_exe_core_dump_t;
extern int board_broadsync_log_configure(uint32 debug_mask, uint64 udp_log_mask,
                                         shr_mac_t src_mac, shr_mac_t dest_mac,
                                         uint16 tpid, uint16 vid, uint8 ttl,
                                         shr_ip_t src_addr, shr_ip_t dest_addr,
                                         uint16 udp_port);
extern int board_broadsync_config_set(int mode, int bitclk_hz, int hb_hz);
extern int board_broadsync_freq_offset_set(int offset_ppb);
extern int board_broadsync_phase_offset_set(int offset_sec, int offset_nsec,
                                            int is_negative);
extern int board_broadsync_ntp_offset_set(int offset_sec, int offset_nsec,
                                          int is_negative);
extern int board_broadsync_debug_1pps_set(uint8 enableOutput);
extern int board_broadsync_reinit(void);
extern int board_broadsync_core_dump_print(void);

#endif /* _BS_H_ */
