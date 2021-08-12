/*
 * $Id: pbsmh.h,v 1.44 Broadcom SDK $
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 *
 * File:        pbsmh.h
 */
#ifndef _SOC_PBSMH_H
#define _SOC_PBSMH_H

#include <types.h>

/****************************************************************************
 * PBS MH Header Format
 *
 * On XGS3 devices this is used to send a Packet Byte Stream out a port.
 *
 * ANSI only permits signed or unsigned int for bit field type.  This
 * structure will only work for compilers for which uint32 is unsigned
 * int, and which completely pack bit fields beginning at the MSbit for
 * big-endian machines and at the LSbit for little-endian machines.
 *
 * NOTE: these structures already put the the individual bytes in memory
 * in big endian order for both big- and little-endian machines, so no
 * further swapping is required.
 */



typedef struct soc_pbsmh_hdr_s {
#if defined(LE_HOST)
                            /* Byte # */
    uint32  start:8;        /* 0 */
    uint32  _rsvd0:24;      /* 1-3 */
    uint32  _rsvd1;         /* 4-7 */
    uint32  _rsvd2:16;      /* 8-9 */
    uint32  src_mod:6;      /* 10 */
    uint32  _rsvd3:2;       /* 10 */
    uint32  dst_port:5;     /* 11 */
    uint32  cos:3;          /* 11 */
#else /* !LE_HOST */
                            /* Byte # */
    uint32  start:8;        /* 0 */
    uint32  _rsvd0:24;      /* 1-3 */
    uint32  _rsvd1;         /* 4-7 */
    uint32  _rsvd2:16;      /* 8-10 */
    uint32  _rsvd3:2;       /* 10 */
    uint32  src_mod:6;      /* 10 */
    uint32  cos:3;          /* 11 */
    uint32  dst_port:5;     /* 11 */
#endif /* !LE_HOST */
} soc_pbsmh_hdr_t;

