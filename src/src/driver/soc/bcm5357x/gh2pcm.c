#define __int8_t_defined
#define __uint32_t_defined
#include <system.h>
#undef _SOC_PHYCTRL_H_
#include <soc/phyctrl.h>
#undef SOC_IF_ERROR_RETURN
#include <soc/error.h>
#include <pcm/pcm_int.h>

/* from sdk/include/sal/compiler.h */
#define COMPILER_REFERENCE(_a)    ((void)(_a))
#define COMPILER_ATTRIBUTE(_a)    __attribute__ (_a)
#ifndef FUNCTION_NAME
#define FUNCTION_NAME() (__FUNCTION__)
#endif

#include <shared/bsl.h>
#include <shared/bslenum.h>
#include <shared/bsltypes.h>
#include <soc/phy/phyctrl.h>
#include <soc/phy/drv.h>
#include <soc/property.h>
#include <utils/system.h>

extern uint32 soc_property_port_get(int unit, uint8 port, 
                      const char *name, uint32 defl);

                  

extern uint8 tsce_interface[TSCE_NUM_OF_CORES][4];
extern uint8 tscf_interface[TSCF_NUM_OF_CORES][4];
extern uint8 qtc_interface[QTCE_NUM_OF_CORES][4];
extern uint8 sgmiipx4_interface[SGMIIP4_NUM_OF_CORES][4];


extern int (*_phy_tscf_firmware_set_helper[1])(int, int, uint8 *, int);
extern int (*_phy_tsce_firmware_set_helper[1])(int, int, uint8 *, int);
extern int (*_phy_qtce_firmware_set_helper[1])(int, int, uint8 *, int);

extern int bcm95357x_firmware_set(int unit, int port, uint8 *data, int size);
extern int soc_greyhound2_info_config(int unit, soc_control_t *soc);

int
pcm_phyctrl_software_init(int unit) {

   soc_greyhound2_info_config(unit, &SOC_CONTROL(unit));

   soc_phyctrl_software_init(unit);

#if !CONFIG_EMULATION
   _phy_tscf_firmware_set_helper[unit] = bcm95357x_firmware_set;   
   _phy_tsce_firmware_set_helper[unit] = bcm95357x_firmware_set;
   _phy_qtce_firmware_set_helper[unit] = bcm95357x_firmware_set;
#endif

   return SYS_OK;
}

#define WITHIN(x, min, max) (((x) >= (min)) && ((x) <= (max)))

int
pcm_phyctrl_greyhound2_port_probe_init(int unit, pbmp_t all_lpbmp, pbmp_t *okay_lpbmp) {

#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED 
    pbmp_t lpbmp;
    sys_error_t rv = SYS_OK;
#endif
    uint8 lport;

    uint8 mac[8];

    rv = pcm_phyctrl_port_probe_init(unit, BCM5357X_ALL_PORTS_MASK, okay_lpbmp);
   
    get_system_mac(mac);

    SOC_LPORT_ITER(lport) {
         pcm_phyctrl_port_pause_addr_set(unit,lport, mac);
         pcm_port_update(unit, lport, 0);
         gh2_sw_info.link[lport] = PORT_LINK_DOWN;
#if defined(CFG_SWITCH_EEE_INCLUDED)
         gh2_sw_info.need_process_for_eee_1s[lport] = FALSE;
#endif /*  CFG_SWITCH_EEE_INCLUDED */
     }


#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED    
        PBMP_CLEAR(lpbmp);
        if (sal_config_pbmp_get(SAL_CONFIG_PHY_AN_PORTS, &lpbmp) == SYS_OK) {
            sal_printf("Vendor Config : Set AN logical pbmp with value 0x%s.\n", SOC_PBMP_FORMAT(lpbmp));
			SOC_LPORT_ITER(lport) {
				if (PBMP_MEMBER(lpbmp, lport)) {
					pcm_port_autoneg_set(unit, lport, 1);					
					
				} else {
					pcm_port_autoneg_set(unit, lport, 0);
					pcm_port_speed_set(unit, lport, SOC_PORT_SPEED_INIT(lport));
					pcm_port_duplex_set(unit, lport, 1);
					pcm_port_pause_set(unit, lport, 1, 1);			  
				}
			}
        }

#endif      
     SOC_LPORT_ITER(lport) {
         pcm_port_enable_set(unit, lport, 1);           
     }

     return rv;

}

