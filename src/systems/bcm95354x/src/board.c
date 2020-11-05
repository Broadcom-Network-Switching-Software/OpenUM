/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#include "bsp_config.h"
#include "brdimpl.h"
#include "utils/ports.h"

#ifdef  SOC_PORT_L2P_MAPPING
#undef  SOC_PORT_L2P_MAPPING
#define SOC_PORT_L2P_MAPPING  SOC_PORT_L2P_MAPPING_VALID
#endif

extern soc_switch_t soc_switch_bcm5354x;
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
/* L2MC size in word */
#define L2MC_ENTRY_SIZE  ((L2MCm_MAX+1+3)/4)

static uint32 mcindex[L2MC_ENTRY_SIZE];

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

/** 
 * <b>Description:</b> Get the board's name.
 * @return Board name.
 */
const char *
board_name(void)
{
    return um_boardname;
}

/**
 * Get the number of chip internal logical ports (lports)
 *
 * @return the number of chip internal logical  ports
 */

uint8
board_lport_count(void)
{
	return bcm5354x_port_count(0);
}

/** 
 * <b>Description:</b> Get the port count of user ports.
 * @return Port count.
 */
uint8
board_uport_count(void)
{
#ifdef CFG_WEB_LARGE_PORT_COUNT
	return BOARD_MAX_NUM_OF_PORTS;
#else
    return bcm5354x_port_count(0);
#endif
}
/**
 * Map a user port to a chip-internal logical port.
 *
 * @param uport (IN)- The user port
 * @param unit (OUT) - the chip unit number of the lport
 * @param lport (OUT) - the chip-internal logical port
 * @return sys_error_t
 *     SYS_OK : there is no error
 *     SYS_ERR_PARAMETER : fail, because parameter is invalid
 */
