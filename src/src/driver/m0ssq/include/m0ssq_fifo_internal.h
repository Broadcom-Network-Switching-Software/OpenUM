/*! \file m0ssq_fifo_internal.h
 *
 *      M0SSQ FIFO driver provides FIFO methods for MBOX driver.
 *  A FIFO is built on top of memory block which is accessible
 *  for host processor and M0 uC. The read/write pointer of FIFO
 *  are kept in the memory block.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef M0SSQ_FIFO_INTERNAL_H
#define M0SSQ_FIFO_INTERNAL_H

#include <m0ssq_internal.h>

/*******************************************************************************
 * M0SSQ FIFO driver.
 */

/*!
 * \brief FIFO driver data.
 */
typedef struct m0ssq_fifo_s {

    /*! FIFO's memory block. */
    m0ssq_mem_t mem;

    /*! Per FIFO entry size in byte. */
    uint32 per_entry_size;

    /*! Size of available data space. */
    uint32 size;

} m0ssq_fifo_t;

/*!
 * \brief FIFO control pointer.
 */
typedef struct m0ssq_fifo_ptr_s {

    /*! head of fifo. */
    uint32 rp;

    /*! tail of fifo. */
    uint32 wp;

} m0ssq_fifo_ptr_t;

/*!
 * \brief Initialize a FIFO instance.
 *
 * \param [in] fifo FIFO instance.
 * \param [in] mem Memory block for FIFO usage.
 * \param [in] ent_size Size of each FIFO entry.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX if FIFO initialization failed.
 */
extern int
m0ssq_fifo_init(m0ssq_fifo_t *fifo,
                m0ssq_mem_t *mem,
                uint32 ent_size);

/*!
 * \brief Cleanup resource of a FIFO instance.
 *
 * \param [in] fifo FIFO instance.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX if FIFO cleanup failed.
 */
extern int
m0ssq_fifo_cleanup(m0ssq_fifo_t *fifo);

/*!
 * \brief Acquire lock for a FIFO.
 *
 * \param [in] fifo FIFO instance.
 */
extern void
m0ssq_fifo_lock(m0ssq_fifo_t *fifo);

/*!
 * \brief Release lock for a FIFO.
 *
 * \param [in] fifo FIFO instance.
 */
extern void
m0ssq_fifo_unlock(m0ssq_fifo_t *fifo);


/*!
 * \brief Get FIFO write pointer.
 *
 * \param [in] fifo FIFO instance.
 *
 * \retval Value of write pointer.
 */
extern uint32
m0ssq_fifo_wp_get(m0ssq_fifo_t *fifo);


/*!
 * \brief Get FIFO read pointer.
 *
 * \param [in] fifo FIFO instance.
 *
 * \retval Read pointer.
 */
extern uint32
m0ssq_fifo_rp_get(m0ssq_fifo_t *fifo);

/*!
 * \brief Check if there's enough free space for write.
 *
 * \param [in] fifo FIFO instance.
 * \param [in] size Size of write data.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX if no enough free space for write.
 */
extern int
m0ssq_fifo_write_check(m0ssq_fifo_t *fifo, uint32 size);

/*!
 * \brief  Check if there's valid data for read.
 *
 * \param [in] fifo FIFO instance.
 * \param [in] size Size of read data.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX If no enough valid data for read.
 */
extern int
m0ssq_fifo_read_check(m0ssq_fifo_t *fifo, uint32 size);

/*!
 * \brief Move FIFO's read pointer forward.
 *
 * \param [in] fifo FIFO instance.
 * \param [in] size Forward step size.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX Failed.
 */
extern int
m0ssq_fifo_rp_forward(m0ssq_fifo_t *fifo, uint32 size);

/*!
 * \brief Move FIFO's write pointer forward.
 *
 * \param [in] fifo FIFO instance.
 * \param [in] size Forward step size.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX Failed.
 */
extern int
m0ssq_fifo_wp_forward(m0ssq_fifo_t *fifo, uint32 size);

/*!
 * \brief Write data from specific position of FIFO.
 *
 *
 * \param [in] fifo FIFO instance.
 * \param [in] pos  Starting write position within FIFO.
 * \param [in] data Buffer for write data.
 * \param [in] len  Data length for data write. It must be
 *                  multiple of per_entry_size.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX if FIFO initialization failed.
 */
extern int
m0ssq_fifo_pos_write(m0ssq_fifo_t *fifo, uint32 pos,
                     void* data, uint32 len);


/*!
 * \brief Read data from specific position of FIFO.
 *
 *
 * \param [in]  fifo FIFO instance.
 * \param [in]  pos  Starting write position within FIFO.
 * \param [out] data Buffer for read data.
 * \param [in]  len  Data length of read data. It must be
 *                   multiple of per_entry_size.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX if FIFO initialization failed.
 */
extern int
m0ssq_fifo_pos_read(m0ssq_fifo_t *fifo, uint32 pos,
                    void* data, uint32 len);

/*!
 * \brief Write data from write pointer of FIFO.
 *
 * This function
 *   - Write data from write pointer of FIFO.
 *   - Move write pointer forward with size.
 *
 * \param [in]  fifo FIFO instance.
 * \param [in]  data Buffer for write data.
 * \param [in]  len  Data length of write data. It must be
 *                   multiple of per_entry_size.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX if FIFO initialization failed.
 */
extern int
m0ssq_fifo_write(m0ssq_fifo_t *fifo,
                 void* data, uint32 len);

/*!
 * \brief Peek data from read pointer of FIFO.
 *
 * This function only read data from write pointer of FIFO.
 * But the read pointer will not be moved forward.
 *
 * \param [in]  fifo FIFO instance.
 * \param [in]  data Buffer for read data.
 * \param [in]  len  Data length for read data. It must be
 *                   multiple of per_entry_size.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX if FIFO initialization failed.
 */
extern int
m0ssq_fifo_peek_read(m0ssq_fifo_t *fifo,
                     void* data, uint32 len);

/*!
 * \brief Read data from read pointer of FIFO.
 *
 * This function
 *     - Read data from write pointer of FIFO.
 *     - Move write pointer forward with size.
 *
 * \param [in]  fifo FIFO instance.
 * \param [in]  data Buffer for read data.
 * \param [in]  len  Data length for read data. It must be
 *                   multiple of per_entry_size.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX if FIFO initialization failed.
 */
extern int
m0ssq_fifo_read(m0ssq_fifo_t *fifo,
                void* data, uint32 len);


#endif /* M0SSQ_FIFO_INTERNAL_H */