int 
bcm5357x_pcm_software_init(int unit) {

#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED 
    pbmp_t lpbmp;
    uint8 core_num, lane_num;
#endif
    uint8 lport;
    uint8 val8;  
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED    

#if CONFIG_EMULATION
    sal_config_set("phy_null", "1");
#endif
    PBMP_CLEAR(lpbmp);        
    if (sal_config_pbmp_get(SAL_CONFIG_PHY_CL73_PORTS, &lpbmp) == SYS_OK) {
        sal_printf("Vendor Config : Set CL73 logical pbmp with value 0x%s.\n", SOC_PBMP_FORMAT(lpbmp));
        sal_config_set("phy_an_c73_logical_ports", sal_config_get(SAL_CONFIG_PHY_CL73_PORTS));
    }

    PBMP_CLEAR(lpbmp);     
    if (sal_config_pbmp_get(SAL_CONFIG_PHY_CL37_PORTS, &lpbmp) == SYS_OK) {
         sal_printf("Vendor Config : Set CL37 logical pbmp with value 0x%s.\n", SOC_PBMP_FORMAT(lpbmp));
         sal_config_set("phy_an_c37_logical_ports", sal_config_get(SAL_CONFIG_PHY_CL37_PORTS));
    }

    /* UM Config WAR: Write a "serdes_fiber_pref" value into config */
    if (sal_config_uint8_get("serdes_fiber_pref", &val8) != SYS_OK) {

        PBMP_CLEAR(lpbmp);
        
        for (core_num = 0; core_num < TSCF_NUM_OF_CORES; core_num++) {                
             for (lane_num = 0; lane_num < 4; lane_num++) {
                  if (tscf_interface[core_num][lane_num] == TSCF_INTERFACE_FIBER) {
                      if (SOC_PORT_P2L_MAPPING(PHY_CLPORT0_BASE + lane_num + core_num * 4) >= 2) {
                          PBMP_PORT_ADD(lpbmp, SOC_PORT_P2L_MAPPING(PHY_CLPORT0_BASE + lane_num + core_num * 4));
                      }
                  }
             }
        }
        
        
        for (core_num = 0; core_num < TSCE_NUM_OF_CORES; core_num++) {                
             for (lane_num = 0; lane_num < 4; lane_num++) {
                  if (tsce_interface[core_num][lane_num] == TSCE_INTERFACE_FIBER) {
                      if (SOC_PORT_P2L_MAPPING(PHY_XLPORT0_BASE + lane_num + core_num * 4) >= 2) {
                          PBMP_PORT_ADD(lpbmp, SOC_PORT_P2L_MAPPING(PHY_XLPORT0_BASE + lane_num + core_num * 4));
                      }
                  }              
             }
        }
        
        for (core_num = 0; core_num < QTCE_NUM_OF_CORES; core_num++) {                
             for (lane_num = 0; lane_num < 4; lane_num++) {
                  if (qtc_interface[core_num][lane_num] == QTC_INTERFACE_FIBER) {
                      if (SOC_PORT_P2L_MAPPING(PHY_GPORT3_BASE + lane_num + core_num * 16) >= 2) {
                          PBMP_PORT_ADD(lpbmp, SOC_PORT_P2L_MAPPING(PHY_GPORT3_BASE + lane_num + core_num * 16));
                      }
                  }
             }
        }
        
        for (core_num = 0; core_num < SGMIIP4_NUM_OF_CORES; core_num++) {                
             for (lane_num = 0; lane_num < 4; lane_num++) {
                  if (sgmiipx4_interface[core_num][lane_num] == SGMIIPX4_INTERFACE_FIBER) {
                      if (SOC_PORT_P2L_MAPPING(PHY_GPORT0_BASE + lane_num + core_num * 4) >= 2) {
                          PBMP_PORT_ADD(lpbmp, SOC_PORT_P2L_MAPPING(PHY_GPORT0_BASE + lane_num + core_num * 4));
                      }
                  }
             }
        }

        sal_config_pbmp_set("serdes_fiber_pref_logical_ports", lpbmp);
    }
#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */    

    SOC_LPORT_ITER(lport) {
        pcm_port_ctrl[lport].f = &pcm_phyctrl;
    }

    return pcm_phyctrl.software_init(unit);       
}

const pcm_port_funtion_t pcm_phyctrl = {
    .phy_reg_get = pcm_phyctrl_phy_reg_get,
    .phy_reg_set = pcm_phyctrl_phy_reg_set,
    .phy_driver_name_get = pcm_phyctrl_phy_get_driver_name,
    .phy_addr_get = pcm_phyctrl_phy_addr_get,
    .phy_cable_diag = pcm_phyctrl_phy_cable_diag,
    .phy_cable_diag_support = pcm_phyctrl_phy_cable_diag_support,
    .port_eee_enable_set = pcm_phyctrl_port_eee_enable_set,
    .port_control_get = pcm_phyctrl_port_control_get,
    .port_control_set = pcm_phyctrl_port_control_set,
    .port_interface_set = pcm_phyctrl_port_interface_set,
    .port_interface_get = pcm_phyctrl_port_interface_get,
    .port_speed_set = pcm_phyctrl_port_speed_set,
    .port_speed_get = pcm_phyctrl_port_speed_get,
    .port_link_status_get = pcm_phyctrl_port_link_status_get,
    .port_ability_advert_set = pcm_phyctrl_port_ability_advert_set,
    .port_ability_local_get = pcm_phyctrl_port_ability_local_get,
    .port_ability_remote_get = pcm_phyctrl_port_ability_remote_get,
    .port_autoneg_set = pcm_phyctrl_port_autoneg_set,
    .port_autoneg_get = pcm_phyctrl_port_autoneg_get,
    .port_pause_set = pcm_phyctrl_port_pause_set,
    .port_pause_get = pcm_phyctrl_port_pause_get,
    .port_duplex_set = pcm_phyctrl_port_duplex_set,
    .port_duplex_get = pcm_phyctrl_port_duplex_get,
    .port_enable_set = pcm_phyctrl_port_enable_set,
    .port_enable_get = pcm_phyctrl_port_enable_get,
    .port_loopback_set = pcm_phyctrl_port_loopback_set,
    .port_loopback_get = pcm_phyctrl_port_loopback_get,
    .port_pause_addr_get = pcm_phyctrl_port_pause_addr_get,
    .port_pause_addr_set = pcm_phyctrl_port_pause_addr_set,
    .port_update = pcm_phyctrl_port_update,
    .port_probe_init = pcm_phyctrl_greyhound2_port_probe_init,
    .software_init = pcm_phyctrl_software_init,
    .port_notify = pcm_phyctrl_phy_notify,
};


