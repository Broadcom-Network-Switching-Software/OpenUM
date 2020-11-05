#define __int8_t_defined
#define __uint32_t_defined

#include <system.h>
#undef SOC_IF_ERROR_RETURN
#include <soc/error.h>   /* for SOC_E_XXXX */
#include <utils/net.h>   /* for htol32 */


/* from sdk/include/sal/compiler.h */
#define COMPILER_REFERENCE(_a)    ((void)(_a))
#define COMPILER_ATTRIBUTE(_a)    __attribute__ (_a)
#ifndef FUNCTION_NAME
#define FUNCTION_NAME() (__FUNCTION__)
#endif

#include <shared/bsl.h>
#include <shared/bslenum.h>
#include <shared/bsltypes.h>
#include <soc/property.h>
#include <soc/phyreg.h>

#define MIIM_PARAM_ID_OFFSET            16
#define MIIM_PARAM_REG_ADDR_OFFSET      24      /* Ignored on Lynx; see */
                                                /* CMIC_MIIM_ADDRESS */

#define MIIM_POLL_TIMEOUT               500



static const uint16 _soc_gh2_int_phy_addr[] = {
    0x00, /* Port  0 (cmic)         N/A */
    0x00, /* Port  1 (N/A)              */
    0x81, /* Port  2 (SGMII4P_0)        IntBus=0 Addr=0x01 */
    0x81, /* Port  3 (SGMII4P_0)        Share the same Addr */
    0x81, /* Port  4 (SGMII4P_0)        Share the same Addr */
    0x81, /* Port  5 (SGMII4P_0)        Share the same Addr */
    0x85, /* Port  6 (SGMII4P_1)        Share the same Addr */
    0x85, /* Port  7 (SGMII4P_1)        Share the same Addr */
    0x85, /* Port  8 (SGMII4P_1)        Share the same Addr */
    0x85, /* Port  9 (SGMII4P_1)        Share the same Addr */
    0x89, /* Port 10 (SGMII4P_2)        IntBus=0 Addr=0x09 */
    0x89, /* Port 11 (SGMII4P_2)        Share the same Addr */
    0x89, /* Port 12 (SGMII4P_2)        Share the same Addr */
    0x89, /* Port 13 (SGMII4P_2)        Share the same Addr */
    0x8d, /* Port 14 (SGMII4P_4)        Share the same Addr */
    0x8d, /* Port 15 (SGMII4P_4)        Share the same Addr */
    0x8d, /* Port 16 (SGMII4P_4)        Share the same Addr */
    0x8d, /* Port 17 (SGMII4P_4)        Share the same Addr */
    0x91, /* Port 18 (SGMII4P_4)        IntBus=0 Addr=0x11 */
    0x91, /* Port 19 (SGMII4P_4)        Share the same Addr */
    0x91, /* Port 20 (SGMII4P_4)        Share the same Addr */
    0x91, /* Port 21 (SGMII4P_4)        Share the same Addr */
    0x95, /* Port 22 (SGMII4P_5)        Share the same Addr */
    0x95, /* Port 23 (SGMII4P_5)        Share the same Addr */
    0x95, /* Port 24 (SGMII4P_5)        Share the same Addr */
    0x95, /* Port 25 (SGMII4P_5)        Share the same Addr */
    0xc1, /* Port 26 (QTC0-SGMII/QSGMII) IntBus=2 Addr=0x01 */
    0xc1, /* Port 27 (QTC0-SGMII/QSGMII)Share the same Addr */
    0xc1, /* Port 28 (QTC0-SGMII/QSGMII)Share the same Addr */
    0xc1, /* Port 29 (QTC0-SGMII/QSGMII)Share the same Addr */
    0xc1, /* Port 30 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port 31 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port 32 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port 33 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port 34 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port 35 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port 36 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port 37 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port 38 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port 39 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port 40 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port 41 (QTC0-QSGMII)      Share the same Addr */
    0xc5, /* Port 42 (QTC1-SGMII/QSGMII) IntBus=2 Addr=0x05 */
    0xc5, /* Port 43 (QTC1-SGMII/QSGMII)Share the same Addr */
     0xc5, /* Port 44 (QTC1-SGMII/QSGMII)Share the same Addr */
     0xc5, /* Port 45 (QTC1-SGMII/QSGMII)Share the same Addr */
     0xc5, /* Port 46 (QTC1-QSGMII)      Share the same Addr */
     0xc5, /* Port 47 (QTC1-QSGMII)      Share the same Addr */
     0xc5, /* Port 48 (QTC1-QSGMII)      Share the same Addr */
     0xc5, /* Port 49 (QTC1-QSGMII)      Share the same Addr */
     0xc5, /* Port 50 (QTC1-QSGMII)      Share the same Addr */
     0xc5, /* Port 51 (QTC1-QSGMII)      Share the same Addr */
     0xc5, /* Port 52 (QTC1-QSGMII)      Share the same Addr */
     0xc5, /* Port 53 (QTC1-QSGMII)      Share the same Addr */
     0xc5, /* Port 54 (QTC1-QSGMII)      Share the same Addr */
     0xc5, /* Port 55 (QTC1-QSGMII)      Share the same Addr */
     0xc5, /* Port 56 (QTC1-QSGMII)      Share the same Addr */
     0xc5, /* Port 57 (QTC1-QSGMII)      Share the same Addr */
     0xa1, /* Port 58 (TSCE0)            IntBus=1 Addr=0x01 */
     0xa1, /* Port 59 (TSCE0)            Share the same Addr */
     0xa1, /* Port 60 (TSCE0)            Share the same Addr */
     0xa1, /* Port 61 (TSCE0)            Share the same Addr */
     0xa5, /* Port 62 (TSCE0)            IntBus=1 Addr=0x05 */
     0xa5, /* Port 63 (TSCE1)            Share the same Addr */
     0xa5, /* Port 64 (TSCE1)            Share the same Addr */
     0xa5, /* Port 65 (TSCE1)            Share the same Addr */
     0xa9, /* Port 66 (TSCE2)            IntBus=1 Addr=0x09 */
     0xa9, /* Port 67 (TSCE2)            Share the same Addr */
     0xa9, /* Port 68 (TSCE2)            Share the same Addr */
     0xa9, /* Port 69 (TSCE2)            Share the same Addr */
     0xad, /* Port 70 (TSCE3)            IntBus=1 Addr=0x0d */
     0xad, /* Port 71 (TSCE3)            Share the same Addr */
     0xad, /* Port 72 (TSCE3)            Share the same Addr */
     0xad, /* Port 73 (TSCE3)            Share the same Addr */
     0xb1, /* Port 74 (TSCE4)            IntBus=1 Addr=0x11 */
     0xb1, /* Port 75 (TSCE4)            Share the same Addr */
     0xb1, /* Port 76 (TSCE4)            Share the same Addr */
     0xb1, /* Port 77 (TSCE4)            Share the same Addr */
     0xb5, /* Port 78 (TSCE5)            IntBus=1 Addr=0x15 */
     0xb5, /* Port 79 (TSCE5)            Share the same Addr */
     0xb5, /* Port 80 (TSCE5)            Share the same Addr */
     0xb5, /* Port 81 (TSCE5)            Share the same Addr */
     0xb9, /* Port 82 (TSCE6)            IntBus=1 Addr=0x19 */
     0xb9, /* Port 83 (TSCE6)            Share the same Addr */
     0xb9, /* Port 84 (TSCE6)            Share the same Addr */
     0xb9, /* Port 85 (TSCF0)            Share the same Addr */
     0xe1, /* Port 86 (TSCF0)            IntBus=3 Addr=0x01 */
     0xe1, /* Port 87 (TSCF0)            Share the same Addr */

    0xe1, /* Port 88 (TSCF0)            Share the same Addr */
    0xe1  /* Port 89 (TSCF0)            Share the same Addr */

};