typedef union soc_pbsmh_v7_hdr_s {
#if defined(LE_HOST)
    struct {                            /* Byte # */
        uint32  header_type:6;          /* 0 */
        uint32  start:2;                /* 0 */
        uint32  ep_cpu_reason_code_3:4;   /* 1 ' */
        uint32  _rsvd5:4;                 /* 1 ' */
        uint32  ep_cpu_reason_code_2:8;   /* 2 ' */
        uint32  ep_cpu_reason_code_1:8;   /* 3 ' */
        uint32  oam_replacement_offset:6;   /* 4 */
        uint32  queue_num_3:2;              /* 4 */
        uint32  set_l3bm:1;             /* 5 */
        uint32  tx_ts:1;                /* 5 */
        uint32  its_sign:1;             /* 5 */
        uint32  regen_udp_checksum:1;   /* 5 */
        uint32  osts:1;                 /* 5 */
        uint32  oam_replacement_type:2; /* 5 */
        uint32  cell_error:1;           /* 5 */
        uint32  hdr_offset:8;           /* 6 */
        uint32  dst_port:7;             /* 7 */
        uint32  set_l2bm:1;             /* 7 */
        uint32  int_pri:4;              /* 8 ' */
        uint32  input_pri:4;            /* 8 */
        uint32  queue_num_2:2;          /* 9 */
        uint32  nlf_port:6;             /* 9 ' */
        uint32  queue_num_1:8;          /* 10 */
        uint32  src_mod:8;              /* 11 */
    } overlay1;

    struct {                            /* Byte # */
        uint32  header_type:6;          /* 0 */
        uint32  start:2;                /* 0 */
        uint32  _rsvd3:8;               /* 1 ' */
        uint32  lm_counter_index_2:8;   /* 2 ' */
        uint32  lm_counter_index_1:8;   /* 3 ' */
        uint32  oam_replacement_offset:6;   /* 4 */
        uint32  queue_num_3:2;              /* 4 */
        uint32  set_l3bm:1;             /* 5 */
        uint32  tx_ts:1;                /* 5 */
        uint32  its_sign:1;             /* 5 */
        uint32  regen_udp_checksum:1;   /* 5 */
        uint32  osts:1;                 /* 5 */
        uint32  oam_replacement_type:2; /* 5 */
        uint32  _rsvd1:1;               /* 5 */
        uint32  hdr_offset:8;           /* 6 */
        uint32  dst_port:7;             /* 7 */
        uint32  set_l2bm:1;             /* 7 */
        uint32  cos:4;                  /* 8 ' */
        uint32  input_pri:4;            /* 8 */
        uint32  queue_num_2:2;          /* 9 */
        uint32  unicast:1;              /* 9 ' */
        uint32  spap:2;                 /* 9 ' */
        uint32  spid:2;                 /* 9 ' */
        uint32  spid_override:1;        /* 9'  */
        uint32  queue_num_1:8;          /* 10 */
        uint32  src_mod:8;              /* 11 */
    } overlay2;
#else /* !LE_HOST */

    struct {                            /* Byte # */
        uint32  start:2;                /* 0 */
        uint32  header_type:6;          /* 0 */
        uint32  _rsvd5:4;               /* 1 ' */
        uint32  ep_cpu_reason_code_3:4; /* 1 ' */
        uint32  ep_cpu_reason_code_2:8; /* 2 ' */
        uint32  ep_cpu_reason_code_1:8; /* 3 ' */
        uint32  queue_num_3:2;              /* 4 */
        uint32  oam_replacement_offset:6;   /* 4 */
        uint32  cell_error:1;           /* 5 */
        uint32  oam_replacement_type:2; /* 5 */
        uint32  osts:1;                 /* 5 */
        uint32  regen_udp_checksum:1;   /* 5 */
        uint32  its_sign:1;             /* 5 */
        uint32  tx_ts:1;                /* 5 */
        uint32  set_l3bm:1;             /* 5 */
        uint32  hdr_offset:8;           /* 6 */
        uint32  set_l2bm:1;             /* 7 */
        uint32  dst_port:7;             /* 7 */
        uint32  input_pri:4;            /* 8 */
        uint32  int_pri:4;              /* 8 ' */
        uint32  nlf_port:6;             /* 9 ' */
        uint32  queue_num_2:2;          /* 9 */
        uint32  queue_num_1:8;          /* 10 */
        uint32  src_mod:8;              /* 11 */
    } overlay1;

    struct {                            /* Byte # */
        uint32  start:2;                /* 0 */
        uint32  header_type:6;          /* 0 */
        uint32  _rsvd3:8;               /* 1 ' */
        uint32  lm_counter_index_2:8;   /* 2 ' */
        uint32  lm_counter_index_1:8;   /* 3 ' */
        uint32  queue_num_3:2;              /* 4 */
        uint32  oam_replacement_offset:6;   /* 4 */
        uint32  _rsvd1:1;                   /* 5 */
        uint32  oam_replacement_type:2;     /* 5 */
        uint32  osts:1;                 /* 5 */
        uint32  regen_udp_checksum:1;   /* 5 */
        uint32  its_sign:1;             /* 5 */
        uint32  tx_ts:1;                /* 5 */
        uint32  set_l3bm:1;             /* 5 */
        uint32  hdr_offset:8;           /* 6 */
        uint32  set_l2bm:1;             /* 7 */
        uint32  dst_port:7;             /* 7 */
        uint32  input_pri:4;            /* 8 */
        uint32  cos:4;                  /* 8 ' */
        uint32  spid_override:1;        /* 9 ' */
        uint32  spid:2;                 /* 9 ' */
        uint32  spap:2;                 /* 9 ' */
        uint32  unicast:1;              /* 9 ' */
        uint32  queue_num_2:2;          /* 9 */
        uint32  queue_num_1:8;          /* 10 */
        uint32  src_mod:8;              /* 11 */
    } overlay2;
#endif /* !LE_HOST */
} soc_pbsmh_v7_hdr_t;


#define PBS_MH_V7_HDR_TYPE_EP_COPY_TO_CPU       0
#define PBS_MH_V7_HDR_TYPE_FROM_CPU             1
#define PBS_MH_V7_HDR_TYPE_MIM                  2
#define PBS_MH_V7_HDR_TYPE_MPLS_PMP             3
#define PBS_MH_V7_HDR_TYPE_TRILL_NW             4
#define PBS_MH_V7_HDR_TYPE_TRILL_AC             5
#define PBS_MH_V7_HDR_TYPE_WLAN_DECAP           6
#define PBS_MH_V7_HDR_TYPE_WLAN_ENCAP           7
#define PBS_MH_V7_HDR_TYPE_QCN                  8
#define PBS_MH_V7_HDR_TYPE_SM_DPI               9
#define PBS_MH_V7_HDR_TYPE_EP_REDIER            0xA
#define PBS_MH_V7_HDR_TYPE_GENERIC              0xB

