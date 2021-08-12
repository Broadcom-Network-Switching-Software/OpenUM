/*
 * $Id: board.c,v 1.17 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#include "bsp_config.h"
#include "brdimpl.h"
#include "utils/ports.h"
#include "pcm.h"

/* Change the port name to the lport# */
#define SOC_PORT_NAME(unit, port)       (lport)

/* from sdk/include/sal/compiler.h */
#define COMPILER_REFERENCE(_a)    ((void)(_a))
#define COMPILER_ATTRIBUTE(_a)    __attribute__ (_a)
#ifndef FUNCTION_NAME
#define FUNCTION_NAME() (__FUNCTION__)
#endif

#include <shared/bsl.h>
#include <shared/bslenum.h>
#include <shared/bsltypes.h>


//#define BSL_LS_SOC_LOOPDETECT_LED_DEBUG
//#define BSL_LS_SOC_LOOPDETECT_LED_READBACK
//#define BSL_LS_SOC_LAG_DEBUG
//#define BSL_LS_SOC_RATE_DEBUG

#if defined(LOG_VERBOSE) && defined(BSL_LS_SOC_LOOPDETECT_LED_DEBUG)
/* Outout the debug message with LOG_VERBOSE() */
#define LOG_LOOP_LED     LOG_VERBOSE
#else
#define LOG_LOOP_LED(ls_, stuff_)
#endif

#if defined(LOG_VERBOSE) && defined(BSL_LS_SOC_LAG_DEBUG)
/* Outout the debug message with LOG_VERBOSE() */
#define LOG_LAG     LOG_VERBOSE
#else
#define LOG_LAG(ls_, stuff_)
#endif

#if defined(LOG_VERBOSE) && defined(BSL_LS_SOC_RATE_DEBUG)
/* Outout the debug message with LOG_VERBOSE() */
#define LOG_RATE     LOG_VERBOSE
#else
#define LOG_RATE(ls_, stuff_)
#endif



extern soc_switch_t soc_switch_bcm5607x;
#ifdef CFG_CHIP_SYMBOLS_INCLUDED
extern chip_info_t bcm5607x_chip_info;
#endif /* CFG_CHIP_SYMBOLS_INCLUDED */
#if CFG_FLASH_SUPPORT_ENABLED
/* Flash device driver. */
extern flash_dev_t n25q256_dev;
#endif /* CFG_FLASH_SUPPORT_ENABLED */

static const char *um_boardname = CFG_BOARDNAME;

#ifdef CFG_SWITCH_QOS_INCLUDED
static qos_type_t  qos_info = QT_COUNT;
#endif

#ifdef CFG_SWITCH_RATE_INCLUDED
static uint8 storm_info = STORM_RATE_NONE;
#endif

#ifdef CFG_SWITCH_MCAST_INCLUDED
typedef struct mcast_list_s {
    uint8 mac[6];
    uint16 vlan_id;
    uint16 index;
    pbmp_t  port_lpbmp; /* all ports' pbmp in this vlan_id */

    struct mcast_list_s *next;
} mcast_list_t;

static mcast_list_t *mlist = NULL;

/* address the minimal l2mc index can be used */
#define L2MC_MAX_ENTRY_SIZE  ((L2MCm_MAX+1+31)/32)

static uint32 mcindex[L2MC_MAX_ENTRY_SIZE];
#endif /* CFG_SWITCH_MCAST_INCLUDED */

#ifdef CFG_SWITCH_LAG_INCLUDED
/* variables for static trunk */
uint8 lag_enable = FALSE;

typedef struct lag_group_s {
    uint8 enable;
    pbmp_t lpbmp;
} lag_group_t;

lag_group_t lag_group[BOARD_MAX_NUM_OF_LAG];
#endif /* CFG_SWITCH_LAG_INCLUDED */

static int *u2l_mapping_auto = NULL;

/*!
 * \brief Get the board's name.
 *
 * \return Board name.
 */
const char *
board_name(void)
{
    return um_boardname;
}

/*!
 * \brief Get the port count of user ports.
 *
 * \return Port count.
 */
uint8
board_uport_count(void)
{
    return bcm5607x_port_count(0);
}

/*!
 * \brief Map a user port to a chip logical port.
 *
 * \param [in] uport The user port
 * \param [out] unit The chip unit number
 * \param [out] lport The chip logical port
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_uport_to_lport(uint16 uport, uint8 *unit, uint8 *lport)
{

    const int *u2l_mapping;
    uint8 index, count;
    uint8 active_port_count = 0;
    int port;
    switch (fl_sw_info.devid) {
       default:
           /* Assign uport to lport mapping in lport order automatically*/
           count = board_uport_count();
           if (u2l_mapping_auto == NULL) {
               u2l_mapping_auto = sal_malloc(sizeof(int) * board_uport_count());
               index = 0;
               SOC_LPORT_ITER(port) {
                   u2l_mapping_auto[index] = port;
                   index++;
                   if (index >= count) {
                       break;
                   }
               }
           }
           u2l_mapping = (const int *) u2l_mapping_auto;
       break;
    }

    if (SAL_UPORT_IS_NOT_VALID(uport) || unit == NULL || lport == NULL) {
        return SYS_ERR_PARAMETER;
    }

    *unit = 0;
    for (index = 0; index < count; index++) {
        if (SOC_PORT_L2P_MAPPING(u2l_mapping[index]) != -1) {
            active_port_count++;
        }

        if (active_port_count == (uport + 1 - SAL_UPORT_BASE)) {
            break;
        }
    }

    *lport = u2l_mapping[index];

    return SYS_OK;

}


/*!
 * \brief Map a chip logical port to a user port.
 *
 * \param [in] unit The chip unit number
 * \param [in] lport The chip logical port
 * \param [out] uport The user port
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_lport_to_uport(uint8 unit, uint8 lport, uint16 *uport)
{

   const int *u2l_mapping;
   uint8 index, count;
   uint8 active_port_count = 0;
   int port;

   switch (fl_sw_info.devid) {
       default:
           count = board_uport_count();
           /* Assign uport to lport mapping in lport order automaticly*/
           if (u2l_mapping_auto == NULL) {
               u2l_mapping_auto = sal_malloc(sizeof(int) * board_uport_count());
               index = 0;
               SOC_LPORT_ITER(port) {
                   u2l_mapping_auto[index] = port;
                   index++;
                   if (index >= count) {
                       break;
                   }
               }
           }
           u2l_mapping = (const int *) u2l_mapping_auto;
       break;
   }


    if (uport == NULL || lport < BCM5607X_LPORT_MIN || lport > BCM5607X_LPORT_MAX || unit > 0) {
        return SYS_ERR_PARAMETER;
    }

    if (SOC_PORT_L2P_MAPPING(lport) == -1) {
        return SYS_ERR_PARAMETER;
    }

    count = board_uport_count();

    for (index = 0; index < count; index++) {
        if (SOC_PORT_L2P_MAPPING(u2l_mapping[index]) != -1) {
            active_port_count++;
        }

        if (u2l_mapping[index] == lport) {
            break;
        }
    }

    *uport = active_port_count + 1 - SAL_UPORT_BASE;

    return SYS_OK;
}

/*!
 * \brief Map the user port bitmap array (uplist) to the chip logical port bitmap (lpbmp) on selected chip unit .
 *
 * \param [in] uplist The user port bit map array which may cover many chips
 * \param [in] unit The selected chip unit number
 * \param [out] lpbmp The chip logical port bit map
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_uplist_to_lpbmp(uint8 *uplist, uint8 unit, pbmp_t *lpbmp)
{
    uint8 lport;
    uint16 uport;
    if (uplist == NULL || lpbmp == NULL || unit > 0) {
        return SYS_ERR_PARAMETER;
    }

    PBMP_CLEAR(*lpbmp);

    SAL_UPORT_ITER(uport) {
            if (uplist_port_matched(uplist, uport) == SYS_OK) {
                if (board_uport_to_lport(uport, &unit, &lport) == SYS_OK) {
                     PBMP_PORT_ADD((*lpbmp), lport);
                }
            }
    }

    return SYS_OK;
}


/*!
 * \brief Map the chip logical port bit map (lpbmp) to a user port bit map array (uplist) on selected chip unit.
 *
 * \param [in] unit The selected chip unit number
 * \param [in] lpbmp The chip logical port bit map
 * \param [out] uplist The user port bit map array which may cover many chips
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_lpbmp_to_uplist(uint8 unit, pbmp_t lpbmp, uint8 *uplist)
{
    uint16 uport;/* uport is front port index from 0 to board_uport_count()-1 */
    uint8 lport;

    if (uplist == NULL || unit > 0) {
        return SYS_ERR_PARAMETER;
    }

    uplist_clear(uplist);

    SOC_LPORT_ITER(lport) {
        if (PBMP_MEMBER(lpbmp, lport)) {
            if (board_lport_to_uport(unit, lport, &uport) == SYS_OK) {
                uplist_port_add(uplist, uport);
            }
        }
    }


    return SYS_OK;
}

/*!
 * \brief Get SoC by unit.
 *
 * \param [in] unit Unit number.
 *
 * \return Pointer to switch device (e.g., soc_switch_bcm5340x).
 */
soc_switch_t *
board_get_soc_by_unit(uint8 unit)
{
    if (unit > 0) {
        return NULL;
    }
    return &soc_switch_bcm5607x;
}

/**
 * <b>Description:</b> Get chip information by unit.
 * @param unit - Unit number.
 * @return Pointer to chip information.
 */

chip_info_t *
board_get_chipinfo_by_unit(uint8 unit)
{
    if (unit > 0) {
        return NULL;
    }
#ifdef CFG_CHIP_SYMBOLS_INCLUDED
    return &bcm5607x_chip_info;
#else
    return NULL;
#endif /* CFG_CHIP_SYMBOLS_INCLUDED */
}

/*!
 * \brief Set the board to reset.
 *
 * \param [in] param 0 or input data.
 *
 * \return VOID
 */
void
board_reset(void *param)
{
    DMU_CRU_RESETr_t dmu_cru_reset;
    if (param) {
        if (*(BOOL *)param) {
            /* Hard reset */
        }
    }
    /* [Bit 0]: switch reset, [Bit 1]: iproc only reset */
    DMU_CRU_RESETr_CLR(dmu_cru_reset);
    WRITE_DMU_CRU_RESETr(0, dmu_cru_reset);
}

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
void
board_firmware_version_get(uint8 *major, uint8 *minor, uint8 *eco, uint8 *misc)
{
    *major = CFE_VER_MAJOR;
    *minor = CFE_VER_MINOR;
    *eco = CFE_VER_BUILD;
    *misc = 0;
}

#if CFG_RXTX_SUPPORT_ENABLED
sys_error_t
board_rx_set_handler(BOARD_RX_HANDLER fn)
{
    return brdimpl_rx_set_handler(fn);
}

sys_error_t
board_rx_fill_buffer(sys_pkt_t *pkt)
{
    return brdimpl_rx_fill_buffer(pkt);
}

/*!
 * \brief Transmit the packet.
 *
 * \param [in] pkt Pointer to packet buffer.
 * \param [in] cbk Callback.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_tx(sys_pkt_t *pkt, BOARD_TX_CALLBACK cbk)
{
    return brdimpl_tx(pkt, cbk);
}

void
board_rxtx_stop(void)
{
    bcm5607x_rxtx_stop();
    return;
}

#endif /* CFG_RXTX_SUPPORT_ENABLED */

#if CFG_FLASH_SUPPORT_ENABLED
/* Get flash device object */

/*!
 * \brief Get the flash device object.
 *
 * \return Pointer to flash device (e.g., n25q256_dev).
 */
flash_dev_t *
board_get_flash_dev()
{
    /* to use auto-probe result */
    return flash_dev_get();

}
#endif /* CFG_FLASH_SUPPORT_ENABLED */

/* Check integrity of firmware image */
BOOL
board_check_image(hsaddr_t address, hsaddr_t *outaddr)
{
    uint16 hdrchksum, chksum = 0;
    uint32 i, size;
    flash_imghdr_t *hdr = (flash_imghdr_t *)address;
    uint8 *ptr = HSADDR2DATAPTR(address);
    char buf[64];

    if (sal_memcmp(hdr->seal, UM_IMAGE_HEADER_SEAL, sizeof(hdr->seal)) != 0) {
#if CFG_CONSOLE_ENABLED && !defined(CFG_DUAL_IMAGE_INCLUDED)
        sal_printf("Invalid header seal.  This is not a valid image.\n");
#endif /* CFG_CONSOLE_ENABLED */
        return FALSE;
    }

    sal_memcpy(buf, hdr->size, 8);
    buf[8] = 0;
    size = (uint32)sal_xtoi((const char*)buf);

    sal_memcpy(buf, hdr->chksum, 4);
    buf[4] = 0;
    hdrchksum = (uint16)sal_xtoi((const char*)buf);
#if CFG_CONSOLE_ENABLED
#ifdef CFG_IMAGE_ID
    sal_printf("Flash image %c is %d bytes, chksum %04X, version %c.%c.%c for board %s\n",
                hdr->miscver, size, hdrchksum, hdr->majver,
                hdr->minver, hdr->ecover, hdr->boardname);
#else
    sal_printf("Flash image is %d bytes, chksum %04X, version %c.%c.%c for board %s\n",
                size, hdrchksum, hdr->majver, hdr->minver, hdr->ecover, hdr->boardname);
#endif /* CFG_IMAGE_ID */
#endif /* CFG_CONSOLE_ENABLED */
    if (sal_strcmp(board_name(), (const char*)hdr->boardname) != 0) {
#if CFG_CONSOLE_ENABLED
        sal_printf("This image is not appropriate for board type '%s'\n",board_name());
#endif /* CFG_CONSOLE_ENABLED */
        return FALSE;
    }

    ptr += sizeof(flash_imghdr_t);
    for (i = 0; i < size; i = i + 50) {
        chksum += *(ptr+i);
    }

    if (chksum != hdrchksum) {
#if CFG_CONSOLE_ENABLED
        sal_printf("Checksum incorrect. Calculated chksum is %04X\n", chksum);
#endif /* CFG_CONSOLE_ENABLED */

        return FALSE;
    }

    *outaddr = DATAPTR2HSADDR(ptr);
    return TRUE;
}

BOOL
board_check_imageheader(msaddr_t address)
{
    flash_imghdr_t *hdr = (flash_imghdr_t *)address;
#ifdef CFG_IMAGE_ID
    int32 image_id;
#endif

    if (sal_memcmp(hdr->seal, UM_IMAGE_HEADER_SEAL, sizeof(hdr->seal)) != 0) {
#if CFG_CONSOLE_ENABLED && !defined(CFG_DUAL_IMAGE_INCLUDED)
        sal_printf("Invalid header seal.  This is not a valid image.\n");
#endif /* CFG_CONSOLE_ENABLED */
        return FALSE;
    }

#if CFG_CONSOLE_ENABLED
#ifdef CFG_IMAGE_ID
    sal_printf("Flash image %c is version %c.%c.%c for board %s\n",
                hdr->miscver, hdr->majver, hdr->minver,
                hdr->ecover, hdr->boardname);
#else
    sal_printf("Flash image is version %c.%c.%c for board %s\n",
                hdr->majver, hdr->minver, hdr->ecover, hdr->boardname);
#endif /* CFG_IMAGE_ID */
#endif /* CFG_CONSOLE_ENABLED */
    if (sal_strcmp(board_name(), (const char*)hdr->boardname) != 0) {
#if CFG_CONSOLE_ENABLED
        sal_printf("This image is not appropriate for board type '%s'\n",board_name());
#endif /* CFG_CONSOLE_ENABLED */
        return FALSE;
    }

    /* check image ID */
#if CFG_IMAGE_ID
    /* miscver is used as image ID */
    image_id = sal_atoi((char*)&hdr->miscver);
    if (address == BOARD_FIRMWARE_ADDR) {
        if (image_id != CFG_IMAGE_1_ID) {
#if CFG_CONSOLE_ENABLED
            sal_printf("First image ID should be %d, but is '%d'\n",
                        CFG_IMAGE_1_ID, image_id);
#endif
            return FALSE;
        }
    } else if (address == BOARD_SECONDARY_FIRMWARE_ADDR) {
        if (image_id != CFG_IMAGE_2_ID) {
#if CFG_CONSOLE_ENABLED
            sal_printf("Second image ID should be %d, but is '%d'\n",
                        CFG_IMAGE_2_ID, image_id);
#endif
            return FALSE;
        }
    }
#endif

    return TRUE;
}

/* Launch program at entry address */
void
board_load_program(hsaddr_t entry)
{
    void (*funcptr)(void) = (void (*)(void))entry;
    (*funcptr)();
}

loader_mode_t
board_loader_mode_get(bookkeeping_t *data, BOOL reset)
{
    bookkeeping_t *pdata = (bookkeeping_t *)BOARD_BOOKKEEPING_ADDR;

    if (pdata->magic == UM_BOOKKEEPING_SEAL) {
        /* Firmware notify loader to do firmware upgrade */
        if (data) {
            sal_memcpy(data, pdata, sizeof(bookkeeping_t));
        }
        if (reset) {
            pdata->magic = 0x0;
        }
        return LM_UPGRADE_FIRMWARE;
    }
    return LM_NORMAL;
}

void
board_loader_mode_set(loader_mode_t mode, bookkeeping_t *data)
{
    bookkeeping_t *pdata = (bookkeeping_t *)BOARD_BOOKKEEPING_ADDR;
#ifdef CFG_DUAL_IMAGE_INCLUDED
    uint32 active_image = pdata->active_image;
#endif
    if (mode == LM_UPGRADE_FIRMWARE) {
        sal_memcpy(pdata, data, sizeof(bookkeeping_t));
#ifdef CFG_DUAL_IMAGE_INCLUDED
        pdata->active_image = active_image;
#endif
    }
}

#ifdef CFG_DUAL_IMAGE_INCLUDED
static int
calculate_score(hsaddr_t address)
{
    flash_imghdr_t *hdr = (flash_imghdr_t *)address;
    hsaddr_t addr;

    /* Check image (header and checksum) first */
    if (!board_check_imageheader(address) || !board_check_image(address, &addr)) {
        return -1;
    }

    /* Header OK, now check if there is any valid timestamp */
    if (hdr->timestamp[0] != TIMESTAMP_MAGIC_START ||
        hdr->timestamp[3] != TIMESTAMP_MAGIC_END) {
        /* No timestamp found, but the image is OK */
        return 0;
    }

    return (int)(((uint16) hdr->timestamp[1] << 8) + (uint16)hdr->timestamp[2]);
}
/* Should be invoked in boot loader only */
BOOL
board_select_boot_image(hsaddr_t *outaddr)
{
    int score1, score2;
    uint8 *ptr;
    /* [31:16] = timestamp, [15:0] = booting partition */
    uint32 booting_partition ;

    score1 = calculate_score((hsaddr_t)BOARD_FIRMWARE_ADDR);
    score2 = calculate_score((hsaddr_t)BOARD_SECONDARY_FIRMWARE_ADDR);

    if (score1 == -1 && score2 == -1) {
#if CFG_CONSOLE_ENABLED
        sal_printf("There is no valid image \n");
#endif /* CFG_CONSOLE_ENABLED */
        return FALSE;
    }

    if (score1 == 0xffff || score2 == 0xffff) {
        /* Deal with overflow condition */
        if (score1 == 1) {
            score2 = 0;
        } else if (score2 == 1) {
            score1 = 0;
        }
    }

    if (score1 == -1) {
        booting_partition = 2 | ((uint32)(score2) << 16);
    } else if (score2 == -1) {
        booting_partition = 1 | ((uint32)(score1) << 16);
    } else {
        /*
         * Partition 1 has priority over partition 2
         */
        if (score1 >= score2) {
            booting_partition = 0x201 | ((uint32)(score1) << 16);
        } else {
            booting_partition = 0x102 | ((uint32)(score2) << 16);
        }
    }

    if (ACTIVE_IMAGE_GET(booting_partition) == 1) {
        ptr = HSADDR2DATAPTR(BOARD_FIRMWARE_ADDR);
    } else {
        ptr = HSADDR2DATAPTR(BOARD_SECONDARY_FIRMWARE_ADDR);
    }

    ptr += sizeof(flash_imghdr_t);
    *outaddr = DATAPTR2HSADDR(ptr);

    board_active_image_set(booting_partition);

    return TRUE;
}

void
board_active_image_set(uint32 partition)
{
    bookkeeping_t *pdata = (bookkeeping_t *)BOARD_BOOKKEEPING_ADDR;
    pdata->active_image = partition;
}

