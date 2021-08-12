/*
 * $Id: symbol.h,v 1.2 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 *
 * Chip symbol table definitions.
 */

#ifndef __SYMBOL_H__
#define __SYMBOL_H__

/*! Max string length of symbol name */
#define SYMBOL_NAME_MAXLEN     80

/*! Max size of register/memory in words */
#define SYMBOL_MAX_WSIZE       32

#ifndef __F_MASK
#define __F_MASK(w) \
        (((uint32_t)1 << w) - 1)
#endif

#ifndef __F_GET
#define __F_GET(d,o,w) \
        (((d) >> o) & __F_MASK(w))
#endif

#ifndef __F_SET
#define __F_SET(d,o,w,v) \
        (d = ((d & ~(__F_MASK(w) << o)) | (((v) & __F_MASK(w)) << o)))
#endif

#ifndef __F_ENCODE

/* Encode a value of a given width at a given offset. Performs compile-time error checking on the value */
/* To ensure it fits within the given width */
#define __F_ENCODE(v,o,w) \
        ( ((v & __F_MASK(w)) == v) ? /* Value fits in width */ ( (uint32_t)(v) << o ) : /* Value does not fit -- compile time error */ 1 << 99)

#endif

/*!
 * Symbol Flags
 *
 * The first 20 bits are reserved for architecture-specific purposes
 * such as block types and address extensions.
 *
 * The final 8 bits are defined as follows:
 */
#define SYMBOL_FLAG_START           0x00100000
#define SYMBOL_FLAG_REGISTER        0x00100000
#define SYMBOL_FLAG_PORT            0x00200000
#define SYMBOL_FLAG_COUNTER         0x00400000
#define SYMBOL_FLAG_MEMORY          0x00800000
#define SYMBOL_FLAG_R64             0x01000000
#define SYMBOL_FLAG_BIG_ENDIAN      0x02000000
#define SYMBOL_FLAG_MEMMAPPED       0x04000000
#define SYMBOL_FLAG_SOFT_PORT       0x08000000
#define SYMBOL_FLAG_TCAM            0x10000000
#define SYMBOL_FLAG_LAST            0x80000000
#define SYMBOL_FLAGS_INVALID        0xffffffff
#define SYMBOL_FLAG_BLKTYPES(_w)    ((_w) & 0x000fffff)

/*!
 * Field Information Structure
 *
 * This structure defines a single field within a symbol
 */
typedef struct symbol_field_info_s {
    const char *name;
    int fid;
    uint16_t minbit;
    uint16_t maxbit;
} symbol_field_info_t;

/*!
 * Index Information
 *
 * Some entries, such as memories and register arrays, have a size and minimum and
 * maximum index value.
 *
 * These are combined in one 32 bit word to save space as follows:
 *
 *      size:   8 bits
 *      min:    5 bits
 *      max:   16 bits
 *      flags:  3 bits
 *
 * The ENCODE macros are used to create the fields in the symbol table entry.
 * Use the GET macro to retrieve it when parsing the symbol.
 *
 * The following flags affect the interpretation of min and max:
 *
 * EXP
 * Used if real max cannot be contained within max field.
 * Real max is encoded as ((1 << max) * min) - 1)
 * Real min is always zero
 * Ex.: (max,min)=(12,17) => real max = ((1 << 12) * 17) - 1) = 0x10fff
 *
 * VAR
 * Used e.g. if register array size is port-dependent.
 * Real max and min depend on configuration
 * min is encoding key which may be passed to an arch-specific function
 *
 * STEP
 * Used e.g. if address step for a register array is non-standard.
 * Real min is always zero
 * min is the address step - 1
 * Ex.: (max,min)=(11,3) => 12 elements with addr(i+1) = addr(i)+4
 */

#define SYMBOL_INDEX_F_EXP  0x1
#define SYMBOL_INDEX_F_VAR  0x2
#define SYMBOL_INDEX_F_STEP 0x4

