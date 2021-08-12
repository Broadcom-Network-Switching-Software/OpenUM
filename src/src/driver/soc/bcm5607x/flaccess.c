/*
 * $Id: flaccess.c,v 1.3 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include <system.h>

/* TCAM maximum size in words. */
#define TCAM_WSIZE_MAX              (16)

/*! For switch schan channel in interrupt context only. */
uint32 bcm5607x_schan_ch = BCM5607X_SCHAN_CH_DEFAULT;

static sys_error_t
bcm5607x_sw_op(uint8 unit,
               uint32 op,
               uint8 block_id,
               uint32 addr,
               uint32 *buf,
               int len)
{
    int i;
    SCHAN_t schan;
    uint32 baseaddr, val32;
    uint32 ch = bcm5607x_schan_ch;

    CMIC_COMMON_POOL_SCHAN_CH0_CTRLr_t cmic_cmc_schan_ctrl;
    CMIC_COMMON_POOL_SCHAN_CH1_CTRLr_t cmic_ch1_cmc_schan_ctrl;

    if (buf == NULL || unit > 0) {
        return SYS_ERR;
    }
    if (block_id == 0xFF) {
        sal_printf("wrong block id =%d addr=%x\n", block_id, addr);
        return SYS_ERR;
    }

    /* Fill-out SCHAN message header. */
    SCHAN_CLR(schan);
    SCHAN_OP_CODEf_SET(schan, op);
    SCHAN_DEST_BLOCKf_SET(schan, block_id);
    SCHAN_DATA_LENf_SET(schan, (len*4));

    /* SCHAN shared lock. */
    SCHAN_LOCK(unit, ch);

    /* Fill out SCHAN message header. */
    baseaddr = CMIC_COMMON_POOL_SCHAN_CH0_MESSAGE(0) + (ch * 0x100);
    val32 = SCHAN_GET(schan);
    bcm5607x_write32(unit, baseaddr, val32);
    val32 = addr;
    bcm5607x_write32(unit, baseaddr + 4, val32);

    if (op == SC_OP_WR_REG_CMD || op == SC_OP_WR_MEM_CMD ||
        op == SC_OP_TBL_INS_CMD || op == SC_OP_TBL_DEL_CMD ||
        op == SC_OP_TBL_LOOKUP_CMD) {
        for (i = 0; i < len; i++) {
            val32 = buf[i];
            bcm5607x_write32(unit, (baseaddr + 8) + (4 * i), val32);
        }
    }

    if (ch == 0) {
        CMIC_COMMON_POOL_SCHAN_CH0_CTRLr_CLR(cmic_cmc_schan_ctrl);
	    CMIC_COMMON_POOL_SCHAN_CH0_CTRLr_MSG_STARTf_SET(cmic_cmc_schan_ctrl, 1);
        WRITE_CMIC_COMMON_POOL_SCHAN_CH0_CTRLr(unit, cmic_cmc_schan_ctrl);
    } else {
        CMIC_COMMON_POOL_SCHAN_CH1_CTRLr_CLR(cmic_ch1_cmc_schan_ctrl);
        CMIC_COMMON_POOL_SCHAN_CH1_CTRLr_MSG_STARTf_SET(cmic_ch1_cmc_schan_ctrl, 1);
        WRITE_CMIC_COMMON_POOL_SCHAN_CH1_CTRLr(unit, cmic_ch1_cmc_schan_ctrl);
    }

    for (i = 0; i < 100; i++) {
        sal_usleep(2);
        if (ch == 0) {
            READ_CMIC_COMMON_POOL_SCHAN_CH0_CTRLr(unit, cmic_cmc_schan_ctrl);
            if (CMIC_COMMON_POOL_SCHAN_CH0_CTRLr_MSG_DONEf_GET(cmic_cmc_schan_ctrl)) {
                break;
            }
        } else {
            READ_CMIC_COMMON_POOL_SCHAN_CH1_CTRLr(unit, cmic_ch1_cmc_schan_ctrl);
            if (CMIC_COMMON_POOL_SCHAN_CH1_CTRLr_MSG_DONEf_GET(cmic_ch1_cmc_schan_ctrl)) {
                break;
            }
        }
    }

#if CFG_CONSOLE_ENABLED
    if (ch == 0) {
        if ((op != SC_OP_TBL_LOOKUP_CMD) &&
            ((i == 100) ||
             CMIC_COMMON_POOL_SCHAN_CH0_CTRLr_SCHAN_ERRORf_GET(cmic_cmc_schan_ctrl) ||
             CMIC_COMMON_POOL_SCHAN_CH0_CTRLr_SER_CHECK_FAILf_GET(cmic_cmc_schan_ctrl) ||
             CMIC_COMMON_POOL_SCHAN_CH0_CTRLr_TIMEOUTf_GET(cmic_cmc_schan_ctrl) ||
             CMIC_COMMON_POOL_SCHAN_CH0_CTRLr_NACKf_GET(cmic_cmc_schan_ctrl))) {
            sal_printf("S-CHAN op=0x%x, %d:0x%x, error(%d-0x%08x)\n", op, block_id, addr, i, CMIC_COMMON_POOL_SCHAN_CH0_CTRLr_GET(cmic_cmc_schan_ctrl));
        }
    } else {
        if ((op != SC_OP_TBL_LOOKUP_CMD) &&
            ((i == 100) ||
             CMIC_COMMON_POOL_SCHAN_CH1_CTRLr_SCHAN_ERRORf_GET(cmic_ch1_cmc_schan_ctrl) ||
             CMIC_COMMON_POOL_SCHAN_CH1_CTRLr_SER_CHECK_FAILf_GET(cmic_ch1_cmc_schan_ctrl) ||
             CMIC_COMMON_POOL_SCHAN_CH1_CTRLr_TIMEOUTf_GET(cmic_ch1_cmc_schan_ctrl) ||
             CMIC_COMMON_POOL_SCHAN_CH1_CTRLr_NACKf_GET(cmic_ch1_cmc_schan_ctrl))) {
            sal_printf("S-CHAN op=0x%x, %d:0x%x, error(%d-0x%08x)\n", op, block_id, addr, i, CMIC_COMMON_POOL_SCHAN_CH0_CTRLr_GET(cmic_cmc_schan_ctrl));
        }
    }
#endif /* CFG_CONSOLE_ENABLED */

    if (ch == 0) {
        READ_CMIC_COMMON_POOL_SCHAN_CH0_CTRLr(unit, cmic_cmc_schan_ctrl);
        CMIC_COMMON_POOL_SCHAN_CH0_CTRLr_MSG_DONEf_SET(cmic_cmc_schan_ctrl, 0);
        WRITE_CMIC_COMMON_POOL_SCHAN_CH0_CTRLr(unit, cmic_cmc_schan_ctrl);
    } else {
        READ_CMIC_COMMON_POOL_SCHAN_CH1_CTRLr(unit, cmic_ch1_cmc_schan_ctrl);
        CMIC_COMMON_POOL_SCHAN_CH1_CTRLr_MSG_DONEf_SET(cmic_ch1_cmc_schan_ctrl, 0);
        WRITE_CMIC_COMMON_POOL_SCHAN_CH1_CTRLr(unit, cmic_ch1_cmc_schan_ctrl);
    }

    if (op == SC_OP_TBL_LOOKUP_CMD) {
        bcm5607x_read32(unit, baseaddr, &val32);
        if (val32 & 0x1) {
            /* SCHAN shared unlock. */
            SCHAN_UNLOCK(unit, ch);
            return SYS_ERR_NOT_FOUND;
        }
    }

    if (op == SC_OP_RD_REG_CMD || op == SC_OP_RD_MEM_CMD || op == SC_OP_TBL_LOOKUP_CMD) {
        for (i = 0; i < len; i++) {
            bcm5607x_read32(unit, (baseaddr + 4) + (4 * i), &val32);
            buf[i] = val32;
        }
    }

    /* SCHAN shared unlock. */
    SCHAN_UNLOCK(unit, ch);

    return SYS_OK;
}
sys_error_t
bcm5607x_phy_reg_get(uint8 unit, uint8 port,
                           uint16 reg_addr, uint16 *p_value)
{
    return SYS_OK;
}

