/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"

extern void sal_alloc_init(void *addr, uint32 len);
extern void sal_dma_alloc_init(void *addr, uint32 len);

typedef uint32 uintptr_t;

/*  *********************************************************************
    *  Constants
 */

#define MEMNODE_SEAL 0xFAAFA123     /* just some random constant */
#define MINBLKSIZE 64

/*  *********************************************************************
    *  Types
 */

typedef enum { memnode_free = 0, memnode_alloc } memnode_status_t;

typedef struct memnode_s {
    uint32 seal;
    struct memnode_s *next;     /* pointer to next node */
    uint32 length;              /* length of the entire data section */
    memnode_status_t status;    /* alloc/free status */
    uint8  *data;               /* points to actual user data */
    void   *memnodeptr;         /* memnode back pointer (see comments) */
} memnode_t;

struct mempool_s {
    memnode_t *root;            /* pointer to root node */
    uint8     *base;            /* base of memory region */
    uint32    length;           /* size of memory region */
};

typedef struct mempool_s mempool_t;

#define memnode_data(t,m) (t) (((memnode_t *) (m))+1)

/*  *********************************************************************
    *  Globals
 */
#define MEMPOOL_SIZE 2
static mempool_t kmempool[MEMPOOL_SIZE] = {{0}};          /* default pool */
static mempool_t dmamempool;        /* DMA pool */

static void
_sal_alloc_init(mempool_t *p, void *addr, uint32 len)
{
    p->root = (memnode_t *)addr;
    p->root->seal = MEMNODE_SEAL;
    p->root->length = (uint32)len - sizeof(memnode_t);
    p->root->data = memnode_data(uint8 *,p->root);
    p->root->status = memnode_free;
    p->root->next = NULL;

    p->base = (uint8 *)addr;
    p->length = len;
}

void
sal_alloc_init(void *addr, uint32 len)
{
   int i;
   for (i = 0; i<MEMPOOL_SIZE; i++) {
        if (kmempool[i].length == 0) {
            _sal_alloc_init(&kmempool[i], addr, len);
            return;
        }
   }
}

void
sal_dma_alloc_init(void *addr, uint32 len)
{
    _sal_alloc_init(&dmamempool, addr, len);
}

/*  *********************************************************************
    *  kmemcompact(pool)
    *
    *  Compact the memory blocks, coalescing consectutive free blocks
    *  on the list.
    *
    *  Input parameters:
    *      pool - pool descriptor
    *
    *  Return value:
    *      nothing
 */

static void kmemcompact(mempool_t *p)
{
    memnode_t *m;
    int compacted;

    do {
        compacted = 0;

        for (m = p->root; m; m = m->next) {

            /* Check seal to be sure that we're doing ok */

            if (m->seal != MEMNODE_SEAL) {
    #ifdef TESTPROG
                printf("Memory list corrupted!\n");
    #endif
                return;
            }

            /*
             * If we're not on the last block and both this
             * block and the next one are free, combine them
             */

            if (m->next &&
               (m->status == memnode_free) &&
               (m->next->status == memnode_free)) {
                m->length += sizeof(memnode_t) + m->next->length;
                m->next->seal = 0;
                m->next = m->next->next;
                compacted++;
            }
            /* Keep going till we make a pass without doing anything. */
        }
    } while (compacted > 0);
}


/*  *********************************************************************
    *  kfree(ptr)
    *
    *  Return some memory to the pool.
    *
    *  Input parameters:
    *      ptr - pointer to something allocated via kmalloc()
    *
    *  Return value:
    *      nothing
 */
static
void _sal_free(mempool_t *p, void *ptr)
{
    memnode_t **backptr;
    memnode_t *m;

    if (ptr == NULL) {
        return;
    }

    if (((uint8 *) ptr < p->base) ||
    ((uint8 *) ptr >= (p->base + p->length))) {
#ifdef TESTPROG
        sal_printf("Pointer %08X does not belong to pool %08X\n",ptr,pool);
#endif
        return;
    }

    backptr = (memnode_t **) (((uint8 *) ptr) - sizeof(memnode_t *));
    m = *backptr;

    if (m->seal != MEMNODE_SEAL) {
#ifdef TESTPROG
    printf("Invalid node freed: %08X\n",m);
#endif
        return;
    }

    m->status = memnode_free;

    kmemcompact(p);
}

/*  *********************************************************************
    *  kmalloc(pool,size,align)
    *
    *  Allocate some memory from the pool.
    *
    *  Input parameters:
    *      pool - pool structure
    *      size - size of item to allocate
    *      align - alignment (must be zero or a power of 2)
    *
    *  Return value:
    *      pointer to data, or NULL if no memory left
 */