uint32
board_active_image_get(void)
{
    bookkeeping_t *pdata = (bookkeeping_t *)BOARD_BOOKKEEPING_ADDR;
    return pdata->active_image;
}
#endif /* CFG_DUAL_IMAGE_INCLUDED */

#ifdef CFG_SWITCH_VLAN_INCLUDED
/*!
 * \brief Set the VLAN type.
 *
 * \param [in] type
 *    \li VT_PORT_BASED
 *    \li VT_DOT1Q
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_vlan_type_set(vlan_type_t type)
{
    return brdimpl_vlan_type_set(type);
}

/*!
 * \brief Get the VLAN type.
 *
 * \param [out] type
 *    \li VT_PORT_BASED
 *    \li VT_DOT1Q
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_vlan_type_get(vlan_type_t *type)
{
    return brdimpl_vlan_type_get(type);
}

/*!
 * \brief Create a new VLAN by the VLAN ID.
 *
 * \param [in] vlan_id VLAN ID number.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_vlan_create(uint16 vlan_id)
{
    return brdimpl_vlan_create(vlan_id);
}

/*!
 * \brief Destroy the selected VLAN.
 *
 * \param [in] vlan_id VLAN ID number.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_vlan_destroy(uint16 vlan_id)
{
    return brdimpl_vlan_destroy(vlan_id);
}

/*!
 * \brief Set the port-based VLAN members for a given VLAN ID.
 *
 * \param [in] vlan_id VLAN ID number.
 * \param [in] uplist VLAN members port list number.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_pvlan_port_set(uint16  vlan_id, uint8 *uplist)
{
    return brdimpl_pvlan_port_set(vlan_id, uplist);
}

/*!
 * \brief Get the port-based VLAN members for a given VLAN ID.
 *
 * \param [in] vlan_id VLAN ID number.
 * \param [out] uplist VLAN members port list number.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_pvlan_port_get(uint16  vlan_id, uint8 *uplist)
{
    return brdimpl_pvlan_port_get(vlan_id, uplist);
}

/*!
 * \brief Get the egress port list for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] uplist Egress mask of the port list.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_pvlan_egress_get(uint16 uport, uint8 *uplist)
{
    return brdimpl_pvlan_egress_get(uport, uplist);
}

/*!
 * \brief Set the PVID for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] vlan_id PVID number.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_untagged_vlan_set(uint16 uport, uint16 vlan_id)
{
    uint8 unit, lport;

    PORTm_t port;

    sys_error_t rv = SYS_OK;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    READ_PORTm(unit, lport, port);
    PORTm_PORT_VIDf_SET(port, vlan_id);
    WRITE_PORTm(unit, lport, port);

  return rv;
}

/*!
 * \brief Get the PVID for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] vlan_id PVID number.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_untagged_vlan_get(uint16 uport, uint16 *vlan_id)
{

    uint8 unit, lport;
    sys_error_t rv = SYS_OK;

    PORTm_t port;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    READ_PORTm(unit, lport, port);
    *vlan_id = PORTm_PORT_VIDf_GET(port);

    return rv;
}

/*!
 * \brief Set IEEE 802.1Q VLAN members and tag members by the VLAN ID.
 *
 * \param [in] vlan_id QVLAN ID number.
 * \param [in] uplist (IN) VLAN members port list number.
 * \param [in] tag_uplist VLAN tagged members port list number.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_qvlan_port_set(uint16  vlan_id, uint8 *uplist, uint8 *tag_uplist)
{
    return brdimpl_qvlan_port_set(vlan_id, uplist, tag_uplist);
}

/*!
 * \brief Get the IEEE 802.1Q VLAN members and tag members by the VLAN ID.
 *
 * \param [in] vlan_id QVLAN ID number.
 * \param [out] uplist VLAN members port list number.
 * \param [out] tag_uplist VLAN tagged members port list number.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_qvlan_port_get(uint16  vlan_id, uint8 *uplist, uint8 *tag_uplist)
{
    return brdimpl_qvlan_port_get(vlan_id, uplist, tag_uplist);
}

/*!
 * \brief Get the count of IEEE 802.1Q VLAN entry.
 *
 * \return VLAN count.
 */
uint16
board_vlan_count(void)
{
    return brdimpl_vlan_count();
}

/*!
 * \brief Get the IEEE 802.1Q VLAN ID, members, and tag members by index
 *
 * \param [in] index Index number.
 * \param [out] vlan_id QVLAN ID number.
 * \param [out] uplist VLAN members port list number.
 * \param [out] tag_uplist VLAN tagged members port list number.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_qvlan_get_by_index(uint16  index, uint16 *vlan_id, uint8 *uplist, uint8 *tag_uplist, BOOL get_uplist)
{
    return brdimpl_qvlan_get_by_index(index, vlan_id, uplist, tag_uplist, get_uplist);
}
#endif /* CFG_SWITCH_VLAN_INCLUDED */

/*!
 * \brief Get the port mode for the specific user port.
 *
 * \param [in] uport User port number.
 * \param [out] mode Port mode.
 *    \li PM_LINKDOWN
 *    \li PM_10MB_HD
 *    \li PM_10MB_FD
 *    \li ...
 *    \li PM_50000MB
 *    \li PM_100000MB
 *    \li PM_AUTO
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_mode_get(uint16 uport, port_mode_t *mode)
{
    int rv = SYS_OK;
    uint8 unit, lport;

    *mode = PM_LINKDOWN;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    if (!SOC_PORT_LINK_STATUS(lport)) {
       *mode = PM_LINKDOWN;
        return rv;
    }

    if(SOC_PORT_SPEED_STATUS(lport) == 10) {
        *mode = SOC_PORT_DUPLEX_STATUS(lport) ? PM_10MB_FD : PM_10MB_HD;
    } else if (SOC_PORT_SPEED_STATUS(lport) == 100) {
        *mode = SOC_PORT_DUPLEX_STATUS(lport) ? PM_100MB_FD : PM_100MB_HD;
    } else if (SOC_PORT_SPEED_STATUS(lport) == 1000) {
        *mode = PM_1000MB;
    } else if (SOC_PORT_SPEED_STATUS(lport) == 2500) {
    	  *mode = PM_2500MB;
    } else if (SOC_PORT_SPEED_STATUS(lport) == 5000) {
    	  *mode = PM_5000MB;
    } else if (SOC_PORT_SPEED_STATUS(lport) == 10000) {
        *mode = PM_10000MB;
    } else if (SOC_PORT_SPEED_STATUS(lport) == 25000) {
    	  *mode = PM_25000MB;
    } else if (SOC_PORT_SPEED_STATUS(lport) == 40000) {
    	  *mode = PM_40000MB;
    } else if (SOC_PORT_SPEED_STATUS(lport) == 50000) {
    	  *mode = PM_50000MB;
    } else if (SOC_PORT_SPEED_STATUS(lport) == 100000) {
        *mode = PM_100000MB;
    } else {
        *mode = PM_AUTO;
    }

    return rv;
}

/*!
 * \brief Run cable diagnostics on the port.
 *
 * \param [in] uport Port number.
 * \param [in] status Cable diag status structure.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_cable_diag(uint16 uport, port_cable_diag_t *status)
{
    return brdimpl_port_cable_diag(uport, status);
}

sys_error_t
board_get_cable_diag_support_by_port(uint16 uport, BOOL *support)
{
    uint8 unit, lport;
    sys_error_t r;
    int value;
    *support = FALSE;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    r = pcm_port_cable_diag_support(unit,lport, &value);

    *support = value;

    return r;
}

uint16
board_cable_diag_port_count(void)
{
    uint8 cnt = 0;
    BOOL support;
    uint16 uport;

    SAL_UPORT_ITER(uport){
        board_get_cable_diag_support_by_port(uport, &support);
        if (support) {
            cnt++;
        }
    }
    /* cnt = 0: no port can support cable count
       cnt is the number of how many port can support cable diag
     */
    return cnt;
}

#ifdef CFG_SWITCH_LAG_INCLUDED
/*!
 * \brief Set the lag to enable or disable.
 *
 * \param [in] enable:
 *    \li TRUE = Enable lag.
 *    \li FALSE = Disable lag.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_lag_set(uint8 enable)
{
    uint8 i, unit;
    pbmp_t hw_lpbmp;
    uint8  lport;
    BOOL   link;
    uint16 uport;

    if (lag_enable != enable) {
        for(i = 1 ; i <= BOARD_MAX_NUM_OF_LAG ; i++) {
            /* add lag setting to HW */
            if (enable == TRUE) {
                PBMP_ASSIGN(hw_lpbmp, lag_group[i-1].lpbmp);

                /* remove link down port from hw_pbmp */
                SAL_UPORT_ITER(uport) {
                    board_uport_to_lport(uport, &unit, &lport);
                    if (PBMP_MEMBER(lag_group[i-1].lpbmp, lport)) {
                        SOC_IF_ERROR_RETURN(
                            (*soc_switch_bcm5607x.link_status)(0, lport, &link));
                        if (!link) {
                            PBMP_PORT_REMOVE(hw_lpbmp, lport);
                        }
                    }
                }

                LOG_LAG(BSL_LS_SOC_COMMON,("%s..:lagid=%d\n", __func__, (i - 1)));
                SOC_IF_ERROR_RETURN(
                    bcm5607x_lag_group_set(0, (i - 1), hw_lpbmp));
            } else {
                /* remove lag setting from HW */
                PBMP_CLEAR(hw_lpbmp);
                SOC_IF_ERROR_RETURN(bcm5607x_lag_group_set(0, (i - 1), hw_lpbmp));
            }
        }
        lag_enable = enable;
    }
    return SYS_OK;
}

/*!
 * \brief Get the lag status.
 *
 * \param [out] enable
 *    \li TRUE = Lag is enabled
 *    \li FALSE = Lag is disabled.
 *
 * \retval SYS_OK No errors.
 */
void
board_lag_get(uint8 *enable)
{
    *enable = lag_enable;
}

/*!
 * \brief Set the lag group members.
 *
 * \param [in] lagid Lag ID number.
 * \param [in] enable
 *    \li TRUE = Enable the lag group.
 *    \li FALSE = Disable the lag group.
 * \param [in] uplist Lag members list.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_lag_group_set(uint8 lagid, uint8 enable, uint8 *uplist)
{
    pbmp_t lpbmp;
    pbmp_t hw_lpbmp;
    uint8 lport, count = 0;
    BOOL  link;

    if ((lagid == 0) || (lagid > BOARD_MAX_NUM_OF_LAG)) {
        return SYS_ERR_PARAMETER;
    }

    if (uplist != NULL) {
        SOC_IF_ERROR_RETURN(board_uplist_to_lpbmp(uplist, 0, &lpbmp));
    } else {
        return SYS_ERR_PARAMETER;
    }

    /* Check the number of trunk member.
     * - Minimum number of ports in each trunk: 2
     * - maximum number of ports in each trunk: 8
     */
    SOC_LPORT_ITER(lport) {
        if(PBMP_MEMBER(lpbmp, lport)) {
            count ++;
            if (count > BOARD_MAX_PORT_PER_LAG) {
                return SYS_ERR_PARAMETER;
            }
        }
    }

    if (count == 0x1) {
        return SYS_ERR_PARAMETER;
    }

    if ((enable == TRUE) && (lag_enable == TRUE)) {
        PBMP_ASSIGN(hw_lpbmp, lpbmp);

        /* remove link down port from hw_pbmp */
        SOC_LPORT_ITER(lport) {
            if (PBMP_MEMBER(lpbmp, lport)) {
                SOC_IF_ERROR_RETURN(
                    (*soc_switch_bcm5607x.link_status)(0, lport, &link));
                if (!link) {
                    PBMP_PORT_REMOVE(hw_lpbmp, lport);
                }
            }
        }
    } else {
        PBMP_CLEAR(hw_lpbmp);
    }

    /* HW setting */
    SOC_IF_ERROR_RETURN(bcm5607x_lag_group_set(0, (lagid - 1), hw_lpbmp));

    /* lag SW database setting */
    lag_group[lagid-1].enable = enable;
    PBMP_ASSIGN(lag_group[lagid-1].lpbmp, lpbmp);
    return SYS_OK;
}

/*!
 * \brief Get the lag group members.
 *
 * \param [in] lagid Lag ID number.
 * \param [out] enable
 *    \li TRUE = Lag group is enabled.
 *    \li FALSE = Lag group is disabled.
 * \param [out] uplist  Lag members list.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_lag_group_get(uint8 lagid, uint8 *enable, uint8 *uplist)
{
    if ((lagid == 0) || (lagid > BOARD_MAX_NUM_OF_LAG)) {
        return SYS_ERR_PARAMETER;
    }

    *enable = lag_group[lagid-1].enable;
    SOC_IF_ERROR_RETURN(board_lpbmp_to_uplist(0, lag_group[lagid-1].lpbmp, uplist));

    return SYS_OK;
}

/*!
 * \brief Get the max lag numbers.
 *
 * \param [out] num Max group numbers.
 *
 * \retval SYS_OK No errors.
 */
void
board_lag_group_max_num(uint8 *num)
{
    *num = BOARD_MAX_NUM_OF_LAG;
}

void
board_lag_linkchange(uint16 uport, BOOL link, void *arg)
{
    uint8 unit, lport;
    pbmp_t hw_lpbmp;
    uint8 i;

    PBMP_CLEAR(hw_lpbmp);

    if (lag_enable == TRUE) {
        board_uport_to_lport(uport, &unit, &lport);
        for (i = 1 ; i <= BOARD_MAX_NUM_OF_LAG ; i++) {
            if ((lag_group[i-1].enable == TRUE) &&
                (PBMP_MEMBER(lag_group[i-1].lpbmp, lport))) {
                /* add lag setting to HW */
                bcm5607x_lag_group_get(0, (i - 1), &hw_lpbmp);
                if (link) {
                    PBMP_PORT_ADD(hw_lpbmp, lport);
                } else {
                    PBMP_PORT_REMOVE(hw_lpbmp, lport);
                }
                bcm5607x_lag_group_set(0, (i - 1), hw_lpbmp);
            }
        }
    }
}
#endif /* CFG_SWITCH_LAG_INCLUDED */

#ifdef CFG_SWITCH_MIRROR_INCLUDED
/*!
 * \brief Set the mirror-to-port.
 *
 * \param [in] uport Port number.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_mirror_to_set(uint16 uport)
{

    uint8 unit, lport = 0;
    sys_error_t rv = SYS_OK;

    IM_MTP_INDEXm_t im_mtp_index;
    EM_MTP_INDEXm_t em_mtp_index;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    IM_MTP_INDEXm_CLR(im_mtp_index);
    IM_MTP_INDEXm_PORT_NUMf_SET(im_mtp_index, lport);
    rv = WRITE_IM_MTP_INDEXm(unit, 0, im_mtp_index);

    EM_MTP_INDEXm_CLR(em_mtp_index);
    EM_MTP_INDEXm_PORT_NUMf_SET(em_mtp_index, lport);
    rv |= WRITE_EM_MTP_INDEXm(unit, 0, em_mtp_index);

    return rv;
}

/*!
 * \brief Get the mirror-to-port.
 *
 * \param [out] uport Port number.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_mirror_to_get(uint16 *uport)
{
    uint8 lport;

    sys_error_t rv;

    EM_MTP_INDEXm_t em_mtp_index;

    rv = READ_EM_MTP_INDEXm(0, 0, em_mtp_index);
    if (rv != SYS_OK) {
        return rv;
    }
    lport = EM_MTP_INDEXm_PORT_NUMf_GET(em_mtp_index);

    return board_lport_to_uport(0, lport, uport);
}

/*!
 * \brief Set the port to be mirrored or disable.
 *
 * \param [in] uport - Port number.
 * \param [in] enable
 *    \li TRUE = Set the port to be mirrored.
 *    \li FALSE = Set the port not to be mirrored.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_mirror_port_set(uint16 uport, uint8 enable)
{

    int i;
    uint8 unit, lport;
    sys_error_t rv = SYS_OK;

    PORTm_t port;
    MIRROR_CONTROLr_t mirror_control;
    EMIRROR_CONTROL_LO_64r_t emirror_control_lo;
    EMIRROR_CONTROL_HI_64r_t emirror_control_hi;

    pbmp_t lpbmp;

    /* check M_ENABLE in PORT_TAB */
    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    /* enable == 0 */
    if (enable == 0) {
        READ_PORTm(unit, lport, port);
        PORTm_MIRROR0f_SET(port, 0);
        WRITE_PORTm(unit, lport, port);

        SOC_LPORT_ITER(i) {
           rv |= READ_EMIRROR_CONTROL_LO_64r(unit, i, emirror_control_lo);
           rv |= READ_EMIRROR_CONTROL_HI_64r(unit, i, emirror_control_hi);
           PBMP_CLEAR(lpbmp);
           EMIRROR_CONTROL_LO_64r_BITMAPf_GET(emirror_control_lo, SOC_PBMP(lpbmp));
           PBMP_WORD_SET(lpbmp, 2, EMIRROR_CONTROL_HI_64r_BITMAPf_GET(emirror_control_hi));
           PBMP_PORT_REMOVE(lpbmp, lport);
           if (PBMP_NOT_NULL(lpbmp)) {
               enable = 1;
           }
           EMIRROR_CONTROL_LO_64r_BITMAPf_SET(emirror_control_lo, SOC_PBMP(lpbmp));
           EMIRROR_CONTROL_HI_64r_BITMAPf_SET(emirror_control_hi, PBMP_WORD_GET(lpbmp, 2));

           rv |= WRITE_EMIRROR_CONTROL_LO_64r(unit, i, emirror_control_lo);
           rv |= WRITE_EMIRROR_CONTROL_HI_64r(unit, i, emirror_control_hi);

        }
        SOC_LPORT_ITER(i) {
            if (enable) {
                rv |= READ_MIRROR_CONTROLr(unit, i, mirror_control);
                MIRROR_CONTROLr_M_ENABLEf_SET(mirror_control, 1);
                rv |= WRITE_MIRROR_CONTROLr(unit, i, mirror_control);
            }
        }
     } else {
        /* enable != 0 */
        /* ingress enable */
        /* Only set Mirror 0 */
        READ_PORTm(unit, lport, port);
        PORTm_MIRROR0f_SET(port, 1);
        WRITE_PORTm(unit, lport, port);

        SOC_LPORT_ITER(i) {
            rv |= READ_MIRROR_CONTROLr(unit, i, mirror_control);
            MIRROR_CONTROLr_M_ENABLEf_SET(mirror_control, 1);
            rv |= WRITE_MIRROR_CONTROLr(unit, i, mirror_control);

            rv |= READ_EMIRROR_CONTROL_LO_64r(unit, i, emirror_control_lo);
            rv |= READ_EMIRROR_CONTROL_HI_64r(unit, i, emirror_control_hi);
            PBMP_CLEAR(lpbmp);
            EMIRROR_CONTROL_LO_64r_BITMAPf_GET(emirror_control_lo, SOC_PBMP(lpbmp));
            PBMP_WORD_SET(lpbmp, 2, EMIRROR_CONTROL_HI_64r_BITMAPf_GET(emirror_control_hi));
            PBMP_PORT_ADD(lpbmp, lport);
            EMIRROR_CONTROL_LO_64r_BITMAPf_SET(emirror_control_lo, SOC_PBMP(lpbmp));
            EMIRROR_CONTROL_HI_64r_BITMAPf_SET(emirror_control_hi, PBMP_WORD_GET(lpbmp, 2));

            rv |= WRITE_EMIRROR_CONTROL_LO_64r(unit, i, emirror_control_lo);
            rv |= WRITE_EMIRROR_CONTROL_HI_64r(unit, i, emirror_control_hi);
        }
    }

    return rv;
}

/*!
 * \brief Get if the port is set to be mirrored.
 *
 * \param [in] uport Port number.
 * \param [out] enable
 *    \li TRUE = The port is mirrored.
 *    \li FALSE = The port is not mirrored.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_mirror_port_get(uint16 uport, uint8 *enable)
{

    uint8 unit, lport = 0;
    sys_error_t rv = SYS_OK;
    PORTm_t port;
    /* check M_ENABLE in PORT_TAB */
    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    READ_PORTm(unit, lport, port);
    *enable = 0;
    if (PORTm_MIRROR0f_GET(port)) {
        *enable = 1;
    }

    return rv;
}
#endif /* CFG_SWITCH_MIRROR_INCLUDED */

#ifdef CFG_SWITCH_QOS_INCLUDED
/*!
 * \brief Set the QoS type.
 *
 * \param [in] type QoS type.
 *    \li QT_PORT_BASED
 *    \li QT_DOT1P_PRIORITY
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_qos_type_set(qos_type_t type)
{

    sys_error_t rv = SYS_OK;
    /*
     * Only support 802.1P for FL
     */
    type = QT_DOT1P_PRIORITY;
    qos_info = type;

    return rv;
}

