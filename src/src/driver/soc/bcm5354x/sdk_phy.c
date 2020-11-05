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


#define MIIM_PARAM_ID_OFFSET            16
#define MIIM_PARAM_REG_ADDR_OFFSET      24      /* Ignored on Lynx; see */
                                                /* CMIC_MIIM_ADDRESS */

#define MIIM_POLL_TIMEOUT               500
//#define LOCAL_PRINTF(fmt, args...)       sal_printf(fmt, args);
#define LOCAL_PRINTF(fmt, args...)       

static const uint16 _soc_wh2_phy_addr[] = {
    0x00, /* Port  0 (cmic)         N/A */
    0x00, /* Port  1 (N/A)              */
    0x81, /* Port  2            IntBus=0 Addr=0x1 */
    0x82, /* Port  3            IntBus=0 Addr=0x2 */
    0x83, /* Port  4            IntBus=0 Addr=0x3 */
    0x84, /* Port  5            IntBus=0 Addr=0x4 */
    0x85, /* Port  6            IntBus=0 Addr=0x5 */
    0x86, /* Port  7            IntBus=0 Addr=0x6 */
    0x87, /* Port  8            IntBus=0 Addr=0x7 */
    0x88, /* Port  9            IntBus=0 Addr=0x8 */
    0x89, /* Port 10            IntBus=0 Addr=0x9 */
    0x8a, /* Port 11            IntBus=0 Addr=0xa */
    0x8b, /* Port 12            IntBus=0 Addr=0xb */
    0x8c, /* Port 13            IntBus=0 Addr=0xc */
    0x00, /* Port 14 */
    0x00, /* Port 15 */
    0x00, /* Port 16 */
    0x00, /* Port 17 */
    0x8d, /* Port 18            IntBus=0 Addr=0xd  */
    0x8e, /* Port 19            IntBus=0 Addr=0xe  */
    0x8f, /* Port 20            IntBus=0 Addr=0xf  */
    0x90, /* Port 21            IntBus=0 Addr=0x10 */
    0x91, /* Port 22            IntBus=0 Addr=0x11 */
    0x92, /* Port 23            IntBus=0 Addr=0x12 */
    0x93, /* Port 24            IntBus=0 Addr=0x13 */
    0x94, /* Port 25            IntBus=0 Addr=0x14 */
    0x95, /* Port 26            IntBus=0 Addr=0x15 */
    0x96, /* Port 27            IntBus=0 Addr=0x16 */
    0x97, /* Port 28            IntBus=0 Addr=0x17 */
    0x98, /* Port 29            IntBus=0 Addr=0x18 */
    0x00, /* Port 30 */
    0x00, /* Port 31 */
    0x00, /* Port 32 */
    0x00, /* Port 33 */
    0x00, /* Port 34 */
    0x00, /* Port 35 */
    0x00, /* Port 36 */
    0x00  /* Port 37 */
};

static const uint16 _soc_wh2_int_phy_addr[] = {
    0x00, /* Port  0 (cmic)         N/A */
    0x00, /* Port  1 (N/A)              */
    0x00, /* Port  2 */
    0x00, /* Port  3 */
    0x00, /* Port  4 */
    0x00, /* Port  5 */
    0x00, /* Port  6 */
    0x00, /* Port  7 */
    0x00, /* Port  8 */
    0x00, /* Port  9 */
    0x00, /* Port 10 */
    0x00, /* Port 11 */
    0x00, /* Port 12 */
    0x00, /* Port 13 */
    0x00, /* Port 14 */
    0x00, /* Port 15 */
    0x00, /* Port 16 */
    0x00, /* Port 17 */
    0x00, /* Port 18 */
    0x00, /* Port 19 */
    0x00, /* Port 20 */
    0x00, /* Port 21 */
    0x00, /* Port 22 */
    0x00, /* Port 23 */
    0x00, /* Port 24 */
    0x00, /* Port 25 */
    0xb4, /* Port 26            IntBus=1 Addr=0x14 */
    0xb4, /* Port 27            IntBus=1 Addr=0x15 */
    0xb4, /* Port 28            IntBus=1 Addr=0x16 */
    0xb4, /* Port 29            IntBus=1 Addr=0x17 */
    0x00, /* Port 30 */
    0x00, /* Port 31 */
    0x00, /* Port 32 */
    0x00, /* Port 33 */
    0xb0, /* Port 34            IntBus=1 Addr=0x10 */
    0xb0, /* Port 35            IntBus=1 Addr=0x11 */
    0xb0, /* Port 36            IntBus=1 Addr=0x12 */
    0xb0  /* Port 37            IntBus=1 Addr=0x13 */
};

    
#define FW_ALIGN_BYTES                  16
#define FW_ALIGN_MASK                   (FW_ALIGN_BYTES - 1)

