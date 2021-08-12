/*! \file m0ssq_fifo.c
 *
 * M0SSQ FIFO driver.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include <system.h>
#include <stddef.h>

#include <utils/shr/shr_debug.h>

#include <m0ssq_internal.h>
#include <m0ssq_mem.h>
#include <m0ssq_fifo_internal.h>

/*******************************************************************************
 * Local definition
 */

/* Macro for FIFO operations. */
#define FIFO_PTR_SIZE       sizeof(m0ssq_fifo_ptr_t)
#define FIFO_PTR_POS        (0)
#define FIFO_PTR_WP_POS     offsetof(m0ssq_fifo_ptr_t, wp)
#define FIFO_PTR_RP_POS     offsetof(m0ssq_fifo_ptr_t, rp)
#define FIFO_WRITE(fifo, pos, data_ptr, len) \
            m0ssq_mem_write(&fifo->mem, pos, (const uint8 *) (data_ptr), len)
#define FIFO_READ(fifo, pos, data_ptr, len) \
            m0ssq_mem_read(&fifo->mem, pos, (uint8 *) (data_ptr), len)
#define FIFO_MEM_SIZE(fifo) (fifo->mem.size)
#define FIFO_SIZE(fifo)     (fifo->size)

/*******************************************************************************
 * M0SSQ FIFO driver.
 */
int
m0ssq_fifo_init(m0ssq_fifo_t *fifo, m0ssq_mem_t *mem, uint32 ent_size)
{
   m0ssq_fifo_ptr_t ptr;

   sal_memcpy(&fifo->mem, mem, sizeof(m0ssq_mem_t));
   fifo->per_entry_size = ent_size;
   ptr.wp = FIFO_PTR_SIZE / fifo->per_entry_size;
   ptr.rp = FIFO_PTR_SIZE / fifo->per_entry_size;
   FIFO_WRITE(fifo, FIFO_PTR_POS, &ptr, FIFO_PTR_SIZE);
   fifo->size = mem->size - FIFO_PTR_SIZE;

   return SYS_OK;
}

int
m0ssq_fifo_cleanup(m0ssq_fifo_t *fifo)
{
    return SYS_OK;
}

static uint32
m0ssq_fifo_used_size(m0ssq_fifo_t *fifo)
{
   uint32 used_size;
   m0ssq_fifo_ptr_t ptr;

   FIFO_READ(fifo, FIFO_PTR_POS, &ptr, FIFO_PTR_SIZE);

   ptr.wp *= fifo->per_entry_size;
   ptr.rp *= fifo->per_entry_size;

   if (ptr.wp >= ptr.rp) {
       used_size = (ptr.wp - ptr.rp);
   } else {
       used_size = (FIFO_MEM_SIZE(fifo) - FIFO_PTR_SIZE) - (ptr.rp - ptr.wp);
   }
   return used_size;
}

static uint32
m0ssq_fifo_free_size(m0ssq_fifo_t *fifo)
{
   uint32 free_size;

   free_size = (FIFO_MEM_SIZE(fifo) - FIFO_PTR_SIZE) -
                m0ssq_fifo_used_size(fifo) -
                fifo->per_entry_size;
   return free_size;
}

int
m0ssq_fifo_write_check(m0ssq_fifo_t *fifo,
                             uint32 size)
{
   if (m0ssq_fifo_free_size(fifo) <= size) {
       /* There is no enough free space. */
       return SYS_ERR;
   }

   /* There is enough free space. */
   return SYS_OK;
}

int
m0ssq_fifo_read_check(m0ssq_fifo_t *fifo,
                            uint32 size)
{
   if (m0ssq_fifo_used_size(fifo) < size) {
       /* There is no enough data. */
       return SYS_ERR_EMPTY;
   }

   /* There is enough data in fifo. */
   return SYS_OK;
}

int
m0ssq_fifo_rp_forward(m0ssq_fifo_t *fifo,
                            uint32 len)
{

   uint32 rp;

   /* Check whether the length excess valid data. */
   if (m0ssq_fifo_read_check(fifo, len)) {
       return SYS_ERR_PARAMETER;
   }

   /* Get current rp. */
   rp = m0ssq_fifo_rp_get(fifo);

   /* Caculate next rp. */
   if ((rp + len) >= FIFO_MEM_SIZE(fifo)) {
       rp = ((rp + len) % FIFO_MEM_SIZE(fifo)) + FIFO_PTR_SIZE;
   } else {
       rp = (rp + len);
   }

   /* Convert point from byte number to entry number. */
   rp /= fifo->per_entry_size;

   /* Update next rp */
   return FIFO_WRITE(fifo, FIFO_PTR_RP_POS, &rp, sizeof(rp));
}

int
m0ssq_fifo_wp_forward(m0ssq_fifo_t *fifo, uint32 len)
{
   uint32 wp;

   /* Check whether request length excess valid data amount. */
   if (m0ssq_fifo_write_check(fifo, len)) {
       return SYS_ERR_PARAMETER;
   }

   /* Get current wp. */
   wp = m0ssq_fifo_wp_get(fifo);

   /* Caculate next wp. */
   if ((wp + len) >= FIFO_MEM_SIZE(fifo)) {
       wp = ((wp + len) % FIFO_MEM_SIZE(fifo)) + FIFO_PTR_SIZE;
   } else {
       wp = (wp + len);
   }

   /* Convert point from byte number to entry number. */
   wp /= fifo->per_entry_size;

   /* Update next wp */
   return FIFO_WRITE(fifo, FIFO_PTR_WP_POS, &wp, sizeof(wp));
}