#define SYMBOL_INDEX_SIZE_ENCODE(s) __F_ENCODE((uint32_t)s, 0, 8)
#define SYMBOL_INDEX_SIZE_GET(w) __F_GET(w, 0, 8)
#define SYMBOL_INDEX_MIN_ENCODE(s) __F_ENCODE((uint32_t)s, 8, 5)
#define SYMBOL_INDEX_MIN_GETRAW(w) __F_GET(w, 8, 5)
#define SYMBOL_INDEX_MAX_ENCODE(s) __F_ENCODE((uint32_t)s, 13, 16)
#define SYMBOL_INDEX_MAX_GETRAW(w) __F_GET(w, 13, 16)
#define SYMBOL_INDEX_FLAGS_ENCODE(s) __F_ENCODE((uint32_t)s, 29, 3)
#define SYMBOL_INDEX_FLAGS_GET(w) __F_GET(w, 29, 3)

#define SYMBOL_INDEX_MIN_GET(w) \
    (SYMBOL_INDEX_FLAGS_GET(w) ? \
       0 : \
       SYMBOL_INDEX_MIN_GETRAW(w))

#define SYMBOL_INDEX_ENC_GET(w) SYMBOL_INDEX_MIN_GETRAW(w)

#define SYMBOL_INDEX_MAX_GET(w) \
    ((SYMBOL_INDEX_FLAGS_GET(w) & SYMBOL_INDEX_F_EXP) ? \
       (SYMBOL_INDEX_MIN_GETRAW(w) * (1 << SYMBOL_INDEX_MAX_GETRAW(w))) - 1 : \
       SYMBOL_INDEX_MAX_GETRAW(w))

#define SYMBOL_INDEX_STEP_GET(w) \
    ((SYMBOL_INDEX_FLAGS_GET(w) & SYMBOL_INDEX_F_STEP) ? \
       (SYMBOL_INDEX_MIN_GETRAW(w) + 1) : \
       1)

#ifdef CFG_CHIP_SYMBOLS_FIELD_INCLUDED
/*!
 * Single Word Field Format
 *
 * Format:
 *
 * {LastField:31:31}{Ext:30:30}{FieldId:29:16}{Maxbit:15:8}{Minbit:7:0}
 *
 * Fields:
 *    LastField:1 Indicates this is the last field descriptor word in a field array
 *    Ext:1       Indicates that this is the first word in a Double Word Field, not
 *                a Single Word Field. This word and the next form a Double Word Field.
 *    FieldId:14  This is the unique field id for this field.
 *    Maxbit:8    This is the field's maxbit.
 *    Minbit:8    This is the field's minbit.
 *
 *
 *
 * Double Word Field Format
 *
 * Format:
 *
 *  {LastField:31:31}{Ext:30:30}{FieldId:29:0}
 *  {Maxbit:31:16}{Minbit:15:0}
 *
 * Fields:
 *     LastField:1 Indicates this is the last field descriptor in a field array.
 *     Ext:1       Indicates this is the start of a Double Word field.
 *     FieldId:30  This is the unique field id for this field.
 *     Maxbit:16   This is the field's maxbit.
 *     Minbit:16   This is the field's minbit.
 */

#define SYMBOL_FIELD_FLAG_LAST      (1L << 31)
#define SYMBOL_FIELD_FLAG_EXT       (1L << 30)

#define SYMBOL_FIELD_LAST(w) ((w) & SYMBOL_FIELD_FLAG_LAST)
#define SYMBOL_FIELD_EXT(w) ((w) & SYMBOL_FIELD_FLAG_EXT)

/*! Single Word Fields */
#define SYMBOL_FIELD_ID_ENCODE(id) __F_ENCODE(id, 16, 14)
#define SYMBOL_FIELD_MAX_ENCODE(b) __F_ENCODE(b, 8, 8)
#define SYMBOL_FIELD_MIN_ENCODE(b) __F_ENCODE(b, 0, 8)

#define SYMBOL_FIELD_ID_GET(w) __F_GET(w, 16, 14)
#define SYMBOL_FIELD_MAX_GET(w) __F_GET(w, 8, 8)
#define SYMBOL_FIELD_MIN_GET(w) __F_GET(w, 0, 8)

#define SYMBOL_FIELD_ENCODE(id, max, min) \
        SYMBOL_FIELD_ID_ENCODE(id) | \
        SYMBOL_FIELD_MAX_ENCODE(max) | \
        SYMBOL_FIELD_MIN_ENCODE(min)