int
bcm95354x_firmware_set(int unit, int port, uint8 *data, int size) {

	  int ioerr = 0;
	  uint32 val;
	  uint32 wbuf[4];
	  uint32 *fw_data;
	  uint32 *fw_entry;
	  uint32 fw_size;
	  uint32 idx, wdx;
	  uint32 offset = 0; // MDIO clock ?
	  CMIC_RATE_ADJUST_EXT_MDIOr_t cmic_rate_adjust_ext_mdio;
	  GPORT_WC_UCMEM_CTRLr_t gport_wc_ucmem_ctrl;
	  GPORT_WC_UCMEM_DATAm_t gport_wc_ucmem_data;
      
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

	  if (size == 0) {
		  return SYS_ERR_PARAMETER;
	  }
	  
	  /* Aligned firmware size */
	  fw_size = (size + FW_ALIGN_MASK) & ~FW_ALIGN_MASK;
	  
      if (IS_GX_PORT(port)) {
    	  /* Enable parallel bus access, ACCESS_MODE [Bit 0] = 0x1 */
    	  GPORT_WC_UCMEM_CTRLr_CLR(gport_wc_ucmem_ctrl);
    	  GPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(gport_wc_ucmem_ctrl, 1);
    	  WRITE_GPORT_WC_UCMEM_CTRLr(unit, SOC_PORT_P2L_MAPPING(PHY_GPORT2_BASE), gport_wc_ucmem_ctrl);
    	  
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
    		  WRITE_GPORT_WC_UCMEM_DATAm(unit, SOC_PORT_P2L_MAPPING(PHY_GPORT2_BASE), (idx >> 4), gport_wc_ucmem_data);
    	  }
    	  
    	  /* Disable parallel bus access */
    	  GPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(gport_wc_ucmem_ctrl, 0);
    	  WRITE_GPORT_WC_UCMEM_CTRLr(unit, SOC_PORT_P2L_MAPPING(PHY_GPORT2_BASE), gport_wc_ucmem_ctrl);
      }	  

    return SYS_OK;
}


static int mdio_addr_to_port(uint32 phy_addr){
	if ((phy_addr & 0xF) == 0xd) {
		return PHY_GPORT2_BASE;
	}
	return PHY_GPORT2_BASE;
}

#define TSC_REG_ADDR_TSCID_SET(_phy_reg, _phyad)    \
                            ((_phy_reg) |= ((_phyad) & 0x1f) << 19)


/*

   Sync with _soc_wolfhound2_tscx_reg_read in wolfhound2.c 

*/
int
soc_sbus_tsc_reg_read(int unit, uint32_t phy_addr, 
							 uint32_t phy_reg, uint32_t *phy_data)
{
	int rv = SOC_E_NONE;
	uint32 xlport_wc_ucmem_data[4];
	uint8 block_id;
	uint32 addr;
	LOCAL_PRINTF("soc_sbus_tsc_reg_read(%d,%d,%d,0x%x,0x%08x,*phy_data)..\n", unit, 10, 14, phy_addr, phy_reg);
	
	MIIM_SCHAN_LOCK(unit);

	/* TSCE sbus access */
	xlport_wc_ucmem_data[0] = phy_reg & 0xffffffff;
	xlport_wc_ucmem_data[1] = 0x0;
	xlport_wc_ucmem_data[2] = 0x0;
	xlport_wc_ucmem_data[3] = 0x0;
	
	
	if (mdio_addr_to_port(phy_addr) == PHY_GPORT2_BASE) {
		addr = M_GPORT_WC_UCMEM_DATA(0);
		block_id = PMQ1_BLOCK_ID;
	
	}
	
	
	LOCAL_PRINTF("	ucmem_data_entry[95:64-63:32-31:0]=0x%08x-0x%08x-0x%08x\n", xlport_wc_ucmem_data[0], xlport_wc_ucmem_data[1], xlport_wc_ucmem_data[2]);
	
	rv = bcm5354x_mem_set(unit, block_id, addr, xlport_wc_ucmem_data, 4);
	
	if (SOC_SUCCESS(rv)) {
		rv = bcm5354x_mem_get(unit, block_id, addr, xlport_wc_ucmem_data, 4);			 
	}
	*phy_data = xlport_wc_ucmem_data[1];
	
	LOCAL_PRINTF("soc_sbus_tsc_reg_read: *phy_data=0x%04x,rv=%d\n", *phy_data, rv);
	
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
	
	LOCAL_PRINTF("soc_sbus_tsc_reg_write(%d,%d,%d,0x%x,0x%08x,0x%04x)..\n", unit, 10, 14, phy_addr, phy_reg, phy_data);    
	
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
	
	
	if (mdio_addr_to_port(phy_addr) == PHY_GPORT2_BASE) {
		addr = M_GPORT_WC_UCMEM_DATA(0);
		block_id = PMQ1_BLOCK_ID;
	
	}
	
	LOCAL_PRINTF("	ucmem_data_entry[95:64-63:32-31:0]=0x%08x-0x%08x-0x%08x\n", xlport_wc_ucmem_data[0], xlport_wc_ucmem_data[1], xlport_wc_ucmem_data[2]);
	
	rv = bcm5354x_mem_set(unit, block_id, addr, xlport_wc_ucmem_data, 4);
	
	LOCAL_PRINTF("soc_sbus_tsc_reg_write : rv=%d\n",rv);
	
	MIIM_SCHAN_UNLOCK(unit);
	return rv;
	
}