/*!
 * \brief Get the QoS type.
 *
 * \param [out] type QoS type.
 *    \li QT_PORT_BASED
 *    \li QT_DOT1P_PRIORITY
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_qos_type_get(qos_type_t *type)
{
    *type = qos_info;
    return SYS_OK;
}

/*!
 * \brief Set the priority for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] priority Priority number.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_untagged_priority_set(uint16 uport, uint8 priority)
{
    /*
     * Not support for FL UM.
     */
    return SYS_ERR_UNAVAIL;
}

/*!
 * \brief Get the priority for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] priority Priority number.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_untagged_priority_get(uint16 uport, uint8 *priority)
{
    /*
     * Not support for FL
     */
    return SYS_ERR_UNAVAIL;
}
#endif /* CFG_SWITCH_QOS_INCLUDED */

#ifdef CFG_SWITCH_RATE_INCLUDED


/*!
 * \brief Set the ingress rate for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] bits_sec Rate
 *    \li 0: No limit
 *    \li 512000
 *    \li 1024000
 *    \li ...
 *    \li 524288000
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_rate_ingress_set(uint16 uport, uint32 bits_sec)
{

    uint8 unit, lport;
    uint32 kbits_sec = bits_sec/1000;

    FP_TCAMm_t fp_tcam;
    FP_METER_TABLEm_t fp_meter_table;
    FP_POLICY_TABLEm_t fp_policy_table;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    if (0 == bits_sec) {
        /* no limit */
        READ_FP_TCAMm(unit, RATE_IGR_IDX + lport - BCM5607X_LPORT_MIN, fp_tcam);
        FP_TCAMm_VALIDf_SET(fp_tcam, 0);
        WRITE_FP_TCAMm(unit, RATE_IGR_IDX + lport - BCM5607X_LPORT_MIN, fp_tcam);
        return SYS_OK;
    } else {
        READ_FP_TCAMm(unit, RATE_IGR_IDX + lport - BCM5607X_LPORT_MIN, fp_tcam);
        FP_TCAMm_VALIDf_SET(fp_tcam, 3);
        WRITE_FP_TCAMm(unit, RATE_IGR_IDX + lport - BCM5607X_LPORT_MIN, fp_tcam);
    }

    FP_METER_TABLEm_CLR(fp_meter_table);

    LOG_RATE(BSL_LS_SOC_COMMON,("%s..:uport=%d lport=%d bits_sec=%d kbps\n", __func__, uport, lport, bits_sec));

    switch(kbits_sec) {
        case 512:
            FP_METER_TABLEm_REFRESHCOUNTf_SET(fp_meter_table, 0x40);
            FP_METER_TABLEm_BUCKETCOUNTf_SET(fp_meter_table, 0x7cffff);
            FP_METER_TABLEm_BUCKETSIZEf_SET(fp_meter_table, 0x7d);
            break;
        case 1024:
            FP_METER_TABLEm_REFRESHCOUNTf_SET(fp_meter_table, 0x80);
            FP_METER_TABLEm_BUCKETCOUNTf_SET(fp_meter_table, 0xf9ffff);
            FP_METER_TABLEm_BUCKETSIZEf_SET(fp_meter_table, 0xFA);
            break;
        case 2048:
            FP_METER_TABLEm_REFRESHCOUNTf_SET(fp_meter_table, 0x100);
            FP_METER_TABLEm_BUCKETCOUNTf_SET(fp_meter_table, 0x01f3ffff);
            FP_METER_TABLEm_BUCKETSIZEf_SET(fp_meter_table, 0x1f4);
            break;
        case 4096:
            FP_METER_TABLEm_REFRESHCOUNTf_SET(fp_meter_table, 0x200);
            FP_METER_TABLEm_BUCKETCOUNTf_SET(fp_meter_table, 0x03e7ffff);
            FP_METER_TABLEm_BUCKETSIZEf_SET(fp_meter_table, 0x3e8);
            break;
        case 8192:
            FP_METER_TABLEm_REFRESHCOUNTf_SET(fp_meter_table, 0x400);
            FP_METER_TABLEm_BUCKETCOUNTf_SET(fp_meter_table, 0x07cfffff);
            FP_METER_TABLEm_BUCKETSIZEf_SET(fp_meter_table, 0x7d0);
            break;
        case 16384:
            FP_METER_TABLEm_REFRESHCOUNTf_SET(fp_meter_table, 0x800);
            FP_METER_TABLEm_BUCKETCOUNTf_SET(fp_meter_table, 0x0f9fffff);
            FP_METER_TABLEm_BUCKETSIZEf_SET(fp_meter_table, 0xfa0);
            break;
        case 32768:
            FP_METER_TABLEm_REFRESHCOUNTf_SET(fp_meter_table, 0x800);
            FP_METER_TABLEm_BUCKETCOUNTf_SET(fp_meter_table, 0x0f9fffff);
            FP_METER_TABLEm_BUCKETSIZEf_SET(fp_meter_table, 0xfa0);
            FP_METER_TABLEm_METER_GRANf_SET(fp_meter_table,1);
            break;
        case 65536:
            FP_METER_TABLEm_REFRESHCOUNTf_SET(fp_meter_table, 0x800);
            FP_METER_TABLEm_BUCKETCOUNTf_SET(fp_meter_table, 0x0f9fffff);
            FP_METER_TABLEm_BUCKETSIZEf_SET(fp_meter_table, 0xfa0);
            FP_METER_TABLEm_METER_GRANf_SET(fp_meter_table, 2);
            break;
        case 131072:
            FP_METER_TABLEm_REFRESHCOUNTf_SET(fp_meter_table, 0x800);
            FP_METER_TABLEm_BUCKETCOUNTf_SET(fp_meter_table, 0x0f9fffff);
            FP_METER_TABLEm_BUCKETSIZEf_SET(fp_meter_table, 0xfa0);
            FP_METER_TABLEm_METER_GRANf_SET(fp_meter_table, 3);
            break;
        case 262144:
            FP_METER_TABLEm_REFRESHCOUNTf_SET(fp_meter_table, 0x800);
            FP_METER_TABLEm_BUCKETCOUNTf_SET(fp_meter_table, 0x0f9fffff);
            FP_METER_TABLEm_BUCKETSIZEf_SET(fp_meter_table, 0xfa0);
            FP_METER_TABLEm_METER_GRANf_SET(fp_meter_table, 4);
            break;
        case 524288:
            FP_METER_TABLEm_REFRESHCOUNTf_SET(fp_meter_table, 0x800);
            FP_METER_TABLEm_BUCKETCOUNTf_SET(fp_meter_table, 0x0f9fffff);
            FP_METER_TABLEm_BUCKETSIZEf_SET(fp_meter_table, 0xfa0);
            FP_METER_TABLEm_METER_GRANf_SET(fp_meter_table, 5);
            break;
        default:
            break;
    }

    READ_FP_POLICY_TABLEm(unit, RATE_IGR_IDX + lport - BCM5607X_LPORT_MIN, fp_policy_table);

    WRITE_FP_METER_TABLEm(unit, 2 * FP_POLICY_TABLEm_METER_PAIR_INDEXf_GET(fp_policy_table)
                                + FP_POLICY_TABLEm_METER_PAIR_MODE_MODIFIERf_GET(fp_policy_table),
                          fp_meter_table);

    LOG_RATE(BSL_LS_SOC_COMMON,("%s..:FP_METER_TABLE..:index=%d\n", __func__, 2 * FP_POLICY_TABLEm_METER_PAIR_INDEXf_GET(fp_policy_table)
                                + FP_POLICY_TABLEm_METER_PAIR_MODE_MODIFIERf_GET(fp_policy_table) ) );

    return SYS_OK;
}

/*!
 * \brief Get the rate ingress setting for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] bits_sec Rate
 *    \li 0: No limit
 *    \li Rate value
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_rate_ingress_get(uint16 uport, uint32 *bits_sec)
{
    uint8 unit, lport;

    FP_TCAMm_t fp_tcam;
    FP_POLICY_TABLEm_t fp_policy_table;
    FP_METER_TABLEm_t fp_meter_table;

    *bits_sec = 0x0;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    READ_FP_TCAMm(unit, RATE_IGR_IDX + lport - BCM5607X_LPORT_MIN, fp_tcam);

    if (FP_TCAMm_VALIDf_GET(fp_tcam) == 0)
    {
        *bits_sec = 0;
        return SYS_OK;
    }

    SOC_IF_ERROR_RETURN(READ_FP_POLICY_TABLEm(unit, RATE_IGR_IDX + lport - BCM5607X_LPORT_MIN, fp_policy_table));


    SOC_IF_ERROR_RETURN(READ_FP_METER_TABLEm(unit, 2 * FP_POLICY_TABLEm_METER_PAIR_INDEXf_GET(fp_policy_table) +
                                             FP_POLICY_TABLEm_METER_PAIR_MODE_MODIFIERf_GET(fp_policy_table), fp_meter_table));

    switch(FP_METER_TABLEm_REFRESHCOUNTf_GET(fp_meter_table)) {
        case 0x40:
            *bits_sec = 512 * 1000;
            break;
        case 0x80:
            *bits_sec = 1024 * 1000;
            break;
        case 0x100:
            *bits_sec = 2048 * 1000;
            break;
        case 0x200:
            *bits_sec = 4096 * 1000;
            break;
        case 0x400:
            *bits_sec = 8192 * 1000;
            break;
        case 0x800:
            *bits_sec = 16384 * 1000 * (1 <<FP_METER_TABLEm_METER_GRANf_GET(fp_meter_table));
            break;
        default:
            break;
    }

    return SYS_OK;
}


/*!
 * \brief Set the egress rate for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] bits_sec Rate
 *    \li 0: No limit
 *    \li 512000
 *    \li 1024000
 *    \li ...
 *    \li 524288000
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_rate_egress_set(uint16 uport, uint32 bits_sec)
{

    uint8 unit, lport;
    uint32 kbits_sec = bits_sec/1000;
    int phy_port;
    int mmu_port;

    EGRMETERINGCONFIGr_t egrmeteringconfig;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));
    phy_port = SOC_PORT_L2P_MAPPING(lport);
    mmu_port = SOC_PORT_P2M_MAPPING(phy_port);

    EGRMETERINGCONFIGr_CLR(egrmeteringconfig);
    SOC_IF_ERROR_RETURN(WRITE_EGRMETERINGCONFIGr(unit, mmu_port, egrmeteringconfig));
    LOG_RATE(BSL_LS_SOC_COMMON,("%s..:uport=%d lport=%d mmu_port=%d bits_sec=%d kbps\n", __func__, uport, lport, mmu_port, bits_sec));

    switch(kbits_sec) {
        case 512:
            EGRMETERINGCONFIGr_REFRESHf_SET(egrmeteringconfig, 0x8);
            EGRMETERINGCONFIGr_THD_SELf_SET(egrmeteringconfig, 0x10);
            break;
        case 1024:
            EGRMETERINGCONFIGr_REFRESHf_SET(egrmeteringconfig, 0x10);
            EGRMETERINGCONFIGr_THD_SELf_SET(egrmeteringconfig, 0x20);
            break;
        case 2048:
            EGRMETERINGCONFIGr_REFRESHf_SET(egrmeteringconfig, 0x20);
            EGRMETERINGCONFIGr_THD_SELf_SET(egrmeteringconfig, 0x3F);
            break;
        case 4096:
            EGRMETERINGCONFIGr_REFRESHf_SET(egrmeteringconfig, 0x40);
            EGRMETERINGCONFIGr_THD_SELf_SET(egrmeteringconfig, 0x7D);
            break;
        case 8192:
            EGRMETERINGCONFIGr_REFRESHf_SET(egrmeteringconfig, 0x80);
            EGRMETERINGCONFIGr_THD_SELf_SET(egrmeteringconfig, 0xFA);
            break;
        case 16384:
            EGRMETERINGCONFIGr_REFRESHf_SET(egrmeteringconfig, 0x100);
            EGRMETERINGCONFIGr_THD_SELf_SET(egrmeteringconfig, 0x1F4);
            break;
        case 32768:
            EGRMETERINGCONFIGr_REFRESHf_SET(egrmeteringconfig, 0x200);
            EGRMETERINGCONFIGr_THD_SELf_SET(egrmeteringconfig, 0x3E8);
            break;
        case 65536:
            EGRMETERINGCONFIGr_REFRESHf_SET(egrmeteringconfig, 0x400);
            EGRMETERINGCONFIGr_THD_SELf_SET(egrmeteringconfig, 0x7D0);
            break;
        case 131072:
            EGRMETERINGCONFIGr_REFRESHf_SET(egrmeteringconfig, 0x800);
            EGRMETERINGCONFIGr_THD_SELf_SET(egrmeteringconfig, 0xFA0);
            break;
        case 262144:
            EGRMETERINGCONFIGr_REFRESHf_SET(egrmeteringconfig, 0x1000);
            EGRMETERINGCONFIGr_THD_SELf_SET(egrmeteringconfig, 0xFFF);
            break;
        case 524288:
            EGRMETERINGCONFIGr_REFRESHf_SET(egrmeteringconfig, 0x2000);
            EGRMETERINGCONFIGr_THD_SELf_SET(egrmeteringconfig, 0xFFF);
            break;
       default:
            break;
    }

    LOG_RATE(BSL_LS_SOC_COMMON,("%s..:uport=%d lport=%d mmu_port=%d bits_sec=%d kbps egrmeteringconfig=0x%08x\n", __func__, uport, lport, mmu_port, bits_sec, egrmeteringconfig.v[0]));
    SOC_IF_ERROR_RETURN(WRITE_EGRMETERINGCONFIGr(unit, mmu_port, egrmeteringconfig));

    return SYS_OK;
}

/*!
 * \brief Get the rate egress setting for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] bits_sec Rate
 *    \li 0: No limit
 *    \li Rate value
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_rate_egress_get(uint16 uport, uint32 *bits_sec)
{

    uint8 unit, lport;
    EGRMETERINGCONFIGr_t egrmeteringconfig;
    int phy_port;
    int mmu_port;


    *bits_sec  = 0;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));
    phy_port = SOC_PORT_L2P_MAPPING(lport);
    mmu_port = SOC_PORT_P2M_MAPPING(phy_port);

    SOC_IF_ERROR_RETURN(READ_EGRMETERINGCONFIGr(unit, mmu_port, egrmeteringconfig));


    if ((EGRMETERINGCONFIGr_REFRESHf_GET(egrmeteringconfig) == 0x8) &&
        (EGRMETERINGCONFIGr_THD_SELf_GET(egrmeteringconfig) == 0x10))
    {
        *bits_sec = 512 * 1000;
    } else if ((EGRMETERINGCONFIGr_REFRESHf_GET(egrmeteringconfig) == 0x10) &&
        (EGRMETERINGCONFIGr_THD_SELf_GET(egrmeteringconfig) == 0x20))
    {
        *bits_sec = 1024 * 1000;

    } else if ((EGRMETERINGCONFIGr_REFRESHf_GET(egrmeteringconfig) == 0x20) &&
        (EGRMETERINGCONFIGr_THD_SELf_GET(egrmeteringconfig) == 0x3F))
    {
        *bits_sec = 2048 * 1000;
    } else if ((EGRMETERINGCONFIGr_REFRESHf_GET(egrmeteringconfig) == 0x40) &&
        (EGRMETERINGCONFIGr_THD_SELf_GET(egrmeteringconfig) == 0x7D))
    {
        *bits_sec = 4096 * 1000;

    } else if ((EGRMETERINGCONFIGr_REFRESHf_GET(egrmeteringconfig) == 0x80) &&
        (EGRMETERINGCONFIGr_THD_SELf_GET(egrmeteringconfig) == 0xFA))
    {
        *bits_sec = 8192 * 1000;
    } else if ((EGRMETERINGCONFIGr_REFRESHf_GET(egrmeteringconfig) == 0x100) &&
        (EGRMETERINGCONFIGr_THD_SELf_GET(egrmeteringconfig) == 0x1F4))
    {
        *bits_sec = 16384 * 1000;
    } else if ((EGRMETERINGCONFIGr_REFRESHf_GET(egrmeteringconfig) == 0x200) &&
        (EGRMETERINGCONFIGr_THD_SELf_GET(egrmeteringconfig) == 0x3E8))
    {
        *bits_sec = 32768 * 1000;
    } else if ((EGRMETERINGCONFIGr_REFRESHf_GET(egrmeteringconfig) == 0x400) &&
          (EGRMETERINGCONFIGr_THD_SELf_GET(egrmeteringconfig) == 0x7D0))
    {
        *bits_sec = 65536 * 1000;
    } else if ((EGRMETERINGCONFIGr_REFRESHf_GET(egrmeteringconfig) == 0x800) &&
          (EGRMETERINGCONFIGr_THD_SELf_GET(egrmeteringconfig) == 0xFA0))
    {
        *bits_sec = 131072 * 1000;
    } else if ((EGRMETERINGCONFIGr_REFRESHf_GET(egrmeteringconfig) == 0x1000) &&
          (EGRMETERINGCONFIGr_THD_SELf_GET(egrmeteringconfig) == 0xFFF))
    {
        *bits_sec = 262144 * 1000;
    } else if ((EGRMETERINGCONFIGr_REFRESHf_GET(egrmeteringconfig) == 0x2000) &&
          (EGRMETERINGCONFIGr_THD_SELf_GET(egrmeteringconfig) == 0xFFF))
    {
        *bits_sec = 524288 * 1000;
    }

    return SYS_OK;
}

/*!
 * \brief Set the storm control type.
 *
 * \param [in] flags
 *    \li STORM_RATE_NONE
 *    \li STORM_RATE_BCAST
 *    \li STORM_RATE_MCAST
 *    \li STORM_RATE_DLF
 *    \li STORM_RATE_ALL
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_rate_type_set(uint8 flags)
{
    uint16 uport;

    if (flags == STORM_RATE_NONE && storm_info != STORM_RATE_NONE) {
        /* Clear all current settings */
        SAL_UPORT_ITER(uport) {
            board_rate_set(uport, 0);
        }
    }
    storm_info = flags;
    return SYS_OK;
}

