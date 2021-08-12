/*! \file cmicx_miim.c
 *
 * CMICx MIIM operations
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include <cmicx_miim.h>

/*******************************************************************************
 * Local definitions
 */

/* Operation types for PARAMS_MDIO_OP_TYPE */
#define OPTYPE_INVALID          -1
#define OPTYPE_CL22_WR          0
#define OPTYPE_CL22_RD          1
#define OPTYPE_CL45_WR          2
#define OPTYPE_CL45_RD          3

typedef struct cmicx_miim_acc_s {
    bool wr;
    bool cl45;
    int optype;
} cmicx_miim_acc_t;

#define NUM_MIIM_CHAN           4
#define NUM_MIIM_RING           12

#define MIIM_OP_USEC_MAX        10000
#define MIIM_POLL_USEC          100

#define CMICX_MIIM_LOCK(_u, _op)
#define CMICX_MIIM_UNLOCK(_u, _op)

/*******************************************************************************
 * Private functions
 */

static void
cmicx_miim_acc_init(cmicx_miim_op_t *op, cmicx_miim_acc_t *acc)
{
    sal_memset(acc, 0, sizeof(*acc));
    acc->optype = OPTYPE_INVALID;
    switch (op->opcode) {
    case CMICX_MIIM_OPC_CL22_WRITE:
        acc->optype = OPTYPE_CL22_WR;
        acc->wr = true;
        break;
    case CMICX_MIIM_OPC_CL22_READ:
        acc->optype = OPTYPE_CL22_RD;
        break;
    case CMICX_MIIM_OPC_CL45_WRITE:
        acc->optype = OPTYPE_CL45_WR;
        acc->cl45 = true;
        acc->wr = true;
        break;
    case CMICX_MIIM_OPC_CL45_READ:
        acc->optype = OPTYPE_CL45_RD;
        acc->cl45 = true;
        break;
    default:
        break;
    }
}

/*******************************************************************************
 * Public functions
 */