static void *
_sal_malloc(mempool_t *p, uint32 size)
{
    memnode_t *m;
    memnode_t *newm;
    memnode_t **backptr;
    uintptr_t daddr = 0;
    uintptr_t realsize = 0;
    uintptr_t extra;
    uintptr_t blkend;
    uintptr_t ptralign;

    /*
     * Everything should be aligned by at least the
     * size of an int32
     */

    /* Make it aligned with cache line size. */
    ptralign = 32;

    /*
     * Everything should be at least a multiple of the
     * size of a pointer.
     */

    if (size == 0) {
        size = sizeof(void *);
    }

    if (size & (sizeof(void *)-1)) {
        size += sizeof(void *);
        size &= ~(sizeof(void *)-1);
    }

    /*
     * Find a memnode at least big enough to hold the storage we
     * want.
     */

    for (m = p->root; m; m = m->next) {

        if (m->status == memnode_alloc) continue;

        /*
         * If we wanted a particular alignment, we will
         * need to adjust the size.
         */

        daddr = memnode_data(uintptr_t,m);
        extra = 0;
        if (daddr & (ptralign-1)) {
            extra = ptralign - (daddr & (ptralign-1));
        }
        realsize = size + extra;

        if (m->length < realsize) continue;
        break;
    }

    /*
     * If m is null, there's no memory left.
     */

    if (m == NULL) {
#ifdef TESTPROG
		sal_printf("out of memory\n");
#endif
        return NULL;
    }

    /*
     * Otherwise, use this block.  Calculate the address of the data
     * to preserve the alignment.
     */

    if (daddr & (ptralign-1)) {
        daddr += ptralign;
        daddr &= ~(ptralign-1);
    }

    /* Mark this node as allocated. */

    m->data   = (unsigned char *) daddr;
    m->status = memnode_alloc;

    /*
     * Okay, this is ugly.  Store a pointer to the original
     * memnode just before what we've allocated.  It's guaranteed
     * to be aligned at least well enough for this pointer.
     * If for some reason the memnode was already exactly
     * aligned, backing up will put us inside the memnode
     * structure itself... that's why the memnodeptr field
     * is there, as a placeholder for this eventuality.
     */

    backptr   = (memnode_t **) (m->data - sizeof(memnode_t *));
    *backptr  = m;

    /*
     * See if we need to split it.
     * Don't bother to split if the resulting size will be
     * less than MINBLKSIZE bytes
     */

    if (m->length - realsize < MINBLKSIZE) {
        return m->data;
    }

    /*
     * Split this block.  Align the address on a pointer-size
     * boundary.
     */

    daddr += size;
    if (daddr & (uintptr_t)(sizeof(void *)-1)) {
        daddr += (uintptr_t)sizeof(void *);
        daddr &= ~(uintptr_t)(sizeof(void *)-1);
    }

    blkend = memnode_data(uintptr_t,m) + (uintptr_t)(m->length);

    newm = (memnode_t *) daddr;

    newm->next   = m->next;
    m->length    = (uint32) (daddr - memnode_data(uintptr_t,m));
    m->next      = newm;
    m->status    = memnode_alloc;
    newm->seal   = MEMNODE_SEAL;
    newm->data    = memnode_data(uint8 *,newm);
    newm->length = (uint32) (blkend - memnode_data(uintptr_t,newm));
    newm->status = memnode_free;

    return m->data;
}

void *
sal_malloc(uint32 size) {
    void *ptr = NULL;
    int i;

    for (i=0; i< MEMPOOL_SIZE; i++) {
        ptr = _sal_malloc(&kmempool[i], size);       
        if (ptr) {
            return ptr;
        }
    }
    return NULL;
}

void * 
sal_alloc(uint32 size, const char *str) {
     return sal_malloc(size);
}

void sal_free(void *ptr)
{
    int i;

    for (i=0; i< MEMPOOL_SIZE; i++) {
        if ((uint32) ptr >= (uint32) kmempool[i].base && 
            (uint32) ptr < (uint32) &kmempool[i].base[kmempool[i].length]) {
            _sal_free(&kmempool[i], ptr);
        }
    }   
}

void *
sal_dma_malloc(uint32 size) {
    return _sal_malloc(&dmamempool, size);
}

void sal_dma_free(void *ptr)
{
    _sal_free(&dmamempool, ptr);
}


#ifdef CFG_PCM_SUPPORT_INCLUDED

void sal_free_safe(void* ptr)
{
    if(ptr) {
        sal_free(ptr);
    }
}

void
sal_dma_flush(void *addr, int len)
{
}

/*
 * Function:
 *      sal_dma_inval
 * Purpose:
 *      Ensure cache memory is discarded and not written out to memory.
 * Parameters:
 *      addr - beginning of address region
 *      len - size of address region
 * Notes:
 *      A region of memory should always be invalidated before telling
 *      hardware to start a DMA write into that memory.
 */

void
sal_dma_inval(void *addr, int len)
{
}

void
_sal_assert(const char *expr, const char *file, int line)
{
    sal_printf("ERROR: Assertion failed: (%s) at %s:%d\n",
           expr, file, line);
    for(;;);

}
#endif /* CFG_PCM_SUPPORT_INCLUDED */