/*!
 * \brief Get the storm control type.
 *
 * \param [out] flags
 *    \li STORM_RATE_NONE
 *    \li STORM_RATE_BCAST
 *    \li STORM_RATE_MCAST
 *    \li STORM_RATE_DLF
 *    \li STORM_RATE_ALL
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_rate_type_get(uint8 *flags)
{
    *flags = storm_info;
    return SYS_OK;
}

/*!
 * \brief Set the storm control setting rate for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] bits_sec Rate
 *    \li 0: No limit
 *    \li Rate value
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_rate_set(uint16 uport, uint32 bits_sec)
{
    sys_error_t rv = SYS_OK;
    uint8 unit, lport;

    uint32 kbits_sec = bits_sec/1000;
    int i;

    STORM_CONTROL_METER_CONFIGr_t storm_control_meter_config;
    FP_STORM_CONTROL_METERSm_t fp_storm_control_meter;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));


    if (!bits_sec) {
        /* metering is disabled on port - program tables to max values */
        /* Need to set bit-mode */
        STORM_CONTROL_METER_CONFIGr_CLR(storm_control_meter_config);
        STORM_CONTROL_METER_CONFIGr_BYTE_MODEf_SET(storm_control_meter_config, 1);
        WRITE_STORM_CONTROL_METER_CONFIGr(0, lport, storm_control_meter_config);

        FP_STORM_CONTROL_METERSm_CLR(fp_storm_control_meter);
        FP_STORM_CONTROL_METERSm_BUCKETCOUNTf_SET(fp_storm_control_meter, 0);
        FP_STORM_CONTROL_METERSm_BUCKETSIZEf_SET(fp_storm_control_meter, 4);
        FP_STORM_CONTROL_METERSm_REFRESHCOUNTf_SET(fp_storm_control_meter, 0x3ffff);
        for (i = 0; i < 4; i++) {
            rv |= WRITE_FP_STORM_CONTROL_METERSm(0, (lport * 4 + i) ,fp_storm_control_meter);
        }
        /* no limit */
        return SYS_OK;
    }

    FP_STORM_CONTROL_METERSm_CLR(fp_storm_control_meter);

    switch(kbits_sec) {
        case 512:
            /* 512K */
            FP_STORM_CONTROL_METERSm_BUCKETCOUNTf_SET(fp_storm_control_meter, 0x100000);
            FP_STORM_CONTROL_METERSm_BUCKETSIZEf_SET(fp_storm_control_meter, 2);
            FP_STORM_CONTROL_METERSm_REFRESHCOUNTf_SET(fp_storm_control_meter, 0x8);
            break;
        case 1024:
            FP_STORM_CONTROL_METERSm_BUCKETCOUNTf_SET(fp_storm_control_meter, 0x400000);
            FP_STORM_CONTROL_METERSm_BUCKETSIZEf_SET(fp_storm_control_meter, 3);
            FP_STORM_CONTROL_METERSm_REFRESHCOUNTf_SET(fp_storm_control_meter, 0x10);
            break;
        case 2048:
            FP_STORM_CONTROL_METERSm_BUCKETCOUNTf_SET(fp_storm_control_meter, 0x400000);
            FP_STORM_CONTROL_METERSm_BUCKETSIZEf_SET(fp_storm_control_meter, 3);
            FP_STORM_CONTROL_METERSm_REFRESHCOUNTf_SET(fp_storm_control_meter, 0x20);
            break;
        case 4096:
            FP_STORM_CONTROL_METERSm_BUCKETCOUNTf_SET(fp_storm_control_meter, 0x1000000);
            FP_STORM_CONTROL_METERSm_BUCKETSIZEf_SET(fp_storm_control_meter, 4);
            FP_STORM_CONTROL_METERSm_REFRESHCOUNTf_SET(fp_storm_control_meter, 0x40);
            break;
        case 8192:
            FP_STORM_CONTROL_METERSm_BUCKETCOUNTf_SET(fp_storm_control_meter, 0x1000000);
            FP_STORM_CONTROL_METERSm_BUCKETSIZEf_SET(fp_storm_control_meter, 4);
            FP_STORM_CONTROL_METERSm_REFRESHCOUNTf_SET(fp_storm_control_meter, 0x80);
            break;
        case 16384:
            FP_STORM_CONTROL_METERSm_BUCKETCOUNTf_SET(fp_storm_control_meter, 0x4000000);
            FP_STORM_CONTROL_METERSm_BUCKETSIZEf_SET(fp_storm_control_meter, 5);
            FP_STORM_CONTROL_METERSm_REFRESHCOUNTf_SET(fp_storm_control_meter, 0x100);
            break;
        case 32768:
            FP_STORM_CONTROL_METERSm_BUCKETCOUNTf_SET(fp_storm_control_meter, 0x4000000);
            FP_STORM_CONTROL_METERSm_BUCKETSIZEf_SET(fp_storm_control_meter, 5);
            FP_STORM_CONTROL_METERSm_REFRESHCOUNTf_SET(fp_storm_control_meter, 0x200);
            break;
        case 65536:
            FP_STORM_CONTROL_METERSm_BUCKETCOUNTf_SET(fp_storm_control_meter, 0x8000000);
            FP_STORM_CONTROL_METERSm_BUCKETSIZEf_SET(fp_storm_control_meter, 6);
            FP_STORM_CONTROL_METERSm_REFRESHCOUNTf_SET(fp_storm_control_meter, 0x400);
            break;
        case 131072:
            FP_STORM_CONTROL_METERSm_BUCKETCOUNTf_SET(fp_storm_control_meter, 0x10000000);
            FP_STORM_CONTROL_METERSm_BUCKETSIZEf_SET(fp_storm_control_meter, 7);
            FP_STORM_CONTROL_METERSm_REFRESHCOUNTf_SET(fp_storm_control_meter, 0x800);
            break;
        case 262144:
            FP_STORM_CONTROL_METERSm_BUCKETCOUNTf_SET(fp_storm_control_meter, 0x10000000);
            FP_STORM_CONTROL_METERSm_BUCKETSIZEf_SET(fp_storm_control_meter, 7);
            FP_STORM_CONTROL_METERSm_REFRESHCOUNTf_SET(fp_storm_control_meter, 0x1000);
            break;
        case 524288:
            FP_STORM_CONTROL_METERSm_BUCKETCOUNTf_SET(fp_storm_control_meter, 0x10000000);
            FP_STORM_CONTROL_METERSm_BUCKETSIZEf_SET(fp_storm_control_meter, 7);
            FP_STORM_CONTROL_METERSm_REFRESHCOUNTf_SET(fp_storm_control_meter, 0x2000);
            break;
       default:
            break;
    }

    /* enable bcast in STORM_CONTROL_METER_CONFIG*/
    STORM_CONTROL_METER_CONFIGr_CLR(storm_control_meter_config);
    STORM_CONTROL_METER_CONFIGr_UNKNOWN_L2MC_ENABLEf_SET(storm_control_meter_config, 1);
    STORM_CONTROL_METER_CONFIGr_UNKNOWN_IPMC_ENABLEf_SET(storm_control_meter_config, 1);
    STORM_CONTROL_METER_CONFIGr_KNOWN_L2MC_ENABLEf_SET(storm_control_meter_config, 1);
    STORM_CONTROL_METER_CONFIGr_KNOWN_IPMC_ENABLEf_SET(storm_control_meter_config, 1);
    STORM_CONTROL_METER_CONFIGr_DLFBC_ENABLEf_SET(storm_control_meter_config, 1);
    STORM_CONTROL_METER_CONFIGr_BYTE_MODEf_SET(storm_control_meter_config, 1);
    STORM_CONTROL_METER_CONFIGr_BCAST_ENABLEf_SET(storm_control_meter_config, 1);
    WRITE_STORM_CONTROL_METER_CONFIGr(unit, lport, storm_control_meter_config);

    for (i = 0; i < 4; i++) {
        rv |= WRITE_FP_STORM_CONTROL_METERSm(unit, (lport * 4) + i, fp_storm_control_meter);
    }

    return rv;
}

/*!
 * \brief Get the storm control setting rate for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] bits_sec Rate
 *    \li 0: No limit
 *    \li Rate value
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_rate_get(uint16 uport, uint32 *bits_sec)
{

    uint8 unit, lport;

    STORM_CONTROL_METER_CONFIGr_t storm_control_meter_config;
    FP_STORM_CONTROL_METERSm_t fp_storm_control_meter;

    *bits_sec = 0;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    SOC_IF_ERROR_RETURN(READ_STORM_CONTROL_METER_CONFIGr(unit, lport, storm_control_meter_config));

    if ((STORM_CONTROL_METER_CONFIGr_UNKNOWN_L2MC_ENABLEf_GET(storm_control_meter_config) == 0) &&
        (STORM_CONTROL_METER_CONFIGr_UNKNOWN_IPMC_ENABLEf_GET(storm_control_meter_config) == 0) &&
        (STORM_CONTROL_METER_CONFIGr_KNOWN_L2MC_ENABLEf_GET(storm_control_meter_config) == 0) &&
        (STORM_CONTROL_METER_CONFIGr_KNOWN_IPMC_ENABLEf_GET(storm_control_meter_config) == 0) &&
        (STORM_CONTROL_METER_CONFIGr_DLFBC_ENABLEf_GET(storm_control_meter_config) == 0) &&
        /* (STORM_CONTROL_METER_CONFIGr_BYTE_MODEf_GET(storm_control_meter_config) == 0) &&    */
        (STORM_CONTROL_METER_CONFIGr_BCAST_ENABLEf_GET(storm_control_meter_config) == 0)
       ) {
        /*disable, no limit */
        *bits_sec = 0;
        return SYS_OK;
    }

    READ_FP_STORM_CONTROL_METERSm(unit, (lport * 4), fp_storm_control_meter);

    *bits_sec = (FP_STORM_CONTROL_METERSm_REFRESHCOUNTf_GET(fp_storm_control_meter) << 6) * 1000;

    return SYS_OK;
}
#endif /* CFG_SWITCH_RATE_INCLUDED */

#ifdef CFG_SWITCH_STAT_INCLUDED

/*!
 * \brief Get the statistic value for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] stat Statistics value.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_stat_get(uint16 uport, port_stat_t *stat)
{
    sys_error_t rv = SYS_OK;
    uint8 unit, lport;

    GTBYTr_t gtbyt;
    GRBYTr_t grbyt;
    GTPKTr_t gtpkt;
    GRPKTr_t grpkt;
    GRX_EEE_LPI_EVENT_COUNTERr_t grx_eee_lpi_event_counter;
    GRX_EEE_LPI_DURATION_COUNTERr_t grx_eee_lpi_duration_counter;
    GTX_EEE_LPI_EVENT_COUNTERr_t gtx_eee_lpi_event_counter;
    GTX_EEE_LPI_DURATION_COUNTERr_t gtx_eee_lpi_duration_counter;
    GRUCr_t gruc;
    GTUCr_t gtuc;
    GRMCAr_t grmca;
    GTMCAr_t gtmca;
    GRBCAr_t grbca;
    GTBCAr_t gtbca;
    GRXPFr_t grxpf;
    GTXPFr_t gtxpf;
    GROVRr_t grovr;
    GTOVRr_t gtovr;
    GRFCSr_t grfcs;

    XLMIB_TBYTr_t xl_tbyt;
    XLMIB_RBYTr_t xl_rbyt;
    XLMIB_TPKTr_t xl_tpkt;
    XLMIB_RPKTr_t xl_rpkt;
    XLMIB_RX_EEE_LPI_EVENT_COUNTERr_t xl_rx_eee_lpi_event_counter;
    XLMIB_RX_EEE_LPI_DURATION_COUNTERr_t xl_rx_eee_lpi_duration_counter;
    XLMIB_TX_EEE_LPI_EVENT_COUNTERr_t xl_tx_eee_lpi_event_counter;
    XLMIB_TX_EEE_LPI_DURATION_COUNTERr_t xl_tx_eee_lpi_duration_counter;
    XLMIB_RUCAr_t xl_ruca;
    XLMIB_TUCAr_t xl_tuca;
    XLMIB_RMCAr_t xl_rmca;
    XLMIB_TMCAr_t xl_tmca;
    XLMIB_RBCAr_t xl_rbca;
    XLMIB_TBCAr_t xl_tbca;
    XLMIB_RXPFr_t xl_rxpf;
    XLMIB_TXPFr_t xl_txpf;
    XLMIB_ROVRr_t xl_rovr;
    XLMIB_TOVRr_t xl_tovr;
    XLMIB_RFCSr_t xl_rfcs;

    CLMIB_TBYTr_t cl_tbyt;
    CLMIB_RBYTr_t cl_rbyt;
    CLMIB_TPKTr_t cl_tpkt;
    CLMIB_RPKTr_t cl_rpkt;
    CLMIB_RX_EEE_LPI_EVENT_COUNTERr_t cl_rx_eee_lpi_event_counter;
    CLMIB_RX_EEE_LPI_DURATION_COUNTERr_t cl_rx_eee_lpi_duration_counter;
    CLMIB_TX_EEE_LPI_EVENT_COUNTERr_t cl_tx_eee_lpi_event_counter;
    CLMIB_TX_EEE_LPI_DURATION_COUNTERr_t cl_tx_eee_lpi_duration_counter;
    CLMIB_RUCAr_t cl_ruca;
    CLMIB_TUCAr_t cl_tuca;
    CLMIB_RMCAr_t cl_rmca;
    CLMIB_TMCAr_t cl_tmca;
    CLMIB_RBCAr_t cl_rbca;
    CLMIB_TBCAr_t cl_tbca;
    CLMIB_RXPFr_t cl_rxpf;
    CLMIB_TXPFr_t cl_txpf;
    CLMIB_ROVRr_t cl_rovr;
    CLMIB_TOVRr_t cl_tovr;
    CLMIB_RFCSr_t cl_rfcs;

    sal_memset(stat, 0, sizeof(port_stat_t));

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    if (IS_GX_PORT(lport)) {
        /* Byte Counter */
        rv |= READ_GTBYTr(unit, lport, gtbyt);
        stat->TxOctets_lo = GTBYTr_GET(gtbyt);
        rv |= READ_GRBYTr(unit, lport, grbyt);
        stat->RxOctets_lo = GRBYTr_GET(grbyt);
        rv |= READ_GTPKTr(unit,lport, gtpkt);
        stat->TxPkts_lo = GTPKTr_GET(gtpkt);
        rv |= READ_GRPKTr(unit,lport, grpkt);
        stat->RxPkts_lo = GRPKTr_GET(grpkt);

        /* Rx FCS Error Frame Counter */
        rv |= READ_GRFCSr(unit, lport, grfcs);
        stat->CRCErrors_lo = GRFCSr_GET(grfcs);

        /* EEE LPI counter */
        rv |= READ_GRX_EEE_LPI_EVENT_COUNTERr(unit, lport, grx_eee_lpi_event_counter);
        stat->RxLPIPkts_lo = GRX_EEE_LPI_EVENT_COUNTERr_GET(grx_eee_lpi_event_counter);
        rv |= READ_GRX_EEE_LPI_DURATION_COUNTERr(unit, lport, grx_eee_lpi_duration_counter);
        stat->RxLPIDuration_lo = GRX_EEE_LPI_DURATION_COUNTERr_GET(grx_eee_lpi_duration_counter);
        rv |= READ_GTX_EEE_LPI_EVENT_COUNTERr(unit, lport, gtx_eee_lpi_event_counter);
        stat->TxLPIPkts_lo = GTX_EEE_LPI_EVENT_COUNTERr_GET(gtx_eee_lpi_event_counter);
        rv |= READ_GTX_EEE_LPI_DURATION_COUNTERr(unit, lport, gtx_eee_lpi_duration_counter);
        stat->TxLPIDuration_lo = GTX_EEE_LPI_DURATION_COUNTERr_GET(gtx_eee_lpi_duration_counter);

        /* Unicast Frame Counter */
        rv |= READ_GRUCr(unit, lport, gruc);
        stat->RxUnicastPkts_lo = GRUCr_GET(gruc);
        rv |= READ_GTUCr(unit, lport, gtuc);
        stat->TxUnicastPkts_lo = GTUCr_GET(gtuc);

        /* Multicast Frame Counter */
        rv |= READ_GRMCAr(unit, lport, grmca);
        stat->RxMulticastPkts_lo = GRMCAr_GET(grmca);
        rv |= READ_GTMCAr(unit, lport, gtmca);
        stat->TxMulticastPkts_lo = GTMCAr_GET(gtmca);

        /* Broadcast Frame Counter */
        rv |= READ_GRBCAr(unit, lport, grbca);
        stat->RxBroadcastPkts_lo = GRBCAr_GET(grbca);
        rv |= READ_GTBCAr(unit, lport, gtbca);
        stat->TxBroadcastPkts_lo = GTBCAr_GET(gtbca);

        /* Pause Frame Counter */
        rv |= READ_GRXPFr(unit, lport, grxpf);
        stat->RxPauseFramePkts_lo = GRXPFr_GET(grxpf);
        rv |= READ_GTXPFr(unit, lport, gtxpf);
        stat->TxPauseFramePkts_lo = GTXPFr_GET(gtxpf);

        /* Oversized Frame Counter */
        rv |= READ_GROVRr(unit, lport, grovr);
        stat->RxOversizePkts_lo = GROVRr_GET(grovr);
        rv |= READ_GTOVRr(unit, lport, gtovr);
        stat->TxOversizePkts_lo = GTOVRr_GET(gtovr);
    } else if (IS_XL_PORT(lport)) {
        /* Byte Counter */
        rv |= READ_XLMIB_TBYTr(unit, lport, xl_tbyt);
        stat->TxOctets_lo = XLMIB_TBYTr_GET(xl_tbyt, 0);
        stat->TxOctets_hi = XLMIB_TBYTr_GET(xl_tbyt, 1);
        rv |= READ_XLMIB_RBYTr(unit, lport, xl_rbyt);
        stat->RxOctets_lo = XLMIB_RBYTr_GET(xl_rbyt, 0);
        stat->RxOctets_hi = XLMIB_RBYTr_GET(xl_rbyt, 1);
        rv |= READ_XLMIB_TPKTr(unit, lport, xl_tpkt);
        stat->TxPkts_lo = XLMIB_TPKTr_GET(xl_tpkt, 0);
        stat->TxPkts_hi = XLMIB_TPKTr_GET(xl_tpkt, 1);
        rv |= READ_XLMIB_RPKTr(unit, lport, xl_rpkt);
        stat->RxPkts_lo = XLMIB_RPKTr_GET(xl_rpkt, 0);
        stat->RxPkts_hi = XLMIB_RPKTr_GET(xl_rpkt, 1);

        /* Rx FCS Error Frame Counter */
        rv |= READ_XLMIB_RFCSr(unit, lport, xl_rfcs);
        stat->CRCErrors_lo = XLMIB_RFCSr_GET(xl_rfcs, 0);
        stat->CRCErrors_hi = XLMIB_RFCSr_GET(xl_rfcs, 1);

        /* EEE LPI counter */
        rv |= READ_XLMIB_RX_EEE_LPI_EVENT_COUNTERr(unit, lport, xl_rx_eee_lpi_event_counter);
        stat->RxLPIPkts_lo = XLMIB_RX_EEE_LPI_EVENT_COUNTERr_GET(xl_rx_eee_lpi_event_counter, 0);
        stat->RxLPIPkts_hi = XLMIB_RX_EEE_LPI_EVENT_COUNTERr_GET(xl_rx_eee_lpi_event_counter, 1);
        rv |= READ_XLMIB_RX_EEE_LPI_DURATION_COUNTERr(unit, lport, xl_rx_eee_lpi_duration_counter);
        stat->RxLPIDuration_lo = XLMIB_RX_EEE_LPI_DURATION_COUNTERr_GET(xl_rx_eee_lpi_duration_counter, 0);
        stat->RxLPIDuration_hi = XLMIB_RX_EEE_LPI_DURATION_COUNTERr_GET(xl_rx_eee_lpi_duration_counter, 1);
        rv |= READ_XLMIB_TX_EEE_LPI_EVENT_COUNTERr(unit, lport, xl_tx_eee_lpi_event_counter);
        stat->TxLPIPkts_lo = XLMIB_TX_EEE_LPI_EVENT_COUNTERr_GET(xl_tx_eee_lpi_event_counter, 0);
        stat->TxLPIPkts_hi = XLMIB_TX_EEE_LPI_EVENT_COUNTERr_GET(xl_tx_eee_lpi_event_counter, 1);
        rv |= READ_XLMIB_TX_EEE_LPI_DURATION_COUNTERr(unit, lport, xl_tx_eee_lpi_duration_counter);
        stat->TxLPIDuration_lo = XLMIB_TX_EEE_LPI_DURATION_COUNTERr_GET(xl_tx_eee_lpi_duration_counter, 0);
        stat->TxLPIDuration_hi = XLMIB_TX_EEE_LPI_DURATION_COUNTERr_GET(xl_tx_eee_lpi_duration_counter, 1);

        /* Unicast Frame Counter */
        rv |= READ_XLMIB_RUCAr(unit, lport, xl_ruca);
        stat->RxUnicastPkts_lo = XLMIB_RUCAr_GET(xl_ruca, 0);
        stat->RxUnicastPkts_hi = XLMIB_RUCAr_GET(xl_ruca, 1);
        rv |= READ_XLMIB_TUCAr(unit, lport, xl_tuca);
        stat->TxUnicastPkts_lo = XLMIB_TUCAr_GET(xl_tuca, 0);
        stat->TxUnicastPkts_hi = XLMIB_TUCAr_GET(xl_tuca, 1);

        /* Multicast Frame Counter */
        rv |= READ_XLMIB_RMCAr(unit, lport, xl_rmca);
        stat->RxMulticastPkts_lo = XLMIB_RMCAr_GET(xl_rmca, 0);
        stat->RxMulticastPkts_hi = XLMIB_RMCAr_GET(xl_rmca, 1);
        rv |= READ_XLMIB_TMCAr(unit, lport, xl_tmca);
        stat->TxMulticastPkts_lo = XLMIB_TMCAr_GET(xl_tmca, 0);
        stat->TxMulticastPkts_hi = XLMIB_TMCAr_GET(xl_tmca, 1);

        /* Broadcast Frame Counter */
        rv |= READ_XLMIB_RBCAr(unit, lport, xl_rbca);
        stat->RxBroadcastPkts_lo = XLMIB_RBCAr_GET(xl_rbca, 0);
        stat->RxBroadcastPkts_hi = XLMIB_RBCAr_GET(xl_rbca, 1);
        rv |= READ_XLMIB_TBCAr(unit, lport, xl_tbca);
        stat->TxBroadcastPkts_lo = XLMIB_TBCAr_GET(xl_tbca, 0);
        stat->TxBroadcastPkts_hi = XLMIB_TBCAr_GET(xl_tbca, 1);

        /* Pause Frame Counter */
        rv |= READ_XLMIB_RXPFr(unit, lport, xl_rxpf);
        stat->RxPauseFramePkts_lo = XLMIB_RXPFr_GET(xl_rxpf, 0);
        stat->RxPauseFramePkts_hi = XLMIB_RXPFr_GET(xl_rxpf, 1);
        rv |= READ_XLMIB_TXPFr(unit, lport, xl_txpf);
        stat->TxPauseFramePkts_lo = XLMIB_TXPFr_GET(xl_txpf, 0);
        stat->TxPauseFramePkts_hi = XLMIB_TXPFr_GET(xl_txpf, 1);

        /* Oversized Frame Counter */
        rv |= READ_XLMIB_ROVRr(unit, lport, xl_rovr);
        stat->RxOversizePkts_lo = XLMIB_ROVRr_GET(xl_rovr, 0);
        stat->RxOversizePkts_hi = XLMIB_ROVRr_GET(xl_rovr, 1);
        rv |= READ_XLMIB_TOVRr(unit, lport, xl_tovr);
        stat->TxOversizePkts_lo = XLMIB_TOVRr_GET(xl_tovr, 0);
        stat->TxOversizePkts_hi = XLMIB_TOVRr_GET(xl_tovr, 1);
    } else if (IS_CL_PORT(lport)) {
        /* Byte Counter */
        rv |= READ_CLMIB_TBYTr(unit, lport, cl_tbyt);
        stat->TxOctets_lo = CLMIB_TBYTr_GET(cl_tbyt, 0);
        stat->TxOctets_hi = CLMIB_TBYTr_GET(cl_tbyt, 1);
        rv |= READ_CLMIB_RBYTr(unit, lport, cl_rbyt);
        stat->RxOctets_lo = CLMIB_RBYTr_GET(cl_rbyt, 0);
        stat->RxOctets_hi = CLMIB_RBYTr_GET(cl_rbyt, 1);
        rv |= READ_CLMIB_TPKTr(unit, lport, cl_tpkt);
        stat->TxPkts_lo = CLMIB_TPKTr_GET(cl_tpkt, 0);
        stat->TxPkts_hi = CLMIB_TPKTr_GET(cl_tpkt, 1);
        rv |= READ_CLMIB_RPKTr(unit, lport, cl_rpkt);
        stat->RxPkts_lo = CLMIB_RPKTr_GET(cl_rpkt, 0);
        stat->RxPkts_hi = CLMIB_RPKTr_GET(cl_rpkt, 1);

        /* Rx FCS Error Frame Counter */
        rv |= READ_CLMIB_RFCSr(unit, lport, cl_rfcs);
        stat->CRCErrors_lo = CLMIB_RFCSr_GET(cl_rfcs, 0);
        stat->CRCErrors_hi = CLMIB_RFCSr_GET(cl_rfcs, 1);

        /* EEE LPI counter */
        rv |= READ_CLMIB_RX_EEE_LPI_EVENT_COUNTERr(unit, lport, cl_rx_eee_lpi_event_counter);
        stat->RxLPIPkts_lo = CLMIB_RX_EEE_LPI_EVENT_COUNTERr_GET(cl_rx_eee_lpi_event_counter, 0);
        stat->RxLPIPkts_hi = CLMIB_RX_EEE_LPI_EVENT_COUNTERr_GET(cl_rx_eee_lpi_event_counter, 1);
        rv |= READ_CLMIB_RX_EEE_LPI_DURATION_COUNTERr(unit, lport, cl_rx_eee_lpi_duration_counter);
        stat->RxLPIDuration_lo = CLMIB_RX_EEE_LPI_DURATION_COUNTERr_GET(cl_rx_eee_lpi_duration_counter, 0);
        stat->RxLPIDuration_hi = CLMIB_RX_EEE_LPI_DURATION_COUNTERr_GET(cl_rx_eee_lpi_duration_counter, 1);
        rv |= READ_CLMIB_TX_EEE_LPI_EVENT_COUNTERr(unit, lport, cl_tx_eee_lpi_event_counter);
        stat->TxLPIPkts_lo = CLMIB_TX_EEE_LPI_EVENT_COUNTERr_GET(cl_tx_eee_lpi_event_counter, 0);
        stat->TxLPIPkts_hi = CLMIB_TX_EEE_LPI_EVENT_COUNTERr_GET(cl_tx_eee_lpi_event_counter, 1);
        rv |= READ_CLMIB_TX_EEE_LPI_DURATION_COUNTERr(unit, lport, cl_tx_eee_lpi_duration_counter);
        stat->TxLPIDuration_lo = CLMIB_TX_EEE_LPI_DURATION_COUNTERr_GET(cl_tx_eee_lpi_duration_counter, 0);
        stat->TxLPIDuration_hi = CLMIB_TX_EEE_LPI_DURATION_COUNTERr_GET(cl_tx_eee_lpi_duration_counter, 1);

        /* Unicast Frame Counter */
        rv |= READ_CLMIB_RUCAr(unit, lport, cl_ruca);
        stat->RxUnicastPkts_lo = CLMIB_RUCAr_GET(cl_ruca, 0);
        stat->RxUnicastPkts_hi = CLMIB_RUCAr_GET(cl_ruca, 1);
        rv |= READ_CLMIB_TUCAr(unit, lport, cl_tuca);
        stat->TxUnicastPkts_lo = CLMIB_TUCAr_GET(cl_tuca, 0);
        stat->TxUnicastPkts_hi = CLMIB_TUCAr_GET(cl_tuca, 1);

        /* Multicast Frame Counter */
        rv |= READ_CLMIB_RMCAr(unit, lport, cl_rmca);
        stat->RxMulticastPkts_lo = CLMIB_RMCAr_GET(cl_rmca, 0);
        stat->RxMulticastPkts_hi = CLMIB_RMCAr_GET(cl_rmca, 1);
        rv |= READ_CLMIB_TMCAr(unit, lport, cl_tmca);
        stat->TxMulticastPkts_lo = CLMIB_TMCAr_GET(cl_tmca, 0);
        stat->TxMulticastPkts_hi = CLMIB_TMCAr_GET(cl_tmca, 1);

        /* Broadcast Frame Counter */
        rv |= READ_CLMIB_RBCAr(unit, lport, cl_rbca);
        stat->RxBroadcastPkts_lo = CLMIB_RBCAr_GET(cl_rbca, 0);
        stat->RxBroadcastPkts_hi = CLMIB_RBCAr_GET(cl_rbca, 1);
        rv |= READ_CLMIB_TBCAr(unit, lport, cl_tbca);
        stat->TxBroadcastPkts_lo = CLMIB_TBCAr_GET(cl_tbca, 0);
        stat->TxBroadcastPkts_hi = CLMIB_TBCAr_GET(cl_tbca, 1);

        /* Pause Frame Counter */
        rv |= READ_CLMIB_RXPFr(unit, lport, cl_rxpf);
        stat->RxPauseFramePkts_lo = CLMIB_RXPFr_GET(cl_rxpf, 0);
        stat->RxPauseFramePkts_hi = CLMIB_RXPFr_GET(cl_rxpf, 1);
        rv |= READ_CLMIB_TXPFr(unit, lport, cl_txpf);
        stat->TxPauseFramePkts_lo = CLMIB_TXPFr_GET(cl_txpf, 0);
        stat->TxPauseFramePkts_hi = CLMIB_TXPFr_GET(cl_txpf, 1);

        /* Oversized Frame Counter */
        rv |= READ_CLMIB_ROVRr(unit, lport, cl_rovr);
        stat->RxOversizePkts_lo = CLMIB_ROVRr_GET(cl_rovr, 0);
        stat->RxOversizePkts_hi = CLMIB_ROVRr_GET(cl_rovr, 1);
        rv |= READ_CLMIB_TOVRr(unit, lport, cl_tovr);
        stat->TxOversizePkts_lo = CLMIB_TOVRr_GET(cl_tovr, 0);
        stat->TxOversizePkts_hi = CLMIB_TOVRr_GET(cl_tovr, 1);
    }

    return rv;
}