sys_error_t
board_uport_to_lport(uint16 uport, uint8 *unit, uint8 *lport)
{

    const int *u2l_mapping;
    uint8 index, count;
    uint8 active_port_count = 0;
    int port;
    switch (wh2_sw_info.devid) {
       default:
           /* Assign uport to lport mapping in lport order automaticly*/  
           count = board_uport_count();
           if (u2l_mapping_auto == NULL) {
               u2l_mapping_auto = sal_malloc(sizeof(int) * board_uport_count());
               index = 0;

                SOC_PPORT_ITER_1(port) {
                    if ((SOC_PORT_L2P_MAPPING(SOC_PORT_P2L_MAPPING(port)) != -1)){
                        u2l_mapping_auto[index] = SOC_PORT_P2L_MAPPING(port); 
                        index++;
                        if (index >= count) {
                            break;
                        }
                    }   
                }
                
                SOC_PPORT_ITER_2(port) {
                    if ((SOC_PORT_L2P_MAPPING(SOC_PORT_P2L_MAPPING(port)) != -1)){
                        u2l_mapping_auto[index] = SOC_PORT_P2L_MAPPING(port); 
                        index++;
                        if (index >= count) {
                            break;
                        }
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


/**
 * Map a chip-internal logical port to a user port.
 *
 * @param lport (IN)- The chip-internal logical port
 * @param unit (IN) - the chip unit number of the lport
 * @param uport (OUT) - the user port
 * @return sys_error_t
 *     SYS_OK : there is no error
 *     SYS_ERR_PARAMETER : fail, because parameter is invalid
 */

sys_error_t
board_lport_to_uport(uint8 unit, uint8 lport, uint16 *uport)
{

   const int *u2l_mapping;
   uint8 index, count;
   uint8 active_port_count = 0;
   int port;

   switch (wh2_sw_info.devid) {
       default:
           count = board_uport_count();
           /* Assign uport to lport mapping in lport order automaticly*/
           if (u2l_mapping_auto == NULL) {
               u2l_mapping_auto = sal_malloc(sizeof(int) * board_uport_count());
               index = 0;

                SOC_PPORT_ITER_1(port) {
                   u2l_mapping_auto[index] = SOC_PORT_P2L_MAPPING(port); 
                   index++;
                   if (index >= count) {
                       break;
                   }
                }
                
                SOC_PPORT_ITER_2(port) {
                   u2l_mapping_auto[index] = SOC_PORT_P2L_MAPPING(port); 
                   index++;
                   if (index >= count) {
                       break;
                   }
                }
           }
           u2l_mapping = (const int *) u2l_mapping_auto;  
       break;
   }

    if (uport == NULL || lport < BCM5354X_LPORT_MIN || lport > BCM5354X_LPORT_MAX || unit > 0) {
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

/**
 * Map the user port bitmap arrary (uplist)  to the chip-internal logical port bitmap (lpbmp) on selected chip unit .
 *
 * @param uplist (IN)- The user port bit map array  which may cover many chips
 * @param unit (IN) - the selected chip unit number
 * @param lpbmp (OUT) - the chip-internal logical port bit map
 * @return  sys_error_t
 *     SYS_OK : there is no error
 *     SYS_ERR_PARAMETER : fail, because parameter is invalid
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


/**
 * Map the chip-internal logical port bit map (lpbmp) to a user port port bit map array (uplist) on selected chip unit.
 *
 *
 * @param unit (IN)- the selected chip unit number
 * @param lpbmp (IN) - The chip-internal logical port bit map
 * @param uplist (OUT) - The user port bit map array which may cover many chips
 * @return sys_error_t
 *             SYS_OK : there is no error
 *             SYS_ERR_PARAMETER : fail, because parameter is invalid
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

/** 
 * <b>Description:</b> Get SoC by unit.
 * @param unit - Unit number.
 * @return Pointer to switch device (e.g., soc_switch_bcm5340x).
 */

soc_switch_t *
board_get_soc_by_unit(uint8 unit)
{
    if (unit > 0) {
        return NULL;
    }
    return &soc_switch_bcm5354x;
}
#if !CONFIG_WOLFHOUND2_ROMCODE
 /** 
 * <b>Description:</b> Get the link status for the specific port.
 * @param uport - Port number.
 * @param link - (OUT) link status:
 *  \li TRUE = Link up.
 *  \li FALSE = Link down.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */

sys_error_t
board_get_port_link_status(uint16 uport, BOOL *link)
{
    uint8 unit, lport;
    sys_error_t r;
    r = board_uport_to_lport(uport, &unit, &lport);
    if (r != SYS_OK) {
        return r;
    }

    return (*soc_switch_bcm5354x.link_status)(unit, lport, link);
}
#endif /* !CONFIG_WOLFHOUND2_ROMCODE */

/** 
 * <b>Description:</b> Set the board to reset.
 * @param param - 0 or input data.
 * @return VOID
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

/** 
 * <b>Description:</b> Get the firmware version.
 * @param major - Major number.
 * @param minor - Minor number.
 * @param eco - Eco number.
 * @param misc - Not used.
 * @return VOID
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

/** 
 * <b>Description:</b> Transmit the packet.
 * @param pkt - Pointer to packet buffer.
 * @param cbk - Callback.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_tx(sys_pkt_t *pkt, BOARD_TX_CALLBACK cbk)
{
    return brdimpl_tx(pkt, cbk);
}

void
board_rxtx_stop(void)
{
    bcm5354x_rxtx_stop();
    return;
}

#endif /* CFG_RXTX_SUPPORT_ENABLED */

#if CFG_FLASH_SUPPORT_ENABLED
/* Get flash device object */

/** 
 * <b>Description:</b> Get the flash device object.
 * @return Pointer to flash device (e.g., n25q256_dev).
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
    sal_printf("Flash image is %d bytes, chksum %04X, version %c.%c.%c for board %s\n",
                size, hdrchksum, hdr->majver, hdr->minver, hdr->ecover, hdr->boardname);
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

    if (sal_memcmp(hdr->seal, UM_IMAGE_HEADER_SEAL, sizeof(hdr->seal)) != 0) {
#if CFG_CONSOLE_ENABLED && !defined(CFG_DUAL_IMAGE_INCLUDED)
        sal_printf("Invalid header seal.  This is not a valid image.\n");
#endif /* CFG_CONSOLE_ENABLED */
        return FALSE;
	}

#if CFG_CONSOLE_ENABLED
    sal_printf("Flash image is version %c.%c.%c for board %s\n",
                hdr->majver, hdr->minver, hdr->ecover, hdr->boardname);
#endif /* CFG_CONSOLE_ENABLED */
    if (sal_strcmp(board_name(), (const char*)hdr->boardname) != 0) {
#if CFG_CONSOLE_ENABLED
        sal_printf("This image is not appropriate for board type '%s'\n",board_name());
#endif /* CFG_CONSOLE_ENABLED */
        return FALSE;
	}

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
        /* Firmware notify looader to do firmware upgrade */
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
/** 
 * <b>Description:</b> Set the VLAN type.
 * @param type - One of the following:
 *  \li VT_PORT_BASED
 *  \li VT_DOT1Q
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_vlan_type_set(vlan_type_t type)
{
    return brdimpl_vlan_type_set(type);
}


/** 
 * <b>Description:</b> Get the VLAN type.
 * @param type - (OUT) one of the following:
 *  \li VT_PORT_BASED
 *  \li VT_DOT1Q
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_vlan_type_get(vlan_type_t *type)
{
    return brdimpl_vlan_type_get(type);
}


/** 
 * <b>Description:</b> Create a new VLAN by the VLAN ID.
 * @param vlan_id - VLAN ID number.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_vlan_create(uint16 vlan_id)
{
    return brdimpl_vlan_create(vlan_id);
}

/** 
 * <b>Description:</b> Destroy the selected VLAN.
 * @param vlan_id - VLAN ID number.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_vlan_destroy(uint16 vlan_id)
{
    return brdimpl_vlan_destroy(vlan_id);
}

/** 
 * <b>Description:</b> Set the port-based VLAN members for a given VLAN ID.
 * @param vlan_id - VLAN ID number.
 * @param uplist - VLAN members port list number.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_pvlan_port_set(uint16  vlan_id, uint8 *uplist)
{
    return brdimpl_pvlan_port_set(vlan_id, uplist);
}

 
/** 
 * <b>Description:</b> Get the port-based VLAN members for a given VLAN ID.
 * @param vlan_id - VLAN ID number.
 * @param uplist - (OUT) VLAN members port list number.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_pvlan_port_get(uint16  vlan_id, uint8 *uplist)
{
    return brdimpl_pvlan_port_get(vlan_id, uplist);
}
 
 /** 
 * <b>Description:</b> Get the egress port list for a given port.
 * @param uport - Port number.
 * @param uplist - (OUT) egress mask of the port list.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_pvlan_egress_get(uint16 uport, uint8 *uplist)
{
    return brdimpl_pvlan_egress_get(uport, uplist);
}

/** 
 * <b>Description:</b> Set the PVID for a given port.
 * @param uport - Port number.
 * @param vlan_id - PVID number.
 * @return SYS_OK
 * \n      Operation completed successfully.
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

/** 
 * <b>Description:</b> Get the PVID for a given port.
 * @param uport - Port number.
 * @param vlan_id - (OUT) PVID number.
 * @return SYS_OK
 * \n      Operation completed successfully.
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

/** 
 * <b>Description:</b> Set IEEE 802.1Q VLAN members and tag members by the VLAN ID.
 * @param vlan_id - QVLAN ID number.
 * @param uplist - VLAN members port list number.
 * @param tag_uplist - VLAN tagged members port list number.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_qvlan_port_set(uint16  vlan_id, uint8 *uplist, uint8 *tag_uplist)
{
    return brdimpl_qvlan_port_set(vlan_id, uplist, tag_uplist);
}

/** 
 * <b>Description:</b> Get the IEEE 802.1Q VLAN members and tag members by the VLAN ID.
 * @param vlan_id - QVLAN ID number.
 * @param uplist - (OUT) VLAN members port list number.
 * @param tag_uplist - (OUT) VLAN tagged members port list number.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_qvlan_port_get(uint16  vlan_id, uint8 *uplist, uint8 *tag_uplist)
{
    return brdimpl_qvlan_port_get(vlan_id, uplist, tag_uplist);
}

/** 
 * <b>Description:</b> Get the count of IEEE 802.1Q VLAN entry.
 * @return VLAN count.
 */
uint16
board_vlan_count(void)
{
    return brdimpl_vlan_count();
}

/** 
 * <b>Description:</b> Get the IEEE 802.1Q VLAN ID, members, and tag members by index
 * @param index - Index number.
 * @param vlan_id - (OUT) QVLAN ID number.
 * @param uplist - (OUT) VLAN members port list number.
 * @param tag_uplist - (OUT) VLAN tagged members port list number.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_qvlan_get_by_index(uint16  index, uint16 *vlan_id, uint8 *uplist, uint8 *tag_uplist, BOOL get_uplist)
{
return brdimpl_qvlan_get_by_index(index, vlan_id, uplist, tag_uplist, get_uplist);
}
#endif /* CFG_SWITCH_VLAN_INCLUDED */

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
    } else if (SOC_PORT_SPEED_STATUS(lport) == 10000) {
        *mode = PM_10000MB;
    } else {
        *mode = PM_AUTO;
    }

    return rv;
}

 /** 
 * <b>Description:</b> Run cable diagnostics on the port.
 * @param uport - Port number.
 * @param status - (OUT) cable diag status structure.
 * @return SYS_OK
 * \n      Operation completed successfully.
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
/** 
 * <b>Description:</b> Set the lag to enable or disable.
 * @param enable:
 *  \li TRUE = Enable lag.
 *  \li FALSE = Disable lag.
 * @return SYS_OK
 * \n      Operation completed successfully.
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
                            (*soc_switch_bcm5354x.link_status)(0, lport, &link));
                        if (!link) {
                            PBMP_PORT_REMOVE(hw_lpbmp, lport);
                        }
                    }
                }

                SOC_IF_ERROR_RETURN(
                    bcm5354x_lag_group_set(0, (i - 1), hw_lpbmp));
            } else {
                /* remove lag setting from HW */
                PBMP_CLEAR(hw_lpbmp);     
                SOC_IF_ERROR_RETURN(bcm5354x_lag_group_set(0, (i - 1), hw_lpbmp));
            }
        }
        lag_enable = enable;
    }
    return SYS_OK;
}

 /** 
 * <b>Description:</b> Get the lag status.
 * @param enable - (OUT)
 *  \li TRUE = Lag is enabled
 *  \li FALSE = Lag is disabled.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */

void
board_lag_get(uint8 *enable)
{
    *enable = lag_enable;
}

 /** 
 * <b>Description:</b> Set the lag group members.
 * @param lagid - Lag ID number.
 * @param enable:
 *  \li TRUE = Enable the lag group.
 *  \li FALSE = Disable the lag group.
 * @param uplist - Lag members list. 
 * @return SYS_OK
 * \n      Operation completed successfully.
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
                    (*soc_switch_bcm5354x.link_status)(0, lport, &link));
                if (!link) {
                    PBMP_PORT_REMOVE(hw_lpbmp, lport);
                }
            }
        }
    } else {
        PBMP_CLEAR(hw_lpbmp);
    }

    /* HW setting */
    SOC_IF_ERROR_RETURN(bcm5354x_lag_group_set(0, (lagid - 1), hw_lpbmp));

    /* lag SW database setting */
    lag_group[lagid-1].enable = enable;
    PBMP_ASSIGN(lag_group[lagid-1].lpbmp, lpbmp);
    return SYS_OK;
}

 /** 
 * <b>Description:</b> Get the lag group members.
 * @param lagid - Lag ID number.
 * @param enable - (OUT)
 *  \li TRUE = Lag group is enabled.
 *  \li FALSE = Lag group is disabled.
 * @param uplist - (OUT) Lag members list. 
 * @return SYS_OK
 * \n      Operation completed successfully.
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

/** 
 * <b>Description:</b> Get the max lag numbers.
 * @param num - (OUT) numbers.
 * @return SYS_OK
 * \n      Operation completed successfully.
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
                bcm5354x_lag_group_get(0, (i - 1), &hw_lpbmp);
                if (link) {
                    PBMP_PORT_ADD(hw_lpbmp, lport);
                } else {
                    PBMP_PORT_REMOVE(hw_lpbmp, lport);
                }
                bcm5354x_lag_group_set(0, (i - 1), hw_lpbmp);
            }
        }
    }
}
#endif /* CFG_SWITCH_LAG_INCLUDED */

#ifdef CFG_SWITCH_MIRROR_INCLUDED
 /** 
 * <b>Description:</b> Set the mirror-to-port.
 * @param uport - Port number.
 * @return SYS_OK
 * \n      Operation completed successfully.
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

 /** 
 * <b>Description:</b> Get the mirror-to-port.
 * @param uport - (OUT) port number.
 * @return SYS_OK
 * \n      Operation completed successfully.
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

    return  board_lport_to_uport(0, lport, uport);
}

 /** 
 * <b>Description:</b> Set the port to be mirrored or disable.
 * @param uport - Port number.
 * @param enable:
 *  \li TRUE = Set the port to be mirrored.
 *  \li FALSE = Set the port not to be mirrored.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_mirror_port_set(uint16 uport, uint8 enable)
{

    int i;
    uint8 unit, lport;
    sys_error_t rv = SYS_OK;

    PORTm_t port;
    MIRROR_CONTROLr_t mirror_control;
    EMIRROR_CONTROL_64r_t emirror_control;
    pbmp_t lpbmp;

    /* check M_ENABLE in PORT_TAB */
    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    /* enable == 0 */
    if (enable == 0) {
        READ_PORTm(unit, lport, port);
        PORTm_MIRROR0f_SET(port, 0);
        WRITE_PORTm(unit, lport, port);

        SOC_LPORT_ITER(i) {
           rv |= READ_EMIRROR_CONTROL_64r(unit, i, emirror_control);
           PBMP_CLEAR(lpbmp);
           PBMP_WORD_SET(lpbmp, 0, EMIRROR_CONTROL_64r_BITMAPf_GET(emirror_control));
           PBMP_PORT_REMOVE(lpbmp, lport);
           if (PBMP_NOT_NULL(lpbmp)) {
               enable = 1;
           }
           EMIRROR_CONTROL_64r_BITMAPf_SET(emirror_control, SOC_PBMP(lpbmp));
           rv |= WRITE_EMIRROR_CONTROL_64r(unit, i, emirror_control);
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

            rv |= READ_EMIRROR_CONTROL_64r(unit, i, emirror_control);
    	    PBMP_CLEAR(lpbmp);
	        PBMP_WORD_SET(lpbmp, 0, EMIRROR_CONTROL_64r_BITMAPf_GET(emirror_control));
			PBMP_PORT_ADD(lpbmp, lport);
			EMIRROR_CONTROL_64r_BITMAPf_SET(emirror_control, SOC_PBMP(lpbmp));
			rv |= WRITE_EMIRROR_CONTROL_64r(unit, i, emirror_control);

        }
    }

    return rv;
}

 /** 
 * <b>Description:</b> Get if the port is set to be mirrored.
 * @param uport - Port number.
 * @param enable - (OUT)
 *  \li TRUE = The port is mirrored.
 *  \li FALSE = The port is not mirrored.
 * @return SYS_OK
 * \n      Operation completed successfully.
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
/** 
 * <b>Description:</b> Set the QoS type.
 * @param type - Type number:
 *  \li QT_PORT_BASED
 *  \li QT_DOT1P_PRIORITY
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_qos_type_set(qos_type_t type)
{

    sys_error_t rv = SYS_OK;

    int i;

    FP_TCAMm_t fp_tcam;
    FP_PORT_FIELD_SELm_t fp_port_field_sel;

    if (type == qos_info) {
        return SYS_OK;
    }

    if (type == QT_DOT1P_PRIORITY) {
        /*
                * 802.1P QoS use entries 24~31.
                */
        for (i = 0; i <= 7; i++) {            
            READ_FP_TCAMm(0, DOT1P_BASE_IDX + i, fp_tcam);
            FP_TCAMm_VALIDf_SET(fp_tcam, 3);
            WRITE_FP_TCAMm(0, DOT1P_BASE_IDX + i, fp_tcam);
        }

        for (i = BCM5354X_LPORT_MIN; i <= BCM5354X_LPORT_MAX; i++) {
            READ_FP_TCAMm(0, QOS_BASE_IDX + (i - BCM5354X_LPORT_MIN), fp_tcam);
            FP_TCAMm_VALIDf_SET(fp_tcam, 0);            
            WRITE_FP_TCAMm(0, QOS_BASE_IDX + (i - BCM5354X_LPORT_MIN), fp_tcam);
        }


        for (i = BCM5354X_LPORT_MIN; i <= BCM5354X_LPORT_MAX; i++) {
            /* Set SLICE2_F3 = 3, clear source port qualifier Slice2_F1=11 */
            READ_FP_PORT_FIELD_SELm(0, i, fp_port_field_sel);
            FP_PORT_FIELD_SELm_SLICE2_F3f_SET(fp_port_field_sel, 0x3);
            WRITE_FP_PORT_FIELD_SELm(0, i, fp_port_field_sel); 
	    }
        bcm5354x_dscp_map_enable(1);
    } else if (type == QT_PORT_BASED) {
        /*
         * Port based QoS use entries 0~23.
         * It'll be created in board_untagged_priority_set later.
         */
        for (i = BCM5354X_LPORT_MIN; i <= BCM5354X_LPORT_MAX; i++) {
            READ_FP_TCAMm(0, QOS_BASE_IDX + (i - BCM5354X_LPORT_MIN), fp_tcam);
            FP_TCAMm_VALIDf_SET(fp_tcam, 3);            
            WRITE_FP_TCAMm(0, QOS_BASE_IDX + (i - BCM5354X_LPORT_MIN), fp_tcam);
        }

        for (i = 0; i <= 7; i++) {
            READ_FP_TCAMm(0, DOT1P_BASE_IDX + i, fp_tcam);
            FP_TCAMm_VALIDf_SET(fp_tcam, 0);
            WRITE_FP_TCAMm(0, DOT1P_BASE_IDX + i, fp_tcam);
        }

        for (i = BCM5354X_LPORT_MIN; i <= BCM5354X_LPORT_MAX; i++) {
            /* Set SLICE2_F1 = 11(0xb), clear VLAN qualifier F3=3*/
            READ_FP_PORT_FIELD_SELm(0, i, fp_port_field_sel);
            FP_PORT_FIELD_SELm_SLICE2_F3f_SET(fp_port_field_sel, 0xb);
            WRITE_FP_PORT_FIELD_SELm(0, i, fp_port_field_sel); 
	    }

        /* disable dscp while Qos in port_based */
        bcm5354x_dscp_map_enable(0);
    }

    qos_info = type;
    return rv;
}

