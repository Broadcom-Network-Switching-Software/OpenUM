/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifdef __C51__

#ifdef CODE_USERCLASS

#pragma userclass (code = uisw)

#endif /* CODE_USERCLASS */

#endif /* __C51__ */



#include "system.h"
#include "appl/cli.h"
#include "utils/ui.h"

void
APIFUNC(cli_cmd_phy_reg_get)(CLI_CMD_OP op) REENTRANT
{

    uint32 uport = 0, rw = 0;
    uint32 addr;
    uint8 unit, lport, max_index = 0, i;
    sys_error_t r;
    ui_ret_t rv;
    uint32_t data;
    const char *drv_name[2];     
    uint32 phy_addr[2];
    if (op == CLI_CMD_OP_HELP) {
           sal_printf("Command to get PHY register value.\n");
    } else if (op == CLI_CMD_OP_DESC) {
           sal_printf("Get value of PHY register");
    } else {
           if (ui_get_decimal(&uport, "User port: ") == UI_RET_OK) {

                if (SAL_UPORT_IS_NOT_VALID(uport)) {
                    sal_printf("User port range from %d to %d\n", SAL_NZUPORT_TO_UPORT(1), board_uport_count());
                    return;
                }
                   
                board_uport_to_lport(uport, &unit, &lport);

#ifdef _BCM95333X_ 
                if (!SOC_IS_DEERHOUND(unit) && lport < PHY_SECOND_QGPHY_PORT0) {
                    /* Get chip local port for BMD_PORT_PHY_CTRL for FH */
                    lport = SOC_PORT_P2L_MAPPING(lport);
                }
#endif /* _BCM95333X_ */

               
                /* internal phy name get */
                drv_name[1] = pcm_phy_driver_name_get(unit, lport, 1);
                /* external phy name get */
                drv_name[0] = pcm_phy_driver_name_get(unit, lport, 0);
        
                if (drv_name[1] == NULL && drv_name[0] == NULL) {
                    sal_printf("There is no valid phy driver\n");
                    return;
                }
        
                pcm_phy_addr_get(unit, lport, &phy_addr[1], &phy_addr[0]);
                
                 
                for (i=0, max_index=0; i<2; i++) {
                     if (drv_name[i] != NULL) {                      
                         sal_printf("%d: %s addr=0x%x on %s bus %d \n", max_index, drv_name[i], 
                                      PCM_PHY_ID_PHY_ADDR(phy_addr[i]), 
                                      PCM_PHY_ID_IS_INTERNAL_BUS(phy_addr[i]) ? "internal" : "external", 
                                      PCM_PHY_ID_BUS_NUM(phy_addr[i]));
                         max_index ++;
                     }
                }
                rv = ui_get_decimal(&rw, "Enter your choice: [0] ");

                if (rv == UI_RET_EMPTY || rv == UI_RET_OK) {
                   if (rv == UI_RET_EMPTY) {
                       rw = 0;
                   } else if (rw >= max_index) {
                       sal_printf("Invalid choice.\n");
                       return;
                   }
                }                

            } 

            if (ui_get_dword(&addr, "Reg address: ") == UI_RET_OK) {
                
                if (rw == 0 && drv_name[0] != NULL) {
                    r = pcm_phy_reg_get(unit, lport, 0, addr, &data);
                } else {
                    r = pcm_phy_reg_get(unit, lport, 1, addr, &data);
                }

                if (r != SYS_OK) {
                    sal_printf("Error!\n");
                    return;
                }
                sal_printf("PHY(Port %d) : Reg:0x%X  Value:0x%08X\n", uport, addr, data);
            }
    }
}

void
APIFUNC(cli_cmd_phy_reg_set)(CLI_CMD_OP op) REENTRANT
{

    uint32 uport, rw = 0, i;
    uint32 addr;
    uint8 unit, lport, max_index = 0;
    ui_ret_t rv;
    sys_error_t r;
    uint32_t data;
    const char *drv_name[2];     
    uint32 phy_addr[2];

    
    if (op == CLI_CMD_OP_HELP) {
        sal_printf("Command to set PHY register value.\n");
    } else if (op == CLI_CMD_OP_DESC) {
        sal_printf("Set value of PHY register");
    } else {

        if (ui_get_decimal(&uport, "User port: ") == UI_RET_OK) {
        
            if (SAL_UPORT_IS_NOT_VALID(uport)) {
                sal_printf("User port range from %d to %d\n", SAL_NZUPORT_TO_UPORT(1), board_uport_count());
                return;
            }
               
            board_uport_to_lport(uport, &unit, &lport);
        
            /* internal phy name get */
            drv_name[1] = pcm_phy_driver_name_get(unit, lport, 1);
            /* external phy name get */
            drv_name[0] = pcm_phy_driver_name_get(unit, lport, 0);
        
            if (drv_name[1] == NULL && drv_name[0] == NULL) {
                sal_printf("There is no valid phy driver\n");
                return;
            }
        
            pcm_phy_addr_get(unit, lport, &phy_addr[1], &phy_addr[0]);
        
         
            for (i=0, max_index=0; i<2; i++) {
                 if (drv_name[i] != NULL) {                      
                     sal_printf("%d: %s addr=0x%x on %s bus %d \n", max_index, drv_name[i], 
                                PCM_PHY_ID_PHY_ADDR(phy_addr[i]), 
                                PCM_PHY_ID_IS_INTERNAL_BUS(phy_addr[i]) ? "internal" : "external", 
                                PCM_PHY_ID_BUS_NUM(phy_addr[i]));
                     max_index ++;
                }
            }  
                
            rv = ui_get_decimal(&rw, "Enter your choice: [0] ");
        
            if (rv == UI_RET_EMPTY || rv == UI_RET_OK) {
                if (rv == UI_RET_EMPTY) {
                    rw = 0;
                } else if (rw >= max_index) {
                    sal_printf("Invalid choice.\n");
                    return;
                }
            }  
        } 

        if (ui_get_dword(&addr, "reg address: ") == UI_RET_OK &&
            ui_get_dword(&data, "data: ") == UI_RET_OK) {

            if (rw == 0 && drv_name[0] != NULL) {
                r = pcm_phy_reg_set(unit, lport, 0, addr, data);
            } else {
                r = pcm_phy_reg_set(unit, lport, 1, addr, data);
            }
           
            if (r != SYS_OK) {
                sal_printf("Error!\n");
                return;
            }
            sal_printf("PHY(Port %d) : Reg:0x%X  Value:0x%08X\n", uport, addr, data);
        }
    }
}

void 
ui_pcm_init(void) {

    /* Phy register get/set */
    cli_add_cmd('p', cli_cmd_phy_reg_get);
    cli_add_cmd('q', cli_cmd_phy_reg_set);

}