/*!
 * \brief Clear the statistic value for a given port.
 *
 * \param [in] uport Port number.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_stat_clear(uint16 uport)
{
    sys_error_t rv = SYS_OK;
    uint8 unit, lport;

    GTBYTr_t gtbyt;
    GRBYTr_t grbyt;
    GTPKTr_t gtpkt;
    GRPKTr_t grpkt;
    GRX_EEE_LPI_EVENT_COUNTERr_t grx_eee_lpi_event_counter;
    GRX_EEE_LPI_DURATION_COUNTERr_t grx_eee_lpi_duration_counter;
    GTX_EEE_LPI_EVENT_COUNTERr_t gtx_eee_lpi_event_counter;
    GTX_EEE_LPI_DURATION_COUNTERr_t gtx_eee_lpi_duration_counter;
    GRUCr_t gruc;
    GTUCr_t gtuc;
    GRMCAr_t grmca;
    GTMCAr_t gtmca;
    GRBCAr_t grbca;
    GTBCAr_t gtbca;
    GRXPFr_t grxpf;
    GTXPFr_t gtxpf;
    GROVRr_t grovr;
    GTOVRr_t gtovr;
    GRFCSr_t  grfcs;

    XLMIB_TBYTr_t xl_tbyt;
    XLMIB_RBYTr_t xl_rbyt;
    XLMIB_TPKTr_t xl_tpkt;
    XLMIB_RPKTr_t xl_rpkt;
    XLMIB_RX_EEE_LPI_EVENT_COUNTERr_t xl_rx_eee_lpi_event_counter;
    XLMIB_RX_EEE_LPI_DURATION_COUNTERr_t xl_rx_eee_lpi_duration_counter;
    XLMIB_TX_EEE_LPI_EVENT_COUNTERr_t xl_tx_eee_lpi_event_counter;
    XLMIB_TX_EEE_LPI_DURATION_COUNTERr_t xl_tx_eee_lpi_duration_counter;
    XLMIB_RUCAr_t xl_ruca;
    XLMIB_TUCAr_t xl_tuca;
    XLMIB_RMCAr_t xl_rmca;
    XLMIB_TMCAr_t xl_tmca;
    XLMIB_RBCAr_t xl_rbca;
    XLMIB_TBCAr_t xl_tbca;
    XLMIB_RXPFr_t xl_rxpf;
    XLMIB_TXPFr_t xl_txpf;
    XLMIB_ROVRr_t xl_rovr;
    XLMIB_TOVRr_t xl_tovr;
    XLMIB_RFCSr_t xl_rfcs;

    CLMIB_TBYTr_t cl_tbyt;
    CLMIB_RBYTr_t cl_rbyt;
    CLMIB_TPKTr_t cl_tpkt;
    CLMIB_RPKTr_t cl_rpkt;
    CLMIB_RX_EEE_LPI_EVENT_COUNTERr_t cl_rx_eee_lpi_event_counter;
    CLMIB_RX_EEE_LPI_DURATION_COUNTERr_t cl_rx_eee_lpi_duration_counter;
    CLMIB_TX_EEE_LPI_EVENT_COUNTERr_t cl_tx_eee_lpi_event_counter;
    CLMIB_TX_EEE_LPI_DURATION_COUNTERr_t cl_tx_eee_lpi_duration_counter;
    CLMIB_RUCAr_t cl_ruca;
    CLMIB_TUCAr_t cl_tuca;
    CLMIB_RMCAr_t cl_rmca;
    CLMIB_TMCAr_t cl_tmca;
    CLMIB_RBCAr_t cl_rbca;
    CLMIB_TBCAr_t cl_tbca;
    CLMIB_RXPFr_t cl_rxpf;
    CLMIB_TXPFr_t cl_txpf;
    CLMIB_ROVRr_t cl_rovr;
    CLMIB_TOVRr_t cl_tovr;
    CLMIB_RFCSr_t cl_rfcs;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    if (IS_GX_PORT(lport)) {
        /* Byte Counter */
        GTBYTr_CLR(gtbyt);
        rv |= WRITE_GTBYTr(unit, lport, gtbyt);
        GRBYTr_CLR(grbyt);
        rv |= WRITE_GRBYTr(unit, lport, grbyt);
        GTPKTr_CLR(gtpkt);
        rv |= WRITE_GTPKTr(unit,lport, gtpkt);
        GRPKTr_CLR(grpkt);
        rv |= WRITE_GRPKTr(unit,lport, grpkt);

        /* Rx FCS Error Frame Counter */
        GRFCSr_CLR(grfcs);
        rv |= WRITE_GRFCSr(unit, lport, grfcs);

        /* EEE LPI counter */
        GRX_EEE_LPI_EVENT_COUNTERr_CLR(grx_eee_lpi_event_counter);
        rv |= WRITE_GRX_EEE_LPI_EVENT_COUNTERr(unit, lport, grx_eee_lpi_event_counter);
        GRX_EEE_LPI_DURATION_COUNTERr_CLR(grx_eee_lpi_duration_counter);
        rv |= WRITE_GRX_EEE_LPI_DURATION_COUNTERr(unit, lport, grx_eee_lpi_duration_counter);
        GTX_EEE_LPI_EVENT_COUNTERr_CLR(gtx_eee_lpi_event_counter);
        rv |= WRITE_GTX_EEE_LPI_EVENT_COUNTERr(unit, lport, gtx_eee_lpi_event_counter);
        GTX_EEE_LPI_DURATION_COUNTERr_CLR(gtx_eee_lpi_duration_counter);
        rv |= WRITE_GTX_EEE_LPI_DURATION_COUNTERr(unit, lport, gtx_eee_lpi_duration_counter);

        /* Unicast Frame Counter */
        GRUCr_CLR(gruc);
        rv |= WRITE_GRUCr(unit, lport, gruc);
        GTUCr_CLR(gtuc);
        rv |= READ_GTUCr(unit, lport, gtuc);

        /* Multicast Frame Counter */
        GRMCAr_CLR(grmca);
        rv |= WRITE_GRMCAr(unit, lport, grmca);
        GTMCAr_CLR(gtmca);
        rv |= WRITE_GTMCAr(unit, lport, gtmca);

        /* Broadcast Frame Counter */
        GRBCAr_CLR(grbca);
        rv |= WRITE_GRBCAr(unit, lport, grbca);
        GTBCAr_CLR(gtbca);
        rv |= WRITE_GTBCAr(unit, lport, gtbca);

        /* Pause Frame Counter */
        GRXPFr_CLR(grxpf);
        rv |= WRITE_GRXPFr(unit, lport, grxpf);
        GTXPFr_CLR(gtxpf);
        rv |= WRITE_GTXPFr(unit, lport, gtxpf);

        /* Oversized Frame Counter */
        GROVRr_CLR(grovr);
        rv |= WRITE_GROVRr(unit, lport, grovr);
        GTOVRr_CLR(gtovr);
        rv |= WRITE_GTOVRr(unit, lport, gtovr);
    } else if (IS_XL_PORT(lport)) {
        /* Byte Counter */
        XLMIB_TBYTr_CLR(xl_tbyt);
        rv |= WRITE_XLMIB_TBYTr(unit, lport, xl_tbyt);
        XLMIB_RBYTr_CLR(xl_rbyt);
        rv |= WRITE_XLMIB_RBYTr(unit, lport, xl_rbyt);
        XLMIB_TPKTr_CLR(xl_tpkt);
        rv |= WRITE_XLMIB_TPKTr(unit, lport, xl_tpkt);
        XLMIB_RPKTr_CLR(xl_rpkt);
        rv |= WRITE_XLMIB_RPKTr(unit, lport, xl_rpkt);

        /* Rx FCS Error Frame Counter */
        XLMIB_RFCSr_CLR(xl_rfcs);
        rv |= WRITE_XLMIB_RFCSr(unit, lport, xl_rfcs);

        /* EEE LPI counter */
        XLMIB_RX_EEE_LPI_EVENT_COUNTERr_CLR(xl_rx_eee_lpi_event_counter);
        rv |= WRITE_XLMIB_RX_EEE_LPI_EVENT_COUNTERr(unit, lport, xl_rx_eee_lpi_event_counter);
        XLMIB_RX_EEE_LPI_DURATION_COUNTERr_CLR(xl_rx_eee_lpi_duration_counter);
        rv |= WRITE_XLMIB_RX_EEE_LPI_DURATION_COUNTERr(unit, lport, xl_rx_eee_lpi_duration_counter);
        XLMIB_TX_EEE_LPI_EVENT_COUNTERr_CLR(xl_tx_eee_lpi_event_counter);
        rv |= WRITE_XLMIB_TX_EEE_LPI_EVENT_COUNTERr(unit, lport, xl_tx_eee_lpi_event_counter);
        XLMIB_TX_EEE_LPI_DURATION_COUNTERr_CLR(xl_tx_eee_lpi_duration_counter);
        rv |= WRITE_XLMIB_TX_EEE_LPI_DURATION_COUNTERr(unit, lport, xl_tx_eee_lpi_duration_counter);

        /* Unicast Frame Counter */
        XLMIB_RUCAr_CLR(xl_ruca);
        rv |= WRITE_XLMIB_RUCAr(unit, lport, xl_ruca);
        XLMIB_TUCAr_CLR(xl_tuca);
        rv |= WRITE_XLMIB_TUCAr(unit, lport, xl_tuca);

        /* Multicast Frame Counter */
        XLMIB_RMCAr_CLR(xl_rmca);
        rv |= WRITE_XLMIB_RMCAr(unit, lport, xl_rmca);
        XLMIB_TMCAr_CLR(xl_tmca);
        rv |= WRITE_XLMIB_TMCAr(unit, lport, xl_tmca);

        /* Broadcast Frame Counter */
        XLMIB_RBCAr_CLR(xl_rbca);
        rv |= WRITE_XLMIB_RBCAr(unit, lport, xl_rbca);
        XLMIB_TBCAr_CLR(xl_tbca);
        rv |= WRITE_XLMIB_TBCAr(unit, lport, xl_tbca);

        /* Pause Frame Counter */
        XLMIB_RXPFr_CLR(xl_rxpf);
        rv |= WRITE_XLMIB_RXPFr(unit, lport, xl_rxpf);
        XLMIB_TXPFr_CLR(xl_txpf);
        rv |= WRITE_XLMIB_TXPFr(unit, lport, xl_txpf);

        /* Oversized Frame Counter */
        XLMIB_ROVRr_CLR(xl_rovr);
        rv |= WRITE_XLMIB_ROVRr(unit, lport, xl_rovr);
        XLMIB_TOVRr_CLR(xl_tovr);
        rv |= WRITE_XLMIB_TOVRr(unit, lport, xl_tovr);
    } else if (IS_CL_PORT(lport)) {
        /* Byte Counter */
        CLMIB_TBYTr_CLR(cl_tbyt);
        rv |= WRITE_CLMIB_TBYTr(unit, lport, cl_tbyt);
        CLMIB_RBYTr_CLR(cl_rbyt);
        rv |= WRITE_CLMIB_RBYTr(unit, lport, cl_rbyt);
        CLMIB_TPKTr_CLR(cl_tpkt);
        rv |= WRITE_CLMIB_TPKTr(unit, lport, cl_tpkt);
        CLMIB_RPKTr_CLR(cl_rpkt);
        rv |= WRITE_CLMIB_RPKTr(unit, lport, cl_rpkt);

        /* Rx FCS Error Frame Counter */
        CLMIB_RFCSr_CLR(cl_rfcs);
        rv |= WRITE_CLMIB_RFCSr(unit, lport, cl_rfcs);

        /* EEE LPI counter */
        CLMIB_RX_EEE_LPI_EVENT_COUNTERr_CLR(cl_rx_eee_lpi_event_counter);
        rv |= WRITE_CLMIB_RX_EEE_LPI_EVENT_COUNTERr(unit, lport, cl_rx_eee_lpi_event_counter);
        CLMIB_RX_EEE_LPI_DURATION_COUNTERr_CLR(cl_rx_eee_lpi_duration_counter);
        rv |= WRITE_CLMIB_RX_EEE_LPI_DURATION_COUNTERr(unit, lport, cl_rx_eee_lpi_duration_counter);
        CLMIB_TX_EEE_LPI_EVENT_COUNTERr_CLR(cl_tx_eee_lpi_event_counter);
        rv |= WRITE_CLMIB_TX_EEE_LPI_EVENT_COUNTERr(unit, lport, cl_tx_eee_lpi_event_counter);
        CLMIB_TX_EEE_LPI_DURATION_COUNTERr_CLR(cl_tx_eee_lpi_duration_counter);
        rv |= WRITE_CLMIB_TX_EEE_LPI_DURATION_COUNTERr(unit, lport, cl_tx_eee_lpi_duration_counter);

        /* Unicast Frame Counter */
        CLMIB_RUCAr_CLR(cl_ruca);
        rv |= WRITE_CLMIB_RUCAr(unit, lport, cl_ruca);
        CLMIB_TUCAr_CLR(cl_tuca);
        rv |= WRITE_CLMIB_TUCAr(unit, lport, cl_tuca);

        /* Multicast Frame Counter */
        CLMIB_RMCAr_CLR(cl_rmca);
        rv |= WRITE_CLMIB_RMCAr(unit, lport, cl_rmca);
        CLMIB_TMCAr_CLR(cl_tmca);
        rv |= WRITE_CLMIB_TMCAr(unit, lport, cl_tmca);

        /* Broadcast Frame Counter */
        CLMIB_RBCAr_CLR(cl_rbca);
        rv |= WRITE_CLMIB_RBCAr(unit, lport, cl_rbca);
        CLMIB_TBCAr_CLR(cl_tbca);
        rv |= WRITE_CLMIB_TBCAr(unit, lport, cl_tbca);

        /* Pause Frame Counter */
        CLMIB_RXPFr_CLR(cl_rxpf);
        rv |= WRITE_CLMIB_RXPFr(unit, lport, cl_rxpf);
        CLMIB_TXPFr_CLR(cl_txpf);
        rv |= WRITE_CLMIB_TXPFr(unit, lport, cl_txpf);

        /* Oversized Frame Counter */
        CLMIB_ROVRr_CLR(cl_rovr);
        rv |= WRITE_CLMIB_ROVRr(unit, lport, cl_rovr);
        CLMIB_TOVRr_CLR(cl_tovr);
        rv |= WRITE_CLMIB_TOVRr(unit, lport, cl_tovr);
    }

    return rv;

}