#define FW_ALIGN_BYTES                  16
#define FW_ALIGN_MASK                   (FW_ALIGN_BYTES - 1)

int
bcm95357x_firmware_set(int unit, int port, uint8 *data, int size) {

//	  int ioerr = 0;
	 // uint32 val;
	  uint32 wbuf[4];
	  uint32 *fw_data;
	  uint32 *fw_entry;
	  uint32 fw_size;
	  uint32 idx, wdx;
	//  uint32 offset = 0; // MDIO clock ?
      int block_pport;
	//  CMIC_RATE_ADJUST_EXT_MDIOr_t cmic_rate_adjust_ext_mdio;
	  GPORT_WC_UCMEM_CTRLr_t gport_wc_ucmem_ctrl;
	  GPORT_WC_UCMEM_DATAm_t gport_wc_ucmem_data;
      XLPORT_WC_UCMEM_CTRLr_t xlport_wc_ucmem_ctrl;
	  XLPORT_WC_UCMEM_DATAm_t xlport_wc_ucmem_data;
      CLPORT_WC_UCMEM_CTRLr_t clport_wc_ucmem_ctrl;
	  CLPORT_WC_UCMEM_DATAm_t clport_wc_ucmem_data; 
#if 0
	  /* Check if PHY driver requests optimized MDC clock */
	  if (data == NULL) {
		  val = 1;
	  
		  /* Offset value is MDC clock in kHz (or zero for default) */
		  if (offset) {
			  val = offset / 1500;
		  }
	  
		  ioerr = READ_CMIC_RATE_ADJUST_EXT_MDIOr(unit, cmic_rate_adjust_ext_mdio);
		  CMIC_RATE_ADJUST_EXT_MDIOr_DIVIDENDf_SET(cmic_rate_adjust_ext_mdio, val);
		  ioerr += WRITE_CMIC_RATE_ADJUST_EXT_MDIOr(unit, cmic_rate_adjust_ext_mdio);
	  
		  return ioerr ? SYS_ERR_STATE : SYS_OK;
	  }
#endif
	  if (size == 0) {
		  return SYS_ERR_PARAMETER;
	  }
	  
	  /* Aligned firmware size */
	  fw_size = (size + FW_ALIGN_MASK) & ~FW_ALIGN_MASK;
	  if (IS_CL_PORT(port)) {
		  /* Enable parallel bus access, ACCESS_MODE [Bit 0] = 0x1 */
		  CLPORT_WC_UCMEM_CTRLr_CLR(clport_wc_ucmem_ctrl);
		  CLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(clport_wc_ucmem_ctrl, 1);
          block_pport = SOC_PORT_L2P_MAPPING(port) - SOC_PORT_BLOCK_INDEX(port);
		  WRITE_CLPORT_WC_UCMEM_CTRLr(unit, SOC_PORT_P2L_MAPPING(block_pport), clport_wc_ucmem_ctrl);

		  
		  /* DMA buffer needs 32-bit words in little endian order */
		  fw_data = (uint32 *)data;
		  for (idx = 0; idx < fw_size; idx += 16) {
			  if (idx + 15 < size) {
				  fw_entry = &fw_data[idx >> 2];
			  } else {
				  /* Use staging buffer for modulo bytes */
				  sal_memset(wbuf, 0, sizeof(wbuf));
				  sal_memcpy(wbuf, &fw_data[idx >> 2], 16 - (fw_size - size));
				  fw_entry = wbuf;
			  }
			  CLPORT_WC_UCMEM_DATAm_CLR(clport_wc_ucmem_data);
			  for (wdx = 0; wdx < 4; wdx++) {
				   CLPORT_WC_UCMEM_DATAm_SET(clport_wc_ucmem_data, wdx, htol32(fw_entry[wdx]));
			  }
			  WRITE_CLPORT_WC_UCMEM_DATAm(unit, SOC_PORT_P2L_MAPPING(block_pport), (idx >> 4), clport_wc_ucmem_data);
		  }
		  
		  /* Disable parallel bus access */
		  CLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(clport_wc_ucmem_ctrl, 0);
		  WRITE_CLPORT_WC_UCMEM_CTRLr(unit, SOC_PORT_P2L_MAPPING(block_pport), clport_wc_ucmem_ctrl);
    
       } else if (IS_XL_PORT(port)) {
		  /* Enable parallel bus access, ACCESS_MODE [Bit 0] = 0x1 */
		  XLPORT_WC_UCMEM_CTRLr_CLR(xlport_wc_ucmem_ctrl);
		  XLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(xlport_wc_ucmem_ctrl, 1);
          block_pport = SOC_PORT_L2P_MAPPING(port) - SOC_PORT_BLOCK_INDEX(port);
		  WRITE_XLPORT_WC_UCMEM_CTRLr(unit, SOC_PORT_P2L_MAPPING(block_pport), xlport_wc_ucmem_ctrl);

		  
		  /* DMA buffer needs 32-bit words in little endian order */
		  fw_data = (uint32 *)data;
		  for (idx = 0; idx < fw_size; idx += 16) {
			  if (idx + 15 < size) {
				  fw_entry = &fw_data[idx >> 2];
			  } else {
				  /* Use staging buffer for modulo bytes */
				  sal_memset(wbuf, 0, sizeof(wbuf));
				  sal_memcpy(wbuf, &fw_data[idx >> 2], 16 - (fw_size - size));
				  fw_entry = wbuf;
			  }
			  XLPORT_WC_UCMEM_DATAm_CLR(xlport_wc_ucmem_data);
			  for (wdx = 0; wdx < 4; wdx++) {
				  XLPORT_WC_UCMEM_DATAm_SET(xlport_wc_ucmem_data, wdx, htol32(fw_entry[wdx]));
			  }
			  WRITE_XLPORT_WC_UCMEM_DATAm(unit, SOC_PORT_P2L_MAPPING(block_pport), (idx >> 4), xlport_wc_ucmem_data);
		  }
		  
		  /* Disable parallel bus access */
		  XLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(xlport_wc_ucmem_ctrl, 0);
		  WRITE_XLPORT_WC_UCMEM_CTRLr(unit, SOC_PORT_P2L_MAPPING(block_pport), xlport_wc_ucmem_ctrl);


      } else if (IS_QTCE_PORT(port)) {	  /* Enable parallel bus access, ACCESS_MODE [Bit 0] = 0x1 */
          GPORT_WC_UCMEM_CTRLr_CLR(gport_wc_ucmem_ctrl);
          GPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(gport_wc_ucmem_ctrl, 1);
          block_pport = SOC_PORT_L2P_MAPPING(port) - SOC_PORT_BLOCK_INDEX(port);
          WRITE_GPORT_WC_UCMEM_CTRLr(unit, SOC_PORT_P2L_MAPPING(block_pport), gport_wc_ucmem_ctrl);
	  
	  /* DMA buffer needs 32-bit words in little endian order */
	  fw_data = (uint32 *)data;
	  for (idx = 0; idx < fw_size; idx += 16) {
		  if (idx + 15 < size) {
			  fw_entry = &fw_data[idx >> 2];
		  } else {
			  /* Use staging buffer for modulo bytes */
			  sal_memset(wbuf, 0, sizeof(wbuf));
			  sal_memcpy(wbuf, &fw_data[idx >> 2], 16 - (fw_size - size));
			  fw_entry = wbuf;
		  }
		  GPORT_WC_UCMEM_DATAm_CLR(gport_wc_ucmem_data);
		  for (wdx = 0; wdx < 4; wdx++) {
			  GPORT_WC_UCMEM_DATAm_SET(gport_wc_ucmem_data, wdx, htol32(fw_entry[wdx]));
		  }
		  WRITE_GPORT_WC_UCMEM_DATAm(unit, SOC_PORT_P2L_MAPPING(block_pport), (idx >> 4), gport_wc_ucmem_data);
	  }
	  
	  /* Disable parallel bus access */
	  GPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(gport_wc_ucmem_ctrl, 0);
	  WRITE_GPORT_WC_UCMEM_CTRLr(unit, SOC_PORT_P2L_MAPPING(block_pport), gport_wc_ucmem_ctrl);
      }

    return SYS_OK;
}