/** 
 * <b>Description:</b> Get the QoS type.
 * @param type - (OUT) type number:
 *  \li QT_PORT_BASED
 *  \li QT_DOT1P_PRIORITY
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_qos_type_get(qos_type_t *type)
{
    *type = qos_info;
    return SYS_OK;
}

/** 
 * <b>Description:</b> Set the priority for a given port.
 * @param uport - Port number.
 * @param priority - Priority number.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_untagged_priority_set(uint16 uport, uint8 priority)
{
    sys_error_t rv;
    uint8 unit, lport;
    int i;

    FP_TCAMm_t fp_tcam;
    FP_GLOBAL_MASK_TCAMm_t fp_global_mask_tcam;
    FP_POLICY_TABLEm_t fp_policy_table;

    GLP_t glp;
    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    /* While enable QoS priority in FP, need to disable "1p priority higher than DSCP,
       using set invalid bit in FP_TCAM */
    for (i = 0; i < 8; i++) {
        READ_FP_TCAMm(unit, (DOT1P_BASE_IDX + i), fp_tcam);
        FP_TCAMm_VALIDf_SET(fp_tcam, 0);
        rv = WRITE_FP_TCAMm(unit, (DOT1P_BASE_IDX + i), fp_tcam); 
    }


    FP_POLICY_TABLEm_CLR(fp_policy_table);
    FP_POLICY_TABLEm_R_COS_INT_PRIf_SET(fp_policy_table, priority);
    FP_POLICY_TABLEm_Y_COS_INT_PRIf_SET(fp_policy_table, priority);
    FP_POLICY_TABLEm_G_COS_INT_PRIf_SET(fp_policy_table, priority);
    FP_POLICY_TABLEm_R_CHANGE_COS_OR_INT_PRIf_SET(fp_policy_table, 5);
    FP_POLICY_TABLEm_Y_CHANGE_COS_OR_INT_PRIf_SET(fp_policy_table, 5);
    FP_POLICY_TABLEm_G_CHANGE_COS_OR_INT_PRIf_SET(fp_policy_table, 5);
    FP_POLICY_TABLEm_METER_SHARING_MODE_MODIFIERf_SET(fp_policy_table, 1);
    FP_POLICY_TABLEm_METER_PAIR_MODE_MODIFIERf_SET(fp_policy_table, 1);
    WRITE_FP_POLICY_TABLEm(unit,QOS_BASE_IDX + (lport - BCM5354X_LPORT_MIN), fp_policy_table);
    

    /* Using FP slice 2, entry 0~ for port based qos */
    GLP_CLR(glp);
    GLP_PORTf_SET(glp, lport);
    FP_TCAMm_CLR(fp_tcam);
    FP_TCAMm_F3_11_SGLPf_SET(fp_tcam, GLP_GET(glp));
    FP_TCAMm_F3_11_SGLP_MASKf_SET(fp_tcam, 0xFFFFFFFF);
    FP_TCAMm_VALIDf_SET(fp_tcam, 3);
    WRITE_FP_TCAMm(unit,QOS_BASE_IDX + (lport - BCM5354X_LPORT_MIN), fp_tcam);
    FP_GLOBAL_MASK_TCAMm_CLR(fp_global_mask_tcam);
    FP_GLOBAL_MASK_TCAMm_VALIDf_SET(fp_global_mask_tcam, 1);
    WRITE_FP_GLOBAL_MASK_TCAMm(unit,QOS_BASE_IDX + (lport - BCM5354X_LPORT_MIN),fp_global_mask_tcam);

    return rv;
}

/** 
 * <b>Description:</b> Get the priority for a given port.
 * @param uport - Port number.
 * @param priority - (OUT) priority number.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_untagged_priority_get(uint16 uport, uint8 *priority)
{
    sys_error_t rv;
    uint8 unit, lport;

    FP_POLICY_TABLEm_t fp_policy_table;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    rv = READ_FP_POLICY_TABLEm(unit, QOS_BASE_IDX + (lport - BCM5354X_LPORT_MIN), fp_policy_table);
    *priority = FP_POLICY_TABLEm_G_COS_INT_PRIf_GET(fp_policy_table);

    return rv;
}
#endif /* CFG_SWITCH_QOS_INCLUDED */