/*!
 * \brief Clear the statistic value for all the ports.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_stat_clear_all(void)
{
    uint16 uport;
    sys_error_t rv = SYS_OK;

    SAL_UPORT_ITER(uport) {
        rv |= board_port_stat_clear(uport);
    }
    return rv;
}
#endif /* CFG_SWITCH_STAT_INCLUDED */

#ifdef CFG_SWITCH_MCAST_INCLUDED
/*!
 * \brief Add an entry in the multicast table.
 *
 * \param [in] mac_addr MAC address.
 * \param [in] vlan_id VLAN ID number.
 * \param [in] uplist Port list.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_mcast_addr_add(uint8 *mac_addr, uint16 vlan_id, uint8 *uplist)
{
    int rv = SYS_OK;
    int i,j;

    pbmp_t lpbmp;
    mcast_list_t *mcast = NULL;
    l2x_entry_t l2x;
    L2MCm_t l2mc;

    if (uplist != NULL) {
        SOC_IF_ERROR_RETURN(board_uplist_to_lpbmp(uplist, 0, &lpbmp));
    } else {
        PBMP_CLEAR(lpbmp);
    }

    mcast = (mcast_list_t *)sal_malloc(sizeof(mcast_list_t));
    if (mcast == NULL) {
        return SYS_ERR_OUT_OF_RESOURCE;
    }

    if (vlan_id == 0xFFF) {
        /* LEARN_VID as '1' in VLAN_CTRL
           set in soc->vlan_type_set */
        vlan_id = 1;
    }

    mcast->vlan_id = vlan_id;
    mcast->next = NULL;
    PBMP_ASSIGN(mcast->port_lpbmp, lpbmp);
    sal_memcpy(mcast->mac, mac_addr, 6);
    if (mlist == NULL) {
        mlist = mcast;
        mlist->index=1;
        sal_memset(mcindex, 0, sizeof(uint32) * L2MC_MAX_ENTRY_SIZE);
        mcindex[0]=0x1;
    } else {
       mcast->next = mlist;
       mcast->index = 0;
       for (i = 0; i < fl_sw_info.l2_mc_size; i++) {
           for (j=0; j<32; j++) {
                if ((mcindex[i] & (1<<j)) == 0) {
                    mcast->index = i*32 + j + 1;
                    mcindex[i] |= (1<<j);
                    break;
                }
           }
           if(mcast->index != 0) {
               break;
           }
       }
       /* get next mc index it can be used */
       mlist = mcast;
    }

    l2x.vlan_id = vlan_id;
    l2x.port = mcast->index;
    l2x.is_static = TRUE;
    sal_memcpy(l2x.mac_addr, mac_addr, 6);
    rv = bcm5607x_l2_op(0, &l2x, SC_OP_L2_INS_CMD, NULL);

    /* after get mcindex from l2_entry */
    READ_L2MCm(0, mcast->index, l2mc);
    L2MCm_PORT_BITMAPf_SET(l2mc, SOC_PBMP(lpbmp));
    L2MCm_VALIDf_SET(l2mc, 1);
    WRITE_L2MCm(0, mcast->index, l2mc);

    return rv;
}

/*!
 * \brief Remove an entry from the multicast table.
 *
 * \param [in] mac_addr MAC address.
 * \param [in] vlan_id VALN ID number.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_mcast_addr_remove(uint8 *mac_addr, uint16 vlan_id)
{
    int rv = SYS_OK;
    pbmp_t  lpbmp;
    mcast_list_t *mcast = NULL;
    int exists=0;
    mcast_list_t *prev_mcast, *tmp_mcast;
    int l2_index=0;
    l2x_entry_t l2x;
    L2MCm_t l2mc;

    if (vlan_id == 0xFFF) {
        /* LEARN_VID as '1' in VLAN_CTRL
           set in soc->vlan_type_set */
        vlan_id = 1;
    }

     /* check if exists */
    mcast = mlist;
    while(mcast != NULL) {
        if((mcast->vlan_id == vlan_id) && (!sal_memcmp(mcast->mac, mac_addr, 6))) {
            exists = 1;
            break;
        }
        mcast = mcast->next;
    }
    if (exists == 0) {
        return SYS_OK;
    }

    /* existed, destroy mcast */
    prev_mcast = mlist;
    if((prev_mcast->vlan_id == vlan_id) && (!sal_memcmp(prev_mcast->mac, mac_addr, 6))) {
        /* first node is vlan need to destroy */
        l2_index = prev_mcast->index;
        mcindex[prev_mcast->index/32] &= ~(1<<((prev_mcast->index - 1)%32));
        mlist = mlist->next;
        sal_free(prev_mcast);
    } else {
        mcast= prev_mcast->next;
        while((prev_mcast->next) != NULL) {
            if((mcast->vlan_id == vlan_id) && (!sal_memcmp(mcast->mac, mac_addr, 6))) {
                l2_index = mcast->index;
                mcindex[prev_mcast->index/32] &= ~(1<<((prev_mcast->index - 1)%32));
                tmp_mcast = mcast->next;
                prev_mcast->next = tmp_mcast;
                sal_free(mcast);
                mcast = mlist;
            } else {
                prev_mcast = mcast;
                mcast = mcast->next;
            }
        }
    }
    if (l2_index == 0) {
        return SYS_OK;
    }

    l2x.vlan_id = vlan_id;
    l2x.port = l2_index;
    sal_memcpy(l2x.mac_addr, mac_addr, 6);
    rv = bcm5607x_l2_op(0, &l2x, SC_OP_L2_DEL_CMD, NULL);

    /* after get mcindex from l2_entry */
    PBMP_CLEAR(lpbmp);
    READ_L2MCm(0, mcast->index, l2mc);
    L2MCm_PORT_BITMAPf_SET(l2mc, SOC_PBMP(lpbmp));
    L2MCm_VALIDf_SET(l2mc, 0);
    WRITE_L2MCm(0, mcast->index, l2mc);

    return rv;
}

/*!
 * \brief Add port for a given entry in multicast table.
 *
 * \param [in] mac_addr MAC address.
 * \param [in] vlan_id VLAN ID number.
 * \param [in] uport Port number.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_mcast_port_add(uint8 *mac_addr, uint16 vlan_id, uint16 uport)
{
    int rv = SYS_OK;
    uint8 unit;

    uint8 lport;
    mcast_list_t *mcast = NULL;
    int exists = 0;
    L2MCm_t l2mc;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));
    /* check if exists */
    mcast = mlist;

    if (vlan_id == 0xFFF) {
        /* LEARN_VID as '1' in VLAN_CTRL
           set in soc->vlan_type_set */
        vlan_id = 1;
    }

    while(mcast != NULL) {
        if((mcast->vlan_id == vlan_id) && (!sal_memcmp(mcast->mac, mac_addr, 6))) {
            PBMP_PORT_ADD(mcast->port_lpbmp, lport);
            exists = 1;
            break;
        }
        mcast = mcast->next;
    }
    if (exists == 0) {
        /* no exists */
        return SYS_ERR_NOT_FOUND;
    }

    /* after get mcindex from l2_entry */
    READ_L2MCm(unit, mcast->index, l2mc);
    L2MCm_PORT_BITMAPf_SET(l2mc, SOC_PBMP(mcast->port_lpbmp));
    WRITE_L2MCm(unit, mcast->index, l2mc);


    return rv;
}

/*!
 * \brief Remove port for a given entry from multicast table.
 *
 * \param [in] mac_addr MAC address.
 * \param [in] vlan_id VLAN ID number.
 * \param [in] uport Port number.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_mcast_port_remove(uint8 *mac_addr, uint16 vlan_id, uint16 uport)
{
    int rv = SYS_OK;
    uint8 unit;
    uint8 lport;
    mcast_list_t *mcast = NULL;
    int exists = 0;
    L2MCm_t l2mc;

    /* check if exists */
    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));
    mcast = mlist;

    if (vlan_id == 0xFFF) {
        /* LEARN_VID as '1' in VLAN_CTRL
           set in soc->vlan_type_set */
        vlan_id = 1;
    }

    while(mcast != NULL) {
        if((mcast->vlan_id == vlan_id) && (!sal_memcmp(mcast->mac, mac_addr, 6))) {
            PBMP_PORT_REMOVE(mcast->port_lpbmp, lport);
            exists = 1;
            break;
        }
        mcast = mcast->next;
    }
    if (exists == 0) {
        /* no exists, not need to remove */
        return SYS_OK;
    }

    /* after get mcindex from l2_entry */
    READ_L2MCm(unit, mcast->index, l2mc);
    L2MCm_PORT_BITMAPf_SET(l2mc, SOC_PBMP(mcast->port_lpbmp));
    WRITE_L2MCm(unit, mcast->index, l2mc);

    return rv;
}

/*!
 * \brief Get the port list for a given entry in multicast table.
 *
 * \param [in] mac_addr MAC address.
 * \param [in] vlan_id VLAN ID number.
 * \param [out] uplist Port list.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_mcast_port_get(uint8 *mac_addr, uint16 vlan_id, uint8 *uplist)
{
    int rv = SYS_OK;
    mcast_list_t *mcast = NULL;
    int exists = 0;

    /* check if exists */
    mcast = mlist;

    if (vlan_id == 0xFFF) {
        /* LEARN_VID as '1' in VLAN_CTRL set in soc->vlan_type_set */
        vlan_id = 1;
    }

    while(mcast != NULL) {
        if((mcast->vlan_id == vlan_id) && (!sal_memcmp(mcast->mac, mac_addr, 6))) {
            exists = 1;
            break;
        }
        mcast = mcast->next;
    }
    if (exists == 0) {
        /* no exists */
        return SYS_ERR_NOT_FOUND;
    }

    board_lpbmp_to_uplist(0, mcast->port_lpbmp, uplist);

    return rv;
}

/*!
 * \brief Set the IGMP snooping state.
 *
 * \param [in] enable
 *    \li TRUE = Enable IGMP snooping.
 *    \li FALSE = Disable IGMP snooping.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_igmp_snoop_enable_set(uint8 enable)
{
 //   uint32 val;
    sys_error_t rv = SYS_OK;

    IGMP_MLD_PKT_CONTROLr_t  igmp_mld_pkt_control;
    /* in M_PORT.PROTOCOL_PKT_INDEX set the index to IGMP_MLD_PKT_CONTROL
     * Per port set in this bit is '0', thus set IGMP_MLD_PKT_CONTROL(0)
     */
    rv = READ_IGMP_MLD_PKT_CONTROLr(0, 0, igmp_mld_pkt_control);
    if (enable == 1) {
        // val = (val & 0xf803ffff) | 0x036c0000;
        /* set bit 26-18 = 011011011 */
        IGMP_MLD_PKT_CONTROLr_IGMP_UNKNOWN_MSG_TO_CPUf_SET(igmp_mld_pkt_control, 1);
        IGMP_MLD_PKT_CONTROLr_IGMP_UNKNOWN_MSG_FWD_ACTIONf_SET(igmp_mld_pkt_control, 1);// drop packet
        IGMP_MLD_PKT_CONTROLr_IGMP_QUERY_TO_CPUf_SET(igmp_mld_pkt_control, 1);
        IGMP_MLD_PKT_CONTROLr_IGMP_QUERY_FWD_ACTIONf_SET(igmp_mld_pkt_control, 1);  // drop packet
        IGMP_MLD_PKT_CONTROLr_IGMP_REP_LEAVE_TO_CPUf_SET(igmp_mld_pkt_control, 1);
        IGMP_MLD_PKT_CONTROLr_IGMP_REP_LEAVE_FWD_ACTIONf_SET(igmp_mld_pkt_control, 1); // drop packet
    } else {
        // val = (val & 0xf803ffff) | 0x04900000;
        /* set bit 26-18 = 100100100 */
        IGMP_MLD_PKT_CONTROLr_IGMP_UNKNOWN_MSG_TO_CPUf_SET(igmp_mld_pkt_control, 0);
        IGMP_MLD_PKT_CONTROLr_IGMP_UNKNOWN_MSG_FWD_ACTIONf_SET(igmp_mld_pkt_control, 2);// Flood the packet to the VLAN members
        IGMP_MLD_PKT_CONTROLr_IGMP_QUERY_TO_CPUf_SET(igmp_mld_pkt_control, 0);
        IGMP_MLD_PKT_CONTROLr_IGMP_QUERY_FWD_ACTIONf_SET(igmp_mld_pkt_control, 2);  // Flood the packet to the VLAN members
        IGMP_MLD_PKT_CONTROLr_IGMP_REP_LEAVE_TO_CPUf_SET(igmp_mld_pkt_control, 0);
        IGMP_MLD_PKT_CONTROLr_IGMP_REP_LEAVE_FWD_ACTIONf_SET(igmp_mld_pkt_control, 2); // Flood the packet to the VLAN members
    }
    rv |= WRITE_IGMP_MLD_PKT_CONTROLr(0, 0, igmp_mld_pkt_control);

    return rv;
}

/*!
 * \brief Get the IGMP snooping state.
 *
 * \param [out] enable
 *    \li TRUE = IGMP snooping is enabled.
 *    \li FALSE = IGMP snooping is disabled.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_igmp_snoop_enable_get(uint8 *enable)
{

    sys_error_t rv = SYS_OK;

    IGMP_MLD_PKT_CONTROLr_t igmp_mld_pkt_control;
    /* select (0) for check due to all port in M_PORT.PROTOCOL_PKT_INDEX
       set to '0'
     */
    rv = READ_IGMP_MLD_PKT_CONTROLr(0,0, igmp_mld_pkt_control);
    if (IGMP_MLD_PKT_CONTROLr_IGMP_UNKNOWN_MSG_TO_CPUf_GET(igmp_mld_pkt_control) &&
        IGMP_MLD_PKT_CONTROLr_IGMP_QUERY_TO_CPUf_GET(igmp_mld_pkt_control) &&
        IGMP_MLD_PKT_CONTROLr_IGMP_REP_LEAVE_TO_CPUf_GET(igmp_mld_pkt_control))
    {
        *enable = 1;
    } else {
        *enable = 0;
    }
    return rv;
}

/*!
 * \brief Set the state of block unknown multicast packet.
 *
 * \param [in] enable
 *    \li TRUE = Enable block unknown multicast.
 *    \li FALSE = Disable block unknown multicast.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_block_unknown_mcast_set(uint8 enable)
{

    int lport;
    int rv;
    uint32_t mask_lo_zero_pbmp[2] = {0x0, 0x0};
    uint32_t mask_lo_one_pbmp[2] = {0xffffffff, 0xffffffff};
    uint32_t mask_hi_one_pbmp = 0x3;

    UNKNOWN_MCAST_BLOCK_MASK_LO_64r_t unknow_mcast_block_mask_lo_64;
    UNKNOWN_MCAST_BLOCK_MASK_HI_64r_t unknow_mcast_block_mask_hi_64;
    IUNKNOWN_MCAST_BLOCK_MASK_LO_64r_t iunknow_mcast_block_mask_lo_64;
    IUNKNOWN_MCAST_BLOCK_MASK_HI_64r_t iunknow_mcast_block_mask_hi_64;

    rv = SYS_OK;

    for (lport = BCM5607X_LPORT_MIN; lport <= BCM5607X_LPORT_MAX; lport++) {
        rv = READ_UNKNOWN_MCAST_BLOCK_MASK_LO_64r(0, lport, unknow_mcast_block_mask_lo_64);
        rv |= READ_UNKNOWN_MCAST_BLOCK_MASK_HI_64r(0, lport, unknow_mcast_block_mask_hi_64);
        if (enable == 1) {
            UNKNOWN_MCAST_BLOCK_MASK_LO_64r_BLK_BITMAPf_SET(unknow_mcast_block_mask_lo_64, mask_lo_one_pbmp); /* block all */
            UNKNOWN_MCAST_BLOCK_MASK_HI_64r_BLK_BITMAPf_SET(unknow_mcast_block_mask_hi_64, mask_hi_one_pbmp); /* block all */

        } else {
            UNKNOWN_MCAST_BLOCK_MASK_LO_64r_BLK_BITMAPf_SET(unknow_mcast_block_mask_lo_64, mask_lo_zero_pbmp); /* allow all */
            UNKNOWN_MCAST_BLOCK_MASK_HI_64r_BLK_BITMAPf_SET(unknow_mcast_block_mask_hi_64, 0x0); /* allow all */
        }
        rv |= WRITE_UNKNOWN_MCAST_BLOCK_MASK_LO_64r(0, lport, unknow_mcast_block_mask_lo_64);
        rv |= WRITE_UNKNOWN_MCAST_BLOCK_MASK_HI_64r(0, lport, unknow_mcast_block_mask_hi_64);

        rv |= READ_IUNKNOWN_MCAST_BLOCK_MASK_LO_64r(0, lport, iunknow_mcast_block_mask_lo_64);
        rv |= READ_IUNKNOWN_MCAST_BLOCK_MASK_HI_64r(0, lport, iunknow_mcast_block_mask_hi_64);

        if (enable == 1) {
            IUNKNOWN_MCAST_BLOCK_MASK_LO_64r_BLK_BITMAPf_SET(iunknow_mcast_block_mask_lo_64, mask_lo_one_pbmp); /* block all */
            IUNKNOWN_MCAST_BLOCK_MASK_HI_64r_BLK_BITMAPf_SET(iunknow_mcast_block_mask_hi_64, mask_hi_one_pbmp); /* block all */
        } else {
            IUNKNOWN_MCAST_BLOCK_MASK_LO_64r_BLK_BITMAPf_SET(iunknow_mcast_block_mask_lo_64, mask_lo_zero_pbmp); /* allow all */
            IUNKNOWN_MCAST_BLOCK_MASK_HI_64r_BLK_BITMAPf_SET(iunknow_mcast_block_mask_hi_64, 0x0); /* allow all */
        }
        rv |= WRITE_IUNKNOWN_MCAST_BLOCK_MASK_LO_64r(0, lport, iunknow_mcast_block_mask_lo_64);
        rv |= WRITE_IUNKNOWN_MCAST_BLOCK_MASK_HI_64r(0, lport, iunknow_mcast_block_mask_hi_64);

    }
    return rv;
}

/*!
 * \brief Get the state of block unknown multicast packet.
 *
 * \param [out] enable
 *    \li TRUE = Block unknown multicast is enabled.
 *    \li FALSE = Block unknown multicast is disabled.
 */
sys_error_t
board_block_unknown_mcast_get(uint8 *enable)
{

    int rv;

    UNKNOWN_MCAST_BLOCK_MASK_HI_64r_t unknow_mcast_block_mask_hi_64;

    rv = SYS_OK;

    /* select port 1 to check */
    *enable = 0;
    rv = READ_UNKNOWN_MCAST_BLOCK_MASK_HI_64r(0, BCM5607X_LPORT_MIN, unknow_mcast_block_mask_hi_64);

    if (UNKNOWN_MCAST_BLOCK_MASK_HI_64r_BLK_BITMAPf_GET(unknow_mcast_block_mask_hi_64)) {
        *enable = 1;
    }

    return rv;
}
#endif /* CFG_SWITCH_MCAST_INCLUDED */

