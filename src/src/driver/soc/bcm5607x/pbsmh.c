/*
 * $Id: flrxtx.c,v 1.1 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#include "soc/pbsmh.h"

#define DEBUG 0

#if DEBUG
#define LOCAL_DEBUG_PRINTF(args ...)  do { sal_printf("%s %d:", __FUNCTION__, __LINE__); sal_printf(args); } while(0)
#else
#define LOCAL_DEBUG_PRINTF(args ...)
#endif

#if CFG_RXTX_SUPPORT_ENABLED

static char *soc_pbsmh_field_names[] = {
    /* NOTE: strings must match soc_pbsmh_field_t */
    "start",
    "src_mod",
    "dst_port",
    "cos",
    "pri",
    "l3pbm_sel",
    "l2pbm_sel",
    "unicast",
    "tx_ts",
    "spid_override",
    "spid",
    "spap",
    "queue_num",
    "osts",
    "its_sign",
    "hdr_offset",
    "regen_udp_checksum",
    "int_pri",
    "nlf_port",
    "lm_ctr_index",
    "oam_replacement_type",
    "oam_replacement_offset",
    "ep_cpu_reasons",
    "header_type",
    "cell_error",
    "ipcf_ptr",
    "oam_ma_ptr",
    "ts_action",
    "sample_rdi",
    "ctr1_action",
    "lm_ctr1_index",
    "ctr2_action",
    "lm_ctr2_index",
    "pp_port",
   "dst_subport_num",
    "amp_ts_act",
    "sobmh_flex",
    NULL
};

soc_pbsmh_field_t
soc_pbsmh_name_to_field(int unit, char *name)
{
    soc_pbsmh_field_t           f;

    COMPILER_REFERENCE(unit);

    for (f = 0; soc_pbsmh_field_names[f] != NULL; f++) {
        if (sal_strcmp(name, soc_pbsmh_field_names[f]) == 0) {
            return f;
        }
    }

    return PBSMH_invalid;
}

char *
soc_pbsmh_field_to_name(int unit, soc_pbsmh_field_t field)
{
    COMPILER_REFERENCE(unit);

    if (field < 0 || field >= PBSMH_COUNT) {
        return "??";
    } else {
        return soc_pbsmh_field_names[field];
    }
}

void
soc_pbsmh_v7_field_set(int unit, soc_pbsmh_v7_hdr_t *mh,
                       soc_pbsmh_field_t field, uint32 val)
{
    COMPILER_REFERENCE(unit);

    switch (field) {
    case PBSMH_start:           mh->overlay1.start = val;
              mh->overlay1.header_type = PBS_MH_V7_HDR_TYPE_FROM_CPU;
                                mh->overlay1.cell_error = 0;
                                mh->overlay1.oam_replacement_offset = 0;
                                mh->overlay1.oam_replacement_type = 0;
                                mh->overlay2._rsvd3 = 0;
                                break;
    case PBSMH_src_mod:         mh->overlay1.src_mod = val; break;
    case PBSMH_dst_port:        mh->overlay1.dst_port = val; break;
    case PBSMH_cos:             mh->overlay2.cos = val; break;
    case PBSMH_pri:             mh->overlay1.input_pri = val; break;
    case PBSMH_l3pbm_sel:       mh->overlay1.set_l3bm = val; break;
    case PBSMH_l2pbm_sel:       mh->overlay1.set_l2bm = val; break;
    case PBSMH_unicast:         mh->overlay2.unicast = val; break;
    case PBSMH_tx_ts:           mh->overlay1.tx_ts = val; break;
    case PBSMH_spid_override:   mh->overlay2.spid_override = val; break;
    case PBSMH_spid:            mh->overlay2.spid = val; break;
    case PBSMH_spap:            mh->overlay2.spap = val; break;
    case PBSMH_queue_num:
        mh->overlay1.queue_num_1 = val & 0xff;
        mh->overlay1.queue_num_2 = (val >> 8) & 0x3;
        break;
    case PBSMH_osts:            mh->overlay1.osts = val; break;
    case PBSMH_its_sign:        mh->overlay1.its_sign = val; break;
    case PBSMH_hdr_offset:      mh->overlay1.hdr_offset = val; break;
    case PBSMH_regen_udp_checksum: mh->overlay1.regen_udp_checksum = val; break;
    case PBSMH_int_pri:         mh->overlay1.int_pri = val; break;
    case PBSMH_nlf_port:        mh->overlay1.nlf_port = val; break;
    case PBSMH_lm_ctr_index:
        mh->overlay2.lm_counter_index_1 = val & 0xff;
        mh->overlay2.lm_counter_index_2 = (val >> 8) & 0xff;
        mh->overlay2._rsvd3 = 0;
        break;
    case PBSMH_oam_replacement_type:
        mh->overlay1.oam_replacement_type = val;
        break;
    case PBSMH_oam_replacement_offset:
        mh->overlay1.oam_replacement_offset = val;
        break;
    case PBSMH_ep_cpu_reasons:
        mh->overlay1._rsvd5 = 0;
        mh->overlay1.ep_cpu_reason_code_1 = val & 0xff;
        mh->overlay1.ep_cpu_reason_code_2 = (val >> 8) & 0xff;
        mh->overlay1.ep_cpu_reason_code_3 = (val >> 16) & 0xf;
        break;
    case PBSMH_header_type:     mh->overlay1.header_type = val; break;
    case PBSMH_cell_error:      mh->overlay1.cell_error = val; break;
    default:
        sal_printf("%s Unknown pbsmh set field=%d val=0x%x\n", __FUNCTION__, field, val);
        break;
    }
}

