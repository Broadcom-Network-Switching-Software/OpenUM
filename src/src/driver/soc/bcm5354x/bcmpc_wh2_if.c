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
#include <phymod/phymod_acc.h>
#include <soc/property.h>

extern int (*_phy_tsce_firmware_set_helper[1])(int, int, uint8 *, int);
extern int (*_phy_qtce_firmware_set_helper[1])(int, int, uint8 *, int);
extern int bcm95354x_firmware_set(int unit, int port, uint8 *data, int size);
extern int soc_wolfhound2_info_config(int unit, soc_control_t *soc);

int
pcm_phyctrl_software_init(int unit) {

	 soc_wolfhound2_info_config(unit, &SOC_CONTROL(unit));

     soc_phyctrl_software_init(unit);
	 
	 _phy_tsce_firmware_set_helper[unit] = bcm95354x_firmware_set;
	 _phy_qtce_firmware_set_helper[unit] = bcm95354x_firmware_set;
    
     return SYS_OK;
}

int
pcm_phyctrl_wolfhound2_port_probe_init(int unit, pbmp_t lpbmp, pbmp_t *okay_lpbmp) {

#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED 
    sys_error_t sal_config_rv = SYS_OK, cl73_rv = SYS_OK, cl37_rv = SYS_OK;
    pbmp_t phy_cl73_pbmp, phy_cl37_pbmp;
    pbmp_t speed_1000_pbmp, speed_2500_pbmp, speed_10000_pbmp;  
    char buffer[128];
    char *buf = buffer;
#endif
    pbmp_t serdes_fiber_prefer_pbmp;
    int rv = 0;
    uint8 lport;

#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED 

#if (CONFIG_EMULATION==1)
    sal_config_set("phy_null", "1");
#endif
   
   
    sal_config_rv = sal_config_pbmp_get(SAL_CONFIG_SPEED_1000_PORTS, &speed_1000_pbmp);
    if (sal_config_rv == SYS_OK) {
        sal_printf("Vendor Config : Set speed (1G) logical pbmp with value 0x%s.\n", SOC_PBMP_FORMAT(speed_1000_pbmp));
        SOC_LPORT_ITER(lport) {
          if (PBMP_MEMBER(speed_1000_pbmp, lport) && (SOC_PORT_SPEED_MAX(lport) >= 1000)){
              SOC_PORT_SPEED_INIT(lport) = 1000;
              sal_sprintf(buf, "port_init_speed_%d", lport);
              sal_config_set(buf, "1000");
          }
        }
    }         
    
    
    sal_config_rv = sal_config_pbmp_get(SAL_CONFIG_SPEED_2500_PORTS, &speed_2500_pbmp);
    if (sal_config_rv == SYS_OK) {
        sal_printf("Vendor Config : Set speed (2.5G) logical pbmp with value 0x%s.\n", SOC_PBMP_FORMAT(speed_2500_pbmp));
        SOC_LPORT_ITER(lport) {
            if (PBMP_MEMBER(speed_2500_pbmp, lport) && (SOC_PORT_SPEED_MAX(lport) >= 2500)){
                SOC_PORT_SPEED_INIT(lport) = 2500;                
                sal_sprintf(buf, "port_init_speed_%d", lport);
                sal_config_set(buf, "2500");
            }
        }
    }     
    
    
    sal_config_rv = sal_config_pbmp_get(SAL_CONFIG_SPEED_10000_PORTS, &speed_10000_pbmp);
    if (sal_config_rv == SYS_OK) {
        sal_printf("Vendor Config : Set speed (10G) logical pbmp with value 0x%s.\n", SOC_PBMP_FORMAT(speed_10000_pbmp));
        SOC_LPORT_ITER(lport) {
            if (PBMP_MEMBER(speed_10000_pbmp, lport) && (SOC_PORT_SPEED_MAX(lport) >= 10000)){
                sal_sprintf(buf, "port_init_speed_%d", lport);
                sal_config_set(buf, "10000");
                SOC_PORT_SPEED_INIT(lport) = 10000;
            }
        }
    }     
    
     
    cl73_rv = sal_config_pbmp_get(SAL_CONFIG_PHY_CL73_PORTS, &phy_cl73_pbmp);
    if (cl73_rv == SYS_OK) {
        sal_printf("Vendor Config : Set CL73 logical pbmp with value 0x%s.\n", SOC_PBMP_FORMAT(phy_cl73_pbmp));
        sal_config_set("phy_an_c73_logical_ports", sal_config_get(SAL_CONFIG_PHY_CL73_PORTS));
    }

    
    cl37_rv = sal_config_pbmp_get(SAL_CONFIG_PHY_CL37_PORTS, &phy_cl37_pbmp);
    if (cl37_rv == SYS_OK) {
         sal_printf("Vendor Config : Set CL37 logical pbmp with value 0x%s.\n", SOC_PBMP_FORMAT(phy_cl37_pbmp));
         sal_config_set("phy_an_c37_logical_ports", sal_config_get(SAL_CONFIG_PHY_CL37_PORTS));
    }
#endif /*  CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */

    
    PBMP_CLEAR(serdes_fiber_prefer_pbmp);
    PBMP_ITER(lpbmp, lport) {
         if (IS_GX_PORT(lport)) {
             if (qtc_interface == QTC_INTERFACE_FIBER) {
                 PBMP_PORT_ADD(serdes_fiber_prefer_pbmp, lport);
             } 
         } else if (IS_XL_PORT(lport)) {
             if (tsce_interface == TSCE_INTERFACE_FIBER) {                 
                 PBMP_PORT_ADD(serdes_fiber_prefer_pbmp, lport);
             } 
         }          
    }
 
    
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED 
    sal_config_pbmp_set("serdes_fiber_pref_logical_ports", serdes_fiber_prefer_pbmp);
#endif
       
    rv = pcm_phyctrl_port_probe_init(unit, lpbmp, okay_lpbmp);
    
    
    return rv;
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
    .port_update = pcm_phyctrl_port_update,
    .port_probe_init = pcm_phyctrl_wolfhound2_port_probe_init,
    .software_init = pcm_phyctrl_software_init,
    .port_notify = pcm_phyctrl_phy_notify,


};