#ifdef CFG_SWITCH_LOOPDETECT_INCLUDED
/*!
* \brief Set the loop-detect state.
*
* \param [in] enable
*    \li TRUE = Enable loop detect.
*    \li FALSE = Disable loop detect.
*/
void
board_loop_detect_enable(BOOL enable)
{
    bcm5607x_loop_detect_enable(enable);
}

/*!
 * \brief Get whether the loop-detect state.
 *
 * \return \li TRUE = Enable loop detect.
 *           \li FALSE = Disable loop detect.
 */
uint8
board_loop_detect_status_get(void)
{
    return bcm5607x_loop_detect_status_get();
}
#endif /* CFG_SWITCH_LOOPDETECT_INCLUDED */

#if defined(CFG_LLDP_INCLUDED)
sys_error_t
board_get_port_phy_eee_ability_remote(uint16 uport, uint8 *types)
{
    return SYS_ERR;
}

sys_error_t
board_get_port_phy_eee_min_wake_time(uint16 uport, uint8 type, uint16 *wake_t)
{
    return SYS_ERR;
}

sys_error_t
board_set_port_phy_eee_tx_wake_time(uint16 uport, uint8 type, uint16 wake_t)
{
    uint8 unit, lport;
    sys_error_t r;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    r = bcm5607x_port_eee_tx_wake_time_set(unit, lport, type, wake_t);

    return r;
}
#endif /*  CFG_LLDP_INCLUDED */

#ifdef CFG_ZEROCONF_MDNS_INCLUDED
/*!
 * \brief Set the mdns state.
 *
 * \param [in] enable
 *  \li TRUE = Enable MDNS.
 *  \li FALSE = Disable MDNS.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_mdns_enable_set(BOOL enable)
{
    return bcm5607x_mdns_enable_set(0, enable);
}

/*!
 * \brief Get the mdns state.
 *
 * \param [out] enable
 *  \li TRUE = MDNS is enabled.
 *  \li FALSE = MDNS is disabled.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_mdns_enable_get(BOOL *enable)
{
    return bcm5607x_mdns_enable_get(0, enable);
}
#endif /* CFG_ZEROCONF_MDNS_INCLUDED */


/*****************************************************************************
 * Port APIs
 */
/*!
 * \brief Port re-init.
 *
 * This function is used to reinit for a given port.
 *
 * \param [in] uport Port number.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_reinit(uint16 uport)
{
    sys_error_t r;
    uint8 unit, lport;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    r = pcm_port_reinit(unit, lport);

    return r;
}

/*!
 * \brief Setting the port speed
 *
 * This function is used to set the speed for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] speed Speed alue in megabits/sec (10, 100, etc)
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_speed_set(uint16 uport, int speed)
{
    uint8 unit, lport;
    sys_error_t r;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    r = pcm_port_speed_set(unit, lport, speed);

    return r;
}

/*!
 * \brief Getting the port speed
 *
 * This function is used to get the speed for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] speed Speed alue in megabits/sec (10, 100, etc)
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_speed_get(uint16 uport, int *speed)
{
    uint8 unit, lport;
    sys_error_t r;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    if (speed == NULL) {
        return SYS_ERR_PARAMETER;
    }

    *speed = 0;
    r = pcm_port_speed_get(unit, lport, speed);

    return r;
}

/*!
 * \brief Setting the interface type
 *
 * This function is used to set the interface type for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] intf PORT_IF_*
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_interface_set(uint16 uport, port_if_t intf)
{
    uint8 unit, lport;
    sys_error_t r;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    r = pcm_port_interface_set(unit, lport, (int)intf);

    return r;
}

/*!
 * \brief Getting the interface type
 *
 * This function is used to get the interface type for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] intf PORT_IF_*
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_interface_get(uint16 uport, port_if_t *intf)
{
    uint8 unit, lport;
    sys_error_t r = SYS_OK;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    if (intf == NULL) {
        return SYS_ERR_PARAMETER;
    }

    *intf = 0;
    r = pcm_port_interface_get(unit, lport, (int *)intf);

    return r;
}

/*!
 * \brief Setting the autonegotiation state
 *
 * This function is used to set the autonegotiation state for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] an Boolean value for autonegotiation state
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_an_set(uint16 uport, BOOL an)
{
    uint8 unit, lport;
    int autoneg = (an == TRUE) ? 1 : 0;
    sys_error_t r;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    r = pcm_port_autoneg_set(unit, lport, autoneg);

    return r;
}

/*!
 * \brief Getting the autonegotiation state of the port
 *
 * This function is used to get the autonegotiation state for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] an Boolean value for autonegotiation state
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_an_get(uint16 uport, BOOL *an)
{
    uint8 unit, lport;
    int autoneg;
    sys_error_t r;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    if (an == NULL) {
        return SYS_ERR_PARAMETER;
    }

    r = pcm_port_autoneg_get(unit, lport, &autoneg);
    *an = (autoneg) ? TRUE : FALSE;

    return r;
}

/*!
 * \brief Setting the port duplex state
 *
 * This function is used to set the duplex mode for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] duplex Duplex mode
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_duplex_set(uint16 uport, port_duplex_t duplex)
{
    uint8 unit, lport;
    int dp;
    sys_error_t r;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    if (duplex >= PORT_DUPLEX_COUNT) {
        return SYS_ERR_PARAMETER;
    } else {
        if (duplex == PORT_DUPLEX_HALF) {
            dp = PCM_PORT_DUPLEX_HALF;
        } else if (duplex == PORT_DUPLEX_FULL) {
            dp = PCM_PORT_DUPLEX_FULL;
        }
    }

    r = pcm_port_duplex_set(unit, lport, dp);

    return r;
}

/*!
 * \brief Getting the port duplex state
 *
 * This function is used to get the duplex mode for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] duplex Duplex mode
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_duplex_get(uint16 uport, port_duplex_t *duplex)
{
    uint8 unit, lport;
    int dp;
    sys_error_t r;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    if (duplex == NULL) {
        return SYS_ERR_PARAMETER;
    }

    r = pcm_port_duplex_get(unit, lport, &dp);
    if (dp == PCM_PORT_DUPLEX_HALF) {
        *duplex = PORT_DUPLEX_HALF;
    } else {
        *duplex = PORT_DUPLEX_FULL;
    }

    return r;
}

/*!
 * \brief Physically enable/disable the port
 *
 * This function is used to enable/disable the MAC/PHY for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] enable Boolean value for enable/disable
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_enable_set(uint16 uport, BOOL enable)
{
    uint8 unit, lport;
    int en = (enable == TRUE) ? 1 : 0;
    sys_error_t r;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    r = pcm_port_enable_set(unit, lport, en);

    return r;
}

/*!
 * \brief Getting the enable state
 *
 * This function is used to get the enable/disable state for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] enable Boolean value for enable/disable
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_enable_get(uint16 uport, BOOL *enable)
{
    uint8 unit, lport;
    sys_error_t r;
    int en;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    if (enable == NULL) {
        return SYS_ERR_PARAMETER;
    }

    r = pcm_port_enable_get(unit, lport, &en);
    *enable = (en) ? TRUE : FALSE;

    return r;
}

/*!
 * \brief Setting the loopback
 *
 * This function is used to set the loopback for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] loopback Loopback value
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_loopback_set(uint16 uport, port_loopback_t loopback)
{
    uint8 unit, lport;
    sys_error_t r;
    int lb;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    if (loopback >= PORT_LOOPBACK_COUNT) {
        return SYS_ERR_PARAMETER;
    } else {
        if (loopback == PORT_LOOPBACK_NONE) {
            lb = PCM_PORT_LOOPBACK_NONE;
        } else if (loopback == PORT_LOOPBACK_MAC) {
            lb = PCM_PORT_LOOPBACK_MAC;
        } else if (loopback == PORT_LOOPBACK_PHY) {
            lb = PCM_PORT_LOOPBACK_PHY;
        } else if (loopback == PORT_LOOPBACK_PHY_REMOTE) {
            lb = PCM_PORT_LOOPBACK_PHY_REMOTE;
        }
    }

    r = pcm_port_loopback_set(unit, lport, lb);

    return r;
}

/*!
 * \brief Getting the loopback
 *
 * This function is used to get the loopback for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] loopback Loopback value
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_loopback_get(uint16 uport, port_loopback_t *loopback)
{
    uint8 unit, lport;
    sys_error_t r;
    int lb;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    r = pcm_port_loopback_get(unit, lport, &lb);

    if (loopback == NULL) {
        return SYS_ERR_PARAMETER;
    }

    if (lb == PCM_PORT_LOOPBACK_MAC) {
        *loopback = PORT_LOOPBACK_MAC;
    } else if (lb == PCM_PORT_LOOPBACK_PHY) {
        *loopback = PORT_LOOPBACK_PHY;
    } else if (lb == PCM_PORT_LOOPBACK_PHY_REMOTE) {
        *loopback = PORT_LOOPBACK_PHY_REMOTE;
    } else {
        *loopback = PORT_LOOPBACK_NONE;
    }

    return r;
}

/*!
 * \brief Setting the new ifg (Inter-frame gap) value
 *
 * This function is used to set the new ifg (Inter-frame gap) value for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] ifg Inter-frame gap in bit-times.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_ifg_set(uint16 uport, int ifg)
{
    uint8 unit, lport;
    sys_error_t r;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    r = pcm_port_ifg_set(unit, lport, ifg);

    return r;
}

/*!
 * \brief Getting the new ifg (Inter-frame gap) value
 *
 * This function is used to get the new ifg (Inter-frame gap) value for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] ifg Inter-frame gap in bit-times.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_ifg_get(uint16 uport, int *ifg)
{
    uint8 unit, lport;
    sys_error_t r;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    if (ifg == NULL) {
        return SYS_ERR_PARAMETER;
    }

    *ifg = 0;
    r = pcm_port_ifg_get(unit, lport, ifg);

    return r;
}

/*!
 * \brief Setting the maximum receive frame size
 *
 * This function is used to set the maximum receive frame size for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] size Maximum frame size in bytes.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_frame_max_set(uint16 uport, int size)
{
    uint8 unit, lport;
    sys_error_t r;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    r = pcm_port_frame_max_set(unit, lport, size);

    return r;
}

/*!
 * \brief Getting the maximum receive frame size
 *
 * This function is used to get the maximum receive frame size for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] size Maximum frame size in bytes.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_frame_max_get(uint16 uport, int *size)
{
    uint8 unit, lport;
    sys_error_t r;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    if (size == NULL) {
        return SYS_ERR_PARAMETER;
    }

    *size = 0;
    r = pcm_port_frame_max_get(unit, lport, size);

    return r;
}

/*!
 * \brief Setting the pause state
 *
 * This function is used to set the pause state for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] pause_tx Boolean value for TX pause state.
 * \param [in] pause_rx Boolean value for RX pause state.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_pause_set(uint16 uport, BOOL pause_tx, BOOL pause_rx)
{
    uint8 unit, lport;
    int txp = (pause_tx == TRUE) ? 1 : 0;
    int rxp = (pause_rx == TRUE) ? 1 : 0;
    sys_error_t r;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    r = pcm_port_pause_set(unit, lport, txp, rxp);

    return r;
}

/*!
 * \brief Getting the pause state
 *
 * This function is used to get the pause state for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] pause_tx Boolean value for TX pause state.
 * \param [out] pause_rx Boolean value for RX pause state.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_pause_get(uint16 uport, BOOL *pause_tx, BOOL *pause_rx)
{
    uint8 unit, lport;
    int txp, rxp;
    sys_error_t r;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    if (pause_tx == NULL || pause_rx == NULL) {
        return SYS_ERR_PARAMETER;
    }

    r = pcm_port_pause_get(unit, lport, &txp, &rxp);

    *pause_tx = (txp) ? TRUE : FALSE;
    *pause_rx = (rxp) ? TRUE : FALSE;

    return r;
}

/*!
 * \brief Setting the ports class ID.
 *
 * This function is used to set the ports class ID for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] pclass Classification type.
 * \param [in] pclass_id New class ID of the port.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_class_set(uint16 uport, port_class_t pclass, uint32 pclass_id)
{
    uint8 unit, lport;
    sys_error_t r = SYS_OK;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    r = bcm5607x_port_class_set(unit, lport, pclass, pclass_id);

    return r;
}

/*!
 * \brief Getting the ports class ID.
 *
 * This function is used to get the ports class ID for a given port.
 *
 * \param [in] uport Port number.
 * \param [in] pclass Classification type.
 * \param [out] pclass_id New class ID of the port.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_class_get(uint16 uport, port_class_t pclass, uint32 *pclass_id)
{
    uint8 unit, lport;
    sys_error_t r = SYS_OK;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    if (pclass_id == NULL) {
        return SYS_ERR_PARAMETER;
    }

    *pclass_id = 0;
    r = bcm5607x_port_class_get(unit, lport, pclass, pclass_id);

    return r;
}

/*!
 * \brief Get the link status
 *
 * This function is used to get the link status for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] link Link status.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_port_link_status_get(uint16 uport, BOOL *link)
{
    uint8 unit, lport;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    if (link == NULL) {
        return SYS_ERR_PARAMETER;
    }

    *link = SOC_PORT_LINK_STATUS(lport);

    return SYS_OK;
}

/*!
 * \brief Get the link status
 *
 * This function is used to get the current speed status for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] speed Speed alue in megabits/sec (10, 100, etc).
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_speed_status_get(uint16 uport, int *speed)
{
    uint8 unit, lport;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    if (speed == NULL) {
        return SYS_ERR_PARAMETER;
    }

    *speed = SOC_PORT_SPEED_STATUS(lport);

    return SYS_OK;
}

/*!
 * \brief Get the link status
 *
 * This function is used to get the current autoneg status for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] an Boolean value for autonegotiation state.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_an_status_get(uint16 uport, BOOL *an)
{
    uint8 unit, lport;
    int autoneg;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    if (an == NULL) {
        return SYS_ERR_PARAMETER;
    }

    autoneg = SOC_PORT_AN_STATUS(lport);
    *an = (autoneg) ? TRUE : FALSE;

    return SYS_OK;
}

/*!
 * \brief Get the link status
 *
 * This function is used to get the current duplex status for a given port.
 *
 * \param [in] uport Port number.
 * \param [out] duplex Duplex mode.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_duplex_status_get(uint16 uport, port_duplex_t *duplex)
{
    uint8 unit, lport;
    int dp;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    if (duplex == NULL) {
        return SYS_ERR_PARAMETER;
    }

    dp = SOC_PORT_DUPLEX_STATUS(lport);
    if (dp == PCM_PORT_DUPLEX_HALF) {
        *duplex = PORT_DUPLEX_HALF;
    } else {
        *duplex = PORT_DUPLEX_FULL;
    }

    return SYS_OK;
}

#if defined(CFG_SWITCH_VLAN_INCLUDED)
/*! For internal use only. */
#ifdef CFG_SWITCH_SNAKETEST_INCLUDED
sys_error_t
board_snaketest_trap_to_cpu(uint16 vlanid, uint8 *uplist) {
    pbmp_t      lpbmp, tag_lpbmp;

    board_uplist_to_lpbmp(uplist, 0, &lpbmp);

    PBMP_CLEAR(tag_lpbmp);
    PBMP_PORT_ADD(lpbmp, 0);
    PBMP_PORT_ADD(tag_lpbmp, 0);

    return bcm5607x_qvlan_port_set(0, vlanid, lpbmp, tag_lpbmp);
}
#endif
sys_error_t
board_qvlan_add_cpu(uint16 vlanid, int tagged) {
    pbmp_t      lpbmp, tag_lpbmp;

    bcm5607x_qvlan_port_get(0, vlanid, &lpbmp, &tag_lpbmp);

    PBMP_PORT_ADD(lpbmp, 0);
    if (tagged) {
        PBMP_PORT_ADD(tag_lpbmp, 0);
    }

    return bcm5607x_qvlan_port_set(0, vlanid, lpbmp, tag_lpbmp);
}
sys_error_t
board_qvlan_remove_cpu(uint16 vlanid) {
    pbmp_t      lpbmp, tag_lpbmp;

    bcm5607x_qvlan_port_get(0, vlanid, &lpbmp, &tag_lpbmp);

    PBMP_PORT_REMOVE(lpbmp, 0);
    PBMP_PORT_REMOVE(tag_lpbmp, 0);

    return bcm5607x_qvlan_port_set(0, vlanid, lpbmp, tag_lpbmp);
}
#endif /* CFG_SWITCH_VLAN_INCLUDED */

#ifdef CFG_RESET_BUTTON_INCLUDED
uint8 sw_simulate_press_reset_button_duration = 0;
uint8 reset_button_active_high = 1;
uint8 reset_button_gpio_bit = 4;

