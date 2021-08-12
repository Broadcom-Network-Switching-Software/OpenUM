/*
 * $Id: flash.h,v 1.10 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _FLASH_H_
#define _FLASH_H_


/* Forward reference of the device structure */
struct flash_dev_s;

/* Structure of pointers to functions in the device driver */
struct flash_dev_funs {
  sys_error_t (*flash_init) (struct flash_dev_s *dev) REENTRANT;
  sys_error_t (*flash_erase_block) (struct flash_dev_s *dev,
                                    hsaddr_t block_base) REENTRANT;
  sys_error_t (*flash_program) (struct flash_dev_s *dev,
                                hsaddr_t base,
                                const void* ram_base, size_t len) REENTRANT;
  sys_error_t (*flash_read) (struct flash_dev_s *dev,
                             const hsaddr_t base,
                             void* ram_base, size_t len) REENTRANT;
};

typedef struct flash_block_info_s
{
  size_t  block_size;
  uint32  blocks;
} flash_block_info_t;

/* Information about what one device driver drives */
typedef struct {
  hsaddr_t                  start;              /* First address */
  hsaddr_t                  end;                /* Last address */
  uint32                    num_block_infos;    /* Number of entries */
  const flash_block_info_t* block_info;         /* Info about block sizes */
} flash_info_t;

/*
 * flash device class
 */
typedef struct flash_dev_s{
  uint8 jedec_id[4];                          /* JEDEC ID for flash probe */
  uint8 jedec_id_mask[4];                     /* JEDEC ID mask to mask unwant field of JEDEC ID */
  const struct flash_dev_funs *funs;          /* Function pointers */
  uint32                      flags;          /* Device characteristics */
  hsaddr_t                    start;          /* First address */
  hsaddr_t                    end;            /* Last address */
  uint32                      num_block_infos;/* Number of entries */
  const flash_block_info_t    *block_info;    /* Info about one block size */
  const char *                name;
  const void                  *priv;          /* Devices private data */
} flash_dev_t;

extern sys_error_t flash_erase(hsaddr_t flash_base, size_t len);
extern sys_error_t flash_program(hsaddr_t flash_base, const void *ram_base,
                                 size_t len);
extern sys_error_t flash_read(hsaddr_t flash_base,
                              void *ram_base, size_t len);
extern size_t flash_block_size(const hsaddr_t addr);
extern hsaddr_t flash_block_begin(hsaddr_t flash_base, flash_dev_t *dev);
extern flash_dev_t *flash_dev_get(void);
extern sys_error_t flash_init(flash_dev_t *dev);
extern sys_error_t flash_dump(uint32 offset, uint32 len);

#ifdef __LINUX__
extern sys_error_t flash_file_write(const char *name, uint32 offset);
extern sys_error_t flash_file_read(const char *name, uint32 offset, uint32 len);
#endif /* __LINUX__ */
#endif /* _FLASH_H_ */