#ifdef CFG_SWITCH_RATE_INCLUDED

 
/** 
 * <b>Description:</b> Set the ingress rate for a given port.
 * @param uport - Port number.
 * @param bits_sec - Rate number
 *  \li 0: No limit
 *  \li 512000
 *  \li 1024000
 *  \li ...
 *  \li 524288000
 * @return SYS_OK
 * \n      Operation completed successfully.
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
        READ_FP_TCAMm(unit, RATE_IGR_IDX + lport - BCM5354X_LPORT_MIN, fp_tcam);
        FP_TCAMm_VALIDf_SET(fp_tcam, 0);
        WRITE_FP_TCAMm(unit, RATE_IGR_IDX + lport - BCM5354X_LPORT_MIN, fp_tcam);
		return SYS_OK;
    } else {
        READ_FP_TCAMm(unit, RATE_IGR_IDX + lport - BCM5354X_LPORT_MIN, fp_tcam);
        FP_TCAMm_VALIDf_SET(fp_tcam, 3);
        WRITE_FP_TCAMm(unit, RATE_IGR_IDX + lport - BCM5354X_LPORT_MIN, fp_tcam);
    }

    FP_METER_TABLEm_CLR(fp_meter_table);

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

    READ_FP_POLICY_TABLEm(unit, RATE_IGR_IDX + lport - BCM5354X_LPORT_MIN, fp_policy_table);

    WRITE_FP_METER_TABLEm(unit, 2 * FP_POLICY_TABLEm_METER_PAIR_INDEXf_GET(fp_policy_table)
                                + FP_POLICY_TABLEm_METER_PAIR_MODE_MODIFIERf_GET(fp_policy_table),
                          fp_meter_table);

    return SYS_OK;
}

 
/** 
 * <b>Description:</b> Get the rate ingress setting for a given port.
 * @param uport - Port number.
 * @param bits_sec - (OUT) rate number.
 *  \li 0: No limit
 *  \li Rate value
 * @return SYS_OK
 * \n      Operation completed successfully.
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

    READ_FP_TCAMm(unit, RATE_IGR_IDX + lport - BCM5354X_LPORT_MIN, fp_tcam);

    if (FP_TCAMm_VALIDf_GET(fp_tcam) == 0)
    {
        *bits_sec = 0;
        return SYS_OK;
    }

    SOC_IF_ERROR_RETURN(READ_FP_POLICY_TABLEm(unit, RATE_IGR_IDX + lport - BCM5354X_LPORT_MIN, fp_policy_table));

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

 
/** 
 * <b>Description:</b> Set the egress rate for a given port.
 * @param uport - Port number.
 * @param bits_sec - Rate number.
 *  \li 0: No limit
 *  \li 512000
 *  \li 1024000
 *  \li ...
 *  \li 524288000
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_port_rate_egress_set(uint16 uport, uint32 bits_sec)
{

    uint8 unit, lport;
    uint32 kbits_sec = bits_sec/1000;

    EGRMETERINGCONFIGr_t egrmeteringconfig;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    EGRMETERINGCONFIGr_CLR(egrmeteringconfig);
    SOC_IF_ERROR_RETURN(WRITE_EGRMETERINGCONFIGr(unit, lport, egrmeteringconfig));

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
            EGRMETERINGCONFIGr_THD_SELf_SET(egrmeteringconfig, 0xFA0);
            break;
        case 524288:
            EGRMETERINGCONFIGr_REFRESHf_SET(egrmeteringconfig, 0x2000);
            EGRMETERINGCONFIGr_THD_SELf_SET(egrmeteringconfig, 0xFA0);
            break;
       default:
            break;
    }
    SOC_IF_ERROR_RETURN(WRITE_EGRMETERINGCONFIGr(unit, lport, egrmeteringconfig));


    return SYS_OK;
}

 
/** 
 * <b>Description:</b> Get the rate egress setting for a given port.
 * @param uport - Port number.
 * @param bits_sec - (OUT) rate number.
 *  \li 0: No limit
 *  \li Rate value
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_port_rate_egress_get(uint16 uport, uint32 *bits_sec)
{

   uint8 unit, lport;
   EGRMETERINGCONFIGr_t egrmeteringconfig;

   *bits_sec  = 0;

   SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

   SOC_IF_ERROR_RETURN(READ_EGRMETERINGCONFIGr(unit, lport, egrmeteringconfig));

   
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
              (EGRMETERINGCONFIGr_THD_SELf_GET(egrmeteringconfig) == 0xFA0))
   {
	   *bits_sec = 262144 * 1000;
   } else if ((EGRMETERINGCONFIGr_REFRESHf_GET(egrmeteringconfig) == 0x2000) &&
              (EGRMETERINGCONFIGr_THD_SELf_GET(egrmeteringconfig) == 0xFA0))
   {
	   *bits_sec = 524288 * 1000;
   } 

   return SYS_OK;
}

/** 
 * <b>Description:</b> Set the storm control type.
 * @param flags - One of the following:
 *  \li STORM_RATE_NONE
 *  \li STORM_RATE_BCAST
 *  \li STORM_RATE_MCAST
 *  \li STORM_RATE_DLF
 *  \li STORM_RATE_ALL
 * @return SYS_OK
 * \n      Operation completed successfully.
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

/** 
 * <b>Description:</b> Get the storm control type.
 * @param flags - (OUT) one of the following:
 *  \li STORM_RATE_NONE
 *  \li STORM_RATE_BCAST
 *  \li STORM_RATE_MCAST
 *  \li STORM_RATE_DLF
 *  \li STORM_RATE_ALL
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_rate_type_get(uint8 *flags)
{
    *flags = storm_info;
    return SYS_OK;
}

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

/** 
 * <b>Description:</b> Get the storm control setting rate for a given port.
 * @param uport - Port number.
 * @param bits_sec - (OUT) rate number
 *  \li 0: No limit
 *  \li Rate value
 * @return SYS_OK
 * \n      Operation completed successfully.
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
 
 /** 
 * <b>Description:</b> Get the statistic value for a given port.
 * @param uport - Port number.
 * @param stat - (OUT) statistics value.
 * @return SYS_OK
 * \n      Operation completed successfully.
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
    GRFCSr_t  grfcs;
    
    HOLD_COSr_t hold_cos;


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
    }

     /* IPIPE HOLD Drop Counter */
     rv |= READ_HOLD_COSr(unit ,lport, hold_cos);

     return rv;
}

 
 /** 
 * <b>Description:</b> Clear the statistic value for a given port.
 * @param uport - Port number.
 * @return SYS_OK
 * \n      Operation completed successfully.
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
    
    HOLD_COSr_t hold_cos;

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

    }

     /* IPIPE HOLD Drop Counter */
     HOLD_COSr_CLR(hold_cos);             
     rv |= WRITE_HOLD_COSr(unit ,lport, hold_cos);

    return rv;

}

 
 /** 
 * <b>Description:</b> Clear the statistic value for all the ports.
 * @return SYS_OK
 * \n      Operation completed successfully.
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
/** 
 * <b>Description:</b> Add an entry in the multicast table.
 * @param mac_addr - MAC address.
 * @param vlan_id - VLAN ID number.
 * @param uplist - Port list.
 * @return SYS_OK
 * \n      Operation completed successfully.
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
        sal_memset(mcindex, 0, L2MC_ENTRY_SIZE*4);
    	mcindex[0]=0x1;
    } else {
       mcast->next = mlist;
       mcast->index = 0;
       for (i=0; i< L2MC_ENTRY_SIZE; i++) {
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
    sal_memcpy(l2x.mac_addr, mac_addr, 6);
    
    rv = bcm5354x_l2_op(0, &l2x, SC_OP_L2_INS_CMD);

    /* after get mcindex from l2_entry */
    READ_L2MCm(0, mcast->index, l2mc);
    L2MCm_PORT_BITMAPf_SET(l2mc, SOC_PBMP(lpbmp));
    L2MCm_VALIDf_SET(l2mc, 1);
    WRITE_L2MCm(0, mcast->index, l2mc); 

    return rv;
}

/** 
 * <b>Description:</b> Remove an entry from the multicast table.
 * @param mac_addr - MAC address.
 * @param vlan_id - VALN ID number.
 * @return SYS_OK
 * \n      Operation completed successfully.
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
    rv = bcm5354x_l2_op(0, &l2x, SC_OP_L2_DEL_CMD);

    /* after get mcindex from l2_entry */
    PBMP_CLEAR(lpbmp);
    READ_L2MCm(0, mcast->index, l2mc);
    L2MCm_PORT_BITMAPf_SET(l2mc, SOC_PBMP(lpbmp));
    L2MCm_VALIDf_SET(l2mc, 0);
    WRITE_L2MCm(0, mcast->index, l2mc); 

    return rv;
}

/** 
 * <b>Description:</b> Add port for a given entry in multicast table.
 * @param mac_addr - MAC address.
 * @param vlan_id - VLAN ID number.
 * @param uport - Port number.
 * @return SYS_OK
 * \n      Operation completed successfully.
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

/** 
 * <b>Description:</b> Remove port for a given entry from multicast table.
 * @param mac_addr - MAC address.
 * @param vlan_id - VLAN ID number.
 * @param uport - Port number.
 * @return SYS_OK
 * \n      Operation completed successfully.
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

/** 
 * <b>Description:</b> Get the port list for a given entry in multicast table.
 * @param mac_addr - MAC address.
 * @param vlan_id - VLAN ID number.
 * @param uplist - (OUT) port list.
 * @return SYS_OK
 * \n      Operation completed successfully.
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

 /** 
 * <b>Description:</b> Set the IGMP snooping state.
 * @param enable:
 *  \li TRUE = Enable IGMP snooping.
 *  \li FALSE = Disable IGMP snooping.
 * @return SYS_OK
 * \n      Operation completed successfully.
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
        /* set bit 26-18 = 011011011 */
        IGMP_MLD_PKT_CONTROLr_IGMP_UNKNOWN_MSG_TO_CPUf_SET(igmp_mld_pkt_control, 1);
        IGMP_MLD_PKT_CONTROLr_IGMP_UNKNOWN_MSG_FWD_ACTIONf_SET(igmp_mld_pkt_control, 1);// drop packet
        IGMP_MLD_PKT_CONTROLr_IGMP_QUERY_TO_CPUf_SET(igmp_mld_pkt_control, 1);
        IGMP_MLD_PKT_CONTROLr_IGMP_QUERY_FWD_ACTIONf_SET(igmp_mld_pkt_control, 1);  // drop packet
        IGMP_MLD_PKT_CONTROLr_IGMP_REP_LEAVE_TO_CPUf_SET(igmp_mld_pkt_control, 1);
        IGMP_MLD_PKT_CONTROLr_IGMP_REP_LEAVE_FWD_ACTIONf_SET(igmp_mld_pkt_control, 1); // drop packet 