static int mdio_addr_to_port(uint32 phy_addr_int){
    int bus, offset = 0;
    int mdio_addr;

    /* Must be internal MDIO address */
    if ((phy_addr_int & 0x80) == 0) {
        return -1;
    }

    bus = PCM_PHY_ID_BUS_NUM(phy_addr_int);
    mdio_addr = phy_addr_int & 0x1f;

    if (bus == 0) {
        /* for SGMII4Px2 */
        if (mdio_addr <= 0x18) {
            offset = 1;
        } else {
            sal_printf("phy_addr_int = %x not found %d \n", phy_addr_int, __LINE__);
            return -1;
        }
    } else if (bus == 1) {
        /* for TSCE */
        if (mdio_addr <= 0x1c) {
            offset = 57;
        } else {
            sal_printf("phy_addr_int = %x not found %d \n", phy_addr_int, __LINE__);
            return -1;
        }
    } else if (bus == 2) {
        if (mdio_addr <= 4) {
            /* for QTC0 */
            offset = 25;
        } else if (mdio_addr <= 8) {
            /* for QTC1 */
            offset = 37;
        } else {
            sal_printf("phy_addr_int = %x not found %d \n", phy_addr_int, __LINE__);
            return -1;
        }
    } else if (bus == 3) {
        /* for TSCF */
        if (mdio_addr <= 0x4) {
            offset = 85;
        } else {
            sal_printf("phy_addr_int = %x not found %d \n", phy_addr_int, __LINE__);
            return -1;
        }
    } else {
        sal_printf("phy_addr_int = %x not found %d \n", phy_addr_int, __LINE__);
        return -1;
    }

    return mdio_addr + offset;

}

#define TSC_REG_ADDR_TSCID_SET(_phy_reg, _phyad)    \
                            ((_phy_reg) |= ((_phyad) & 0x1f) << 19)


/*

   Sync with _soc_hurricane3_tscx_reg_read in hurricane3.c 

*/
int
soc_sbus_tsc_reg_read(int unit, uint32_t phy_addr, 
							 uint32_t phy_reg, uint32_t *phy_data)
{
	int rv = SOC_E_NONE;
	uint32 xlport_wc_ucmem_data[4];
	uint8 block_id;
	uint32 addr;
    int lport;
	SAL_DEBUGF(("soc_sbus_tsc_reg_read(%d,%d,%d,0x%x,0x%08x,*phy_data)..\n", unit, 10, 14, phy_addr, phy_reg));
	
	MIIM_SCHAN_LOCK(unit);

	/* TSCE sbus access */
	xlport_wc_ucmem_data[0] = phy_reg & 0xffffffff;
	xlport_wc_ucmem_data[1] = 0x0;
	xlport_wc_ucmem_data[2] = 0x0;
	xlport_wc_ucmem_data[3] = 0x0;
	
    lport = SOC_PORT_P2L_MAPPING(mdio_addr_to_port(phy_addr));
    if (IS_SGMIIPX4_PORT(lport)) {
		addr = M_GPORT_WC_UCMEM_DATA(0);
        block_id = PMQ0_BLOCK_ID  + SGMIIPX4_CORE_NUM_GET(lport) * 4;              
    } else if (IS_QTCE_PORT(lport)) {
		addr = M_GPORT_WC_UCMEM_DATA(0);
        block_id = PMQ3_BLOCK_ID  + QTCE_CORE_NUM_GET(lport) * 4;              
    } else if (IS_TSCE_PORT(lport)) {
		addr = M_XLPORT_WC_UCMEM_DATA(0);
        if (TSCE_CORE_NUM_GET(lport) < 4) {
     		block_id = XLPORT0_BLOCK_ID + TSCE_CORE_NUM_GET(lport);
        } else {
     		block_id = XLPORT4_BLOCK_ID + TSCE_CORE_NUM_GET(lport) - 4;
        }
    } else if (IS_TSCF_PORT(lport)) {
		addr = M_CLPORT_WC_UCMEM_DATA(0);
		block_id = CLPORT0_BLOCK_ID;
    }
	
	
	SAL_DEBUGF(("	ucmem_data_entry[95:64-63:32-31:0]=0x%08x-0x%08x-0x%08x\n", xlport_wc_ucmem_data[0], xlport_wc_ucmem_data[1], xlport_wc_ucmem_data[2]));
	
	rv = bcm5357x_mem_set(unit, block_id, addr, xlport_wc_ucmem_data, 4);
	
	if (SOC_SUCCESS(rv)) {
		rv = bcm5357x_mem_get(unit, block_id, addr, xlport_wc_ucmem_data, 4);			 
	}
	*phy_data = xlport_wc_ucmem_data[1];
	
	SAL_DEBUGF(("soc_sbus_tsc_reg_read: *phy_data=0x%04x,rv=%d\n", *phy_data, rv));
	
	MIIM_SCHAN_UNLOCK(unit);
	return rv;
}
	
	
int
soc_sbus_tsc_reg_write(int unit, uint32_t phy_addr,
							  uint32_t phy_reg, uint32_t phy_data)
{
	int rv = SOC_E_NONE;
	uint32 xlport_wc_ucmem_data[4];
	uint8 block_id;
	uint32 addr;
    int lport;

	SAL_DEBUGF(("soc_sbus_tsc_reg_write(%d,%d,%d,0x%x,0x%08x,0x%04x)..\n", unit, 10, 14, phy_addr, phy_reg, phy_data));    
	
	MIIM_SCHAN_LOCK(unit);
	
	/* TSCE sbus access */
	if ((phy_data & 0xffff0000) == 0) {
		phy_data |= 0xffff0000;
	}
	
	xlport_wc_ucmem_data[0] = phy_reg & 0xffffffff;;
	xlport_wc_ucmem_data[1] = ((phy_data & 0xffff) << 16) | 
				  ((~phy_data & 0xffff0000) >> 16);
	xlport_wc_ucmem_data[2] = 1; /* for TSC register write */
	xlport_wc_ucmem_data[3] = 0x0;
	
	
    lport = SOC_PORT_P2L_MAPPING(mdio_addr_to_port(phy_addr));
    if (IS_SGMIIPX4_PORT(lport)) {
		addr = M_GPORT_WC_UCMEM_DATA(0);
        block_id = PMQ0_BLOCK_ID  + SGMIIPX4_CORE_NUM_GET(lport) * 4;              
    } else if (IS_QTCE_PORT(lport)) {
		addr = M_GPORT_WC_UCMEM_DATA(0);
        block_id = PMQ3_BLOCK_ID  + QTCE_CORE_NUM_GET(lport) * 4;              
    } else if (IS_TSCE_PORT(lport)) {
		addr = M_XLPORT_WC_UCMEM_DATA(0);
        if (TSCE_CORE_NUM_GET(lport) < 4) {
     		block_id = XLPORT0_BLOCK_ID + TSCE_CORE_NUM_GET(lport);
        } else {
     		block_id = XLPORT4_BLOCK_ID + TSCE_CORE_NUM_GET(lport) - 4;
        }
    } else if (IS_TSCF_PORT(lport)) {
		addr = M_CLPORT_WC_UCMEM_DATA(0);
		block_id = CLPORT0_BLOCK_ID;
    }
		
	SAL_DEBUGF(("	ucmem_data_entry[95:64-63:32-31:0]=0x%08x-0x%08x-0x%08x\n", xlport_wc_ucmem_data[0], xlport_wc_ucmem_data[1], xlport_wc_ucmem_data[2]));
	
	rv = bcm5357x_mem_set(unit, block_id, addr, xlport_wc_ucmem_data, 4);
	
	SAL_DEBUGF(("soc_sbus_tsc_reg_write : rv=%d\n",rv));
	
	MIIM_SCHAN_UNLOCK(unit);
	return rv;
	
}


