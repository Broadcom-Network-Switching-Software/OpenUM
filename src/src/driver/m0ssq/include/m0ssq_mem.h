/*! \file m0ssq_mem.h
 *
 * Description
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef M0SSQ_MEM_H
#define M0SSQ_MEM_H

/*******************************************************************************
 * Public APIs
 */
/*!
 * \biref Free shared memory object.
 *
 * \param [in] pshmem Pointer to share memory.
 *
 * \retval None.
 */
extern void
m0ssq_mem_free(m0ssq_mem_t *pshmem);

/*!
 * \biref Get shared memory object by name.
 *
 * \param [in] unit Device unit number.
 * \param [in] name Name of memory object.
 * \param [in] uc uC number.
 * \param [in] pshmem Shared memory object pointer.
 *
 * \retval SYS_OK No errors, otherwise return SYS_ERR_XXX.
 */
extern int
m0ssq_mem_get(int unit, int uc, char *name, m0ssq_mem_t **pmem);

/*!
 * \biref Write 32 bits data into shared memory.
 *
 * \param [in] shmem Shared memory object.
 * \param [in] offset Offset within memory object.
 * \param [in] value 32 bits data.
 *
 * \retval SYS_OK No errors, otherwise return SYS_ERR_XXX.
 */
extern int
m0ssq_mem_write(m0ssq_mem_t *mem, uint32 offset, const uint8* buf, int len);

/*!
 * \biref Read 32 bits data from shared memory.
 *
 * \param [in] shmem Shared memory object.
 * \param [in] offset Offset within memory object.
 * \param [out] value Pointer 32 bits data.
 *
 * \retval SYS_OK No errors.
 */
extern int
m0ssq_mem_read(m0ssq_mem_t *mem, uint32 offset, uint8* buf, int len);

/*!
 * \biref Write 32 bits data into shared memory.
 *
 * \param [in] shmem Shared memory object.
 * \param [in] offset Offset within memory object.
 * \param [in] value 32 bits data.
 *
 * \retval SYS_OK No errors, otherwise return SYS_ERR_XXX.
 */
extern int
m0ssq_mem_write32(m0ssq_mem_t *mem, uint32 offset, uint32 value);

/*!
 * \biref Read 32 bits data from shared memory.
 *
 * \param [in] shmem Shared memory object.
 * \param [in] offset Offset within memory object.
 * \param [out] value Pointer 32 bits data.
 *
 * \retval SYS_OK No errors.
 */
extern int
m0ssq_mem_read32(m0ssq_mem_t *mem, uint32 offset, uint32 *value);

/*!
 * \biref Clear shared memory object as zero.
 *
 * \param [in] pshmem Shared memory object..
 *
 * \retval SYS_OK No errors.
 */
extern int
m0ssq_mem_clear(m0ssq_mem_t *pmem);

#endif /* M0SSQ_MEM_H */
