/*! \file mcs_internal.h
 *
 * Internal data structures and APIs for MCS.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef MCS_INTERNAL_H
#define MCS_INTERNAL_H

/*!
 *\brief Maximum number of supported uC.
 */
#define MCS_MAX_NUM_OF_UC             (2)

/*!
 *\brief MCS memory object.
 */
typedef struct mcs_mem_s mcs_mem_t;

/*!
 * \brief Write data into a MCS memory object.
 *
 * \param [in] unit Unit number.
 * \param [in] mem Memory object object, possible memory objects are listed in
 *                 \ref bcmbd_mcs_dev_t.
 * \param [in] offset Starting position of writei operation.
 * \param [in] data Data buffer should be word aligned.
 * \param [in] len  Byte length.
 *
 * \retval SYS_OK Write memory successfully.
 * \retval SYS_ERR Write operation failed.
 */
typedef int
(*mcs_mem_write32_f) (mcs_mem_t *mem,
                      uint32 offset,
                      const uint32 *data,
                      uint32 len);



/*!
 * \brief Read data from a MCS memory object.
 *
 * \param [in] unit Unit number.
 * \param [in] mem Memory object object, possible memory objects are listed in
 *                 \ref mcs_dev_t.
 * \param [in] offset Starting position of read operation.
 * \param [in] data Data buffer should be word aligned.
 * \param [in] len  Byte length,a mutiple of 4.
 *
 * \retval SYS_OK Read memory successfully.
 * \retval SYS_ERR Read operation failed.
 */
typedef int
(*mcs_mem_read32_f) (mcs_mem_t *mem,
                     uint32 offset,
                     uint32 *data,
                     uint32 len);


/*!
 * \brief Structure of a MCS memory object.
 */
struct mcs_mem_s {

    /*! Device unit. */
    int unit;

    /*! Base address of memory object. */
    uint32 base;

    /*! Size of memory object. */
    uint32 size;

    /*! Read function of memory object. */
    mcs_mem_read32_f read32;

    /*! Write function of memory object. */
    mcs_mem_write32_f write32;

};

/*!
 * \brief MCS internal data structure.
 */
typedef struct mcs_dev_s {

    /*! Base address of MCS. */
    uint32 base_addr;

    /*! Number of uC in MCS. */
    uint32 num_of_uc;

    /*! Memory object for shared SRAM in MCS. */
    mcs_mem_t sram;

    /*! Memory object for uC's TCM in MCS. */
    mcs_mem_t tcm[MCS_MAX_NUM_OF_UC];

} mcs_dev_t;

/*!
 * \brief Device-specific MCS driver.
 */
typedef struct mcs_drv_s {

    /*! Init MCS driver. */
    int (*init) (int unit);

    /*! Cleanup MCS driver. */
    int (*cleanup) (int unit);

    /*! Start/reset MCS device. */
    int (*uc_start_set)(int unit, uint32 uc, bool start);

    /*! Get MCS Device start status. */
    int (*uc_start_get)(int unit, uint32 uc, bool *start);

} mcs_drv_t;

/*!
 * \brief Initialize device-specific MCS driver.
 *
 * Note that the driver object will not be copied, so it must be store
 * as static data by the caller.
 *
 * \param [in] unit Unit number.
 * \param [in] drv Pointer to MCS driver object.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX Driver init fail.
 */
extern int
mcs_drv_init(int unit, const mcs_drv_t *drv);

/*!
 * \brief Get device-specific data.
 *
 * \param [in] unit Unit number.
 * \param [out] dev Device data of MCS driver.
 *
 * \retval SYS_OK No errors.
 */
extern int
mcs_dev_get(int unit, mcs_dev_t **dev);

/*!
 * \brief Device-specific initialization.
 *
 * This function is used to initialize the chip-specific
 * configuration, which may include both software and hardware
 * operations.
 *
 * \param [in] unit Unit number.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR MCS driver initialization failed.
 */
extern int
mcs_init(int unit);

/*!
 * \brief Device-specific driver cleanup.
 *
 * This function is used to cleanup the chip-specific configuration.
 *
 * \param [in] unit Unit number.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR Failed to driver cleanup.
 */
extern int
mcs_cleanup(int unit);

/*!
 * \brief Initial CMICx device data template.
 *
 * \param [in] unit Unit number.
 *
 * \retval SYS_OK No errors.
 */
extern int
cmicx_mcs_dev_init(int unit);

#endif /* MCS_INTERNAL_H */