STATIC int
soc_greyhound2_tscx_reg_read(int unit, uint32 phy_addr,
                            uint32 phy_reg, uint32_t *phy_data)
{
    int rv = SYS_OK;
    TSC_REG_ADDR_TSCID_SET(phy_reg, phy_addr); 
	rv = soc_sbus_tsc_reg_read(unit, phy_addr, phy_reg, phy_data);
//    sal_printf("soc_greyhound2_tscx_reg_read[%d]: %x/%x/%x\n", unit, phy_addr, phy_reg, *phy_data);
    return rv;
}

/*

   Sync with _soc_hurricane3_tscx_reg_read in hurricane3.c 

*/


STATIC int
soc_greyhound2_tscx_reg_write(int unit, uint32 phy_addr,
                             uint32 phy_reg, uint32_t phy_data)
{
    int rv = SYS_OK;

//    sal_printf("soc_greyhound2_tscx_reg_write[%d]: 0x%x/0x%x/%x\n", unit, phy_addr, phy_reg, phy_data);

    TSC_REG_ADDR_TSCID_SET(phy_reg, phy_addr); 
    rv = soc_sbus_tsc_reg_write(unit, phy_addr, phy_reg, phy_data);

    return rv;
}

/*
 * Function:
 *      soc_miim_write
 * Purpose:
 *      Write a value to a MIIM register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_wr_data - Data to write.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Temporarily disables auto link scan if it was enabled.  The MIIM
 *      registers are locked during the operation to prevent multiple
 *      tasks from trying to access PHY registers simultaneously.
 */

int 
soc_miim_write(int unit, uint16 phy_id,
               uint8 phy_reg_addr, uint16 phy_wr_data)
{
    int                 rv = SOC_E_NONE;
    int                 clause45;
    uint32              phy_param;


    CMIC_CMC1_MIIM_STATr_t miim_stat;
    CMIC_CMC1_MIIM_PARAMr_t miim_param;
    CMIC_CMC1_MIIM_CTRLr_t miim_ctrl;
    CMIC_CMC1_MIIM_ADDRESSr_t miim_address;
    tick_t to;   


    LOG_VERBOSE(BSL_LS_SOC_MIIM,
             (BSL_META_U(unit,
                         "soc_miim_write: id=0x%02x addr=0x%02x data=0x%04x\n"),
              phy_id, phy_reg_addr, phy_wr_data));

//    clause45 = soc_feature(unit, soc_feature_phy_cl45);
    clause45 = 1;

     uint32 bus_sel = PCM_PHY_ID_BUS_NUM(phy_id);

     /* set 5-bit PHY MDIO address */
     phy_param = (uint32)phy_wr_data | 
                    (((uint32)phy_id & 0x1f) << MIIM_PARAM_ID_OFFSET);

     /* select internal MDIO bus if set */
    if (phy_id & 0x80) { /* Internal/External select */
        phy_param |= (1 << (MIIM_PARAM_ID_OFFSET + 9));
    }

        /* set MDIO bus number */
       phy_param |= (bus_sel << (MIIM_PARAM_ID_OFFSET + 6));


    /* Write parameter register and tell CMIC to start */

    /* Clause 45 support changes Clause 22 access method */
    if (clause45) {
        CMIC_CMC1_MIIM_ADDRESSr_CLR(miim_address);        
        CMIC_CMC1_MIIM_ADDRESSr_SET(miim_address, phy_reg_addr);        
        WRITE_CMIC_CMC1_MIIM_ADDRESSr(0, miim_address);
    } else {
        phy_param |= (uint32)phy_reg_addr << MIIM_PARAM_REG_ADDR_OFFSET;
    }
    CMIC_CMC1_MIIM_PARAMr_CLR(miim_param);
    CMIC_CMC1_MIIM_PARAMr_PHY_IDf_SET(miim_param, phy_id);
    CMIC_CMC1_MIIM_PARAMr_BUS_IDf_SET(miim_param, bus_sel);
    CMIC_CMC1_MIIM_PARAMr_SET(miim_param, phy_param);    
    WRITE_CMIC_CMC1_MIIM_PARAMr(0,miim_param);
	READ_CMIC_CMC1_MIIM_CTRLr(0, miim_ctrl);
    CMIC_CMC1_MIIM_CTRLr_CLR(miim_ctrl);
    CMIC_CMC1_MIIM_CTRLr_MIIM_WR_STARTf_SET(miim_ctrl,1);
    WRITE_CMIC_CMC1_MIIM_CTRLr(0,miim_ctrl);

    /* Wait for completion using either the interrupt or polling method */

	to = sal_get_ticks();

    READ_CMIC_CMC1_MIIM_STATr(0, miim_stat);

    while (CMIC_CMC1_MIIM_STATr_MIIM_OPN_DONEf_GET(miim_stat) == 0) {
           if (SAL_TIME_EXPIRED(to, MIIM_POLL_TIMEOUT)) {
               rv = SOC_E_TIMEOUT;
               break;
           }
           READ_CMIC_CMC1_MIIM_STATr(0, miim_stat);    
    }
    if (rv == SOC_E_NONE) {
            CMIC_CMC1_MIIM_CTRLr_CLR(miim_ctrl);
            WRITE_CMIC_CMC1_MIIM_CTRLr(0, miim_ctrl);

    }
    if (rv == SOC_E_TIMEOUT) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "MiimTimeOut:soc_miim_write, "
                              "timeout (id=0x%02x addr=0x%02x data=0x%04x)\n"),
                   phy_id, phy_reg_addr, phy_wr_data));
    }

    return rv;

}


/*
 * Function:    soc_cm_get_name
 * Purpose:     returns the symbolic name of the device.
 *
 * Parameters:  dev - device handle
 *
 * Returns:     symbolic name of this device.
 *
 * Note:        The return value is symbolic name of this device.
 *              String values are valid, and should not be interpreted as error.
 *              Negative values indicate an error.
 */

const char *
soc_cm_get_name(int dev)
{
   return "GREYHOUND2";
}


const char *
soc_cm_config_var_get(int dev, const char *name) {
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
     return sal_config_get(name);
#else
     return NULL;
#endif
}