STATIC int
soc_wolfhound2_tscx_reg_read(int unit, uint32 phy_addr,
                            uint32 phy_reg, uint32_t *phy_data)
{
    int rv = SYS_OK;
    LOCAL_PRINTF("soc_wolfhound2_tscx_reg_read[%d]: 0x%X/0x%X/%d/%d/%d\n", unit, phy_addr, phy_reg, 18, 10, 14);
    TSC_REG_ADDR_TSCID_SET(phy_reg, phy_addr); 
	rv = soc_sbus_tsc_reg_read(unit, phy_addr, phy_reg, phy_data);
    return rv;
}

/*

   Sync with _soc_wolfhound2_tscx_reg_read in wolfhound2.c 

*/


STATIC int
soc_wolfhound2_tscx_reg_write(int unit, uint32 phy_addr,
                             uint32 phy_reg, uint32_t phy_data)
{
    int rv = SYS_OK;

    LOCAL_PRINTF("soc_wolfhound2_tscx_reg_write[%d]: 0x%X/0x%X/%d/%d/%d\n", unit, phy_addr, phy_reg, 18, 10, 14);
    if (phy_reg == 0x3C010) {
		LOCAL_PRINTF("soc_wolfhound2_tscx_reg_write[%d]: 0x%X/0x%X/%d/%d/%d\n", unit, phy_addr, phy_reg, 18, 10, 14);
    }
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


    CMIC_CMC0_MIIM_STATr_t miim_stat;
    CMIC_CMC0_MIIM_PARAMr_t miim_param;
    CMIC_CMC0_MIIM_CTRLr_t miim_ctrl;
    CMIC_CMC0_MIIM_ADDRESSr_t miim_address;
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
        CMIC_CMC0_MIIM_ADDRESSr_CLR(miim_address);        
        CMIC_CMC0_MIIM_ADDRESSr_SET(miim_address, phy_reg_addr);        
        WRITE_CMIC_CMC0_MIIM_ADDRESSr(0, miim_address);
    } else {
        phy_param |= (uint32)phy_reg_addr << MIIM_PARAM_REG_ADDR_OFFSET;
    }
    CMIC_CMC0_MIIM_PARAMr_CLR(miim_param);
    CMIC_CMC0_MIIM_PARAMr_PHY_IDf_SET(miim_param, phy_id);
    CMIC_CMC0_MIIM_PARAMr_BUS_IDf_SET(miim_param, bus_sel);
    CMIC_CMC0_MIIM_PARAMr_SET(miim_param, phy_param);    
    WRITE_CMIC_CMC0_MIIM_PARAMr(0,miim_param);
	READ_CMIC_CMC0_MIIM_CTRLr(0, miim_ctrl);
    CMIC_CMC0_MIIM_CTRLr_CLR(miim_ctrl);
    CMIC_CMC0_MIIM_CTRLr_MIIM_WR_STARTf_SET(miim_ctrl,1);
    WRITE_CMIC_CMC0_MIIM_CTRLr(0,miim_ctrl);

    /* Wait for completion using either the interrupt or polling method */

	to = sal_get_ticks();

    READ_CMIC_CMC0_MIIM_STATr(0, miim_stat);

    while (CMIC_CMC0_MIIM_STATr_MIIM_OPN_DONEf_GET(miim_stat) == 0) {
           if (SAL_TIME_EXPIRED(to, MIIM_POLL_TIMEOUT)) {
               rv = SOC_E_TIMEOUT;
               break;
           }
           READ_CMIC_CMC0_MIIM_STATr(0, miim_stat);    
    }
    if (rv == SOC_E_NONE) {
            CMIC_CMC0_MIIM_CTRLr_CLR(miim_ctrl);
            WRITE_CMIC_CMC0_MIIM_CTRLr(0, miim_ctrl);

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
   return "WOLFHOUND2";
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
    bcm5354x_chip_revision(unit, &devid, &rev);
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
    CMIC_CMC0_MIIM_STATr_t miim_stat;
    CMIC_CMC0_MIIM_PARAMr_t miim_param;
    CMIC_CMC0_MIIM_CTRLr_t miim_ctrl;
    CMIC_CMC0_MIIM_READ_DATAr_t miim_read_data;
    CMIC_CMC0_MIIM_ADDRESSr_t miim_address;

    LOG_VERBOSE(BSL_LS_SOC_MIIM,
             (BSL_META_U(unit,
                         "soc_miim_read: id=0x%02x addr=0x%02x  "),
              phy_id, phy_reg_addr));
    
    //clause45 = soc_feature(unit, soc_feature_phy_cl45);
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
        CMIC_CMC0_MIIM_ADDRESSr_CLR(miim_address);        
        CMIC_CMC0_MIIM_ADDRESSr_SET(miim_address, phy_reg_addr);        
        WRITE_CMIC_CMC0_MIIM_ADDRESSr(0, miim_address);
    } else {
        phy_param |= (uint32)phy_reg_addr << MIIM_PARAM_REG_ADDR_OFFSET;
    }

    CMIC_CMC0_MIIM_PARAMr_CLR(miim_param);
    CMIC_CMC0_MIIM_PARAMr_SET(miim_param, phy_param);    
    WRITE_CMIC_CMC0_MIIM_PARAMr(0,miim_param);
    
    READ_CMIC_CMC0_MIIM_CTRLr(0, miim_ctrl);
    CMIC_CMC0_MIIM_CTRLr_CLR(miim_ctrl);
    CMIC_CMC0_MIIM_CTRLr_MIIM_RD_STARTf_SET(miim_ctrl,1);
    WRITE_CMIC_CMC0_MIIM_CTRLr(0, miim_ctrl);

    /* Wait for completion using either the interrupt or polling method */
    to = sal_get_ticks();
        
    READ_CMIC_CMC0_MIIM_STATr(0, miim_stat);
        
    while (CMIC_CMC0_MIIM_STATr_MIIM_OPN_DONEf_GET(miim_stat) == 0) {
           if (SAL_TIME_EXPIRED(to, MIIM_POLL_TIMEOUT)) {
                        rv = SOC_E_TIMEOUT;
                        break;
           }
           READ_CMIC_CMC0_MIIM_STATr(0, miim_stat);    
    }

    READ_CMIC_CMC0_MIIM_CTRLr(0, miim_ctrl);
	CMIC_CMC0_MIIM_CTRLr_MIIM_RD_STARTf_SET(miim_ctrl,0);
    WRITE_CMIC_CMC0_MIIM_CTRLr(0, miim_ctrl);

    if (rv == SOC_E_TIMEOUT) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "MiimTimeOut:soc_miim_read, "
                              "timeout (id=0x%02x addr=0x%02x)\n"),
                   phy_id, phy_reg_addr));
    } else {

        READ_CMIC_CMC0_MIIM_READ_DATAr(0, miim_read_data);
        *phy_rd_data = CMIC_CMC0_MIIM_READ_DATAr_DATAf_GET(miim_read_data);

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
    return SOC_E_NONE;
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
    return SOC_E_NONE;
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
    return SOC_E_NONE;
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
   return SOC_E_NONE;
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
	return SOC_E_NONE;
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
	return SOC_E_NONE;
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
    return SOC_E_NONE;
}



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
    int  rv = SOC_E_NONE;
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
                  phy_reg_addr, phy_wr_data, 0);
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
    return SOC_E_NONE;
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
    int rv = SOC_E_NONE;  

    return rv;

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
    return soc_wolfhound2_tscx_reg_write(unit, phy_id, phy_reg_addr, phy_wr_data);
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
    return soc_wolfhound2_tscx_reg_read(unit, phy_id, phy_reg_addr, phy_rd_data);
}

int
soc_mac_probe(int unit, uint8 port, mac_driver_t **macdp)
{
      if(IS_GX_PORT(port)) {
          *macdp = &soc_mac_uni; 
      }
      return SYS_OK;
}


int soc_counter_port_pbmp_add(int unit, int port) {
    return SYS_OK;
}



soc_functions_t soc_wolfhound2_drv_funs = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, 
    NULL

};

soc_functions_t *
soc_chip_drv_funs_find(uint16 dev_id, uint8 rev_id) {	
      return &soc_wolfhound2_drv_funs;
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
    uint8 is_gphy;
    
    if (phy_port >= 34) {
        /* SGMII_4P1 ports */
        *phy_addr_int = _soc_wh2_int_phy_addr[phy_port];
        *phy_addr = 0xff;
    } else {
        soc_wolfhound2_gphy_get(unit, lport, &is_gphy);
        if (!is_gphy) {
            *phy_addr_int = _soc_wh2_int_phy_addr[phy_port];
            *phy_addr = 0xff;
        } else {
            *phy_addr_int = 0;
            *phy_addr = _soc_wh2_phy_addr[phy_port];
        }
    }

}