//        val = (val & 0xf803ffff) | 0x036c0000;
    } else {
        /* set bit 26-18 = 100100100 */
//        val = (val & 0xf803ffff) | 0x04900000;
        IGMP_MLD_PKT_CONTROLr_IGMP_UNKNOWN_MSG_TO_CPUf_SET(igmp_mld_pkt_control, 0);
        IGMP_MLD_PKT_CONTROLr_IGMP_UNKNOWN_MSG_FWD_ACTIONf_SET(igmp_mld_pkt_control, 2);// Flood the packet to the VLAN members ?? TO-CHECK
        IGMP_MLD_PKT_CONTROLr_IGMP_QUERY_TO_CPUf_SET(igmp_mld_pkt_control, 0);
        IGMP_MLD_PKT_CONTROLr_IGMP_QUERY_FWD_ACTIONf_SET(igmp_mld_pkt_control, 2);  // Flood the packet to the VLAN members ?? TO-CHECK
        IGMP_MLD_PKT_CONTROLr_IGMP_REP_LEAVE_TO_CPUf_SET(igmp_mld_pkt_control, 0);
        IGMP_MLD_PKT_CONTROLr_IGMP_REP_LEAVE_FWD_ACTIONf_SET(igmp_mld_pkt_control, 2); // Flood the packet to the VLAN members ?? TO-CHECK

    }
    rv |= WRITE_IGMP_MLD_PKT_CONTROLr(0, 0, igmp_mld_pkt_control);

    return rv;
}

 /** 
 * <b>Description:</b> Get the IGMP snooping state.
 * @param enable - (OUT)
 *  \li TRUE = IGMP snooping is enabled.
 *  \li FALSE = IGMP snooping is disabled.
 * @return SYS_OK
 * \n      Operation completed successfully.
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

/** 
 * <b>Description:</b> Set the state of block unknown multicast packet.
 * @param enable
 *  \li TRUE = Enable block unknown multicast.
 *  \li FALSE = Disable block unknown multicast.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
 
sys_error_t
board_block_unknown_mcast_set(uint8 enable)
{

    int lport;
    int rv;

    UNKNOWN_MCAST_BLOCK_MASK_64r_t unknow_mcast_block_mask_64;
    IUNKNOWN_MCAST_BLOCK_MASK_64r_t iunknow_mcast_block_mask_64;
    rv = SYS_OK;

    for (lport = BCM5354X_LPORT_MIN; lport <= BCM5354X_LPORT_MAX; lport++) {
        rv = READ_UNKNOWN_MCAST_BLOCK_MASK_64r(0, lport, unknow_mcast_block_mask_64);
        if (enable == 1) {
            UNKNOWN_MCAST_BLOCK_MASK_64r_BLK_BITMAPf_SET(unknow_mcast_block_mask_64, 0xFFFFFFFF); /* block all */
        } else {
            UNKNOWN_MCAST_BLOCK_MASK_64r_BLK_BITMAPf_SET(unknow_mcast_block_mask_64, 0x0);
        }

        rv = WRITE_UNKNOWN_MCAST_BLOCK_MASK_64r(0, lport, unknow_mcast_block_mask_64);

        rv = READ_IUNKNOWN_MCAST_BLOCK_MASK_64r(0, lport, iunknow_mcast_block_mask_64);
        if (enable == 1) {
            IUNKNOWN_MCAST_BLOCK_MASK_64r_BLK_BITMAPf_SET(iunknow_mcast_block_mask_64, 0xffffffff); /* block all */
        } else {
            IUNKNOWN_MCAST_BLOCK_MASK_64r_BLK_BITMAPf_SET(iunknow_mcast_block_mask_64, 0x0); 
        }
        rv = WRITE_IUNKNOWN_MCAST_BLOCK_MASK_64r(0, lport, iunknow_mcast_block_mask_64);
    }
    return rv;
}

/** 
 * <b>Description:</b> Get the state of block unknown multicast packet.
 * @param enable (OUT)
 *  \li TRUE = Block unknown multicast is enabled.
 *  \li FALSE = Block unknown multicast is disabled.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
 
sys_error_t
board_block_unknown_mcast_get(uint8 *enable)
{

    int rv;

    UNKNOWN_MCAST_BLOCK_MASK_64r_t unknow_mcast_block_mask_64;

    rv = SYS_OK;

    /* select port 1 to check */
    *enable = 0;
    rv = READ_UNKNOWN_MCAST_BLOCK_MASK_64r(0, BCM5354X_LPORT_MIN, unknow_mcast_block_mask_64);
    if (UNKNOWN_MCAST_BLOCK_MASK_64r_BLK_BITMAPf_GET(unknow_mcast_block_mask_64)) {
        *enable = 1;
    }

    return rv;
}
#endif /* CFG_SWITCH_MCAST_INCLUDED */

#ifdef CFG_SWITCH_LOOPDETECT_INCLUDED
 /** 
 * <b>Description:</b> Set the loop-detect state.
 * @param enable:
 *  \li TRUE = Enable loop detect.
 *  \li FALSE = Disable loop detect.
 */
void
board_loop_detect_enable(BOOL enable)
{
    bcm5354x_loop_detect_enable(enable);
}

 /** 
 * <b>Description:</b> Get whether the loop-detect state.
 * @return \li TRUE = Enable loop detect.
 *         \li FALSE = Disable loop detect.
 */
uint8
board_loop_detect_status_get(void)
{
    return bcm5354x_loop_detect_status_get();
}
#endif /* CFG_SWITCH_LOOPDETECT_INCLUDED */
 
 /** 
 * <b>Description:</b> Set the loopback mode for a given port.
 * @param uport - Port number.
 * @param loopback_mode - One of the following:
 *  \li BCM_PORT_LOOPBACK_NONE
 *  \li BCM_PORT_LOOPBACK_MAC
 *  \li BCM_PORT_LOOPBACK_PHY
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_port_loopback_enable_set(uint16 uport, int loopback_mode)
{
    uint8 unit, lport;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    bcm5354x_loopback_enable(unit, lport, loopback_mode);

    return SYS_OK;
}

 /** 
 * <b>Description:</b> Get the enable state as defined by board_port_enable_set().
 * @param uport - Port number.
 * @param enable - (OUT)
 *  \li TRUE = Port is enabled.
 *  \li FALSE = Port is disabled.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_port_enable_get(uint16 uport, BOOL *enable)
{
    uint8 unit, lport;
    sys_error_t r;
    int en;

    *enable = FALSE;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

	r = pcm_port_enable_get(unit, lport, &en);

    *enable = en;


    return r;
}

 /** 
 * <b>Description:</b> Physically enable/disable the MAC/PHY on this port.
 * @param uport - Port number.
  * @param enable:
 *  \li TRUE = Port is enabled.
 *  \li FALSE = Port is disabled.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_port_enable_set(uint16 uport, BOOL enable)
{
    uint8 unit;
    uint8 lport;
    sys_error_t r;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    r = pcm_port_enable_set(unit,lport,enable);

    return r;
}


#ifdef CFG_ZEROCONF_MDNS_INCLUDED
 /** 
 * <b>Description:</b> Set the mdns state.
 * @param enable:
 *  \li TRUE = Enable MDNS.
 *  \li FALSE = Disable MDNS.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_mdns_enable_set(BOOL enable)
{
    return bcm5354x_mdns_enable_set(0, enable);
}

 /** 
 * <b>Description:</b> Get the mdns state.
 * @param enable - (OUT)
 *  \li TRUE = MDNS is enabled.
 *  \li FALSE = MDNS is disabled.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_mdns_enable_get(BOOL *enable)
{
    return bcm5354x_mdns_enable_get(0, enable);
}
#endif /* CFG_ZEROCONF_MDNS_INCLUDED */
 
 /** 
 * <b>Description:</b> Get the port status for a given port.
 * @param uport - Port number.
 * @param tx - (OUT) TRUE/FALSE.
 * @param rx - (OUT) TRUE/FALSE.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_port_pause_get(uint16 uport, BOOL *tx, BOOL *rx)
{
    uint8 unit, lport;

    *tx = 0;
    *rx = 0;
    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    *tx = SOC_PORT_TX_PAUSE_STATUS(lport);
    *rx = SOC_PORT_RX_PAUSE_STATUS(lport);


    return SYS_OK;

}

 /** 
 * <b>Description:</b> Get the auto-negotiation state of the port.
 * @param uport - Port number.
 * @param an - (OUT) auto-negotiation status.
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
board_port_an_get(uint16 uport, BOOL *an)
{
    uint8 unit, lport;

    *an = 0;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));

    *an = SOC_PORT_AN_STATUS(lport);

    return SYS_OK;
}
#if defined(CFG_SWITCH_SNAKETEST_INCLUDED) && defined(CFG_SWITCH_VLAN_INCLUDED)
sys_error_t
board_snaketest_trap_to_cpu(uint16 vlanid, uint8 *uplist) {
    pbmp_t      lpbmp, tag_lpbmp;

    board_uplist_to_lpbmp(uplist, 0, &lpbmp);

    PBMP_CLEAR(tag_lpbmp);
    PBMP_PORT_ADD(lpbmp, 0);
    PBMP_PORT_ADD(tag_lpbmp, 0);

    return bcm5354x_qvlan_port_set(0, vlanid, lpbmp, tag_lpbmp);
}
#endif /* CFG_SWITCH_SNAKETEST_INCLUDED && CFG_SWITCH_VLAN_INCLUDED */

