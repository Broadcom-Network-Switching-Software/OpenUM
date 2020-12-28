/*! \file cmicx_gpio.c
 *
 * CMICx GPIO driver.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <system.h>

#include <boardapi/gpio.h>
#include <intr.h>

/*******************************************************************************
 * Local definitions
 */
#define GPIO_LOCK(_u)
#define GPIO_UNLOCK(_u)
#define INTR_MASK_LOCK(_u)
#define INTR_MASK_UNLOCK(_u)

#define CMICX_GPIO_NUM               32
#define IPROC_IRQ_GPIO_INTR          44
#define CMICX_MAX_UNITS               1

/* Structure for interrupt vectors. */
typedef struct intr_vect_s {
    board_intr_f func;
    uint32 param;
} intr_vect_t;

typedef struct intr_vect_info_s {
    intr_vect_t intr_vects[CMICX_GPIO_NUM];
} intr_vect_info_t;

/* Variables to store interrupt vectors. */
static intr_vect_info_t cmicx_gpio_intr_vects[CMICX_MAX_UNITS];

/*******************************************************************************
 * Private functions
 */
static inline void
setbit(uint32 *val, int offset, bool set)
{
    if (set) {
        *val |= 1 << offset;
    } else {
        *val &= ~(1 << offset);
    }
}

static inline bool
getbit(uint32 val, int offset)
{
    return (val & (1 << offset)) > 0;
}

/*!
 * \brief Primary GPIO ISR.
 *
 * \param [in] unit Unit number.
 * \param [in] param Interrupt context.
 */
static void
cmicx_gpio_intr_isr(uint32 param)
{
    intr_vect_info_t *ivi;
    intr_vect_t *iv;
    GPIO_INT_MSTATr_t intr_stat;
    GPIO_INT_CLRr_t intr_clear;
    uint32 stat, intr, gpio;
    int unit = 0;

    ivi = &cmicx_gpio_intr_vects[unit];

    /* Read and Clear Masked Interrupt Status Register */
    READ_GPIO_INT_MSTATr(unit, intr_stat);
    GPIO_INT_CLRr_SET(intr_clear, GPIO_INT_MSTATr_GET(intr_stat));
    WRITE_GPIO_INT_CLRr(unit, intr_clear);
    stat = GPIO_INT_MSTATr_INT_MSTATf_GET(intr_stat);


    /* Handle all received interrupts */
    for (gpio = 0; gpio < CMICX_GPIO_NUM; gpio++) {
        intr = (1 << gpio);
        if (stat & intr) {
            /* Received an interrupt. Invoke handler function. */
            iv = &ivi->intr_vects[gpio];
            if (iv->func) {
                iv->func(iv->param);
            }
        }
    }
}

/*******************************************************************************
 * Public functions
 */
int
cmicx_gpio_mode_set(int unit, uint32 gpio, board_gpio_mode_t mode)
{
    int ioerr = 0;
    uint32 en;
    GPIO_OUT_ENr_t out_en;

    /* Sanity checks */
    if (gpio >= CMICX_GPIO_NUM) {
        return SYS_ERR_PARAMETER;
    }

    GPIO_LOCK(unit);

    ioerr += READ_GPIO_OUT_ENr(unit, out_en);
    en = GPIO_OUT_ENr_OUT_ENABLEf_GET(out_en);
    if (mode == BOARD_GPIO_MODE_OUT) {
        setbit(&en, gpio, true);
    } else {
        setbit(&en, gpio, false);
    }
    GPIO_OUT_ENr_OUT_ENABLEf_SET(out_en, en);
    ioerr += WRITE_GPIO_OUT_ENr(unit, out_en);

    GPIO_UNLOCK(unit);

    return (ioerr == 0) ? SYS_OK : SYS_ERR;
}

int
cmicx_gpio_value_set(int unit, uint32 gpio, bool val)
{
    int ioerr = 0;
    uint32 data;
    GPIO_DATA_OUTr_t data_out;

    /* Sanity checks */
    if (gpio >= CMICX_GPIO_NUM) {
        return SYS_ERR_PARAMETER;
    }

    GPIO_LOCK(unit);

    ioerr += READ_GPIO_DATA_OUTr(unit, data_out);
    data = GPIO_DATA_OUTr_DATA_OUTf_GET(data_out);
    setbit(&data, gpio, val);
    GPIO_DATA_OUTr_DATA_OUTf_SET(data_out, data);
    ioerr += WRITE_GPIO_DATA_OUTr(unit, data_out);

    GPIO_UNLOCK(unit);

    return (ioerr == 0) ? SYS_OK : SYS_ERR;
}

int
cmicx_gpio_value_get(int unit, uint32 gpio, bool *val)
{
    int ioerr = 0;
    uint32 data;
    GPIO_DATA_INr_t data_in;

    /* Sanity checks */
    if (gpio >= CMICX_GPIO_NUM) {
        return SYS_ERR_PARAMETER;
    }

    ioerr += READ_GPIO_DATA_INr(unit, data_in);
    data = GPIO_DATA_INr_DATA_INf_GET(data_in);
    *val = getbit(data, gpio);

    return (ioerr == 0) ? SYS_OK : SYS_ERR;
}

/*******************************************************************************
 * Interrupt driver functions
 */
