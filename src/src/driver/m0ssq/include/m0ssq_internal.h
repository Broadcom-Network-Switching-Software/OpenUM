/*! \file m0ssq_internal.h
 *
 * M0SSQ driver internal definitions.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef M0SSQ_INTERNAL_H
#define M0SSQ_INTERNAL_H

#include <m0ssq.h>

/*******************************************************************************
 * Local definition.
 */

/*! Maximum supported unit number. */
#define CONFIG_MAX_UNITS                          1

/*! Maximum ports. */
#define CONFIG_MAX_PORTS                          SOC_MAX_NUM_PORTS

/*! Unit number check macro. */
#define UNIT_VALID(_u)                            (_u < CONFIG_MAX_UNITS)

/*! sal_alloc for porting. */
#define sal_alloc(_size_, _desc_)                 sal_malloc(_size_)

/*! brief Maximun number of supported uC. */
#define M0SSQ_MAX_NUM_OF_UC             (4)

/*!
 * \brief M0SSQ memory object.
 */
typedef struct m0ssq_mem_s m0ssq_mem_t;

/*******************************************************************************
 * M0SSQ driver function prototypes
 */

/*!
 * \brief Device-specific M0SSQ driver initialization.
 *
 * This function is used to initialize the chip-specific M0SSQ
 * configuration, which may include both software and hardware
 * operations.
 *
 * \param [in] unit Unit number.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX M0SSQ driver initialization failed.
 */
typedef int
(*m0ssq_init_f)(int unit);

/*!
 * \brief Device-specific M0SSQ driver cleanup.
 *
 * This function is used to cleanup the chip-specific configuration.
 *
 * \param [in] unit Unit number.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX Failed to driver cleanup.
 */
typedef int
(*m0ssq_cleanup_f)(int unit);

/*!
 *\brief Start/Stop a uC.
 *
 * This function is to start/stop a uC.
 *
 * \param [in]  unit Unit number.
 * \param [in]  uc uC number.
 * \param [in]  start !0: start a uC.
 *                    0: stop a uC.
 *
 * \retval SYS_OK Start/stop of uC successed.
 * \retval SYS_ERR_XXX Start/stop of uC failed .
 */
typedef int
(*m0ssq_uc_start_set_f)(int unit, uint32 uc, bool start);


/*!
 *\brief Check uC is started or not.
 *
 * This function is to check if a uC is started or not.
 *
 * \param [in]   unit Unit number.
 * \param [in]   uc uC number in M0SSQ.
 * \param [out]  start Indicate uC is started or not.
 *
 * \return SYS_OK if successful.
 */
typedef int
(*m0ssq_uc_start_get_f)(int unit, uint32 uc, bool *start);

/*******************************************************************************
 * M0SSQ and M0SSQ-MBOX shared structure.
 */
/*!
 * \brief Write data into M0SSQ memory block.
 *
 * \param [in] mem Memory block object, possible memory objects are listed in
 *                 \ref m0ssq_dev_t.
 * \param [in] offset Starting position of writei operation.
 * \param [in] data Data buffer should be word aligned.
 * \param [in] len  Byte length.
 *
 * \retval SYS_OK Write memory successfully.
 * \retval SYS_ERR_XXX Write operation failed.
 */
typedef int
(*m0ssq_mem_write32_f) (int unit, uint32 addr, uint32 data);

/*!
 * \brief Read data from M0SSQ memory block.
 *
 * \param [in] mem Memory block object, possible memory objects are listed in
 *                 \ref m0ssq_dev_t.
 * \param [in] offset Starting position of read operation.
 * \param [in] data Data buffer should be word aligned.
 * \param [in] len  Byte length,a mutiple of 4.
 *
 * \retval SYS_OK Read memory successfully.
 * \retval SYS_ERR_XXX Read operation failed.
 */
typedef int
(*m0ssq_mem_read32_f) (int unit, uint32 addr, uint32 *data);

/*!
 * \brief Structure of M0SSQ memory object.
 */
struct m0ssq_mem_s {

    /*! Base address of memory block. */
    int unit;

    /*! Base address of memory block. */
    uint32 base;

    /*! Size of memory block. */
    uint32 size;

    /*! Read function of memory block. */
    m0ssq_mem_read32_f read32;

    /*! Write function of memory block. */
    m0ssq_mem_write32_f write32;

};

/*!
 * \brief M0SSQ driver object.
 */
typedef struct m0ssq_drv_s {

    /*! Initialize M0SSQ driver. */
    m0ssq_init_f init;

    /*! Cleanup M0SSQ driver. */
    m0ssq_cleanup_f cleanup;

    /*! Start/Stop uC. */
    m0ssq_uc_start_set_f uc_start_set;

    /*! Get uC is started or not. */
    m0ssq_uc_start_get_f uc_start_get;

} m0ssq_drv_t;