typedef struct soc_pbsmh_field_attr {
    /* use SOC_PBSMH_GET_BIT_POS to convert regsfile bit offset */
    unsigned int off;
    unsigned int size;  /* field's size 1 ... 31 */
} soc_pbsmh_field_attr_t;


/*
 * Fast macros to setup the PBSMH
 */
#if defined(LE_HOST)
#define PBS_MH_W0_START_SET(mh) (((uint32 *)(mh))[0] = 0x000000FF)
#define PBS_MH_W1_RSVD_SET(mh)  (((uint32 *)(mh))[1] = 0x00000000)

#define PBS_MH_V1_W2_SMOD_DPORT_COS_SET(mh, smod, dport, cos, dc1, dc2) \
        (((uint32 *)(mh))[2] = (smod << 16) | (cos << 29) | (dport << 24))

#define PBS_MH_V2_W2_SMOD_DPORT_COS_SET(mh, smod, dport, cos, ipri, l3pbm_sel) \
        (((uint32 *)(mh))[2] = (smod << 8) | (ipri << 20) | (cos << 16) | \
                               (l3pbm_sel << 31) | (dport << 24))

#define PBS_MH_V3_W2_SMOD_DPORT_COS_SET(mh, smod, dport, cos, ipri, l3pbm_sel) \
        (((uint32 *)(mh))[2] = ((smod >> 4) & 0xf) | ((smod & 0xf) << 12) | \
                               (ipri << 8) | (cos << 16) | \
                               (l3pbm_sel << 31) | (dport << 24))

#define PBS_MH_V4_W2_SMOD_DPORT_COS_SET(mh, smod, dport, cos, ipri, l3pbm_sel) \
        (((uint32 *)(mh))[2] = ((smod >> 7) & 0x1) | ((smod & 0x7f) << 9) | \
                               (((ipri >> 3) & 0x1) << 8) | \
                               ((ipri & 0x7) << 21) | (cos << 16) | \
                               (l3pbm_sel << 31) | (dport << 24))

#define PBS_MH_V5_W1_SMOD_SET(mh, smod, unicast, set_l2bm, set_l3bm) \
        (((uint32 *)(mh))[1] = (((smod & 0xff) << 24) | \
                                (unicast ? 0x10000 : 0) | \
                                (set_l2bm ? 0x20000 : 0) | \
                                (set_l3bm ? 0x40000 : 0)))

#define PBS_MH_V5_W2_DPORT_COS_SET(mh, dport, cos, ipri) \
        (((uint32 *)(mh))[2] = ((ipri & 0xf) | \
                                ((cos & 0x7f) << 8) | \
                                ((cos & 0xf) << 16) | \
                                ((dport & 0x7f) << 24)))

#define PBS_MH_V6_W2_DPORT_COS_QNUM_SET(mh, dport, cos, qnum, ipri) \
        (((uint32 *)(mh))[2] = (((dport & 0x7f) << 24) |  \
                                ((cos & 0x3f) << 16) |    \
                                ((ipri & 0xf) << 2) |    \
                                ((qnum & 0x3) << 22) |        \
                                ((qnum & 0x3fc) << 6) | \
                                ((qnum & 0xc00) >> 10)))
/* 43: Offset of soc_pbsmh_v6_hdr_t.tx_ts */
#define PBS_MH_V6_TS_PKT_SET(mh) (((uint32 *)(mh))[1] |=  (1 << 12))

/* 26: Offset of soc_pbsmh_v6_hdr_t.its_sign */
#define PBS_MH_V6_TS_ONE_STEP_INGRESS_SIGN_PKT_SET(mh) \
                                (((uint32 *)(mh))[0] |=  (1 << 26))
/* 25: Offset of soc_pbsmh_v6_hdr_t.regen_udp_chksum */
#define PBS_MH_V6_TS_ONE_STEP_HDR_START_REGEN_UDP_CHEKSUM_PKT_SET(mh) \
                                (((uint32 *)(mh))[0] |=  (1 << 25))

