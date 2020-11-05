/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <system.h>

static sys_error_t
bcm5357x_sw_op(uint8 unit,
               uint32 op,
               uint8 block_id,
               uint32 addr,
               uint32 *buf,
               int len)
{
    int i;
    SCHAN_t schan;
    CMIC_CMC1_SCHAN_MESSAGEr_t cmc_schan_message;
    CMIC_CMC1_SCHAN_CTRLr_t cmic_cmc_schan_ctrl;

    if (buf == NULL || unit > 0) {
        return SYS_ERR;
    }
    if (block_id == 0xFF) {
        sal_printf("wrong block id =%d addr=%x\n", block_id, addr);
        return SYS_ERR;
    }
    SCHAN_CLR(schan);

    SCHAN_OP_CODEf_SET(schan, op);
    SCHAN_DEST_BLOCKf_SET(schan, block_id);
    SCHAN_DATA_LENf_SET(schan, (len*4));

    SCHAN_LOCK(unit);

    CMIC_CMC1_SCHAN_MESSAGEr_SET(cmc_schan_message, SCHAN_GET(schan));
    WRITE_CMIC_CMC1_SCHAN_MESSAGEr(unit, 0, cmc_schan_message);
    CMIC_CMC1_SCHAN_MESSAGEr_DATAf_SET(cmc_schan_message, addr);
    WRITE_CMIC_CMC1_SCHAN_MESSAGEr(unit, 1, cmc_schan_message);

    if (op == SC_OP_WR_REG_CMD || op == SC_OP_WR_MEM_CMD || 
        op == SC_OP_TBL_INS_CMD || op == SC_OP_TBL_DEL_CMD ||
        op == SC_OP_TBL_LOOKUP_CMD) {
        for (i = 0; i < len; i++) {
            CMIC_CMC1_SCHAN_MESSAGEr_SET(cmc_schan_message, buf[i]);
            WRITE_CMIC_CMC1_SCHAN_MESSAGEr(unit, (2+i), cmc_schan_message);
        }
    }

    CMIC_CMC1_SCHAN_CTRLr_CLR(cmic_cmc_schan_ctrl);
	CMIC_CMC1_SCHAN_CTRLr_MSG_STARTf_SET(cmic_cmc_schan_ctrl, 1);
    WRITE_CMIC_CMC1_SCHAN_CTRLr(unit, cmic_cmc_schan_ctrl);

    for (i = 0; i < 100; i++) {
        sal_usleep(2);
		READ_CMIC_CMC1_SCHAN_CTRLr(unit, cmic_cmc_schan_ctrl);	
        if (CMIC_CMC1_SCHAN_CTRLr_MSG_DONEf_GET(cmic_cmc_schan_ctrl)) {
            break;
        }
    }

#if CFG_CONSOLE_ENABLED
    if ((op != SC_OP_TBL_LOOKUP_CMD) &&
        ((i == 100) || 
         CMIC_CMC1_SCHAN_CTRLr_SCHAN_ERRORf_GET(cmic_cmc_schan_ctrl) || 
         CMIC_CMC1_SCHAN_CTRLr_SER_CHECK_FAILf_GET(cmic_cmc_schan_ctrl) ||
         CMIC_CMC1_SCHAN_CTRLr_TIMEOUTf_GET(cmic_cmc_schan_ctrl) ||
         CMIC_CMC1_SCHAN_CTRLr_NACKf_GET(cmic_cmc_schan_ctrl))) {
        sal_printf("S-CHAN op=0x%x, %d:0x%x, error(%d-0x%08x)\n", op, block_id, addr, i, CMIC_CMC1_SCHAN_CTRLr_GET(cmic_cmc_schan_ctrl));
    }
#endif /* CFG_CONSOLE_ENABLED */

	READ_CMIC_CMC1_SCHAN_CTRLr(unit, cmic_cmc_schan_ctrl);	
    CMIC_CMC1_SCHAN_CTRLr_MSG_DONEf_SET(cmic_cmc_schan_ctrl, 0);
    WRITE_CMIC_CMC1_SCHAN_CTRLr(unit, cmic_cmc_schan_ctrl);	

    if (op == SC_OP_TBL_LOOKUP_CMD) {
        READ_CMIC_CMC1_SCHAN_MESSAGEr(unit, 0, cmc_schan_message);
        if (CMIC_CMC1_SCHAN_MESSAGEr_DATAf_GET(cmc_schan_message) & 0x1) {            

            SCHAN_UNLOCK(unit);

            return SYS_ERR_NOT_FOUND;
        }
    }

    if (op == SC_OP_RD_REG_CMD || op == SC_OP_RD_MEM_CMD || op == SC_OP_TBL_LOOKUP_CMD) {
        for (i = 0; i < len; i++) {
            READ_CMIC_CMC1_SCHAN_MESSAGEr(unit, (1+i), cmc_schan_message);
   		    buf[i] = CMIC_CMC1_SCHAN_MESSAGEr_DATAf_GET(cmc_schan_message);
        }
    }

    SCHAN_UNLOCK(unit);

    return SYS_OK;
}
sys_error_t
bcm5357x_phy_reg_get(uint8 unit, uint8 port,
                           uint16 reg_addr, uint16 *p_value)
{
    return SYS_OK;
}