/*******************************************************************************
 * M0SSQ FIFO public APIs.
 */
void
m0ssq_fifo_unlock(m0ssq_fifo_t *fifo)
{
}

void
m0ssq_fifo_lock(m0ssq_fifo_t *fifo)
{
}


uint32
m0ssq_fifo_wp_get(m0ssq_fifo_t *fifo)
{
    uint32 wp;

    if (fifo == NULL) {
        SHR_LOG_ERROR("Pointer is null.\n");
        return 0;
    }

    FIFO_READ(fifo, FIFO_PTR_WP_POS, &wp, sizeof(wp));

    return (wp * fifo->per_entry_size);
}

uint32
m0ssq_fifo_rp_get(m0ssq_fifo_t *fifo)
{
    uint32 rp;

    if (fifo == NULL) {
        SHR_LOG_ERROR("Pointer is null.\n");
        return 0;
    }

    FIFO_READ(fifo, FIFO_PTR_RP_POS, &rp, sizeof(rp));

    return (rp * fifo->per_entry_size);
}

int
m0ssq_fifo_pos_write(m0ssq_fifo_t *fifo,
                           uint32 pos,
                           void* data, uint32 len)
{
    uint32 len1, len2;
    int rv;

    /* Parameter sanity check. */
    if ((fifo == NULL) ||
        (data == NULL) ||
        (pos >= FIFO_MEM_SIZE(fifo)) ||
        (pos < FIFO_PTR_SIZE) ||
        (len >= FIFO_SIZE(fifo)) ||
        (len == 0))
    {
        return SYS_ERR_PARAMETER;
    }

    /* Write data into FIFO. */
    if ((pos + len) > FIFO_MEM_SIZE(fifo)) {
        len1 = FIFO_MEM_SIZE(fifo) - pos;
        len2 = len - len1;
        rv = FIFO_WRITE(fifo, pos, data, len1);
        if (rv != SYS_OK) {
            return rv;
        }
        rv = FIFO_WRITE(fifo, FIFO_PTR_SIZE,
                       (uint8_t *) data + len1, len2);
    } else {
        rv = FIFO_WRITE(fifo, pos, data, len);
    }

    return rv;
}

int
m0ssq_fifo_pos_read(m0ssq_fifo_t *fifo,
                          uint32 pos,
                          void* data, uint32 len)
{
    uint32 len1, len2;
    int rv;

    /* Parameter sanity check. */
    if ((fifo == NULL) ||
        (data == NULL) ||
        (pos >= FIFO_MEM_SIZE(fifo)) ||
        (pos < FIFO_PTR_SIZE) ||
        (len >= FIFO_SIZE(fifo)) ||
        (len == 0))
    {
        return SYS_ERR_PARAMETER;
    }

    /* Read data from FIFO. */
    if ((pos + len) > FIFO_MEM_SIZE(fifo)) {
        len1 = FIFO_MEM_SIZE(fifo) - pos;
        len2 = len - len1;
        rv = FIFO_READ(fifo, pos, data, len1);
        if (rv != SYS_OK) {
            return rv;
        }
        rv = FIFO_READ(fifo, FIFO_PTR_SIZE,
                      (uint8_t *) data + len1, len2);
    } else {
        rv = FIFO_READ(fifo, pos, data, len);
    }

    return rv;
}

int
m0ssq_fifo_write(m0ssq_fifo_t *fifo,
                       void * data, uint32 len)
{
    uint32 wp;

    /* Check if there is enough free space to write. */
    if (m0ssq_fifo_write_check(fifo, len)) {
        return SYS_ERR_UNAVAIL;
    }

    /* Get current write point of FIFO. */
    wp = m0ssq_fifo_wp_get(fifo);

    /* Write data into FIFO. */
    if (m0ssq_fifo_pos_write(fifo, wp, data, len)) {
        return SYS_ERR;
    }

    /* Update write pointer. */
    return m0ssq_fifo_wp_forward(fifo, len);
}

int
m0ssq_fifo_peek_read(m0ssq_fifo_t *fifo,
                           void * data, uint32 len)
{
    uint32 rp;
    int rv;

    /* Check if there is enough valid data. */
    rv = m0ssq_fifo_read_check(fifo, len);
    if (SHR_FAILURE(rv)) {
        return rv;
    }

    /* Get current read point of FIFO. */
    rp = m0ssq_fifo_rp_get(fifo);

    /* Read data from FIFO. */
    return m0ssq_fifo_pos_read(fifo, rp, data, len);
}

int
m0ssq_fifo_read(m0ssq_fifo_t *fifo,
                      void * data, uint32 len)
{
    int rv = SYS_OK;

    /* Read data from FIFO. */
    rv = m0ssq_fifo_peek_read(fifo, data, len);

    /* Update read pointer. */
    if (rv == SYS_OK) {
        return m0ssq_fifo_rp_forward(fifo, len);
    };

    return rv;
}
