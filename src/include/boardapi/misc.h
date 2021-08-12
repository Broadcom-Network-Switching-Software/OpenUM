/*! \file misc.h
 *
 * Misc board APIs.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_MISC_H_
#define _BOARDAPI_MISC_H_

#if CFG_FLASH_SUPPORT_ENABLED
/* Get flash device object */
/*!
 * \brief Get the flash device object.
 *
 * \return Pointer to flash device (e.g., n25q256_dev).
 */
extern flash_dev_t * board_get_flash_dev(void) REENTRANT;
#endif /* CFG_FLASH_SUPPORT_ENABLED */

/*! length of board name */
#define MAX_BOARD_NAME_LEN   32

/*! flash image header, should be 64 bytes  */
typedef struct flash_imghdr_s {
    /*! UMHD */
    uint8 seal[4];
    /*! Image size in hex, in bytes, always big-endian */
    uint8 size[8];
    /*! Various flags in hex, always big-endian */
    uint8 flags[4];
    /*! checksum in hex, always big-endian */
    uint8 chksum[4];
    /*! Board name */
    uint8 boardname[MAX_BOARD_NAME_LEN];
    /**@{*/
    /*! Firmware version */
    uint8 majver,minver,ecover,miscver;
    /**@}*/
    /*! Timestamp to be used in dual image boot */
    uint8 timestamp[4];
    /*! Reserved field */
    uint8 reserved[4];
} flash_imghdr_t;

/*! image header seal */
#define UM_IMAGE_HEADER_SEAL    "UMHD"

/*!
 * Shared structure between loader and firmware. Currently used when firmware
 * needs to upgrade image and jump back to loader to do it.
 */
typedef struct bookkeeping_s
{
    /*! magic number */
    uint32 magic;
    /*! IP addess of agent */
    uint16 agent_ip[2];
    /*! Netmask of agent */
    uint16 agent_netmask[2];
    /*! Gateway of agent */
    uint16 agent_gateway[2];
    /*! reserved field */
    uint16 reserved;

#if CFG_UIP_IPV6_ENABLED
    /*! IPv6 addess of agent */
    uint8 agent_ipv6[16];
#endif /* CFG_UIP_IPV6_ENABLED */

#ifdef CFG_DUAL_IMAGE_INCLUDED
    /*!
     * [31:16] = timestamp of active image,
     * [15:8]  = backup image
     * [7:0]   = active image 
     */
    uint32 active_image;
#endif
} bookkeeping_t;

/*! bookkeeping seal */
#define UM_BOOKKEEPING_SEAL    (0x5441574E)

/*! timestamp magic number start */
#define TIMESTAMP_MAGIC_START  (0xAC)
/*! timestamp magic number end */
#define TIMESTAMP_MAGIC_END    (0x47)

/*! get active image */
#define ACTIVE_IMAGE_GET(flag)      ((flag) & 0xFF)
/*! get active timestamp */
#define ACTIVE_TIMESTAMP_GET(flag)  ((flag) >> 16)
/*! check backup image valid or not */
#define IS_BACKUP_IMAGE_VALID(flag) (((flag) & 0xFF00) != 0)

/*! loader mode */
typedef enum loader_mode_s {
    LM_NORMAL,
    LM_UPGRADE_FIRMWARE
} loader_mode_t;

#ifdef __BOOTLOADER__

/* Check integrity of firmware image */
extern BOOL board_check_image(hsaddr_t address, hsaddr_t *outaddr) REENTRANT;

/* Launch program at entry address */
extern void board_load_program(hsaddr_t entry) REENTRANT;

/*
 * For loader to check if requested by firmware to upgrade image.
 */
extern loader_mode_t board_loader_mode_get(bookkeeping_t *bk_data, BOOL reset) REENTRANT;

#else
/*!
 * \brief For firmware to request loader to upgrade image.
 *
 * \param [in] mode loader mode.
 * \param [in] bk_data input bookkeeping data.
 */
