#include "system.h"
#include "xgsm_miim.h"
#include "soc/mdk_phy.h"
#include "soc/bmd_phy_ctrl.h"
#include "soc/bmd_phy.h"
#include <utils/net.h>
#include <pcm/pcm_int.h>

/*
 *    From $SDK/include/soc/phyctrl.h for SDK_MAC/PHY driver and upper layer of SDK_MAC (PCM)
 */

typedef enum soc_phy_event_e {
    phyEventInterface,
    phyEventDuplex,
    phyEventSpeed,
    phyEventStop,
    phyEventResume,
    phyEventAutoneg,
    phyEventTxFifoReset,
    phyEventMacLoopback,
    phyEventTxSquelch,
    phyEventLpiBypass,
    phyEventLinkWait,
    phyEventCount            /* Always last */
} soc_phy_event_t;


/* PHY_STOP flag to use with phyEventStop and phyEventResume */
#define PHY_STOP_MAC_DIS        (0x01)
#define PHY_STOP_PHY_DIS        (0x02)
#define PHY_STOP_DRAIN          (0x04)
#define PHY_STOP_DUPLEX_CHG     (0x08)
#define PHY_STOP_SPEED_CHG      (0x10)
#define PHY_STOP_COPPER         (0x20)

int
bmd_phyctrl_notify(phy_ctrl_t *pc, phy_event_t event, uint32 value)
{
    int rv = CDK_E_NONE;
    int lport;
#if 0
    
    if (auto_cfg[unit][port]) {
        return CDK_E_NONE;
    }
#endif

    PHY_CTRL_CHECK(pc);

    lport = SOC_PORT_P2L_MAPPING(pc->port);
    if (!PBMP_MEMBER(phy_external_mode, lport) || !(pc->next)) {
        return CDK_E_NONE;
    }

    switch (event) {
    case PhyEvent_Speed:
        rv = PHY_SPEED_SET(pc->next, value);
        break;
    case PhyEvent_Duplex:
        rv = PHY_DUPLEX_SET(pc->next, value);
        break;
    default:
        return CDK_E_NONE;
    }

    return rv;
}



int
soc_phyctrl_notify(int unit, int lport, int event, uint32 data)
{
    
    phy_ctrl_t *pc = BMD_PORT_PHY_CTRL(unit, lport);
    phy_event_t bmd_event;

    /* Mapping SDK event code into MDK event code */
    switch (event) {
       case phyEventDuplex:
            bmd_event = PhyEvent_Duplex;
       break;
       case phyEventSpeed:
            bmd_event = PhyEvent_Speed;
       break;
       default:
            /* sal_printf("%s :Unknow event %d\n", __FUNCTION__, event); */
            return CDK_E_NONE;

       break;
    }

    return bmd_phyctrl_notify(pc, bmd_event, data);
       
}


/*
 * Function:
 *      bmd_phy_addr_get
 * Description:
 *      Get PHY driver address
 * Parameters:
 *      unit - Device number
 *      lport - Logical Port number
 *      iaddr - Internal Serdes/PHY address
 *      addr -  External PHY address
 * Returns:
 *      SYS_OK
 */


int
bmdpc_phy_addr_get(int unit, int lport, uint32 *iaddr, uint32 *addr) {

	 int rv = SYS_OK;
	 phy_ctrl_t *pc = BMD_PORT_PHY_CTRL(unit, lport);
     uint32 int_addr, ext_addr;	 
     ext_addr = 0xFF;
     int_addr = 0xFF;

     if (pc == NULL) {
         return SYS_ERR_NOT_FOUND;
     }
     if ((PBMP_MEMBER(phy_external_mode, lport))) {
         int_addr = pc->bus->phy_addr(pc->port);         
         if (pc->next) {
             pc = pc->next;
			 ext_addr = pc->bus->phy_addr(pc->port);		  
         }
     } else {
         int_addr = pc->bus->phy_addr(pc->port);		          
     }

     if (ext_addr == 0xFF) { *addr = 0xFF; } else {
         *addr = PCM_PHY_ID_PHY_ADDR_SET(*addr, CDK_XGSM_MIIM_PHY_ID_GET(ext_addr)) | \
                 PCM_PHY_ID_INTERNAL_BUS_SET(*addr, CDK_XGSM_MIIM_IS_IBUS(ext_addr)) | \
		         PCM_PHY_ID_PHY_ADDR_SET(*addr, CDK_XGSM_MIIM_BUS_NUM_GET(ext_addr));
         
     };
     if (int_addr == 0) { *iaddr = 0xFF; } else {
          *iaddr = PCM_PHY_ID_PHY_ADDR_SET(*iaddr, CDK_XGSM_MIIM_PHY_ID_GET(int_addr)) | \
                  PCM_PHY_ID_INTERNAL_BUS_SET(*iaddr, CDK_XGSM_MIIM_IS_IBUS(int_addr)) | \
                  PCM_PHY_ID_PHY_ADDR_SET(*iaddr, CDK_XGSM_MIIM_BUS_NUM_GET(int_addr));
     };

	 return rv;   

}


const pcm_port_funtion_t mdk_port = {	  
    .software_init = bmdpc_software_init,
    .phy_driver_name_get = bmdpc_phy_driver_name_get,
    .phy_addr_get =  bmdpc_phy_addr_get,
    .phy_reg_set = bmdpc_phy_reg_set,
    .phy_reg_get = bmdpc_phy_reg_get,
    .port_probe_init = bmdpc_port_probe_init, 
    .port_loopback_set = bmdpc_port_loopback_set,
    .port_control_get = bmdpc_port_control_get,
    .port_control_set = bmdpc_port_control_set,
	.port_ability_advert_set = bmdpc_port_ability_advert_set,
	.port_ability_local_get = bmdpc_port_ability_local_get,
	.port_ability_remote_get = bmdpc_port_ability_remote_get,
	.port_autoneg_get = bmdpc_port_autoneg_get,
	.port_autoneg_set = bmdpc_port_autoneg_set,
	.port_pause_set = bmdpc_port_pause_set,
	.port_pause_get = bmdpc_port_pause_get,
    .port_speed_set = bmdpc_port_speed_set,
    .port_speed_get = bmdpc_port_speed_get,
    .port_duplex_set = bmdpc_port_duplex_set,
    .port_duplex_get = bmdpc_port_duplex_get,
    .port_enable_set = bmdpc_port_enable_set,
    .port_enable_get = bmdpc_port_enable_get, 
    .port_link_status_get = bmdpc_port_link_status_get, 
    .port_eee_enable_set = bmdpc_port_eee_enable_set,
	.port_update = bmdpc_port_update,
    .phy_cable_diag = bmdpc_port_cable_diag,
	.phy_cable_diag_support = bmdpc_port_cable_diag_support,
    .port_notify = soc_phyctrl_notify
};