#ifdef CFG_RESET_BUTTON_INCLUDED
uint8 sw_simulate_press_reset_button_duration = 0;
uint8 reset_button_active_high = 1;
uint8 reset_button_gpio_bit = 4;

BOOL
board_reset_button_get(void)
{
    uint32 val;
    /*
     * PAD_GPIO 0 ~ 3 : from CMICD
     * PAD_GPIO 4 ~ 7 : from IPROC
     */
    val = (READCSR(R_CCG_GP_DATA_IN) & 0x00f0) | (READCSR(CMIC_GP_DATA_IN) & 0x000f);
    
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
    rv = bcm5354x_l2_op(0, &l2x, SC_OP_L2_LOOKUP_CMD, &index);
    if (rv == SYS_OK) {
        return SYS_ERR_EXISTS; 
    }
    rv = bcm5354x_l2_op(0, &l2x, SC_OP_L2_INS_CMD, NULL);

    return rv;
}

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
    rv = bcm5354x_l2_op(0, &l2x, SC_OP_L2_LOOKUP_CMD, &index);
    if (rv == SYS_ERR_NOT_FOUND) {
        return rv;
    }
    rv = bcm5354x_l2_op(0, &l2x, SC_OP_L2_DEL_CMD, NULL);

    return rv;
}

#define L2_AGE_TIMEOUT_COUNT 150

/*
 * PER_PORT_AGE_CONTROL REGISTER : 
 * DEST_TYPE [Bit 32 : 31]
 * T [Bit 31]
 * COMPLETE [Bit 30]
 * START [Bit 29]
 * PPA_MODE [Bit 28 : 26]
 *    0x0 = PORTMOD_DEL - If match module ID and port/TGID, delete entry
 *    0x1 = VLAN_DEL - If match VLAN ID, delete entry
 *    0x2 = PORTMOD_VLAN_DEL - If match VLAN ID, module ID and port/TGID, delete entry
 *    0x3 = ALL_DEL - Delete All entries - no match necessary. 
 * VLAN_ID [Bit 25 : 14]
 * MODULE_ID [Bit 13 : 6]
 * TGID [Bit 6 : 0]
 * PORT_NUM [Bit 5 : 0]
 */

sys_error_t 
board_l2_addr_delete_by_port(uint16 uport)
{
    uint32 val;
    uint8 unit = 0, lport;
    int i;
    uint32 entry[2] = {0, 0};

    bcm5354x_reg_get(unit, R_L2_AGE_TIMER, &val);
    val &= ~(0x1 << 20);
    bcm5354x_reg_set(unit, R_L2_AGE_TIMER, val);

    board_uport_to_lport(uport, &unit, &lport);
    entry[0] = (lport & 0x3F);
    entry[0] |= (0x1 << 29);
    bcm5354x_reg64_set(unit, R_PER_PORT_AGE_CONTROL, entry, 2);

    for (i = 0; i < L2_AGE_TIMEOUT_COUNT; i++) {
        sal_usleep(100000);
        bcm5354x_reg64_get(unit, R_PER_PORT_AGE_CONTROL, entry, 2);
        if (entry[0] & (0x1 << 30)) {
            break;
        }
    }

    bcm5354x_reg_get(unit, R_L2_AGE_TIMER, &val);
    val |= (0x1 << 20);
    bcm5354x_reg_set(unit, R_L2_AGE_TIMER, val);

    if (i == L2_AGE_TIMEOUT_COUNT) {
        return SYS_ERR_TIMEOUT;
    }

    return SYS_OK;
}

sys_error_t 
board_l2_addr_delete_by_vlan(uint16 vid)
{
    uint32 val;
    uint8 unit = 0;
    int i;
    uint32 entry[2] = {0, 0};

    bcm5354x_reg_get(unit, R_L2_AGE_TIMER, &val);
    val &= ~(0x1 << 20);
    bcm5354x_reg_set(unit, R_L2_AGE_TIMER, val);

    entry[0] = (vid & 0xFFF) << 14;
    entry[0] |= (0x1 << 26);
    entry[0] |= (0x1 << 29);
    
    bcm5354x_reg64_set(unit, R_PER_PORT_AGE_CONTROL, entry, 2);
    for (i = 0; i < L2_AGE_TIMEOUT_COUNT; i++) {
        sal_usleep(100000);
        bcm5354x_reg64_get(unit, R_PER_PORT_AGE_CONTROL, entry, 2);
        if (entry[0] & (0x1 << 30)) {
            break;
        }
    }

    bcm5354x_reg_get(unit, R_L2_AGE_TIMER, &val);
    val |= (0x1 << 20);
    bcm5354x_reg_set(unit, R_L2_AGE_TIMER, val);

    if (i == L2_AGE_TIMEOUT_COUNT) {
        return SYS_ERR_TIMEOUT;
    }

    return SYS_OK;
}

sys_error_t 
board_l2_addr_delete_by_trunk(uint8 lagid)
{
    uint32 val;
    uint8 unit = 0;
    int i;
    uint32 entry[2] = {0, 0};

    bcm5354x_reg_get(unit, R_L2_AGE_TIMER, &val);
    val &= ~(0x1 << 20);
    bcm5354x_reg_set(unit, R_L2_AGE_TIMER, val);

    entry[0] = ((lagid-1) & 0x7F);
    entry[0] |= (0x1 << 29);
    entry[0] |= (0x1 << 31);
    bcm5354x_reg64_set(unit, R_PER_PORT_AGE_CONTROL, entry, 2);
    

    for (i = 0; i < L2_AGE_TIMEOUT_COUNT; i++) {
        sal_usleep(100000);
        bcm5354x_reg64_get(unit, R_PER_PORT_AGE_CONTROL, entry, 2);
        if (entry[0] & (0x1 << 30)) {
            break;
        }
    }

    bcm5354x_reg_get(unit, R_L2_AGE_TIMER, &val);
    val |= (0x1 << 20);
    bcm5354x_reg_set(unit, R_L2_AGE_TIMER, val);

    if (i == L2_AGE_TIMEOUT_COUNT) {
        return SYS_ERR_TIMEOUT;
    }

    return SYS_OK;
}