sys_error_t
bcm5607x_phy_reg_set(uint8 unit, uint8 port,
                           uint16 reg_addr, uint16 value)
{
    return SYS_OK;
}

sys_error_t bcm5607x_read32(uint8 unit, uint32 addr, uint32 *val)
                                __attribute__((section(".2ram")));
sys_error_t bcm5607x_read32(uint8 unit, uint32 addr, uint32 *val) {

    *val = SYS_REG_READ32(addr);
    return SYS_OK;
}

sys_error_t bcm5607x_write32(uint8 unit, uint32 addr, uint32 val)
                                __attribute__((section(".2ram")));
sys_error_t bcm5607x_write32(uint8 unit, uint32 addr, uint32 val) {

           SYS_REG_WRITE32(addr, val);
		   return SYS_OK;
}

sys_error_t
bcm5607x_reg_get(uint8 unit,
                 uint8 block_id,
                 uint32 addr,
                 uint32 *val)
{
	return bcm5607x_sw_op(unit, SC_OP_RD_REG_CMD, block_id, addr, val, 1);
}

sys_error_t
bcm5607x_reg_set(uint8 unit,
                 uint8 block_id,
                 uint32 addr,
                 uint32 val)
{
	return bcm5607x_sw_op(unit, SC_OP_WR_REG_CMD, block_id, addr, &val, 1);
}