int
soc_cm_get_id(int unit, uint16 *dev_id, uint8 *rev_id) {
    
    uint16 devid, rev;
    bcm5357x_chip_revision(unit, &devid, &rev);
    *dev_id = devid;
    *rev_id = rev;

    return SYS_OK;
}

/*
 * Function:
 *      soc_miim_read
 * Purpose:
 *      Read a value from an MII register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_rd_data - 16bit data to write into
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Temporarily disables auto link scan if it was enabled.  The MIIM
 *      registers are locked during the operation to prevent multiple
 *      tasks from trying to access PHY registers simultaneously.
 */

int
soc_miim_read(int unit, uint16 phy_id,
              uint8 phy_reg_addr, uint16 *phy_rd_data)
{
    int                 rv = SOC_E_NONE;
    int                 clause45;
    uint32              phy_param;

    tick_t to;
    CMIC_CMC1_MIIM_STATr_t miim_stat;
    CMIC_CMC1_MIIM_PARAMr_t miim_param;
    CMIC_CMC1_MIIM_CTRLr_t miim_ctrl;
    CMIC_CMC1_MIIM_READ_DATAr_t miim_read_data;
    CMIC_CMC1_MIIM_ADDRESSr_t miim_address;

    LOG_VERBOSE(BSL_LS_SOC_MIIM,
             (BSL_META_U(unit,
                         "soc_miim_read: id=0x%02x addr=0x%02x  "),
              phy_id, phy_reg_addr));

    clause45 = 1;

    /* 
         * Trident Switch And Later
         * internal select bit 25
         * BUS_ID  bit 22-24
         * C45_SEL bit 21
         */
      
    uint32 bus_sel = PCM_PHY_ID_BUS_NUM(phy_id);
        
    /* set 5-bit PHY MDIO address */ 
    phy_param = (((uint32)phy_id & 0x1f) << MIIM_PARAM_ID_OFFSET);

    /* select internal MDIO bus if set */
        if (phy_id & 0x80) { /* Internal/External select */
            phy_param |= (1 << (MIIM_PARAM_ID_OFFSET + 9));
        }
  
        /* set MDIO bus number */
        phy_param |= (bus_sel << (MIIM_PARAM_ID_OFFSET + 6));



    /* Write parameter register and tell CMIC to start */

    /* Clause 45 support changes Clause 22 access method */
    if (clause45) {
        CMIC_CMC1_MIIM_ADDRESSr_CLR(miim_address);        
        CMIC_CMC1_MIIM_ADDRESSr_SET(miim_address, phy_reg_addr);        
        WRITE_CMIC_CMC1_MIIM_ADDRESSr(0, miim_address);
    } else {
        phy_param |= (uint32)phy_reg_addr << MIIM_PARAM_REG_ADDR_OFFSET;
    }

    CMIC_CMC1_MIIM_PARAMr_CLR(miim_param);
    CMIC_CMC1_MIIM_PARAMr_SET(miim_param, phy_param);    
    WRITE_CMIC_CMC1_MIIM_PARAMr(0,miim_param);
    
    READ_CMIC_CMC1_MIIM_CTRLr(0, miim_ctrl);
    CMIC_CMC1_MIIM_CTRLr_CLR(miim_ctrl);
    CMIC_CMC1_MIIM_CTRLr_MIIM_RD_STARTf_SET(miim_ctrl,1);
    WRITE_CMIC_CMC1_MIIM_CTRLr(0, miim_ctrl);

    /* Wait for completion using either the interrupt or polling method */
    to = sal_get_ticks();
        
    READ_CMIC_CMC1_MIIM_STATr(0, miim_stat);
        
    while (CMIC_CMC1_MIIM_STATr_MIIM_OPN_DONEf_GET(miim_stat) == 0) {
           if (SAL_TIME_EXPIRED(to, MIIM_POLL_TIMEOUT)) {
                        rv = SOC_E_TIMEOUT;
                        break;
           }
           READ_CMIC_CMC1_MIIM_STATr(0, miim_stat);    
    }

    READ_CMIC_CMC1_MIIM_CTRLr(0, miim_ctrl);
	CMIC_CMC1_MIIM_CTRLr_MIIM_RD_STARTf_SET(miim_ctrl,0);
    WRITE_CMIC_CMC1_MIIM_CTRLr(0, miim_ctrl);

    if (rv == SOC_E_TIMEOUT) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "MiimTimeOut:soc_miim_read, "
                              "timeout (id=0x%02x addr=0x%02x)\n"),
                   phy_id, phy_reg_addr));
    } else {

        READ_CMIC_CMC1_MIIM_READ_DATAr(0, miim_read_data);
        *phy_rd_data = CMIC_CMC1_MIIM_READ_DATAr_DATAf_GET(miim_read_data);

        LOG_VERBOSE(BSL_LS_SOC_MIIM,
                 (BSL_META_U(unit,
                             "data=0x%04x\n"), *phy_rd_data));
    }


    return rv;

}

/*
 * Function:
 *      soc_esw_miim_write
 * Purpose:
 *      New interface to write a value to a MIIM register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_wr_data - Data to write.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 * Added to have the same interface for PHY register access among 
 * ESW, ROBO, SBX
 */
int 
soc_esw_miim_write(int unit, uint32 phy_id,
                   uint32 phy_reg_addr, uint16 phy_wr_data)
{
    return soc_miim_write(unit, (uint16)phy_id,
                          (uint16)phy_reg_addr, phy_wr_data);
}

/*
 * Function:
 *      soc_esw_miim_read
 * Purpose:
 *      New interface to read a value from a MIIM register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_rd_data - Data read.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 * Added to have the same interface for PHY register access among 
 * ESW, ROBO, SBX.
 */
int
soc_esw_miim_read(int unit, uint32 phy_id,
                  uint32 phy_reg_addr, uint16 *phy_rd_data)
{
    return soc_miim_read(unit, (uint16)phy_id, 
                         (uint16)phy_reg_addr, phy_rd_data);
}

/*
 * Function:
 *      soc_miim_modify
 * Purpose:
 *      Modify a value from an MII register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_rd_data - 16bit data to write into
 *      phy_rd_mask - 16bit mask to indicate the bits to modify.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */

int
soc_miim_modify(int unit, uint16 phy_id, uint16 phy_reg_addr, 
                uint16 phy_rd_data, uint16 phy_rd_mask)
{
    uint16  tmp, otmp;

    phy_rd_data = phy_rd_data & phy_rd_mask;

    SOC_IF_ERROR_RETURN
        (soc_miim_read(unit, phy_id, phy_reg_addr, &tmp));
    otmp = tmp;
    tmp &= ~(phy_rd_mask);
    tmp |= phy_rd_data;

    if (otmp != tmp) {
        SOC_IF_ERROR_RETURN
            (soc_miim_write(unit, phy_id, phy_reg_addr, tmp));
    }
    return SOC_E_NONE;
}


/* CL45 warmboot write disable override */
#define MIIM_WB_C45                     (1 << 0)

#define MIIM_CYCLE_AUTO                 0x0
#define MIIM_CYCLE_C22_REG_WR           0x1
#define MIIM_CYCLE_C22_REG_RD           0x2
#define MIIM_CYCLE_C45_REG_AD           0x4
#define MIIM_CYCLE_C45_REG_WR           0x5
#define MIIM_CYCLE_C45_REG_RD_ADINC     0x6
#define MIIM_CYCLE_C45_REG_RD           0x7