/* 24: Offset of soc_pbsmh_v6_hdr_t.osts */
#define PBS_MH_V6_TS_ONE_STEP_PKT_SET(mh) \
                                (((uint32 *)(mh))[0] |=  (1 << 24))

/* 27: Offset of soc_pbsmh_v6_hdr_t.hdr_offset_1 */
#define PBS_MH_V6_TS_ONE_STEP_HDR_START_OFFSET_PKT_SET(mh, offset) \
        (((uint32 *)(mh))[0] |= (((offset & 0x1f) << 27) | \
                                 ((offset & 0xe0) << 11)))


#define PBS_MH_V2_TS_PKT_SET(mh) (((uint32 *)(mh))[2] |=  (1 << 4))
#define PBS_MH_V3_TS_PKT_SET(mh) (((uint32 *)(mh))[2] |=  (1 << 4))
#define PBS_MH_V5_TS_PKT_SET(mh) (((uint32 *)(mh))[1] |=  (1 << 12))

#define PBS_MH_V7_W0_START_SET(mh) (((uint32 *)(mh))[0] = 0x00000081)
#define PBS_MH_V7_W1_DPORT_QNUM_SET(mh, dport, qnum)         \
    (((uint32 *)(mh))[1] = (((dport & 0x7f) << 24) | ((qnum & 0xc00) >> 4)))

/* 40: Offset of soc_pbsmh_v7_hdr_t.tx_ts */
#define PBS_MH_V7_TS_PKT_SET(mh) (((uint32 *)(mh))[1] |=  (1 << 9))

/* 41: Offset of soc_pbsmh_v7_hdr_t.its_sign */
#define PBS_MH_V7_TS_ONE_STEP_INGRESS_SIGN_PKT_SET(mh) \
                                (((uint32 *)(mh))[1] |=  (1 << 10))

/* 42: Offset of soc_pbsmh_v7_hdr_t.regen_udp_chksum */
#define PBS_MH_V7_TS_ONE_STEP_HDR_START_REGEN_UDP_CHEKSUM_PKT_SET(mh) \
                                (((uint32 *)(mh))[1] |=  (1 << 11))

/* 43: Offset of soc_pbsmh_v7_hdr_t.osts */
#define PBS_MH_V7_TS_ONE_STEP_PKT_SET(mh) \
                                (((uint32 *)(mh))[1] |=  (1 << 12))

/* 47: Offset of soc_pbsmh_v7_hdr_t.hdr_offset_1 */
#define PBS_MH_V7_TS_ONE_STEP_HDR_START_OFFSET_PKT_SET(mh, offset) \
                                (((uint32 *)(mh))[1] |= ((offset & 0xff) << 16))

#define PBS_MH_V7_W2_SMOD_COS_QNUM_SET(mh, smod, unicast, cos, qnum, ipri) \
        (((uint32 *)(mh))[2] = (((smod & 0xff) << 24) | \
                                ((qnum & 0xff) << 16) | \
                                (qnum & 0x300) | \
                                (unicast ? 0x400 : 0) | \
                                ((ipri & 0xf) << 4) | \
                                 (cos & 0xf)))

#define PBS_MH_V8_W1_DPORT_IPRI_SET(mh, dport,ipri)                    \
    (((uint32 *)(mh))[1] = (((dport & 0xff) << 24) | \
                            ((ipri & 0xf) << 16)))
#define PBS_MH_V8_W2_SMOD_COS_QNUM_SET(mh, smod, unicast, cos, qnum) \
        (((uint32 *)(mh))[2] = (((smod & 0xff) << 24) | \
                                ((qnum & 0xff) << 16) | \
                                (qnum & 0xf00) | \
                                (unicast ? 0x1000 : 0) | \
                                 (cos & 0xf)))

#define PBS_MH_V9_W0_START_SET(mh) (((uint32 *)(mh))[0] = 0x00000081)

#define PBS_MH_V9_W1_DPORT_SET(mh, dport) \
        (((uint32 *)(mh))[1] = ((dport & 0xff) << 24))