sys_error_t
bcm5607x_reg64_get(uint8 unit,
                 uint8 block_id,
                 uint32 addr,
                 uint32 *val,
                 int len)
{
	return bcm5607x_sw_op(unit, SC_OP_RD_REG_CMD, block_id, addr, val, len);
}

sys_error_t
bcm5607x_reg64_set(uint8 unit,
                 uint8 block_id,
                 uint32 addr,
                 uint32 *buf,
                 int len)
{
	return bcm5607x_sw_op(unit, SC_OP_WR_REG_CMD, block_id, addr, buf, len);
}

sys_error_t
bcm5607x_mem_get(uint8 unit,
                 uint8 block_id,
                 uint32 addr,
                 uint32 *buf,
                 int len)
{
	return bcm5607x_sw_op(unit, SC_OP_RD_MEM_CMD, block_id, addr, buf, len);
}

sys_error_t
bcm5607x_mem_set(uint8 unit,
                 uint8 block_id,
                 uint32 addr,
                 uint32 *buf,
                 int len)
{
    return bcm5607x_sw_op(unit, SC_OP_WR_MEM_CMD, block_id, addr, buf, len);
}

sys_error_t
bcm5607x_tcam_mem_set(uint8 unit,
                 uint8 block_id,
                 uint32 addr,
                 uint32 *buf,
                 int len,
                 uint32 key_sp,
                 uint32 mask_sp,
                 int key_len
                 )
{

     uint32 step, key, mask, encoded_key, encoded_mask;
     uint32 xor_value = 0xffffffff;
     uint32 tcam_buffer[TCAM_WSIZE_MAX];

     if (len >= TCAM_WSIZE_MAX) {
         return SYS_ERR_PARAMETER;
     }

     sal_memcpy(tcam_buffer, buf, (len*4));

     while(key_len != 0) {

           step = 32;

           if (step > key_len) {
               step = key_len;
           }

           key =  field32_get(buf, key_sp, key_sp + step - 1);
           mask = field32_get(buf, mask_sp, mask_sp + step - 1);

           encoded_key = key & mask;
           encoded_mask = (~mask | key) ^ xor_value;

           field32_set(tcam_buffer, key_sp, key_sp + step - 1, encoded_key);
           field32_set(tcam_buffer, mask_sp, mask_sp + step - 1, encoded_mask);

           key_len -= step;
           key_sp += step;
           mask_sp += step;
     }
     return bcm5607x_mem_set(unit, block_id, addr, tcam_buffer, len);


}