uint32
soc_pbsmh_v7_field_get(int unit,
                       soc_pbsmh_v7_hdr_t *mh,
                       soc_pbsmh_field_t field)
{
    COMPILER_REFERENCE(unit);

    switch (field) {
    case PBSMH_start:         return mh->overlay1.start;
    case PBSMH_src_mod:       return mh->overlay1.src_mod;
    case PBSMH_dst_port:      return mh->overlay1.dst_port;
    case PBSMH_cos:           return mh->overlay2.cos;
    case PBSMH_pri:           return mh->overlay1.input_pri;
    case PBSMH_l3pbm_sel:     return mh->overlay1.set_l3bm;
    case PBSMH_l2pbm_sel:     return mh->overlay1.set_l2bm;
    case PBSMH_unicast:       return mh->overlay2.unicast;
    case PBSMH_tx_ts:         return mh->overlay1.tx_ts;
    case PBSMH_spid_override: return mh->overlay2.spid_override;
    case PBSMH_spid:          return mh->overlay2.spid;
    case PBSMH_spap:          return mh->overlay2.spap;
    case PBSMH_queue_num:
        return (mh->overlay1.queue_num_1 | (mh->overlay1.queue_num_2 << 8));
    case PBSMH_osts:          return mh->overlay1.osts;
    case PBSMH_its_sign:      return mh->overlay1.its_sign;
    case PBSMH_hdr_offset:    return mh->overlay1.hdr_offset;
    case PBSMH_regen_udp_checksum: return mh->overlay1.regen_udp_checksum;
    case PBSMH_int_pri:       return mh->overlay1.int_pri;
    case PBSMH_nlf_port:      return mh->overlay1.nlf_port;
    case PBSMH_lm_ctr_index:
        return (mh->overlay2.lm_counter_index_1 |
                (mh->overlay2.lm_counter_index_2 << 8));
    case PBSMH_oam_replacement_type:
        return mh->overlay1.oam_replacement_type;
    case PBSMH_oam_replacement_offset:
        return mh->overlay1.oam_replacement_offset;
    case PBSMH_ep_cpu_reasons:
        return (mh->overlay1.ep_cpu_reason_code_1 |
                (mh->overlay1.ep_cpu_reason_code_2 << 8) |
                (mh->overlay1.ep_cpu_reason_code_3 << 16));
    case PBSMH_header_type:   return mh->overlay1.header_type;
    case PBSMH_cell_error:    return mh->overlay1.cell_error;
    default:
        sal_printf("%s Unknown pbsmh get field=%d\n", __FUNCTION__, field);
        return 0;
    }
}
#endif