int
cmicx_gpio_intr_func_set(int unit, uint32 gpio,
                             board_intr_f intr_func, uint32 intr_param)
{
    intr_vect_t *vi;
    int rv = SYS_OK;

    /* Sanity checks */
    if (gpio >= CMICX_GPIO_NUM) {
        return SYS_ERR_PARAMETER;
    }

    vi = &cmicx_gpio_intr_vects[unit].intr_vects[gpio];
    vi->func = intr_func;
    vi->param= intr_param;

    /* Install GPIO interrupt handler */
    rv = sys_intr_func_set(IPROC_IRQ_GPIO_INTR,
                           (sys_intr_f) cmicx_gpio_intr_isr, 0);

    return rv;
}

int
cmicx_gpio_intr_enable_set(int unit, uint32 gpio, bool enable)
{
    uint32 mask;
    GPIO_INT_MSKr_t intr_mask;
    int rv = SYS_OK;

    /* Sanity checks */
    if (gpio >= CMICX_GPIO_NUM) {
        return SYS_ERR_PARAMETER;
    }

    INTR_MASK_LOCK(unit);

    if (enable) {
        READ_GPIO_INT_MSKr(unit, intr_mask);
        mask = GPIO_INT_MSKr_INT_MSKf_GET(intr_mask);
        mask |= (1 << gpio);
        GPIO_INT_MSKr_INT_MSKf_SET(intr_mask, mask);
        WRITE_GPIO_INT_MSKr(unit, intr_mask);

        /* Enable GPIO interrupt */
        sys_intr_enable_set(IPROC_IRQ_GPIO_INTR, 1);
    } else {
        READ_GPIO_INT_MSKr(unit, intr_mask);
        mask = GPIO_INT_MSKr_INT_MSKf_GET(intr_mask);
        mask &= ~(1 << gpio);
        GPIO_INT_MSKr_INT_MSKf_SET(intr_mask, mask);
        WRITE_GPIO_INT_MSKr(unit, intr_mask);
        if (mask == 0) {
            /* Disable GPIO interrupt */
            sys_intr_enable_set(IPROC_IRQ_GPIO_INTR, 0);
        }
    }

    INTR_MASK_UNLOCK(unit);

    return rv;
}

int
cmicx_gpio_intr_type_set(int unit, uint32 gpio, board_gpio_intr_type_t type)
{
    int ioerr = 0;

    GPIO_INT_DEr_t intr_de;
    GPIO_INT_TYPEr_t intr_type;
    GPIO_INT_EDGEr_t intr_edge;

    /* Sanity checks */
    if (gpio >= CMICX_GPIO_NUM) {
        return SYS_ERR_PARAMETER;
    }

    /* Read GPIO interrupt register. */
    ioerr = READ_GPIO_INT_DEr(unit, intr_de);
    ioerr += READ_GPIO_INT_TYPEr(unit, intr_type);
    ioerr += READ_GPIO_INT_EDGEr(unit, intr_edge);

    /* Modify to falling edge trigger. */
    GPIO_INT_DEr_SET(intr_de, GPIO_INT_DEr_GET(intr_de) & ~(1 << gpio));
    GPIO_INT_TYPEr_SET(intr_type, GPIO_INT_TYPEr_GET(intr_type) & ~(1 << gpio));
    GPIO_INT_EDGEr_SET(intr_edge, GPIO_INT_EDGEr_GET(intr_edge) & ~(1 << gpio));

    switch (type) {

    /* Dual edge trigger. */
    case BOARD_GPIO_INTR_DUAL_EDGE_TRIGGER:
        GPIO_INT_DEr_SET(intr_de, GPIO_INT_DEr_GET(intr_de) | (1 << gpio));
        break;

    /* Falling edge trigger. */
    case BOARD_GPIO_INTR_FALL_EDGE_TRIGGER:
        break;

    /* Rising edge trigger. */
    case BOARD_GPIO_INTR_RISE_EDGE_TRIGGER:
        GPIO_INT_EDGEr_SET(intr_edge, GPIO_INT_EDGEr_GET(intr_edge) | (1 << gpio));
        break;

    /* Low level trigger. */
    case BOARD_GPIO_INTR_LOW_LEVEL_TRIGGER:
        GPIO_INT_TYPEr_SET(intr_type, GPIO_INT_TYPEr_GET(intr_type) | (1 << gpio));
        break;

    /* High level trigger. */
    case BOARD_GPIO_INTR_HIGH_LEVEL_TRIGGER:
        GPIO_INT_TYPEr_SET(intr_type, GPIO_INT_TYPEr_GET(intr_type) | (1 << gpio));
        GPIO_INT_EDGEr_SET(intr_edge, GPIO_INT_EDGEr_GET(intr_edge) | (1 << gpio));
        break;

    /* Unknown value. */
    default:
        return SYS_ERR_PARAMETER;
    }

    /* Write GPIO interrupt register. */
    ioerr += WRITE_GPIO_INT_DEr(unit, intr_de);
    ioerr += WRITE_GPIO_INT_TYPEr(unit, intr_type);
    ioerr += WRITE_GPIO_INT_EDGEr(unit, intr_edge);

    return (ioerr == 0) ? SYS_OK : SYS_ERR;
}

int
cmicx_gpio_intr_status_clear(int unit, uint32 gpio)
{
    int rv = SYS_OK;
    GPIO_INT_CLRr_t intr_clear;

    /* Sanity checks */
    if (gpio >= CMICX_GPIO_NUM) {
        return SYS_ERR_PARAMETER;
    }

    GPIO_INT_CLRr_SET(intr_clear, (1 << gpio));
    WRITE_GPIO_INT_CLRr(unit, intr_clear);

    return rv;
}