#define PBS_MH_V9_W2_SMOD_COS_SET(mh, smod, unicast, cos)   \
        (((uint32 *)(mh))[2] = (((smod & 0xff) << 24) | \
                                ((cos & 0x3f) << 16) | \
                                (unicast ? 0x400000 : 0)))

#define PBS_MH_V9_W2_SMOD_COS_L2BM_SET(mh, smod, unicast, cos, l2bm)   \
        (((uint32 *)(mh))[2] = (((smod & 0xff) << 24) | \
                                ((cos & 0x3f) << 16) | \
                                (unicast ? 0x400000 : 0) |\
                                (l2bm ? 0x800000 : 0)))

#define PBS_MH_V9_W2_RQE_SET(mh, rqe)   \
        (((uint32 *)(mh))[2] = ((rqe << 8 )))
#define PBS_MH_V11_W0_START_SET(mh) (((uint32 *)(mh))[0] = 0x00000081)

#define PBS_MH_V11_W1_DPORT_SET(mh, dport) \
        (((uint32 *)(mh))[1] = ((dport & 0xff) << 24))

#define PBS_MH_V11_W2_SMOD_COS_SET(mh, smod, unicast, cos)   \
        (((uint32 *)(mh))[2] = (((smod & 0xff) << 25) | \
                                ((cos & 0x3f) << 17) | \
                                (unicast ? 0x800000 : 0)))

#define PBS_MH_V11_W2_SMOD_COS_L2BM_SET(mh, smod, unicast, cos, l2bm)   \
        (((uint32 *)(mh))[2] = (((smod & 0xff) << 25) | \
                                ((cos & 0x3f) << 17) | \
                                (unicast ? 0x800000 : 0) |\
                                (l2bm ? 0x100 : 0)))

#define PBS_MH_START_IS_SET(mh) \
        (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit) || \
         SOC_IS_HURRICANE2(unit) || SOC_IS_KATANA2(unit)|| \
         SOC_IS_GREYHOUND(unit) || SOC_IS_HURRICANE3(unit) || \
         SOC_IS_GREYHOUND2(unit) ? \
        (((((uint32 *)(mh))[0]) & 0x000000C0) == 0x00000080) : \
        (((((uint32 *)(mh))[0]) & 0x000000FF) == 0x000000FF))

#define PBS_MH_V8_W0_OAM_UPMEP_START_SET(mh) \
           (((uint32 *)(mh))[0] = 0x0000008d)
#define PBS_MH_V8_W0_OAM_DOWNMEP_START_SET(mh) \
           (((uint32 *)(mh))[0] = 0x0000008c)
#define PBS_MH_V8_W0_OAM_DOWNMEP_CTR_LOCATION_SET(unit, mh) \
           (SOC_IS_METROLITE(unit) ? (((uint32 *)(mh))[1] = 0x00000300) : \
           (((uint32 *)(mh))[1] = 0x00003000))

#else /* !LE_HOST */
#define PBS_MH_W0_START_SET(mh) (((uint32 *)(mh))[0] = 0xFF000000)
#define PBS_MH_W1_RSVD_SET(mh)  (((uint32 *)(mh))[1] = 0x00000000)

#define PBS_MH_V1_W2_SMOD_DPORT_COS_SET(mh, smod, dport, cos, dc1, dc2) \
        (((uint32 *)(mh))[2] = (smod << 8) | (cos << 5) | (dport << 0))

#define PBS_MH_V2_W2_SMOD_DPORT_COS_SET(mh, smod, dport, cos, ipri, l3pbm_sel) \
        (((uint32 *)(mh))[2] = (smod << 16) | (ipri << 12) | (cos << 8) |      \
                               (l3pbm_sel << 7) | (dport << 0))

#define PBS_MH_V3_W2_SMOD_DPORT_COS_SET(mh, smod, dport, cos, ipri, l3pbm_sel) \
        (((uint32 *)(mh))[2] = (smod << 20) | (ipri << 16) | (cos << 8) |      \
                               (l3pbm_sel << 7) | (dport << 0))

#define PBS_MH_V4_W2_SMOD_DPORT_COS_SET(mh, smod, dport, cos, ipri, l3pbm_sel) \
        (((uint32 *)(mh))[2] = (smod << 17) | (ipri << 13) | (cos << 8) |      \
                               (l3pbm_sel << 7) | (dport << 0))