extern void board_loader_mode_set(loader_mode_t mode, bookkeeping_t *bk_data);
#endif /* __BOOTLOADER__ */

/*!
 * \brief Validate firmware image header
 *
 * \param [in] address addess.
 */
extern BOOL board_check_imageheader(msaddr_t address) REENTRANT;

/*
 * System Reset
 */
/*!
 * \brief Set the board to reset.
 *
 * \param [in] param 0 or input data.
 *
 * \return VOID
 */
extern void board_reset(void *param) REENTRANT;

/*
 * Enable/Disable loop detect functionality.
 */
/*!
* \brief Set the loop-detect state.
*
* \param [in] enable
*    \li TRUE = Enable loop detect.
*    \li FALSE = Disable loop detect.
*/
extern void board_loop_detect_enable(BOOL enable);

/*!
 * \brief Get whether the loop-detect state.
 *
 * \return \li TRUE = Enable loop detect.
 *           \li FALSE = Disable loop detect.
 */
extern uint8 board_loop_detect_status_get(void);

/*! led mode */
enum {
   BOARD_PHY_LED_NORMAL,
   BOARD_PHY_LED_LOOP_FOUND,
} board_phy_led_mode;

/*!
 * \brief Set phy led mode.
 *
 * \param [in] lport logical port.
 * \param [in] mode led mode.
 *
 * \return \li TRUE = success.
 *         \li FALSE = failed.
 */
extern BOOL board_phy_led_mode_set(uint8 lport, int mode);

/*!
 * \brief Get the firmware version.
 *
 * \param [out] major Major number.
 * \param [out] minor Minor number.
 * \param [out] eco Eco number.
 * \param [out] misc Not used.
 *
 * \return VOID
 */
extern void board_firmware_version_get(uint8 *major, uint8 *minor,
                                       uint8 *eco, uint8 *misc) REENTRANT;


#ifdef CFG_ZEROCONF_MDNS_INCLUDED
/*!
 * \brief Set the mdns state.
 *
 * \param [in] enable
 *    \li TRUE = Enable MDNS.
 *    \li FALSE = Disable MDNS.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_mdns_enable_set(BOOL enable);

/*!
 * \brief Get the mdns state.
 *
 * \param [out] enable
 *    \li TRUE = MDNS is enabled.
 *    \li FALSE = MDNS is disabled.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_mdns_enable_get(BOOL *enable);
#endif /* CFG_ZEROCONF_MDNS_INCLUDED */

#ifdef CFG_DUAL_IMAGE_INCLUDED
/*!
 * \brief Set the active image info.
 *
 * \param [in] partition Active image partition info.
 *
 * \retval VOID.
 */
extern void board_active_image_set(uint32 partition);

/*!
 * \brief Get the active image info.
 *
 * \return Active image partition info.
 */
extern uint32 board_active_image_get(void);

/*!
 * \brief Select the available image to boot.
 *
 * \param [out] outaddr The active image address.
 *
 * \return \li TRUE = success.
 *         \li FALSE = failed.
 */
extern BOOL board_select_boot_image(hsaddr_t *outaddr);
#endif /* CFG_DUAL_IMAGE_INCLUDED */

#ifdef CFG_RESET_BUTTON_INCLUDED
/*! the duration (in seconds) for software simulate user to press the reset button */
extern uint8 sw_simulate_press_reset_button_duration;
/*! reset button gpio is active high */
extern uint8 reset_button_active_high;
/*! reset button gpio bit */
extern uint8 reset_button_gpio_bit;
/*! reset button feature is enabled or not (by config variable) */
extern uint8 reset_button_enable;

/*!
 * \brief Get whether the reset button is triggered by user.
 *
 * \retval TRUE Reset button is triggered .
 */
extern BOOL board_reset_button_get(void);
#endif /* CFG_RESET_BUTTON_INCLUDED */



#endif /* _BOARDAPI_MISC_H_ */