/*!
 * \brief Firmware initial configuration.
 */
typedef struct fwconfig_s {

    /*! chip device id. */
    uint32 devid;

    /*! chip revision id. */
    uint32 revid;

    /*! uC number. */
    uint32 uc;

    /*! Size of fwconfig_t */
    uint32 len;

    /*! Checksum */
    uint32 chksum;

    /*! Magic number. */
    uint32 magic;

} fwconfig_t;

/*!
 *\brief Magic number of fwconfig_t.
 */
#define M0SSQ_FWCFG_MAGIC          (0x4D304657)

/*!
 * \brief M0SSQ internal data structure.
 */
typedef struct m0ssq_dev_s {

    /*! Base address of M0SSQ. */
    uint32 base_addr;

    /*! Number of uC in M0SSQ. */
    uint32 num_of_uc;

    /*! uC swintr handler on host processor side. */
    sys_intr_f uc_swintr_handler[M0SSQ_MAX_NUM_OF_UC];

    /*! Parameter of uC's swintr handler. */
    uint32 uc_swintr_param[M0SSQ_MAX_NUM_OF_UC];

    /*! uC's software interrupt enable. */
    bool uc_swintr_enable[M0SSQ_MAX_NUM_OF_UC];

    /*! Memory object for shared SRAM in M0SSQ. */
    m0ssq_mem_t sram;

    /*! Memory object for LED accumulation RAM in M0SSQ. */
    m0ssq_mem_t led_accu_ram;

    /*! Memory object for LED send RAM in M0SSQ. */
    m0ssq_mem_t led_send_ram;

    /*! Memory object for uC's TCM in M0SSQ. */
    m0ssq_mem_t m0_tcm[M0SSQ_MAX_NUM_OF_UC];

    /*! Memory object for firmware initial configuration. */
    m0ssq_mem_t fwconfig_mem[M0SSQ_MAX_NUM_OF_UC];

    /*! Firmware initial configuration. */
    fwconfig_t fwconfig[M0SSQ_MAX_NUM_OF_UC];

} m0ssq_dev_t;

/*******************************************************************************
 * BCMBD internal APIs.
 */

/*!
 * \brief Install device-specific M0SSQ driver.
 *
 * Install device-specific M0SSQ driver into top-level M0SSQ API.
 *
 * Use \c m0ssq_drv = NULL to uninstall a driver.
 *
 * \param [in] unit Unit number.
 * \param [in] m0ssq_drv M0SSQ driver object.
 *
 * \retval 0 No errors.
 */
extern int
m0ssq_drv_init(int unit, const m0ssq_drv_t *m0ssq_drv);

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
 * \retval SYS_ERR_XXX M0SSQ driver initialization failed.
 */
extern int
m0ssq_init(int unit);

/*!
 * \brief Device-specific driver cleanup.
 *
 * This function is used to cleanup the chip-specific configuration.
 *
 * \param [in] unit Unit number.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX Failed to driver cleanup.
 */
extern int
m0ssq_cleanup(int unit);

/*!
 * \brief Get device-specific data.
 *
 * \param [in] unit Unit number.
 * \param [out] dev Device data of M0SSQ driver.
 *
 * \retval SYS_OK No errors.
 */
extern int
m0ssq_dev_get(int unit, m0ssq_dev_t **dev);

/*!
 * \brief Read function for iProc memory block.
 *
 * \param [in] mem Memory block object.
 * \param [in] offset Starting offset of data read within a memory block.
 * \param [out] data Buffer for read data.
 * \param [in] len Byte length of data read.
 *
 * \retval SYS_OK No errors.
 */
extern int
m0ssq_iproc_mem_read32(m0ssq_mem_t *mem,
                       uint32 offset,
                       uint32 *data, uint32 len);

/*!
 * \brief Write function for iProc memory block.
 *
 * \param [in] mem Memory block object.
 * \param [in] offset Starting offset of data write within a memory block.
 * \param [in] data Buffer for write data.
 * \param [in] len Byte length of write data.
 *
 * \retval SYS_OK No errors.
 */
extern int
m0ssq_iproc_mem_write32(m0ssq_mem_t *mem,
                        uint32 offset,
                        const uint32 *data, uint32 len);


/*!
 * \brief Clear memory block by zeros.
 *
 * \param [in] mem Memory block object.
 *
 * \retval SYS_OK No errors.
 */
extern int
m0ssq_mem_clear(m0ssq_mem_t *mem);

/*!
 * \brief Calculate checksum of M0 firmware config.
 *
 * \param [in] fwcfg M0 firmware config.
 * \param [out] pchksum Checksum of M0 firmware config.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_PARAMETER Wrong parameter.
 */
extern int
m0ssq_fwconfig_chksum(fwconfig_t fwcfg, uint32 *pchksum);

#endif /* M0SSQ_INTERNAL_H */