/*! Double Word Fields */
#define SYMBOL_FIELD_EXT_ID_ENCODE(id) __F_ENCODE(id, 0, 30)
#define SYMBOL_FIELD_EXT_MAX_ENCODE(b) __F_ENCODE(b, 16, 16)
#define SYMBOL_FIELD_EXT_MIN_ENCODE(b) __F_ENCODE(b, 0, 16)

#define SYMBOL_FIELD_EXT_ENCODE(id, max, min) \
        SYMBOL_FIELD_FLAG_EXT | SYMBOL_FIELD_EXT_ID_ENCODE(id), \
        SYMBOL_FIELD_EXT_MAX_ENCODE(max) | \
        SYMBOL_FIELD_EXT_MIN_ENCODE(min)

#define SYMBOL_FIELD_EXT_ID_GET(w) __F_GET(w, 0, 30)
#define SYMBOL_FIELD_EXT_MAX_GET(w) __F_GET((&(w))[1], 16, 16)
#define SYMBOL_FIELD_EXT_MIN_GET(w) __F_GET((&(w))[1], 0, 16)

/* Operate on Field Descriptors */
extern uint32_t* symbol_field_info_decode(uint32_t* fp,
                                          symbol_field_info_t* finfo,
                                          const char** fnames);

/*!
 * \brief Extract multi-word field value from multi-word register/memory.
 *
 * \param [in] buf current contents of register/memory (word array)
 * \param [in] finfo field information
 * \param [out] fbuf buffer where to store extracted field value
 * \param [in] buf_wsize word size of buf and fbuf
 *
 * \return 0 on succcess, -1 on failure
 */
extern int symbol_field_value_get(const uint32_t *buf,
                                  const symbol_field_info_t *finfo,
                                  uint32_t *fbuf, uint32_t buf_wsize);

/*!
 * \brief Extract 32-bit field value from multi-word register/memory.
 *
 * \param [in] buf current contents of register/memory (word array)
 * \param [in] finfo field information
 * \param [out] ret_val returned extracted field value
 *
 * \return 0 on succcess, -1 on failure
 */
extern int symbol_field_value_get32(const uint32_t *buf,
                                    const symbol_field_info_t *finfo,
                                    uint32_t *ret_val);

/*!
 * \brief Assign multi-word field value in multi-word register/memory.
 *
 * \param [out] buf current contents of register/memory (word array)
 * \param [in] finfo field information
 * \param [in] fbuf buffer with new field value
 * \param [in] buf_wsize word size of buf and fbuf
 *
 * \return 0 on succcess, -1 on failure
 */
extern int symbol_field_value_set(uint32_t *buf,
                                  const symbol_field_info_t *finfo,
                                  const uint32_t *fbuf, uint32_t buf_wsize);


extern uint32_t
symbol_field_info_count(uint32_t* fp);

#define SYMBOL_FIELDS_ITER_BEGIN(start, finfo, fnames_arr)     \
{                                                              \
    uint32_t* __fp = start;                                    \
    for(;;) {                                                  \
      if(!__fp) {                                              \
        break;                                                 \
      }                                                        \
      else {                                                   \
        __fp = symbol_field_info_decode(__fp, &(finfo), (fnames_arr));

#define SYMBOL_FIELDS_ITER_END() }}}
#endif /* CFG_CHIP_SYMBOLS_FIELD_INCLUDED */

/*!
 * Symbol Information Structure (single symbol)
 */
typedef struct symbol_s {
    uint32_t addr;
#ifdef CFG_CHIP_SYMBOLS_FIELD_INCLUDED
    uint32_t* fields;
#endif
    uint32_t index;
    uint32_t flags;
    const char *name;
#ifdef CDK_CONFIG_INCLUDE_ALIAS_NAMES
    const char *alias;
    const char *ufname;
#endif
} symbol_t;

/*!
 * Symbols Information Structure (plural)
 */
typedef struct symbols_s {
    const symbol_t *symbols;
    const uint32_t size;
    const char** field_names;
} symbols_t;

#endif /* __SYMBOL_H__ */