#define PBS_MH_V5_W1_SMOD_SET(mh, smod, unicast, set_l2bm, set_l3bm) \
        (((uint32 *)(mh))[1] = ((smod & 0xff) | (unicast ? 0x0100 : 0) | \
                                (set_l2bm ? 0x0200 : 0) | \
                                (set_l3bm ? 0x0400 : 0)))

#define PBS_MH_V5_W2_DPORT_COS_SET(mh, dport, cos, ipri) \
        (((uint32 *)(mh))[2] = (((ipri & 0xf) << 24) | \
                                ((cos & 0x7f) << 16) | \
                                ((cos & 0xf) << 8) | \
                                (dport & 0x7f)))

#define PBS_MH_V6_W2_DPORT_COS_QNUM_SET(mh, dport, cos, qnum, ipri) \
        (((uint32 *)(mh))[2] = (((ipri & 0xf) << 26) | \
                                ((qnum & 0xfff) << 14) | \
                                ((cos & 0x3f) << 8) | \
                                (dport & 0x7f)))

/* 42: Offset of soc_pbsmh_v6_hdr_t.tx_ts */
#define PBS_MH_V6_TS_PKT_SET(mh) (((uint32 *)(mh))[1] |=  (1 << 20))

/* 29: Offset of soc_pbsmh_v6_hdr_t.its_sign */
#define PBS_MH_V6_TS_ONE_STEP_INGRESS_SIGN_PKT_SET(mh) \
                                (((uint32 *)(mh))[0] |=  (1 << 2))

/* 30: Offset of soc_pbsmh_v6_hdr_t.regen_udp_chksum */
#define PBS_MH_V6_TS_ONE_STEP_HDR_START_REGEN_UDP_CHEKSUM_PKT_SET(mh) \
                                (((uint32 *)(mh))[0] |=  (1 << 1))

/* 31: Offset of soc_pbsmh_v6_hdr_t.osts */
#define PBS_MH_V6_TS_ONE_STEP_PKT_SET(mh) \
                                (((uint32 *)(mh))[0] |=  (1 << 0))

/* 23: Offset of soc_pbsmh_v6_hdr_t.hdr_offset_1 */
#define PBS_MH_V6_TS_ONE_STEP_HDR_START_OFFSET_PKT_SET(mh, offset) \
                                (((uint32 *)(mh))[0] |=  ((offset & 0xff) << 3))

#define PBS_MH_V2_TS_PKT_SET(mh) (((uint32 *)(mh))[2] |=  (1 << 28))
#define PBS_MH_V3_TS_PKT_SET(mh) (((uint32 *)(mh))[2] |=  (1 << 28))
#define PBS_MH_V5_TS_PKT_SET(mh) (((uint32 *)(mh))[1] |=  (1 << 20))

#define PBS_MH_V7_W0_START_SET(mh) (((uint32 *)(mh))[0] = 0x81000000)
#define PBS_MH_V7_W1_DPORT_QNUM_SET(mh, dport, qnum)                    \
    (((uint32 *)(mh))[1] = (((qnum & 0xc00) << 20) | (dport & 0x7f)))

/* 47: Offset of soc_pbsmh_v7_hdr_t.tx_ts */
#define PBS_MH_V7_TS_PKT_SET(mh) (((uint32 *)(mh))[1] |=  (1 << 17))

/* 46: Offset of soc_pbsmh_v7_hdr_t.its_sign */
#define PBS_MH_V7_TS_ONE_STEP_INGRESS_SIGN_PKT_SET(mh) \
                                (((uint32 *)(mh))[1] |=  (1 << 18))

/* 45: Offset of soc_pbsmh_v7_hdr_t.regen_udp_chksum */
#define PBS_MH_V7_TS_ONE_STEP_HDR_START_REGEN_UDP_CHEKSUM_PKT_SET(mh) \
                                (((uint32 *)(mh))[1] |=  (1 << 19))

/* 41: Offset of soc_pbsmh_v7_hdr_t.osts */
#define PBS_MH_V7_TS_ONE_STEP_PKT_SET(mh) \
                                (((uint32 *)(mh))[1] |=  (1 << 20))