sys_error_t 
board_l2_addr_delete_all(void)
{
    uint32 val;
    uint8 unit = 0;
    int i;
    uint32 entry[2] = {0, 0};

    bcm5354x_reg_get(unit, R_L2_AGE_TIMER, &val);
    val &= ~(0x1 << 20);
    bcm5354x_reg_set(unit, R_L2_AGE_TIMER, val);

    entry[0] = (0x3 << 26);
    entry[0] |= (0x1 << 29);
    bcm5354x_reg64_set(unit, R_PER_PORT_AGE_CONTROL, entry, 2);

    for (i = 0; i < L2_AGE_TIMEOUT_COUNT; i++) {
        sal_usleep(100000);
        bcm5354x_reg64_get(unit, R_PER_PORT_AGE_CONTROL, entry, 2);
        if (entry[0] & (0x1 << 30)) {
            break;
        }
    }

    bcm5354x_reg_get(unit, R_L2_AGE_TIMER, &val);
    val |= (0x1 << 20);
    bcm5354x_reg_set(unit, R_L2_AGE_TIMER, val);

    if (i == L2_AGE_TIMEOUT_COUNT) {
        return SYS_ERR_TIMEOUT;
    }

    return SYS_OK;
}

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
    rv = bcm5354x_l2_op(0, &l2x, SC_OP_L2_LOOKUP_CMD, index);

    return rv;
}

/*
 * L2 ENTRY TABLE : 
 * VLAN_ID [Bit 14 : 3]
 * MAC_ADDR [Bit 62 : 15]
 * PORT_NUM [Bit 69 : 64]
 * T [Bit 78]
 * STATIC_BIT [Bit 100]
 * VALID [Bit 102]
 */

static int l2_addr_index = 0;

sys_error_t 
board_l2_addr_get_first(board_l2_addr_t *l2addr, uint16 *index)
{
    int rv = SYS_OK;
    uint32 entry[4];
    uint32 i;
    uint8 unit = 0, lport;

    l2_addr_index = 0;
    *index = 0x0;
    sal_memset(l2addr, 0, sizeof(board_l2_addr_t));

    for (i=0 ; i<L2_ENTRY_SIZE ; i++) {    
        rv |= bcm5354x_mem_get(0, M_L2_ENTRY(i), entry, 4);
        if (entry[3] & 0x40) {
            l2addr->mac[5] = (entry[0] >> 15) & 0xFF;
            l2addr->mac[4] = (entry[0] >> 23) & 0xFF;
            l2addr->mac[3] = ((entry[1] & 0x7F) << 1) | ((entry[0] >> 31) & 0x1);
            l2addr->mac[2] = (entry[1] >> 7) & 0xFF;
            l2addr->mac[1] = (entry[1] >> 15) & 0xFF;
            l2addr->mac[0] = (entry[1] >> 23) & 0xFF;
            l2addr->vid = (entry[0] >> 3) & 0xFFF;
            lport = entry[2] & 0x3F;
            if (entry[2] & 0x4000) {
                l2addr->is_trunk = TRUE;
            } else {
                l2addr->is_trunk = FALSE;
            }
            if (l2addr->mac[0] & 0x1) {
                /* multicast */
                l2addr->mcidx = entry[2] & 0x3FF;
            } else {
                /* unicast */
                if (l2addr->is_trunk) {
                    l2addr->tgid = lport + 1;
                } else {
                    board_lport_to_uport(unit, lport, &(l2addr->uport));
                }
            }
            if (entry[3] & 0x10) {
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

    if (i == L2_ENTRY_SIZE) {
        return SYS_ERR_NOT_FOUND;
    }

    return rv ? SYS_ERR : SYS_OK;
}

sys_error_t 
board_l2_addr_get_next(board_l2_addr_t *l2addr, uint16 *index)
{
    int rv = SYS_OK;
    uint32 entry[4];
    int i;
    uint8 unit = 0, lport;

    *index = 0x0;
    sal_memset(l2addr, 0, sizeof(board_l2_addr_t));

    for (i=(l2_addr_index+1) ; i<L2_ENTRY_SIZE ; i++) {    
        rv |= bcm5354x_mem_get(0, M_L2_ENTRY(i), entry, 4);
        if (entry[3] & 0x40) {
            l2addr->mac[5] = (entry[0] >> 15) & 0xFF;
            l2addr->mac[4] = (entry[0] >> 23) & 0xFF;
            l2addr->mac[3] = ((entry[1] & 0x7F) << 1) | ((entry[0] >> 31) & 0x1);
            l2addr->mac[2] = (entry[1] >> 7) & 0xFF;
            l2addr->mac[1] = (entry[1] >> 15) & 0xFF;
            l2addr->mac[0] = (entry[1] >> 23) & 0xFF;
            l2addr->vid = (entry[0] >> 3) & 0xFFF;
            lport = entry[2] & 0x3F;
            if (entry[2] & 0x4000) {
                l2addr->is_trunk = TRUE;
            } else {
                l2addr->is_trunk = FALSE;
            }
            if (l2addr->mac[0] & 0x1) {
                /* multicast */
                l2addr->mcidx = entry[2] & 0x3FF;
            } else {
                /* unicast */
                if (l2addr->is_trunk) {
                    l2addr->tgid = lport + 1;
                } else {
                    board_lport_to_uport(unit, lport, &(l2addr->uport));
                }
            }
            if (entry[3] & 0x10) {
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

    if (i == L2_ENTRY_SIZE) {
        return SYS_ERR_NOT_FOUND;
    }

    return rv ? SYS_ERR : SYS_OK;

}

sys_error_t 
board_l2_addr_get_last(board_l2_addr_t *l2addr, uint16 *index)
{
    int rv = SYS_OK;
    uint32 entry[4];
    int i;
    uint8 unit = 0, lport;

    l2_addr_index = 0;
    *index = 0x0;
    sal_memset(l2addr, 0, sizeof(board_l2_addr_t));

    for (i=(L2_ENTRY_SIZE-1) ; i>=0 ; i--) {    
        rv |= bcm5354x_mem_get(0, M_L2_ENTRY(i), entry, 4);
        if (entry[3] & 0x40) {
            l2addr->mac[5] = (entry[0] >> 15) & 0xFF;
            l2addr->mac[4] = (entry[0] >> 23) & 0xFF;
            l2addr->mac[3] = ((entry[1] & 0x7F) << 1) | ((entry[0] >> 31) & 0x1);
            l2addr->mac[2] = (entry[1] >> 7) & 0xFF;
            l2addr->mac[1] = (entry[1] >> 15) & 0xFF;
            l2addr->mac[0] = (entry[1] >> 23) & 0xFF;
            l2addr->vid = (entry[0] >> 3) & 0xFFF;
            lport = entry[2] & 0x3F;
            if (entry[2] & 0x4000) {
                l2addr->is_trunk = TRUE;
            } else {
                l2addr->is_trunk = FALSE;
            }
            if (l2addr->mac[0] & 0x1) {
                /* multicast */
                l2addr->mcidx = entry[2] & 0x3FF;
            } else {
                /* unicast */
                if (l2addr->is_trunk) {
                    l2addr->tgid = lport + 1;
                } else {
                    board_lport_to_uport(unit, lport, &(l2addr->uport));
                }
            }
            if (entry[3] & 0x10) {
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