#define MIIM_CYCLE_C45_SHFT             6
#define MIIM_CYCLE_C45_MASK             (0x3 << MIIM_CYCLE_C45_SHFT)
#define MIIM_CYCLE_C45_WR               (1 << (MIIM_CYCLE_C45_SHFT + 0))
#define MIIM_CYCLE_C45_WR_AD            (1 << (MIIM_CYCLE_C45_SHFT + 1))
#define MIIM_CYCLE_C45_RD               (1 << (MIIM_CYCLE_C45_SHFT + 0))
#define MIIM_CYCLE_C45_RD_ADINC         (1 << (MIIM_CYCLE_C45_SHFT + 1))

#define SOC_PHY_CLAUSE45_ADDR(_devad, _regad) \
            _SHR_PORT_PHY_CLAUSE45_ADDR(_devad, _regad)
#define SOC_PHY_CLAUSE45_DEVAD(_reg_addr) \
            _SHR_PORT_PHY_CLAUSE45_DEVAD(_reg_addr)


/*
 * Function:
 *      __soc_miimc45_write
 * Purpose:
 *      Write a value to a MIIM register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_devad - Device type (PMA/PMD, PCS, PHY XS)
 *      phy_reg_addr - PHY register to write
 *      phy_wr_data - Data to write.
 *      flags - flags altering the behavior of the function.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Temporarily disables auto link scan if it was enabled.  The MIIM
 *      registers are locked during the operation to prevent multiple
 *      tasks from trying to access PHY registers simultaneously.
 */

int 
__soc_miimc45_write(int unit, uint16 phy_id, uint8 phy_devad,
                  uint16 phy_reg_addr, uint16 phy_wr_data, uint32 flags)
{
    int      rv = SOC_E_NONE;
    uint32   bus_sel = PCM_PHY_ID_BUS_NUM(phy_id);
    tick_t to;
    uint32          cycle_type;

    CMIC_CMC1_MIIM_PARAMr_t cmic_cmc1_miim_param;
    CMIC_CMC1_MIIM_ADDRESSr_t cmic_cmc1_miim_address;
    CMIC_CMC1_MIIM_CTRLr_t cmic_cmc1_miim_ctrl;
    CMIC_CMC1_MIIM_STATr_t miim_stat;

    LOG_VERBOSE(BSL_LS_SOC_MIIM,
             (BSL_META_U(unit,
                         "__soc_miimc45_write: id=0x%02x phy_devad=0x%02x "
                         "addr=0x%02x data=0x%04x\n"),
              phy_id, phy_devad, phy_reg_addr, phy_wr_data));

    /* Write parameter registers and tell CMIC to start */
    CMIC_CMC1_MIIM_PARAMr_CLR(cmic_cmc1_miim_param);
    if (phy_devad & MIIM_CYCLE_C45_WR) {
        cycle_type = MIIM_CYCLE_C45_REG_WR;
    } else if (phy_devad & MIIM_CYCLE_C45_WR_AD) {
        cycle_type = MIIM_CYCLE_C45_REG_AD;
    } else {
        cycle_type = MIIM_CYCLE_AUTO;
    }

    phy_devad &= ~MIIM_CYCLE_C45_MASK;
    CMIC_CMC1_MIIM_PARAMr_MIIM_CYCLEf_SET(cmic_cmc1_miim_param, cycle_type);

    if (phy_id & 0x80) {
        phy_id &= (~0x80);
        CMIC_CMC1_MIIM_PARAMr_INTERNAL_SELf_SET(cmic_cmc1_miim_param, 1); 
    }
    CMIC_CMC1_MIIM_PARAMr_C45_SELf_SET(cmic_cmc1_miim_param, 1);
    CMIC_CMC1_MIIM_PARAMr_BUS_IDf_SET(cmic_cmc1_miim_param, bus_sel);

    phy_id &= ~(PCM_PHY_ID_BUS_UPPER_MASK | PCM_PHY_ID_BUS_LOWER_MASK);
    phy_id &= (~0x40);

    CMIC_CMC1_MIIM_PARAMr_PHY_IDf_SET(cmic_cmc1_miim_param, phy_id);
    CMIC_CMC1_MIIM_PARAMr_PHY_DATAf_SET(cmic_cmc1_miim_param, phy_wr_data);

    CMIC_CMC1_MIIM_ADDRESSr_CLR(cmic_cmc1_miim_address);
    CMIC_CMC1_MIIM_ADDRESSr_CLAUSE_45_DTYPEf_SET(cmic_cmc1_miim_address, phy_devad);
    CMIC_CMC1_MIIM_ADDRESSr_CLAUSE_45_REGADRf_SET(cmic_cmc1_miim_address, phy_reg_addr);

       
    CMIC_CMC1_MIIM_CTRLr_CLR(cmic_cmc1_miim_ctrl);
    CMIC_CMC1_MIIM_CTRLr_MIIM_WR_STARTf_SET(cmic_cmc1_miim_ctrl, 1);

    WRITE_CMIC_CMC1_MIIM_ADDRESSr(unit, cmic_cmc1_miim_address);
    WRITE_CMIC_CMC1_MIIM_PARAMr(unit, cmic_cmc1_miim_param);
    WRITE_CMIC_CMC1_MIIM_CTRLr(unit, cmic_cmc1_miim_ctrl);

    /* Wait for completion using either the interrupt or polling method */
    to = sal_get_ticks();
            
    READ_CMIC_CMC1_MIIM_STATr(0, miim_stat);            
    while (CMIC_CMC1_MIIM_STATr_MIIM_OPN_DONEf_GET(miim_stat) == 0) {
           if (SAL_TIME_EXPIRED(to, MIIM_POLL_TIMEOUT)) {
               rv = SOC_E_TIMEOUT;
               break;
           }
           READ_CMIC_CMC1_MIIM_STATr(0, miim_stat);    
    }
    CMIC_CMC1_MIIM_CTRLr_CLR(cmic_cmc1_miim_ctrl);
    WRITE_CMIC_CMC1_MIIM_CTRLr(0, cmic_cmc1_miim_ctrl);

    if (rv == SOC_E_NONE) {
        LOG_DEBUG(BSL_LS_SOC_MIIM,
                  (BSL_META_U(unit,
                                    "  Done in polls\n")));
    }  else if (rv == SOC_E_TIMEOUT) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "MiimTimeOut:__soc_miimc45_write, "
                              "timeout (id=0x%02x addr=0x%02x data=0x%04x)\n"),
                  phy_id, phy_reg_addr, phy_wr_data));
   }

    return rv;

}

/*
 * Function:
 *      soc_miimc45_write
 * Purpose:
 *      Write a value to a MIIM register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_devad - Device type (PMA/PMD, PCS, PHY XS)
 *      phy_reg_addr - PHY register to write
 *      phy_wr_data - Data to write.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Temporarily disables auto link scan if it was enabled.  The MIIM
 *      registers are locked during the operation to prevent multiple
 *      tasks from trying to access PHY registers simultaneously. This
 *      is a wrapper around __soc_miimc45_write that should be used for
 *      oridinary clause 45 writes that are disabled during warmboot.
 */

int
soc_miimc45_write(int unit, uint16 phy_id, uint8 phy_devad,
                  uint16 phy_reg_addr, uint16 phy_wr_data)
{

    return __soc_miimc45_write(unit, phy_id, phy_devad,
                  phy_reg_addr, phy_wr_data, 0);;
}

