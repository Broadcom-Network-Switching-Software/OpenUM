/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _UTILS_SYSTEM_H_
#define _UTILS_SYSTEM_H_

#define __STR(x) #x
#define STR(x) __STR(x)

/*
 * System name/description
 */
#define MAX_SYSTEM_NAME_LEN (64)
#ifdef CFG_ZEROCONF_MDNS_INCLUDED
#if CFG_XGS_CHIP
#define DEFAULT_SYSTEM_NAME  CFG_BOARDNAME
#else /* ROBO */
#define DEFAULT_SYSTEM_NAME  STR(_BOARD_NAME_)
#endif /* CFG_XGS_CHIP */
#else /* !CFG_ZEROCONF_MDNS_INCLUDED */
#define DEFAULT_SYSTEM_NAME  ""
#endif /* CFG_ZEROCONF_MDNS_INCLUDED */

/*
 * Default MAC address
 */
#define DEFAULT_MAC_ADDR    { 0x00, 0x10, 0x18, 0x55, 0x44, 0x4b }

typedef enum {
    REGISTRATION_STATUS_TURN_OF_REMIND = 0,
    REGISTRATION_STATUS_REMIND_ME_LATER,
    REGISTRATION_STATUS_REGISTERED
} REGISTRATION_STATUS;

/*
 * Product registration status
 */
#define MAX_REG_STATUS_LEN              (1)
#define DEFAULT_REGISTRATION_STATUS     REGISTRATION_STATUS_REMIND_ME_LATER

/*
 * Product serial number
 */
#define MAX_SERIAL_NUM_MAGIC_LEN        (4) /* magic number for serial number */
#define MAX_SERIAL_NUM_LEN              (20)
#define DEFAULT_SERIAL_NUMBER           ""

/* Parse mac address nvram format to array */
extern sys_error_t parse_mac_address(const char *str, uint8 *macaddr);

/* Set/Get product registration status */
extern sys_error_t set_registration_status(uint8 status);
extern sys_error_t get_registration_status(uint8 *status);

/* Set/Get product serial number */
extern sys_error_t get_serial_num(uint8 *valid, char *serial_num);
extern sys_error_t set_serial_num(uint8 valid, const char *serial_num);

/* Set/Get system name or description */
extern sys_error_t set_system_name(const char *name);
extern sys_error_t get_system_name(char *buf, uint8 len);

/* Get system mac */
extern sys_error_t get_system_mac(uint8 *mac_buf);

extern void system_utils_init(void);

#endif /* _UTILS_SYSTEM_H_ */