sys_error_t
bcm5357x_phy_reg_set(uint8 unit, uint8 port,
                           uint16 reg_addr, uint16 value)
{
    return SYS_OK;
}

sys_error_t bcm5357x_read32(uint8 unit, uint32 addr, uint32 *val) {

    *val = SYS_REG_READ32(addr);
    return SYS_OK;
}

sys_error_t bcm5357x_write32(uint8 unit, uint32 addr, uint32 val) {

           SYS_REG_WRITE32(addr, val);
		   return SYS_OK;
}

sys_error_t
bcm5357x_reg_get(uint8 unit,
                 uint8 block_id,
                 uint32 addr,
                 uint32 *val)
{
	return bcm5357x_sw_op(unit, SC_OP_RD_REG_CMD, block_id, addr, val, 1);
}

sys_error_t
bcm5357x_reg_set(uint8 unit,
                 uint8 block_id,
                 uint32 addr,
                 uint32 val)
{
	return bcm5357x_sw_op(unit, SC_OP_WR_REG_CMD, block_id, addr, &val, 1);
}

sys_error_t
bcm5357x_reg64_get(uint8 unit,
                 uint8 block_id,
                 uint32 addr,
                 uint32 *val,
                 int len)
{
	return bcm5357x_sw_op(unit, SC_OP_RD_REG_CMD, block_id, addr, val, len);
}

sys_error_t
bcm5357x_reg64_set(uint8 unit,
                 uint8 block_id,
                 uint32 addr,
                 uint32 *buf,
                 int len)
{
	return bcm5357x_sw_op(unit, SC_OP_WR_REG_CMD, block_id, addr, buf, len);
}

sys_error_t
bcm5357x_mem_get(uint8 unit,
                 uint8 block_id,
                 uint32 addr,
                 uint32 *buf,
                 int len)
{
	return bcm5357x_sw_op(unit, SC_OP_RD_MEM_CMD, block_id, addr, buf, len);
}

sys_error_t
bcm5357x_mem_set(uint8 unit,
                 uint8 block_id,
                 uint32 addr,
                 uint32 *buf,
                 int len)
{
    return bcm5357x_sw_op(unit, SC_OP_WR_MEM_CMD, block_id, addr, buf, len);
}

static uint32 *bcm5357x_tcam_buffer = NULL;
static int bcm5357x_tcam_maxlen = 0;

sys_error_t 
bcm5357x_tcam_mem_set(uint8 unit,
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

     if (bcm5357x_tcam_maxlen < len || bcm5357x_tcam_buffer == NULL) {
          if (bcm5357x_tcam_buffer) {
             sal_free(bcm5357x_tcam_buffer);
             bcm5357x_tcam_buffer = NULL;
         }
         bcm5357x_tcam_buffer = sal_malloc((len * 4));
         bcm5357x_tcam_maxlen = len;
     }

     sal_memcpy(bcm5357x_tcam_buffer, buf, (len*4)); 
     
     while(key_len != 0) {

           step = 32;

           if (step > key_len) {
               step = key_len;
           }
            
           key =  field32_get(buf, key_sp, key_sp + step - 1);
           mask = field32_get(buf, mask_sp, mask_sp + step - 1);

           encoded_key = key & mask;
           encoded_mask = (~mask | key) ^ xor_value;

           field32_set(bcm5357x_tcam_buffer, key_sp, key_sp + step - 1, encoded_key);
           field32_set(bcm5357x_tcam_buffer, mask_sp, mask_sp + step - 1, encoded_mask);

           key_len -= step;
           key_sp += step;
           mask_sp += step;
     }
     return bcm5357x_mem_set(unit, block_id, addr, bcm5357x_tcam_buffer, len);

	 
}

sys_error_t 
bcm5357x_tcam_mem_get(uint8 unit,
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

    if (bcm5357x_tcam_maxlen < len || bcm5357x_tcam_buffer == NULL) {
        if (bcm5357x_tcam_buffer) {
            sal_free(bcm5357x_tcam_buffer);
            bcm5357x_tcam_buffer = NULL;
        }
        bcm5357x_tcam_buffer = sal_malloc((len * 4));
        bcm5357x_tcam_maxlen = len;
    }

    rv = bcm5357x_mem_get(unit, block_id, addr, bcm5357x_tcam_buffer, len);

    if (rv != SYS_OK) {
        return rv;
    }

    sal_memcpy(buf, bcm5357x_tcam_buffer, (len*4));
    
    while(key_len != 0) {

          step = 32;

          if (step > key_len) {
              step = key_len;
          }
          encoded_key =  field32_get(bcm5357x_tcam_buffer, key_sp, key_sp + step - 1);
          encoded_mask = field32_get(bcm5357x_tcam_buffer, mask_sp, mask_sp + step - 1);
    
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
bcm5357x_l2_op(uint8 unit,
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

    rv = bcm5357x_sw_op(unit, op_code, M_L2_ENTRY(0), l2_entry.v, (sizeof(l2_entry)+3)/4);

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