/*
 * Function:
 *      soc_miimc45_read
 * Purpose:
 *      Read a value from an MII register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_devad - Device type (PMA/PMD, PCS, PHY XS)
 *      phy_reg_addr - PHY register to write
 *      phy_rd_data - 16bit data to write into
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Temporarily disables auto link scan if it was enabled.  The MIIM
 *      registers are locked during the operation to prevent multiple
 *      tasks from trying to access PHY registers simultaneously.
 */

int
soc_miimc45_read(int unit, uint16 phy_id, uint8 phy_devad,
                 uint16 phy_reg_addr, uint16 *phy_rd_data)
{
    int  rv = SOC_E_NONE;    
    tick_t to;
    uint32          cycle_type;
    uint32 bus_sel = PCM_PHY_ID_BUS_NUM(phy_id);
    CMIC_CMC1_MIIM_PARAMr_t cmic_cmc1_miim_param;
    CMIC_CMC1_MIIM_ADDRESSr_t  cmic_cmc1_miim_address;
    CMIC_CMC1_MIIM_CTRLr_t cmic_cmc1_miim_ctrl;
    CMIC_CMC1_MIIM_STATr_t cmic_cmc1_miim_stat;    
    CMIC_CMC1_MIIM_READ_DATAr_t cmic_cmc1_miim_read_data;

    LOG_VERBOSE(BSL_LS_SOC_MIIM,
               (BSL_META_U(unit,
                          "soc_miimc45_read: id=0x%02x "
                           "phy_devad=0x%02x addr=0x%02x\n"),
                phy_id, phy_devad, phy_reg_addr));
       
    /* Write parameter registers and tell CMIC to start */
    CMIC_CMC1_MIIM_PARAMr_CLR(cmic_cmc1_miim_param);
    if (phy_devad & MIIM_CYCLE_C45_RD) {
        cycle_type = MIIM_CYCLE_C45_REG_RD;
    } else if (phy_devad & MIIM_CYCLE_C45_RD_ADINC) {
        cycle_type = MIIM_CYCLE_C45_REG_RD_ADINC;
    } else {
        cycle_type = MIIM_CYCLE_AUTO;
    }
    phy_devad &= ~MIIM_CYCLE_C45_MASK;

    CMIC_CMC1_MIIM_PARAMr_MIIM_CYCLEf_SET(cmic_cmc1_miim_param, cycle_type);
    if (phy_id & 0x80) {
        phy_id &= (~0x80);
        CMIC_CMC1_MIIM_PARAMr_INTERNAL_SELf_SET(cmic_cmc1_miim_param, 1);
    }


    CMIC_CMC1_MIIM_PARAMr_C45_SELf_SET(cmic_cmc1_miim_param, 1);
    CMIC_CMC1_MIIM_PARAMr_BUS_IDf_SET(cmic_cmc1_miim_param, bus_sel);
    phy_id &= ~(PCM_PHY_ID_BUS_UPPER_MASK | PCM_PHY_ID_BUS_LOWER_MASK);
    CMIC_CMC1_MIIM_PARAMr_PHY_IDf_SET(cmic_cmc1_miim_param, phy_id);

    CMIC_CMC1_MIIM_ADDRESSr_CLR(cmic_cmc1_miim_address);
    CMIC_CMC1_MIIM_ADDRESSr_CLAUSE_45_DTYPEf_SET(cmic_cmc1_miim_address, phy_devad);
    CMIC_CMC1_MIIM_ADDRESSr_CLAUSE_45_REGADRf_SET(cmic_cmc1_miim_address, phy_reg_addr);

    CMIC_CMC1_MIIM_CTRLr_CLR(cmic_cmc1_miim_ctrl);
    CMIC_CMC1_MIIM_CTRLr_MIIM_RD_STARTf_SET(cmic_cmc1_miim_ctrl, 1);
   
    WRITE_CMIC_CMC1_MIIM_ADDRESSr(unit, cmic_cmc1_miim_address);
    WRITE_CMIC_CMC1_MIIM_PARAMr(unit, cmic_cmc1_miim_param);
    WRITE_CMIC_CMC1_MIIM_CTRLr(unit, cmic_cmc1_miim_ctrl);
      
    to = sal_get_ticks();
        
    READ_CMIC_CMC1_MIIM_STATr(0, cmic_cmc1_miim_stat);
        
    while (CMIC_CMC1_MIIM_STATr_MIIM_OPN_DONEf_GET(cmic_cmc1_miim_stat) == 0) {
           if (SAL_TIME_EXPIRED(to, MIIM_POLL_TIMEOUT)) {
                        rv = SOC_E_TIMEOUT;
                        break;
           }
           READ_CMIC_CMC1_MIIM_STATr(0, cmic_cmc1_miim_stat);    
    }

    CMIC_CMC1_MIIM_CTRLr_CLR(cmic_cmc1_miim_ctrl);
    WRITE_CMIC_CMC1_MIIM_CTRLr(0, cmic_cmc1_miim_ctrl);
    
    if (rv == SOC_E_NONE) {
        READ_CMIC_CMC1_MIIM_READ_DATAr(unit, cmic_cmc1_miim_read_data);
        *phy_rd_data = CMIC_CMC1_MIIM_READ_DATAr_DATAf_GET(cmic_cmc1_miim_read_data);


        LOG_VERBOSE(BSL_LS_SOC_MIIM,
                 (BSL_META_U(unit,
                             "soc_miimc45_read: read data=0x%04x\n"), *phy_rd_data));

    }  else if (rv == SOC_E_TIMEOUT) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "MiimTimeOut:soc_miimc45_read, "
                              "timeout (id=0x%02x addr=0x%02x)\n"),
                   phy_id, phy_reg_addr));

    }
    
    return rv;
}


/*
 * Function:
 *      soc_esw_miimc45_write
 * Purpose:
 *      Write a value from an MII register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write (Encoded with PMA/PMD, PCS, PHY XS)
 *      phy_rd_data - 16bit data to write into
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */
int
soc_esw_miimc45_write(int unit, uint32 phy_id,
                     uint32 phy_reg_addr, uint16 phy_wr_data)
{
    uint8  dev_addr;
    uint16 reg_addr;

    dev_addr = SOC_PHY_CLAUSE45_DEVAD(phy_reg_addr);
    reg_addr = SOC_PHY_CLAUSE45_REGAD(phy_reg_addr);

    return soc_miimc45_write(unit, (uint16)phy_id, dev_addr,
                            reg_addr, phy_wr_data);

}

/*
 * Function:
 *      soc_esw_miimc45_write
 * Purpose:
 *      Write a value from an MII register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write (Encoded with PMA/PMD, PCS, PHY XS)
 *      phy_rd_data - 16bit data to write into
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */
int
soc_esw_miimc45_read(int unit, uint32 phy_id,
                    uint32 phy_reg_addr, uint16 *phy_rd_data)
{
    uint8  dev_addr;
    uint16 reg_addr;

    dev_addr   = SOC_PHY_CLAUSE45_DEVAD(phy_reg_addr);
    reg_addr   = SOC_PHY_CLAUSE45_REGAD(phy_reg_addr);
 
    return soc_miimc45_read(unit, (uint16)phy_id, dev_addr,
                          reg_addr, phy_rd_data);;
}