sys_error_t
bcm5607x_tcam_mem_get(uint8 unit,
                 uint8 block_id,
                 uint32 addr,
                 uint32 *buf,
                 int len,
                 uint32 key_sp,
                 uint32 mask_sp,
                 int key_len
                 )
{
    sys_error_t rv;
    uint32 step, key, mask, encoded_key, encoded_mask;
    uint32 tcam_buffer[TCAM_WSIZE_MAX];

    if (len >= TCAM_WSIZE_MAX) {
        return SYS_ERR_PARAMETER;
    }

    sal_memset(tcam_buffer, 0, sizeof(tcam_buffer));
    rv = bcm5607x_mem_get(unit, block_id, addr, tcam_buffer, len);

    if (rv != SYS_OK) {
        return rv;
    }

    sal_memcpy(buf, tcam_buffer, (len*4));

    while(key_len != 0) {

          step = 32;

          if (step > key_len) {
              step = key_len;
          }
          encoded_key =  field32_get(tcam_buffer, key_sp, key_sp + step - 1);
          encoded_mask = field32_get(tcam_buffer, mask_sp, mask_sp + step - 1);

          key = encoded_key;
          mask = encoded_key | encoded_mask;

          field32_set(buf, key_sp, key_sp + step - 1, key);
          field32_set(buf, mask_sp, mask_sp + step - 1, mask);

          key_len -= step;
          key_sp += step;
          mask_sp += step;
    }

    return SYS_OK;

}


sys_error_t
bcm5607x_l2_op(uint8 unit,
               l2x_entry_t *entry,
               uint8 op_code,
               uint16 *index
               )

{
	L2_ENTRYm_t l2_entry;
    uint32 mac_addr[2];
    int rv = 0;

    /* To handle SW/HW mac address byte order mismatch */
	mac_addr[0] = ((entry->mac_addr[5]) | (entry->mac_addr[4] << 8) |  (entry->mac_addr[3] << 16) | (entry->mac_addr[2] << 24));
	mac_addr[1] = ((entry->mac_addr[1]) | (entry->mac_addr[0] << 8));

    L2_ENTRYm_CLR(l2_entry);
	L2_ENTRYm_L2_MAC_ADDRf_SET(l2_entry, mac_addr);
	L2_ENTRYm_L2_VLAN_IDf_SET(l2_entry, entry->vlan_id);
	L2_ENTRYm_L2_PORT_NUMf_SET(l2_entry, entry->port);
    if (entry->is_static) {
        L2_ENTRYm_L2_STATIC_BITf_SET(l2_entry, 1);
    }
    L2_ENTRYm_VALIDf_SET(l2_entry, 1);

    if (op_code == SC_OP_L2_INS_CMD) {
        op_code = SC_OP_TBL_INS_CMD;
    } else if (op_code == SC_OP_L2_DEL_CMD) {
        op_code = SC_OP_TBL_DEL_CMD;
    } else if (op_code == SC_OP_L2_LOOKUP_CMD) {
        op_code = SC_OP_TBL_LOOKUP_CMD;
    }

    rv = bcm5607x_sw_op(unit, op_code, M_L2_ENTRY(0), l2_entry.v, (sizeof(l2_entry)+3)/4);

    if (op_code == SC_OP_TBL_LOOKUP_CMD) {
        /* Check result */
        if (rv == SYS_ERR_NOT_FOUND) {
            return SYS_ERR_NOT_FOUND;
        } else {
            *index = (uint16) l2_entry.v[0];
        }
    }

    return rv;
}


