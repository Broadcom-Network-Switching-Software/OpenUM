/*! \file l2.h
 *
 * L2 board APIs.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_L2_H_
#define _BOARDAPI_L2_H_

/*! Flag for L2 static entry */
#define BOARD_L2_STATIC   (0x1 << 0)

/*! L2 command type. */
typedef enum l2_cmd_s {
    /*! Add an entry. */
    L2_CMD_ADD = 0,

    /*! Delete an entry. */
    L2_CMD_DELETE,

    /*! Delete entries by port. */
    L2_CMD_DELETE_BY_PORT,

    /*! Delete entries by vlan. */
    L2_CMD_DELETE_BY_VLAN,

    /*! Delete entries by trunk. */
    L2_CMD_DELETE_BY_TRUNK,

    /*! Delete all entries. */
    L2_CMD_DELETE_ALL,

    /*! Lookup an entry. */
    L2_CMD_LOOKUP,

    /*! Get the first entry. */
    L2_CMD_GET_FIRST,

    /*! Get the next entry. */
    L2_CMD_GET_NEXT,

    /*! Get the last entry. */
    L2_CMD_GET_LAST,

    /*! Get all entries. */
    L2_CMD_GET_ALL,

    /*! Must be the last. */
    L2_CMD_COUNT
} l2_cmd_t;

/*! L2 address. */
typedef struct board_l2_addr_s {
    /*! BOARD_L2_xxx flags. */
    uint16 flags;

    /*! 802.3 MAC address. */
    uint8 mac[6];

    /*! VLAN identifier. */
    uint16 vid;

    /*! User port number. */
    uint16 uport;

    /*! L2MC pointer. */
    uint16 mcidx;

    /*! Trunk ID value. */
    uint8 tgid;

    /*! Trunk bit. */
    uint8 is_trunk;
} board_l2_addr_t;

/*!
 * \brief Add an entry in the L2 address table.
 *
 * \param [in] l2addr L2 address entry
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_l2_addr_add(board_l2_addr_t *l2addr);

/*!
 * \brief Delete an entry from the L2 address table.
 *
 * \param [in] l2addr L2 address entry
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_l2_addr_delete(board_l2_addr_t *l2addr);

/*!
 * \brief Delete entries from the L2 address table by user port number.
 *
 * \param [in] uport User port number
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_l2_addr_delete_by_port(uint16 uport);

/*!
 * \brief Delete entries from the L2 address table by VLAN ID.
 *
 * \param [in] vid VLAN ID
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_l2_addr_delete_by_vlan(uint16 vid);

/*!
 * \brief Delete entries from the L2 address table by TRUNK ID.
 *
 * \param [in] lagid TRUNK ID
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_l2_addr_delete_by_trunk(uint8 lagid);

/*!
 * \brief Delete all entries from the L2 address table.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_l2_addr_delete_all(void);

/*!
 * \brief Lookup entry from the L2 address table.
 *
 * \param [in] l2addr L2 address entry
 * \param [out] index L2 uni table
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_l2_addr_lookup(board_l2_addr_t *l2addr, 
                                                         uint16 *index);

/*!
 * \brief Get first entry from the L2 address table.
 *
 * \param [out] l2addr L2 address entry
 * \param [out] index L2 uni table
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_l2_addr_get_first(board_l2_addr_t *l2addr, 
                                                            uint16 *index);

/*!
 * \brief Get next entry from the L2 address table. This function needs to go with board_l2_addr_get_first.
 *
 * \param [out] l2addr L2 address entry
 * \param [out] index L2 uni table
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_l2_addr_get_next(board_l2_addr_t *l2addr, 
                                                            uint16 *index);

/*!
 * \brief Get last L2 entry from the L2 address table.
 *
 * \param [out] l2addr L2 address entry
 * \param [out] index L2 uni table
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t board_l2_addr_get_last(board_l2_addr_t *l2addr, 
                                                            uint16 *index);
#endif /* _BOARDAPI_L2_H_ */