/*
 * Function:
 *      soc_esw_miimc45_data_write
 * Purpose:
 *      Write a value to the present MII register without a address cycle.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_devad - Device type (PMA/PMD, PCS, PHY XS)
 *      phy_wr_data - 16 bit data for write
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */
int
soc_esw_miimc45_data_write(int unit, uint32 phy_id,
                           uint8 phy_devad, uint16 phy_wr_data)
{
    uint8  dev_addr;
    uint16 reg_addr;

    dev_addr = (phy_devad & ~MIIM_CYCLE_C45_MASK) | MIIM_CYCLE_C45_WR;
    reg_addr = 0;

    return soc_miimc45_write(unit, (uint16)phy_id, dev_addr,
                             reg_addr, phy_wr_data);

}

/*
 * Function:
 *      soc_esw_miimc45_data_read
 * Purpose:
 *      Read a value from the present MII register without a address cycle.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_devad - Device type (PMA/PMD, PCS, PHY XS)
 *      phy_rd_data - point to 16 bit data buffer
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */
int
soc_esw_miimc45_data_read(int unit, uint32 phy_id,
                          uint8 phy_devad, uint16 *phy_rd_data)
{
    uint8  dev_addr;
    uint16 reg_addr;

    dev_addr = (phy_devad & ~MIIM_CYCLE_C45_MASK) | MIIM_CYCLE_C45_RD;
    reg_addr = 0;

    return soc_miimc45_read(unit, (uint16)phy_id, dev_addr,
                            reg_addr, phy_rd_data);

}

/*
 * Function:
 *      soc_esw_miimc45_addr_write
 * Purpose:
 *      Write the address register with a new address to access.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_devad - Device type (PMA/PMD, PCS, PHY XS)
 *      phy_rd_data - 16 bit register address
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */
int
soc_esw_miimc45_addr_write(int unit, uint32 phy_id,
                           uint8 phy_devad, uint16 phy_ad_data)
{
    uint8  dev_addr;
    uint16 reg_addr;

    dev_addr = (phy_devad & ~MIIM_CYCLE_C45_MASK) | MIIM_CYCLE_C45_WR_AD;
    reg_addr = phy_ad_data;

    return soc_miimc45_write(unit, (uint16)phy_id, dev_addr,
                             reg_addr, 0);

}

/*
 * Function:
 *      soc_esw_miimc45_post_read
 * Purpose:
 *      Read a value from the present MII register and increase register addr.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_devad - Device type (PMA/PMD, PCS, PHY XS)
 *      phy_rd_data - point to 16 bit data buffer
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */
int
soc_esw_miimc45_post_read(int unit, uint32 phy_id,
                          uint8 phy_devad, uint16 *phy_rd_data)
{
    uint8  dev_addr;
    uint16 reg_addr;

    dev_addr = (phy_devad & ~MIIM_CYCLE_C45_MASK) | MIIM_CYCLE_C45_RD_ADINC;
    reg_addr = 0;

    return soc_miimc45_read(unit, (uint16)phy_id, dev_addr,
                            reg_addr, phy_rd_data);

}

/*
 * Function:
 *      soc_miimc45_modify
 * Purpose:
 *      Modify a value from an MII register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_devad - Device type (PMA/PMD, PCS, PHY XS)
 *      phy_reg_addr - PHY register to write
 *      phy_rd_data - 16bit data to write into
 *      phy_rd_mask - 16bit mask to indicate the bits to modify.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */

int
soc_miimc45_modify(int unit, uint16 phy_id, uint8 phy_devad,
                 uint16 phy_reg_addr, uint16 phy_rd_data, uint16 phy_rd_mask)
{
    uint16  tmp = 0, otmp;

    phy_rd_data = phy_rd_data & phy_rd_mask;

    SOC_IF_ERROR_RETURN
        (soc_miimc45_read(unit, phy_id, phy_devad, phy_reg_addr, &tmp));
    otmp = tmp;
    tmp &= ~(phy_rd_mask);
    tmp |= phy_rd_data;

    if (otmp != tmp) {
        SOC_IF_ERROR_RETURN
            (soc_miimc45_write(unit, phy_id, phy_devad, phy_reg_addr, tmp));
    }
    return SOC_E_NONE;

}






/*
 * Function:
 *      soc_esw_miimc45_wb_write
 * Purpose:
 *      Write a value from an MII register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write (Encoded with PMA/PMD, PCS, PHY XS)
 *      phy_wr_data - 16bit data to write into
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      This is a wrapper around __soc_miimc45_write that should be used for
 *      clause 45 writes that should succeed during warmboot.
 */
int
soc_esw_miimc45_wb_write(int unit, uint32 phy_id,
                     uint32 phy_reg_addr, uint16 phy_wr_data)
{
    uint8  dev_addr;
    uint16 reg_addr;

    dev_addr = SOC_PHY_CLAUSE45_DEVAD(phy_reg_addr);
    reg_addr = SOC_PHY_CLAUSE45_REGAD(phy_reg_addr);

    return __soc_miimc45_write(unit, (uint16)phy_id, dev_addr,
                             reg_addr, phy_wr_data, MIIM_WB_C45);

}


/*
 * Function:
 *      soc_sbus_mdio_write
 * Purpose:
 *      Write to an MII register via CMIC SBUS (S-channel) interface.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_wr_data - 32-bit data to write
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      This function is simply a wrapper for a device-specific implmentation.
 */

int
soc_sbus_mdio_write(int unit, uint16 phy_id,
                    uint32 phy_reg_addr, uint32 phy_wr_data)
{
    return soc_greyhound2_tscx_reg_write(unit, phy_id, phy_reg_addr, phy_wr_data);
}
int
soc_greyhound2_sbus_tsc_block(int unit, int pport, int *blk) {

        if (pport < PHY_GPORT3_BASE) return SOC_E_PORT;

        return SYS_OK;
}

/*
 * Function:
 *      soc_sbus_mdio_read
 * Purpose:
 *      Read from an MII register via CMIC SBUS (S-channel) interface.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_rd_data - 32-bit data to read into
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      This function is simply a wrapper for a device-specific implmentation.
 */

int
soc_sbus_mdio_read(int unit, uint16 phy_id,
                   uint32 phy_reg_addr, uint32 *phy_rd_data)
{
    return soc_greyhound2_tscx_reg_read(unit, phy_id, phy_reg_addr, phy_rd_data);
}

int
soc_mac_probe(int unit, uint8 port, mac_driver_t **macdp)
{
      if(IS_CL_PORT(port)) {
          *macdp = &soc_mac_cl;  
      } else if (IS_XL_PORT(port)) {
          *macdp = &soc_mac_xl;  
      } else {
          *macdp = &soc_mac_uni; 
      }
      return SYS_OK;
}


int soc_counter_port_pbmp_add(int unit, int port) {
    return SYS_OK;
}




soc_functions_t soc_greyhound2_drv_funs = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    bcm95357x_firmware_set,
    soc_greyhound2_tscx_reg_read,
    soc_greyhound2_tscx_reg_write, 
    NULL

};

soc_functions_t *
soc_chip_drv_funs_find(uint16 dev_id, uint8 rev_id) {	
      return &soc_greyhound2_drv_funs;
}

int
soc_link_mask2_set(int unit, pbmp_t mask)
{
  return 0;
}

int
soc_link_mask2_get(int unit, pbmp_t *mask)
{
  return 0;
}


int
_bcm_esw_link_force(int unit, uint32 flags, int port,
                    int force, int link)
{
     return 0;
}


void
soc_phy_addr_default(int unit, int lport,
                          uint16 *phy_addr, uint16 *phy_addr_int)
{
   int phy_port = SOC_PORT_L2P_MAPPING(lport);
   *phy_addr = 0xFF;
   *phy_addr_int = _soc_gh2_int_phy_addr[phy_port];

}