/* 49: Offset of soc_pbsmh_v7_hdr_t.hdr_offset_1 */
#define PBS_MH_V7_TS_ONE_STEP_HDR_START_OFFSET_PKT_SET(mh, offset) \
                                (((uint32 *)(mh))[1] |=  ((offset & 0xff) << 8))

#define PBS_MH_V7_W2_SMOD_COS_QNUM_SET(mh, smod, unicast, cos, qnum, ipri) \
        (((uint32 *)(mh))[2] = (((ipri & 0xf) << 28) | \
                                ((cos & 0xf) << 24) | \
                                (unicast ? 0x40000 : 0) | \
                                ((qnum & 0x3ff) << 8) | \
                                (smod & 0xff)))

#define PBS_MH_V8_W1_DPORT_IPRI_SET(mh, dport,ipri)                    \
        (((uint32 *)(mh))[1] = ((dport & 0xff) | \
                                ((ipri & 0xf) << 8)))
#define PBS_MH_V8_W2_SMOD_COS_QNUM_SET(mh, smod, unicast, cos, qnum) \
        (((uint32 *)(mh))[2] = (((cos & 0xf) << 24) | \
                                (unicast ? 0x100000 : 0) | \
                                ((qnum & 0xfff) << 8) | \
                                (smod & 0xff)))

#define PBS_MH_V9_W0_START_SET(mh) (((uint32 *)(mh))[0] = 0x81000000)

#define PBS_MH_V9_W1_DPORT_SET(mh, dport) \
        (((uint32 *)(mh))[1] = (dport & 0xff))

#define PBS_MH_V9_W2_SMOD_COS_SET(mh, smod, unicast, cos)   \
        (((uint32 *)(mh))[2] = ((smod & 0xff) | \
                                ((cos & 0x3f) << 8) | \
                                (unicast ? 0x4000 : 0)))

#define PBS_MH_V9_W2_SMOD_COS_L2BM_SET(mh, smod, unicast, cos, l2bm)   \
        (((uint32 *)(mh))[2] = ((smod & 0xff) | \
                                ((cos & 0x3f) << 8) | \
                                (unicast ? 0x4000 : 0) | \
                                (l2bm ? 0x8000 : 0)))

#define PBS_MH_V9_W2_RQE_SET(mh, rqe)   \
        (((uint32 *)(mh))[2] = ((rqe << 8 )))

#define PBS_MH_V11_W0_START_SET(mh) (((uint32 *)(mh))[0] = 0x81000000)

#define PBS_MH_V11_W1_DPORT_SET(mh, dport) \
        (((uint32 *)(mh))[1] = (dport & 0xff))

#define PBS_MH_V11_W2_SMOD_COS_SET(mh, smod, unicast, cos)   \
        (((uint32 *)(mh))[2] = (((smod & 0xff) << 1) | \
                                ((cos & 0x3f) << 9) | \
                                (unicast ? 0x8000 : 0)))

#define PBS_MH_V11_W2_SMOD_COS_L2BM_SET(mh, smod, unicast, cos, l2bm)   \
        (((uint32 *)(mh))[2] = (((smod & 0xff) << 1) | \
                                ((cos & 0x3f) << 9) | \
                                (unicast ? 0x8000 : 0) | \
                                (l2bm ? 0x10000 : 0)))

#define PBS_MH_START_IS_SET(mh) \
    (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit) ||  \
     SOC_IS_HURRICANE2(unit) || SOC_IS_KATANA2(unit) || \
     SOC_IS_GREYHOUND(unit) || SOC_IS_HURRICANE3(unit) || \
     SOC_IS_GREYHOUND2(unit) ? \
        (((((uint32 *)(mh))[0]) & 0xC0000000) == 0x80000000) : \
        (((((uint32 *)(mh))[0]) & 0xFF000000) == 0xFF000000))

#define PBS_MH_V8_W0_OAM_UPMEP_START_SET(mh) \
            (((uint32 *)(mh))[0] = 0x8d000000)
#define PBS_MH_V8_W0_OAM_DOWNMEP_START_SET(mh) \
            (((uint32 *)(mh))[0] = 0x8c000000)
#define PBS_MH_V8_W0_OAM_DOWNMEP_CTR_LOCATION_SET(unit, mh) \
           (SOC_IS_METROLITE(unit) ? (((uint32 *)(mh))[1] = 0x00030000) : \
           (((uint32 *)(mh))[1] = 0x00300000))