int
cmicx_miim_op(int unit, cmicx_miim_op_t *op, uint32_t *data)
{
    int ioerr = 0;
    int rv = SYS_OK;
    MIIM_CH_PARAMSr_t params;
    MIIM_CH_ADDRESSr_t addr;
    MIIM_CH_CONTROLr_t ctrl;
    MIIM_CH_STATUSr_t stat;
    cmicx_miim_acc_t acc;
    int to = MIIM_OP_USEC_MAX;

    /* Sanity checks */
    if (op == NULL || data == NULL) {
        return SYS_ERR_PARAMETER;
    }
    if (op->chan < 0) {
        op->chan = 0;
    } else if (op->chan >= NUM_MIIM_CHAN) {
        return SYS_ERR_PARAMETER;
    }

    /* Protocol selection */
    cmicx_miim_acc_init(op, &acc);
    if (acc.optype == OPTYPE_INVALID) {
        return SYS_ERR_PARAMETER;
    }

    /* Calculate PARAMS register */
    MIIM_CH_PARAMSr_CLR(params);
    if (op->internal) {
        MIIM_CH_PARAMSr_SEL_INT_PHYf_SET(params, 1);
    }
    if (op->busmap) {
        MIIM_CH_PARAMSr_RING_MAPf_SET(params, op->busmap);
    } else {
        MIIM_CH_PARAMSr_RING_MAPf_SET(params, 1 << op->busno);
    }
    MIIM_CH_PARAMSr_MDIO_OP_TYPEf_SET(params, acc.optype);
    if (acc.wr) {
        MIIM_CH_PARAMSr_PHY_WR_DATAf_SET(params, *data);
    }

    /* Calculate ADDRESS register */
    MIIM_CH_ADDRESSr_CLR(addr);
    MIIM_CH_ADDRESSr_PHY_IDf_SET(addr, op->phyad);
    if (acc.cl45) {
        MIIM_CH_ADDRESSr_CLAUSE_22_REGADRR_OR_45_DTYPEf_SET(addr, op->devad);
        MIIM_CH_ADDRESSr_CLAUSE_45_REG_ADRRf_SET(addr, op->regad);
    } else {
        MIIM_CH_ADDRESSr_CLAUSE_22_REGADRR_OR_45_DTYPEf_SET(addr, op->regad);
    }

    CMICX_MIIM_LOCK(unit, op);

    /* Write PARAMS and ADDRESS registers */
    ioerr += WRITE_MIIM_CH_PARAMSr(unit, op->chan, params);
    ioerr += WRITE_MIIM_CH_ADDRESSr(unit, op->chan, addr);

    /* Start MIIM operation */
    MIIM_CH_CONTROLr_CLR(ctrl);
    ioerr += WRITE_MIIM_CH_CONTROLr(unit, op->chan, ctrl);
    MIIM_CH_CONTROLr_STARTf_SET(ctrl, 1);
    ioerr += WRITE_MIIM_CH_CONTROLr(unit, op->chan, ctrl);

    /* Poll for completion */
    do {
        ioerr += READ_MIIM_CH_STATUSr(unit, op->chan, stat);
        if (MIIM_CH_STATUSr_DONEf_GET(stat) != 0) {
            break;
        }
        sal_usleep(MIIM_POLL_USEC);
        to -= MIIM_POLL_USEC;
    } while (to >= 0);

    if (MIIM_CH_STATUSr_ERRORf_GET(stat) != 0) {
        rv = SYS_ERR;
    } else if (MIIM_CH_STATUSr_DONEf_GET(stat) == 0) {
        rv = SYS_ERR_TIMEOUT;
    } else if (!acc.wr) {
        *data = MIIM_CH_STATUSr_PHY_RD_DATAf_GET(stat);
    }

    CMICX_MIIM_UNLOCK(unit, op);

    if (ioerr) {
        rv = SYS_ERR;
    }

    if (rv != SYS_OK) {
        sal_printf("MIIM operation failed (op=%d %sbus=%d "
                   "phy=%d dev=%d reg=0x%x)\n",
                   (int)op->opcode, op->internal ? "i" : "e",
                   op->busno, op->phyad, op->devad, op->regad);
    }

#ifdef CMICX_MIIM_DEBUG
    sal_printf("MIIM cl%d-%s %sbus=%d phyad=%d "
               "devad=%d reg=0x%x "
               "data=0x%x\n",
               acc.cl45 ? 45 : 22,
               acc.wr ? "write" : "read",
               op->internal ? "i" : "e",
               op->busno, op->phyad, op->devad, op->regad, *data);
#endif /* CMICX_MIIM_DEBUG */

    return rv;
}

int
cmicx_miim_rate_config_set(int unit, bool internal, int busno,
                           cmicx_miim_rate_config_t *ratecfg)
{
    int ioerr = 0;
    int rx;
    uint32_t divider;
    MIIM_RING_CONTROLr_t ring_ctl;

    if (ratecfg == NULL) {
        return SYS_ERR_PARAMETER;
    }

    if (busno < -1 || busno >= NUM_MIIM_RING) {
        return SYS_ERR_PARAMETER;
    }

    if (ratecfg->dividend != 1) {
        return SYS_ERR_PARAMETER;
    }

    for (rx = 0; rx < NUM_MIIM_RING; rx++) {
        if (busno >= 0 && rx != busno) {
            continue;
        }
        ioerr += READ_MIIM_RING_CONTROLr(unit, rx, ring_ctl);
        divider = ratecfg->divisor;
        if (internal) {
            MIIM_RING_CONTROLr_CLOCK_DIVIDER_INTf_SET(ring_ctl, divider);
        } else {
            MIIM_RING_CONTROLr_CLOCK_DIVIDER_EXTf_SET(ring_ctl, divider);
        }
        ioerr += WRITE_MIIM_RING_CONTROLr(unit, rx, ring_ctl);
    }

    return ioerr ? SYS_ERR : SYS_OK;
}
