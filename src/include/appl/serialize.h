/*
 * $Id: serialize.h,v 1.5 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _SERIALIZE_H_
#define _SERIALIZE_H_

#ifdef CFG_PERSISTENCE_STRING_SUPPORT_INCLUDED
/* 
 * Medium functions for string-type serializer
 */
typedef struct {
    const char * (*read_string)(void) REENTRANT;
    BOOL (*write_string)(const char *str) REENTRANT;
    
    void *dummy1; /* Dummy entry for validation; Do not touch it. */
    void *dummy2; /* Dummy entry for validation; Do not touch it. */
    void *dummy3; /* Dummy entry for validation; Do not touch it. */
    void *dummy4; /* Dummy entry for validation; Do not touch it. */
    void *dummy5; /* Dummy entry for validation; Do not touch it. */
    void *dummy6; /* Dummy entry for validation; Do not touch it. */
} SERL_MEDIUM_STR;
#endif /* CFG_PERSISTENCE_STRING_SUPPORT_INCLUDED */

/* 
 * Medium functions for binary-type serializer
 */
typedef struct {

#ifdef CFG_PERSISTENCE_STRING_SUPPORT_INCLUDED
    void *dummy1; /* Dummy entry for validation; Do not touch it. */
    void *dummy2; /* Dummy entry for validation; Do not touch it. */
#endif /* CFG_PERSISTENCE_STRING_SUPPORT_INCLUDED */
    
    int32 (*read)(uint8 *buf, uint32 len) REENTRANT;
    int32 (*write)(uint8 *buf, uint32 len) REENTRANT;
    int32 (*read_uint32)(uint32 *pval) REENTRANT;
    int32 (*write_uint32)(uint32 val) REENTRANT;
    int32 (*read_uint16)(uint16 *pval) REENTRANT;
    int32 (*write_uint16)(uint16 val) REENTRANT;
} SERL_MEDIUM_BIN;

/*
 * Operations supported for a serializer.
 */
typedef enum {
    SERIALIZE_OP_COUNT = 0,         /* To get the size (bytes) of data */
    SERIALIZE_OP_LOAD,              /* To load data from persistent medium */
    SERIALIZE_OP_SAVE,              /* To save data to persistent medium */
    SERIALIZE_OP_VERSION,           /* To get version of this serializer */
    SERIALIZE_OP_LOAD_DEFAULTS,     /* To load factory defaults */
    SERIALIZE_OP_SAVE_DEFAULTS,     /* Used only in serialization level */
    SERIALIZE_OP_VALIDATE,          /* Used internally */
    SERIALIZE_OP_VALIDATE_DEFAULTS  /* Used internally */
} SERIALIZE_OP;

/*
 * Serializer version: (16 bit)
 *      The value returned from any serializer fro SERIALIZE_OP_VERSION
 *      must contain this magic number as upper word.
 *      This is to avoid developer error for not returning version number.
 */
#define SERIALIZE_VERSION_MASK  (0x0000FFFF)
#define SERIALIZE_VERSION_MAGIC (0x88740000)

/*
 * Convenient macro for making up a valid serializer version
 *      that can be used in the implementation of serializer
 */
#define MAKE_SERIALIZER_VERSION(x) \
        (int32)((x & SERIALIZE_VERSION_MASK) | SERIALIZE_VERSION_MAGIC)

#ifdef CFG_PERSISTENCE_STRING_SUPPORT_INCLUDED
/* 
 * Prototype for string-type serializer
 *   parameters:
 *        op - SERIALIZE_OP_COUNT, 
 *             SERIALIZE_OP_LOAD, 
 *             SERIALIZE_OP_SAVE, or
 *             SERIALIZE_OP_VERSION
 *        medium - use it to load/save string
 *   return:
 *        If op is SERIALIZE_OP_VERSION, the return value is a valid serializer
 *        version containing SERIALIZE_VERSION_MAGIC magic number; Otherwise, 
 *        it's the max string length (excluding string terminator);
 *        (Used mainly for compact medium to reserve space)
 *        return -1 if error.
 */
typedef int32 (*SERIALIZER_STR)(SERIALIZE_OP op, SERL_MEDIUM_STR *medium) REENTRANT;
#endif /* CFG_PERSISTENCE_STRING_SUPPORT_INCLUDED */

/* 
 * Prototype for binary-type serializer
 *   parameters:
 *        op - SERIALIZE_OP_COUNT, 
 *             SERIALIZE_OP_LOAD, 
 *             SERIALIZE_OP_SAVE, or
 *             SERIALIZE_OP_VERSION
 *        medium - use it to read/write data from/to
 *   Return value:
 *        If op is SERIALIZE_OP_VERSION, the return value is a valid serializer
 *        version containing SERIALIZE_VERSION_MAGIC magic number; Otherwise, 
 *        it's number of bytes required (for SERIALIZE_OP_COUNT) or 
 *        processed (for SERIALIZE_OP_LOAD or SERIALIZE_OP_SAVE).
 *        return -1 if error.
 */
typedef int32 (*SERIALIZER_BIN)(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT;

#ifdef CFG_PERSISTENCE_STRING_SUPPORT_INCLUDED
/*
 * register_string_serializer(): register string-type serializer
 *   Parameters:
 *        name - friendly name of this serializer
 *        groups - 16-bit bitmap of groups this serializer belong to
 *        func - serializer function
 *        defaults - whether it supports SERIALIZE_OP_LOAD_DEFAULTS or not
 *   Return value:
 *        true if succeeded; false otherwise.
 */
BOOL register_string_serializer(
        const char *name, 
        uint32 groups, 
        SERIALIZER_STR func,
        BOOL defaults
        );
#endif /* CFG_PERSISTENCE_STRING_SUPPORT_INCLUDED */

/*
 * register_binary_serializer(): register binary-type serializer
 *   Parameters:
 *        name - friendly name of this serializer
 *        groups - 16-bit bitmap of groups this serializer belong to
 *        func - serializer function
 *        defaults - whether it supports SERIALIZE_OP_LOAD_DEFAULTS or not
 *   Return value:
 *        true if succeeded; false otherwise.
 */
BOOL register_binary_serializer(
        const char *name, 
        uint32 groups, 
        SERIALIZER_BIN func,
        BOOL defaults
        );

#endif /* _SERIALIZE_H_ */