BOOL
board_reset_button_get(void)
{
    uint32 val;
    /*
     * IPROC 15 supports 32 GPIOs
     */
    val = (READCSR(R_GPIO_DATA_IN) & 0xFFFFFFFF);

    if (sw_simulate_press_reset_button_duration) {
        sw_simulate_press_reset_button_duration--;
        return TRUE;
    }

    if  (reset_button_active_high) {
        if (val & (1 << (reset_button_gpio_bit))) {
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        if (val & (1 << (reset_button_gpio_bit))) {
            return FALSE;
        } else {
            return TRUE;
        }
    }
}
#endif /* CFG_RESET_BUTTON_INCLUDED */

#if defined(CFG_SWITCH_L2_ADDR_INCLUDED)

/*!
 * \brief Add an entry in the L2 address table.
 *
 * \param [in] l2addr L2 address entry
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_l2_addr_add(board_l2_addr_t *l2addr)
{
    int rv = SYS_OK;
    l2x_entry_t l2x;
    uint8 unit, lport;
    uint16 index;

    l2x.vlan_id = l2addr->vid;
    board_uport_to_lport(l2addr->uport, &unit, &lport);
    l2x.port = lport;
    l2x.is_static = (l2addr->flags & BOARD_L2_STATIC) ? TRUE : FALSE;
    sal_memcpy(l2x.mac_addr, l2addr->mac, 6);
    rv = bcm5607x_l2_op(0, &l2x, SC_OP_L2_LOOKUP_CMD, &index);
    if (rv == SYS_OK) {
        return SYS_ERR_EXISTS;
    }
    rv = bcm5607x_l2_op(0, &l2x, SC_OP_L2_INS_CMD, NULL);

    return rv;
}

/*!
 * \brief Delete an entry from the L2 address table.
 *
 * \param [in] l2addr L2 address entry
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_l2_addr_delete(board_l2_addr_t *l2addr)
{
    int rv = SYS_OK;
    l2x_entry_t l2x;
    uint8 unit, lport;
    uint16 index;

    l2x.vlan_id = l2addr->vid;
    board_uport_to_lport(l2addr->uport, &unit, &lport);
    l2x.port = lport;
    sal_memcpy(l2x.mac_addr, l2addr->mac, 6);
    rv = bcm5607x_l2_op(0, &l2x, SC_OP_L2_LOOKUP_CMD, &index);
    if (rv == SYS_ERR_NOT_FOUND) {
        return rv;
    }
    rv = bcm5607x_l2_op(0, &l2x, SC_OP_L2_DEL_CMD, NULL);

    return rv;
}

#define L2_AGE_TIMEOUT_COUNT 150

/*!
 * \brief Delete entries from the L2 address table by user port number.
 *
 * \param [in] uport User port number
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_l2_addr_delete_by_port(uint16 uport)
{
    sys_error_t rv = SYS_OK;
    uint8 unit = 0, lport;
    int i;
    L2_AGE_TIMERr_t l2_age_timer;
    PER_PORT_AGE_CONTROL_64r_t per_port_age_control_64;

    rv |= READ_L2_AGE_TIMERr(unit, l2_age_timer);
    L2_AGE_TIMERr_AGE_ENAf_SET(l2_age_timer, 0x0);
    rv |= WRITE_L2_AGE_TIMERr(unit, l2_age_timer);

    board_uport_to_lport(uport, &unit, &lport);

    PER_PORT_AGE_CONTROL_64r_CLR(per_port_age_control_64);
    PER_PORT_AGE_CONTROL_64r_PORT_NUMf_SET(per_port_age_control_64, lport);
    PER_PORT_AGE_CONTROL_64r_PPA_MODEf_SET(per_port_age_control_64, 0x0);
    PER_PORT_AGE_CONTROL_64r_STARTf_SET(per_port_age_control_64, 0x1);
    rv |= WRITE_PER_PORT_AGE_CONTROL_64r(unit, per_port_age_control_64);

    for (i = 0; i < L2_AGE_TIMEOUT_COUNT; i++) {
        sal_usleep(100000);
        rv |= READ_PER_PORT_AGE_CONTROL_64r(unit, per_port_age_control_64);
        if (PER_PORT_AGE_CONTROL_64r_COMPLETEf_GET(per_port_age_control_64)) {
            break;
        }
    }

    rv |= READ_L2_AGE_TIMERr(unit, l2_age_timer);
    L2_AGE_TIMERr_AGE_ENAf_SET(l2_age_timer, 0x1);
    rv |= WRITE_L2_AGE_TIMERr(unit, l2_age_timer);

    if (i == L2_AGE_TIMEOUT_COUNT) {
        return SYS_ERR_TIMEOUT;
    }

    return SYS_OK;
}

/*!
 * \brief Delete entries from the L2 address table by VLAN ID.
 *
 * \param [in] vid VLAN ID
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_l2_addr_delete_by_vlan(uint16 vid)
{
    sys_error_t rv = SYS_OK;
    uint8 unit = 0;
    int i;
    L2_AGE_TIMERr_t l2_age_timer;
    PER_PORT_AGE_CONTROL_64r_t per_port_age_control_64;

    rv |= READ_L2_AGE_TIMERr(unit, l2_age_timer);
    L2_AGE_TIMERr_AGE_ENAf_SET(l2_age_timer, 0x0);
    rv |= WRITE_L2_AGE_TIMERr(unit, l2_age_timer);

    PER_PORT_AGE_CONTROL_64r_CLR(per_port_age_control_64);
    PER_PORT_AGE_CONTROL_64r_VLAN_IDf_SET(per_port_age_control_64, vid);
    PER_PORT_AGE_CONTROL_64r_PPA_MODEf_SET(per_port_age_control_64, 0x1);
    PER_PORT_AGE_CONTROL_64r_STARTf_SET(per_port_age_control_64, 0x1);
    rv |= WRITE_PER_PORT_AGE_CONTROL_64r(unit, per_port_age_control_64);

    for (i = 0; i < L2_AGE_TIMEOUT_COUNT; i++) {
        sal_usleep(100000);
        rv |= READ_PER_PORT_AGE_CONTROL_64r(unit, per_port_age_control_64);
        if (PER_PORT_AGE_CONTROL_64r_COMPLETEf_GET(per_port_age_control_64)) {
            break;
        }
    }

    rv |= READ_L2_AGE_TIMERr(unit, l2_age_timer);
    L2_AGE_TIMERr_AGE_ENAf_SET(l2_age_timer, 0x1);
    rv |= WRITE_L2_AGE_TIMERr(unit, l2_age_timer);

    if (i == L2_AGE_TIMEOUT_COUNT) {
        return SYS_ERR_TIMEOUT;
    }

    return SYS_OK;
}

/*!
 * \brief Delete entries from the L2 address table by TRUNK ID.
 *
 * \param [in] lagid TRUNK ID
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_l2_addr_delete_by_trunk(uint8 lagid)
{
    sys_error_t rv = SYS_OK;
    uint8 unit = 0;
    int i;
    L2_AGE_TIMERr_t l2_age_timer;
    PER_PORT_AGE_CONTROL_64r_t per_port_age_control_64;

    rv |= READ_L2_AGE_TIMERr(unit, l2_age_timer);
    L2_AGE_TIMERr_AGE_ENAf_SET(l2_age_timer, 0x0);
    rv |= WRITE_L2_AGE_TIMERr(unit, l2_age_timer);

    PER_PORT_AGE_CONTROL_64r_CLR(per_port_age_control_64);
    PER_PORT_AGE_CONTROL_64r_TGIDf_SET(per_port_age_control_64, lagid-1);
    PER_PORT_AGE_CONTROL_64r_Tf_SET(per_port_age_control_64, 0x1);
    PER_PORT_AGE_CONTROL_64r_PPA_MODEf_SET(per_port_age_control_64, 0x0);
    PER_PORT_AGE_CONTROL_64r_STARTf_SET(per_port_age_control_64, 0x1);
    rv |= WRITE_PER_PORT_AGE_CONTROL_64r(unit, per_port_age_control_64);

    for (i = 0; i < L2_AGE_TIMEOUT_COUNT; i++) {
        sal_usleep(100000);
        rv |= READ_PER_PORT_AGE_CONTROL_64r(unit, per_port_age_control_64);
        if (PER_PORT_AGE_CONTROL_64r_COMPLETEf_GET(per_port_age_control_64)) {
            break;
        }
    }

    rv |= READ_L2_AGE_TIMERr(unit, l2_age_timer);
    L2_AGE_TIMERr_AGE_ENAf_SET(l2_age_timer, 0x1);
    rv |= WRITE_L2_AGE_TIMERr(unit, l2_age_timer);

    if (i == L2_AGE_TIMEOUT_COUNT) {
        return SYS_ERR_TIMEOUT;
    }

    return SYS_OK;
}

/*!
 * \brief Delete all entries from the L2 address table.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_l2_addr_delete_all(void)
{
    sys_error_t rv = SYS_OK;
    uint8 unit = 0;
    int i;
    L2_AGE_TIMERr_t l2_age_timer;
    PER_PORT_AGE_CONTROL_64r_t per_port_age_control_64;

    rv |= READ_L2_AGE_TIMERr(unit, l2_age_timer);
    L2_AGE_TIMERr_AGE_ENAf_SET(l2_age_timer, 0x0);
    rv |= WRITE_L2_AGE_TIMERr(unit, l2_age_timer);

    PER_PORT_AGE_CONTROL_64r_CLR(per_port_age_control_64);
    PER_PORT_AGE_CONTROL_64r_PPA_MODEf_SET(per_port_age_control_64, 0x3);
    PER_PORT_AGE_CONTROL_64r_STARTf_SET(per_port_age_control_64, 0x1);
    rv |= WRITE_PER_PORT_AGE_CONTROL_64r(unit, per_port_age_control_64);

    for (i = 0; i < L2_AGE_TIMEOUT_COUNT; i++) {
        sal_usleep(100000);
        rv |= READ_PER_PORT_AGE_CONTROL_64r(unit, per_port_age_control_64);
        if (PER_PORT_AGE_CONTROL_64r_COMPLETEf_GET(per_port_age_control_64)) {
            break;
        }
    }

    rv |= READ_L2_AGE_TIMERr(unit, l2_age_timer);
    L2_AGE_TIMERr_AGE_ENAf_SET(l2_age_timer, 0x1);
    rv |= WRITE_L2_AGE_TIMERr(unit, l2_age_timer);

    if (i == L2_AGE_TIMEOUT_COUNT) {
        return SYS_ERR_TIMEOUT;
    }

    return SYS_OK;
}

/*!
 * \brief Lookup entry from the L2 address table.
 *
 * \param [in] l2addr L2 address entry
 * \param [out] index L2 uni table
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_l2_addr_lookup(board_l2_addr_t *l2addr, uint16 *index)
{
    int rv = SYS_OK;
    l2x_entry_t l2x;
    uint8 unit, lport;

    l2x.vlan_id = l2addr->vid;
    board_uport_to_lport(l2addr->uport, &unit, &lport);
    l2x.port = lport;
    sal_memcpy(l2x.mac_addr, l2addr->mac, 6);
    rv = bcm5607x_l2_op(0, &l2x, SC_OP_L2_LOOKUP_CMD, index);

    return rv;
}

/*
 * L2 ENTRY TABLE :
 * VLAN_ID [Bit 14 : 3]
 * MAC_ADDR [Bit 62 : 15]
 * PORT_NUM [Bit 69 : 63]
 * STATIC_BIT [Bit 100]
 * VALID [Bit 102]
 */

static int l2_addr_index = 0;

/*!
 * \brief Get first entry from the L2 address table.
 *
 * \param [out] l2addr L2 address entry
 * \param [out] index L2 uni table
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_l2_addr_get_first(board_l2_addr_t *l2addr, uint16 *index)
{
    int rv = SYS_OK;
    uint32 i;
    uint8 unit = 0, lport;
    uint32 mac_addr[2] = { 0x0,   0x0 };
    L2_ENTRYm_t l2_entry;

    l2_addr_index = 0;

    for (i=0 ; i< fl_sw_info.l2_entry_size ; i++) {
        rv |= READ_L2_ENTRYm(unit, i, l2_entry);
        if (L2_ENTRYm_VALIDf_GET(l2_entry)) {
            L2_ENTRYm_L2_MAC_ADDRf_GET(l2_entry, mac_addr);
            l2addr->mac[5] = mac_addr[0] & 0xFF;
            l2addr->mac[4] = (mac_addr[0] >> 8) & 0xFF;
            l2addr->mac[3] = (mac_addr[0] >> 16) & 0xFF;
            l2addr->mac[2] = (mac_addr[0] >> 24) & 0xFF;
            l2addr->mac[1] = mac_addr[1] & 0xFF;
            l2addr->mac[0] = (mac_addr[1] >> 8) & 0xFF;
            l2addr->vid = L2_ENTRYm_L2_VLAN_IDf_GET(l2_entry);
            lport = L2_ENTRYm_L2_PORT_NUMf_GET(l2_entry);
            if (L2_ENTRYm_L2_Tf_GET(l2_entry)) {
                l2addr->is_trunk = TRUE;
            } else {
                l2addr->is_trunk = FALSE;
            }
            if (l2addr->mac[0] & 0x1) {
                /* multicast */
                l2addr->mcidx = L2_ENTRYm_L2_L2MC_PTRf_GET(l2_entry);
            } else {
                /* unicast */
                if (l2addr->is_trunk) {
                    l2addr->tgid = L2_ENTRYm_L2_TGIDf_GET(l2_entry) + 1;
                } else {
                    board_lport_to_uport(unit, lport, &(l2addr->uport));
                }
            }
            if (L2_ENTRYm_L2_STATIC_BITf_GET(l2_entry)) {
                l2addr->flags = BOARD_L2_STATIC;
            } else {
                l2addr->flags = 0x0;
            }

            l2_addr_index = i;
            *index = (uint16)l2_addr_index;
            break;
        }

        POLL();
    }

    if (i == fl_sw_info.l2_entry_size) {
        return SYS_ERR_NOT_FOUND;
    }

    return rv ? SYS_ERR : SYS_OK;
}

/*!
 * \brief Get next entry from the L2 address table. This function needs to go with board_l2_addr_get_first.
 *
 * \param [out] l2addr L2 address entry
 * \param [out] index L2 uni table
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_l2_addr_get_next(board_l2_addr_t *l2addr, uint16 *index)
{
    int rv = SYS_OK;
    int i;
    uint8 unit = 0, lport;
    uint32 mac_addr[2] = { 0x0,   0x0 };
    L2_ENTRYm_t l2_entry;

    for (i=(l2_addr_index+1) ; i < fl_sw_info.l2_entry_size ; i++) {
        rv |= READ_L2_ENTRYm(unit, i, l2_entry);
        if (L2_ENTRYm_VALIDf_GET(l2_entry)) {
            L2_ENTRYm_L2_MAC_ADDRf_GET(l2_entry, mac_addr);
            l2addr->mac[5] = mac_addr[0] & 0xFF;
            l2addr->mac[4] = (mac_addr[0] >> 8) & 0xFF;
            l2addr->mac[3] = (mac_addr[0] >> 16) & 0xFF;
            l2addr->mac[2] = (mac_addr[0] >> 24) & 0xFF;
            l2addr->mac[1] = mac_addr[1] & 0xFF;
            l2addr->mac[0] = (mac_addr[1] >> 8) & 0xFF;
            l2addr->vid = L2_ENTRYm_L2_VLAN_IDf_GET(l2_entry);
            lport = L2_ENTRYm_L2_PORT_NUMf_GET(l2_entry);
            if (L2_ENTRYm_L2_Tf_GET(l2_entry)) {
                l2addr->is_trunk = TRUE;
            } else {
                l2addr->is_trunk = FALSE;
            }
            if (l2addr->mac[0] & 0x1) {
                /* multicast */
                l2addr->mcidx = L2_ENTRYm_L2_L2MC_PTRf_GET(l2_entry);
            } else {
                /* unicast */
                if (l2addr->is_trunk) {
                    l2addr->tgid = L2_ENTRYm_L2_TGIDf_GET(l2_entry) + 1;
                } else {
                    board_lport_to_uport(unit, lport, &(l2addr->uport));
                }
            }
            if (L2_ENTRYm_L2_STATIC_BITf_GET(l2_entry)) {
                l2addr->flags = BOARD_L2_STATIC;
            } else {
                l2addr->flags = 0x0;
            }

            l2_addr_index = i;
            *index = (uint16)l2_addr_index;
            break;
        }

        POLL();
    }

    if (i == fl_sw_info.l2_entry_size) {
        return SYS_ERR_NOT_FOUND;
    }

    return rv ? SYS_ERR : SYS_OK;

}

/*!
 * \brief Get last L2 entry from the L2 address table.
 *
 * \param [out] l2addr L2 address entry
 * \param [out] index L2 uni table
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_l2_addr_get_last(board_l2_addr_t *l2addr, uint16 *index)
{
    int rv = SYS_OK;
    int i;
    uint8 unit = 0, lport;
    uint32 mac_addr[2] = { 0x0,   0x0 };
    L2_ENTRYm_t l2_entry;

    l2_addr_index = 0;

    for (i=(fl_sw_info.l2_entry_size - 1) ; i>=0 ; i--) {
        rv |= READ_L2_ENTRYm(unit, i, l2_entry);
        if (L2_ENTRYm_VALIDf_GET(l2_entry)) {
            L2_ENTRYm_L2_MAC_ADDRf_GET(l2_entry, mac_addr);
            l2addr->mac[5] = mac_addr[0] & 0xFF;
            l2addr->mac[4] = (mac_addr[0] >> 8) & 0xFF;
            l2addr->mac[3] = (mac_addr[0] >> 16) & 0xFF;
            l2addr->mac[2] = (mac_addr[0] >> 24) & 0xFF;
            l2addr->mac[1] = mac_addr[1] & 0xFF;
            l2addr->mac[0] = (mac_addr[1] >> 8) & 0xFF;
            l2addr->vid = L2_ENTRYm_L2_VLAN_IDf_GET(l2_entry);
            lport = L2_ENTRYm_L2_PORT_NUMf_GET(l2_entry);
            if (L2_ENTRYm_L2_Tf_GET(l2_entry)) {
                l2addr->is_trunk = TRUE;
            } else {
                l2addr->is_trunk = FALSE;
            }
            if (l2addr->mac[0] & 0x1) {
                /* multicast */
                l2addr->mcidx = L2_ENTRYm_L2_L2MC_PTRf_GET(l2_entry);
            } else {
                /* unicast */
                if (l2addr->is_trunk) {
                    l2addr->tgid = L2_ENTRYm_L2_TGIDf_GET(l2_entry) + 1;
                } else {
                    board_lport_to_uport(unit, lport, &(l2addr->uport));
                }
            }
            if (L2_ENTRYm_L2_STATIC_BITf_GET(l2_entry)) {
                l2addr->flags = BOARD_L2_STATIC;
            } else {
                l2addr->flags = 0x0;
            }

            l2_addr_index = i;
            *index = (uint16)l2_addr_index;
            break;
        }

        POLL();
    }

    if (i < 0) {
        return SYS_ERR_NOT_FOUND;
    }

    return rv ? SYS_ERR : SYS_OK;

}
#endif /* CFG_SWITCH_L2_ADDR_INCLUDED */

#ifdef CFG_SWITCH_SYNCE_INCLUDED
/*!
 * \brief Get syncE clock source control option.
 * \param [in] clk_src_config Clock source config.
 * \param [in] control SyncE source.
 *  \li 0 = bcmTimeSynceClockSourceControlSquelch.
 *  \li 1 = bcmTimeSynceClockSourceControlFrequency.
 * \param value control value.
 *
 * \retval SYS_OK No errors.
 */

sys_error_t
board_time_synce_clock_source_control_get(
                          bcm_time_synce_clock_source_config_t *clk_src_config,
                          bcm_time_synce_clock_source_control_t control,
                          int *value)
{
    return bcm5607x_time_synce_clock_source_control_get(0, clk_src_config,
                                                        control, value);
}

/*!
 * \brief Set syncE clock source squelch option.
 * \param [in] clk_src_config Clock source config.
 * \param [in] control SyncE source.
 *  \li 0 = bcmTimeSynceClockSourceControlSquelch.
 *  \li 1 = bcmTimeSynceClockSourceControlFrequency.
 * \param [in] value Control value.
 *
 * \retval SYS_OK No errors.
 */

sys_error_t
board_time_synce_clock_source_control_set(
                             bcm_time_synce_clock_source_config_t *clk_src_config,
                             bcm_time_synce_clock_source_control_t control,
                             int value)
{
    return bcm5607x_time_synce_clock_source_control_set(0, clk_src_config,
                             control, value);
}

#endif /* CFG_SWITCH_SYNCE_INCLUDED */

#ifdef CFG_SWITCH_TIMESYNC_INCLUDED
/*!
 * \brief Set timesync configurations for the port.
 *
 * \param [in] uport Port number.
 * \param [in] config_count Count of timesync configurations.
 * \param [in] config_array Pointer to timesync configurations.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_timesync_config_set(uint16 uport,
                            int config_count, bcm_port_timesync_config_t *config_array)
{
    uint8 unit, lport;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    return bcm5607x_port_timesync_config_set(unit, lport, config_count, config_array);
}

/*!
 * \brief Get timesync configurations for the port.
 *
 * \param [in] uport Port number.
 * \param [in] array_size Required Count of timesync configurations.
 * \param [in/out] config_array Pointer to timesync configurations.
 * \param [out] array_count Pointer to timesync configuration array count.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_timesync_config_get(uint16 uport, int array_size,
                                bcm_port_timesync_config_t *config_array,
                                int *array_count)
{
    uint8 unit, lport;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    return bcm5607x_port_timesync_config_get(unit, lport, array_size, config_array, array_count);
}

/*!
 * \brief Set PHY/PCS timesync configurations for the port.
 *
 * \param [in] uport Port number.
 * \param [in] en value for enable/disable.
 *    \li TRUE = Enable PHY/PCS timesync.
 *    \li FALSE = Disable PHY/PCS timesync.
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_phy_timesync_enable_set(uint16 uport, int en)
{
    uint8 unit, lport;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    return bcm5607x_port_phy_timesync_enable_set(unit, lport, en);
}

/*!
 * \brief Get PHY/PCS timesync configurations for the port.
 *
 * \param [in] uport Port number.
 * \param [out] en value for enable/disable.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_phy_timesync_enable_get(uint16 uport, int *en)
{
    uint8 unit, lport;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    return bcm5607x_port_phy_timesync_enable_get(unit, lport, en);
}

/*!
 * \brief Set timesync PHY/PCS features for the port.
 *
 * \param [in] uport Port number.
 * \param [in] type Port feature enumerator.
 *    \li bcmPortControlPhyTimesyncTimestampOffset
 *    \li bcmPortControlPhyTimesyncTimestampAdjust
 *
 * \param [in] value Value to configure for the feature.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_control_phy_timesync_set(uint16 uport,
                            bcm_port_control_phy_timesync_t type,
                            uint64 value)
{
    uint8 unit, lport;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    return bcm5607x_port_control_phy_timesync_set(unit, lport, type, value);
}

/*!
 * \brief Get timesync PHY/PCS features for the port.
 *
 * \param [in] uport Port number.
 * \param [in] type Port feature enumerator.
 * \param [out] value Value configured for the feature.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_control_phy_timesync_get(uint16 uport,
                            bcm_port_control_phy_timesync_t type,
                            uint64 *value)
{
    /* Not implemented in PHYMOD */
    return SYS_ERR_UNAVAIL;
}

/*!
 * \brief Get 1588 packet's transmit information form the port.
 *
 * \param [in] uport Port number.
 * \param [in/out] tx_info Pointer to structure to get timesync tx informatio.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_port_timesync_tx_info_get(uint16 uport,
                            bcm_port_timesync_tx_info_t *tx_info)
{
    uint8 unit, lport;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    return bcm5607x_port_timesync_tx_info_get(unit, lport, tx_info);
}
#endif /* CFG_SWITCH_TIMESYNC_INCLUDED */

/*!
 * \brief Get chip devID and revID by unit.
 *
 * \param [in] unit Unit number.
 * \param [out] dev The chip devID.
 * \param [out] rev The chip devID.
 *
 * \retval SYS_OK No errors.
 */
sys_error_t
board_chip_revision(uint8 unit, uint16 *dev, uint16 *rev)
{
    SOC_IF_ERROR_RETURN(bcm5607x_chip_revision(unit, dev, rev));

    return SYS_OK;
}