#endif /* !LE_HOST */

#define SOC_PBSMH_HDR_SIZE      ((int) sizeof(soc_pbsmh_hdr_t))

/*
 * PBS MH Field Manipulation
 */

typedef enum {
    /* NOTE: strings in soc_pbsmh_field_names[] must match */
    PBSMH_invalid = -1,
    PBSMH_start = 0,
    PBSMH_src_mod,      /* Source Modid to disable E2E checks */
    PBSMH_dst_port,     /* Destination port */
    PBSMH_cos,          /* COS queue */
    PBSMH_pri,          /* Input Priority */
    PBSMH_l3pbm_sel,    /* Use L3 PBM */
    PBSMH_l2pbm_sel,    /* Use L2 PBM */
    PBSMH_unicast,      /* Unicast */
    PBSMH_tx_ts,        /* Capture transmit timestamp */
    PBSMH_spid_override,
    PBSMH_spid,         /* Service pool ID */
    PBSMH_spap,         /* Service pool priority */
    PBSMH_queue_num,    /* Queue number */
    PBSMH_osts,         /* 1588-TS 1-step */
    PBSMH_its_sign,     /* 1588-TS Ingress timestamp sign (for 1-step) */
    PBSMH_hdr_offset,   /* 1588-TS header start offset (for 1-step) */
    PBSMH_regen_udp_checksum, /* 588-TS UDP checksum update (for 1-step) */
    PBSMH_int_pri,      /* Internal Priority */
    PBSMH_nlf_port,     /* ? */
    PBSMH_lm_ctr_index, /* ? */
    PBSMH_oam_replacement_type, /* See OAM replacement types below */
    PBSMH_oam_replacement_offset, /* OAM insertion point = 14 + (2 * val) */
    PBSMH_ep_cpu_reasons, /* EP Copy to CPU reasons */
    PBSMH_header_type,  /* See PBSMH header types below */
    PBSMH_cell_error,   /* Force cell error status */
    PBSMH_ipcf_ptr,  /* Pointer to Cell Buffer */
    PBSMH_oam_ma_ptr,   /* OAM MA pointer */
    PBSMH_ts_action,    /* OAM DM timestamp action */
    PBSMH_sample_rdi,   /* OAM Sample RDI bit */
    PBSMH_ctr1_action, /* OAM LM counter 1 action */
    PBSMH_lm_ctr1_index,/* OAM 1st LM counter index */
    PBSMH_ctr2_action, /* OAM LM counter 2 action */
    PBSMH_lm_ctr2_index,/* OAM 2nd LM counter index */
    PBSMH_pp_port,      /* OAM-PP port              */
    PBSMH_dst_subport_num,      /* subport number for that port. */
    PBSMH_amp_ts_act,   /* Insert timestamp in EP for TWAMP/OWAMP */
    PBSMH_sobmh_flex,   /* Semi-SOBMH flow */
    PBSMH_COUNT
} soc_pbsmh_field_t;

extern soc_pbsmh_field_t soc_pbsmh_name_to_field(int unit, char *name);
extern char *soc_pbsmh_field_to_name(int unit, soc_pbsmh_field_t f);

extern uint32 soc_pbsmh_v7_field_get(int unit, soc_pbsmh_v7_hdr_t *mh,
                                  soc_pbsmh_field_t field);
extern void soc_pbsmh_v7_field_set(int unit, soc_pbsmh_v7_hdr_t *mh,
                                soc_pbsmh_field_t field, uint32 val);

extern void soc_pbsmh_dump(int unit, char *pfx, soc_pbsmh_v7_hdr_t *mh);

#define soc_pbsmh_field_init(unit, mh) \
        sal_memset((mh), 0, sizeof (soc_pbsmh_hdr_t))

/* PBSMH field default values */
/* default SOP Symbol */
#define SOC_PBSMH_START         0xff /* PBS MH indicator */
#define SOC_PBSMH_START_INTERNAL 0x2 /* PBS MH internal header indicator */

#define SOC_SOBMH_FROM_CPU 0x1       /* From CPU to Passthru NLF" */

#endif  /* !_SOC_PBSMH_H */
