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
#include "utils/system.h"
#ifdef _BCM95333X_    
#include "soc/bcm5333x.h"
#endif /* _BCM95333X_ */

#if defined(CFG_SWITCH_SNAKETEST_INCLUDED)
#include "appl/snaketest.h"
#endif

#ifdef CFG_XCOMMAND_INCLUDED
#include "appl/xcmd/xcmd_public.h"
#endif /* CFG_XCOMMAND_INCLUDED */

uint8 board_linkscan_disable = 0;

#if (CFG_CLI_ENABLED && CFG_CLI_SWITCH_CMD_ENABLED)

/* Forwards */
extern void APIFUNC(ui_switch_init)(void) REENTRANT;


#ifndef CFG_PCM_SUPPORT_INCLUDED
/* Please refer to mdk/phy/include/phy/phy_reg.h for access method */
#define LSHIFT32(_val, _cnt) ((uint32_t)(_val) << (_cnt))

/* Register layout */
#define PHY_REG_ACCESS_METHOD_SHIFT     28

/* Access methods */
#define PHY_REG_ACC_RAW                 LSHIFT32(0, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_BRCM_SHADOW         LSHIFT32(1, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_BRCM_1000X          LSHIFT32(2, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_XGS_IBLK            LSHIFT32(3, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_XAUI_IBLK           LSHIFT32(4, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_AER_IBLK            LSHIFT32(5, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_BRCM_XE             LSHIFT32(6, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_TSC_IBLK            LSHIFT32(7, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_BRCM_RDB            LSHIFT32(8, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_MASK                LSHIFT32(0xF, PHY_REG_ACCESS_METHOD_SHIFT)

extern int
phy_brcm_shadow_read(phy_ctrl_t *pc, uint32_t addr, uint32_t *data);
extern int
phy_brcm_shadow_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data);

extern int
phy_brcm_1000x_read(phy_ctrl_t *pc, uint32_t addr, uint32_t *data);
extern int
phy_brcm_1000x_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data);

extern int
phy_xgs_iblk_read(phy_ctrl_t *pc, uint32_t addr, uint32_t *data);
extern int
phy_xgs_iblk_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data);

extern int
phy_xaui_iblk_read(phy_ctrl_t *pc, uint32_t addr, uint32_t *data);
extern int
phy_xaui_iblk_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data);

extern int
phy_aer_iblk_read(phy_ctrl_t *pc, uint32_t addr, uint32_t *data);
extern int
phy_aer_iblk_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data);

extern int
phy_brcm_xe_read(phy_ctrl_t *pc, uint32_t addr, uint32_t *data);
extern int
phy_brcm_xe_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data);

extern int
phy_tsc_iblk_read(phy_ctrl_t *pc, uint32_t addr, uint32_t *data);
extern int
phy_tsc_iblk_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data);

extern int
phy_brcm_rdb_read(phy_ctrl_t *pc, uint32_t addr, uint32_t *data);
extern int
phy_brcm_rdb_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data);

APISTATIC void cli_cmd_phy_reg_get(CLI_CMD_OP op) REENTRANT;
APISTATIC void cli_cmd_phy_reg_set(CLI_CMD_OP op) REENTRANT;

#else /* CFG_PCM_SUPPORT_INCLUDED */

/*
 *  Move the PHY access commmand to ui_pcm when CFG_PCM_SUPPORT_INCLUDED 
 */ 
extern void cli_cmd_phy_reg_get(CLI_CMD_OP op) REENTRANT;
extern void cli_cmd_phy_reg_set(CLI_CMD_OP op) REENTRANT;

#endif /* CFG_PCM_SUPPORT_INCLUDED */

APISTATIC void cli_cmd_switch_reg_get(CLI_CMD_OP op) REENTRANT;
APISTATIC void cli_cmd_switch_reg_set(CLI_CMD_OP op) REENTRANT;
#if CFG_XGS_CHIP
APISTATIC void cli_cmd_switch_mem_get(CLI_CMD_OP op) REENTRANT;
APISTATIC void cli_cmd_switch_mem_set(CLI_CMD_OP op) REENTRANT;
#endif /* CFG_XGS_CHIP */

#ifdef BRD_VLAN_DEBUG
APISTATIC void cli_cmd_switch_vlan_info_dump(CLI_CMD_OP op) REENTRANT;
#endif /* BRD_VLAN_DEBUG */

#ifdef CFG_XCOMMAND_INCLUDED
APISTATIC void cli_cmd_switch_xcmd_shell(CLI_CMD_OP op) REENTRANT;
#ifdef CFG_XCOMMAND_BUILDER_CLI_INCLUDED
APISTATIC void cli_cmd_switch_xcmd_builder(CLI_CMD_OP op) REENTRANT;
#define XCMD_BUILDER_BUFFER_LEN (4 * 1024)
char xcmd_builder_buffer[XCMD_BUILDER_BUFFER_LEN];
#endif /* CFG_XCOMMAND_BUILDER_CLI_INCLUDED */
#endif /* CFG_XCOMMAND_INCLUDED */

#if defined(CFG_SWITCH_L2_ADDR_INCLUDED)
APISTATIC void cli_cmd_l2(CLI_CMD_OP op) REENTRANT;
#endif /* CFG_SWITCH_L2_ADDR_INCLUDED */

#ifdef CFG_XCOMMAND_INCLUDED
APISTATIC void
APIFUNC(cli_cmd_switch_xcmd_shell)(CLI_CMD_OP op) REENTRANT

{
    char user_name[16];
    char password[16];
    ui_ret_t r;

    if (op == CLI_CMD_OP_HELP) {
        sal_printf("Command to enter xcommand shell.\n");
    } else if (op == CLI_CMD_OP_DESC) {
        sal_printf("Command to enter xcommand shell");
    } else {
       if (ui_get_string(user_name,   15, "login:") == UI_RET_OK) {
           r = ui_get_secure_string(password, 15, "password:");
           if (r == UI_RET_EMPTY) {
               password[0] = 0;
               r = UI_RET_OK;
           }
           if (r == UI_RET_OK) {
               xcli_start_shell("um", user_name, password);
           }
       }
    }
}

#ifdef CFG_XCOMMAND_BUILDER_CLI_INCLUDED
APISTATIC void
APIFUNC(cli_cmd_switch_xcmd_builder)(CLI_CMD_OP op) REENTRANT

{
    if (op == CLI_CMD_OP_HELP) {
        sal_printf("Command to build xcommands to buffer.\n");
    } else if (op == CLI_CMD_OP_DESC) {
        sal_printf("Command to build xcommands to buffer");
    } else {
        XCMD_ERROR r;
        unsigned int len = XCMD_BUILDER_BUFFER_LEN;
        int i;

        r = xcli_build_commands_to_buffer(xcmd_builder_buffer, &len);
        if (r == XCMD_ERR_OK) {
            xcmd_builder_buffer[len] = 0;
            if (sal_strlen(xcmd_builder_buffer) >= (XCMD_BUILDER_BUFFER_LEN - 512)) {
                sal_printf("The usage of xcmd_builder_buffer is %d bytes and is close to %d bytes!"
                           "Need to expand the size of xcmd_builder_buffer\n", 
                           sal_strlen(xcmd_builder_buffer), XCMD_BUILDER_BUFFER_LEN);
            } else {
                sal_printf("OUTPUT:\n\n");
                for (i=0; i<sal_strlen(xcmd_builder_buffer); i++) {
                     sal_printf("%c", xcmd_builder_buffer[i]);
                }
            }
        } else {
            sal_printf("return: ERROR %d!\n", r);
        }
    }
}
#endif /* CFG_XCOMMAND_BUILDER_CLI_INCLUDED */
#endif /* CFG_XCOMMAND_INCLUDED */

#ifdef BRD_VLAN_DEBUG   /* included in CFG_SWITCH_VLAN_INCLUDED */
extern void _brdimpl_dump_vlan_info(void) REENTRANT;

extern sys_error_t
_bcm53128_mstp_enable_get(uint8 unit, uint8 *en);

extern sys_error_t
bcm53128_vlan_type_get(uint8 unit, soc_vlan_type_t *type);

APISTATIC void
APIFUNC(cli_cmd_switch_vlan_info_dump)(CLI_CMD_OP op) REENTRANT
{
    if (op == CLI_CMD_OP_HELP) {
        sal_printf("Command to dump SW VLAN database.\n");
    } else if (op == CLI_CMD_OP_DESC) {
        sal_printf("Dump SW VLAN database");
    } else {
        /* dump SW VLAN database */
        _brdimpl_dump_vlan_info();

/* === CHECKME : HW dump won't be CHECK-IN to CVS(debug usage only) ==== */
{
        uint16  pvid = 0, vid = 0;
        uint8   pbmp = 0, tag_pbmp = 0;
        soc_vlan_type_t qvlan_type;
        uint8 mstp_en = 0, p = 0;
        soc_switch_t    *soc;

        soc = board_get_soc_by_unit(0);
        /* dump HW VLAN configuration */
        bcm53128_vlan_type_get(0, &qvlan_type);
        _bcm53128_mstp_enable_get(0, &mstp_en);

        sal_printf("\nDUMP HW VLAN:QVLAN_EN=%d, MSTP_EN=%d\n\t PVID&FWD_PBMP:",
                (int)((qvlan_type==SOC_VT_DOT1Q_BASED) ? 1 : 0), (int)mstp_en);
        for (p=0;p<8;p++){
            board_untagged_vlan_get((uint16)p, &vid);
            soc->pvlan_egress_get(0, p, &pbmp);
            sal_printf("\n\t pport_%d: PVID=%d, FWD_PBMP=%02x",(int)p,vid,(int)pbmp);
        }
        sal_printf("\n");
}
    }
}
#endif /* BRD_VLAN_DEBUG */

#ifndef CFG_PCM_SUPPORT_INCLUDED
APISTATIC void
APIFUNC(cli_cmd_phy_reg_get)(CLI_CMD_OP op) REENTRANT
{
    uint32 uport = 0, rw = 0;
    uint32 addr;
    uint8 unit, lport, max_index = 0;
    sys_error_t r;
    ui_ret_t rv;
    phy_ctrl_t *pc = NULL, *pc_temp = NULL;
    uint32_t data;
    

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

                pc = BMD_PORT_PHY_CTRL(0, lport);

                if (pc == NULL) {
                    sal_printf("There is no valid phy driver\n");
                    return;
                }
            
                sal_printf(" 0: %s addr=0x%x on bus %d\n", pc->drv->drv_name, (pc->addr & 0x1f), (pc->addr & 0x40) ? 1 : 0);

                max_index ++;

                if (pc->next != NULL) {
                    sal_printf(" 1: %s addr=0x%x on bus %d\n", pc->next->drv->drv_name, (pc->next->addr & 0x1f), (pc->next->addr & 0x40) ? 1 : 0);
                    max_index ++;
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
                
                if (rw == 0) {
                    pc_temp = pc;
                } else {
                    pc_temp = pc->next;
                }

                if ((addr & PHY_REG_ACC_MASK) == PHY_REG_ACC_BRCM_SHADOW) {
                    r = phy_brcm_shadow_read(pc_temp, addr, &data);
                } else if ((addr & PHY_REG_ACC_MASK) == PHY_REG_ACC_BRCM_1000X) {
                    r = phy_brcm_1000x_read(pc_temp, addr, &data);
                } else if ((addr & PHY_REG_ACC_MASK) == PHY_REG_ACC_XGS_IBLK) {
                    r = phy_xgs_iblk_read(pc_temp, addr, &data);
                } else if ((addr & PHY_REG_ACC_MASK) == PHY_REG_ACC_XAUI_IBLK) {
                    r = phy_xaui_iblk_read(pc_temp, addr, &data);
                } else if ((addr & PHY_REG_ACC_MASK) == PHY_REG_ACC_AER_IBLK) {
                    r = phy_aer_iblk_read(pc_temp, addr, &data);
                } else if ((addr & PHY_REG_ACC_MASK) == PHY_REG_ACC_BRCM_XE) {
                    r = phy_brcm_xe_read(pc_temp, addr, &data);
                } else if ((addr & PHY_REG_ACC_MASK) == PHY_REG_ACC_TSC_IBLK) {
                    if (sal_strcmp(pc_temp->drv->drv_name, "bcmi_tsc_xgxs") == 0) {
                        r = phy_tsc_iblk_read(pc_temp, addr, &data);
                    } else {
                        r = PHY_CONFIG_GET(pc_temp, PhyConfig_RegisterAccess, &addr, (void *)&data);
                    }
                } else if ((addr & PHY_REG_ACC_MASK) == PHY_REG_ACC_BRCM_RDB) {
                    r = phy_brcm_rdb_read(pc_temp, addr, &data);
                } else {
                    r = PHY_BUS_READ(pc_temp, addr, &data);
                }

                if (r != SYS_OK) {
                    sal_printf("Error!\n");
                    return;
                }
                sal_printf("PHY(Port %d) : Reg:0x%X  Value:0x%08X\n", uport, addr, data);
            }
    }
}

APISTATIC void
APIFUNC(cli_cmd_phy_reg_set)(CLI_CMD_OP op) REENTRANT
{
    uint32 uport, rw = 0;
    uint32 addr;
    uint8 unit, lport, max_index = 0;
    sys_error_t r;
    ui_ret_t rv;
    uint32_t data;
    phy_ctrl_t *pc = NULL, *pc_temp = NULL;
    
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

#ifdef _BCM95333X_ 
            if (!SOC_IS_DEERHOUND(unit) && lport < PHY_SECOND_QGPHY_PORT0) {
                /* Get chip local port for BMD_PORT_PHY_CTRL for FH */
                lport = SOC_PORT_P2L_MAPPING(lport);
            }
#endif /* _BCM95333X_ */

            pc = BMD_PORT_PHY_CTRL(0, lport);
        
            if (pc == NULL) {
                sal_printf("There is no valid phy driver\n");
                return;
            }
        
            sal_printf(" 0: %s addr=0x%x on bus %d\n", pc->drv->drv_name, (pc->addr & 0x1f), (pc->addr & 0x40) ? 1 : 0);
        
            max_index ++;
        
            if (pc->next != NULL) {
                sal_printf(" 1: %s addr=0x%x on bus %d\n", pc->next->drv->drv_name, (pc->next->addr & 0x1f), (pc->next->addr & 0x40) ? 1 : 0);
                max_index ++;
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
        if(ui_get_dword(&addr, "reg address: ") == UI_RET_OK &&
            ui_get_dword(&data, "data: ") == UI_RET_OK) {

            if (rw == 0) {
                pc_temp = pc;
            } else {
                pc_temp = pc->next;
            }
            
            if ((addr & PHY_REG_ACC_MASK) == PHY_REG_ACC_BRCM_SHADOW) {
                r = phy_brcm_shadow_write(pc_temp, addr, data);
            } else if ((addr & PHY_REG_ACC_MASK) == PHY_REG_ACC_BRCM_1000X) {
                r = phy_brcm_1000x_write(pc_temp, addr, data);
            } else if ((addr & PHY_REG_ACC_MASK) == PHY_REG_ACC_XGS_IBLK) {
                r = phy_xgs_iblk_write(pc_temp, addr, data);
            } else if ((addr & PHY_REG_ACC_MASK) == PHY_REG_ACC_XAUI_IBLK) {
                r = phy_xaui_iblk_write(pc_temp, addr, data);
            } else if ((addr & PHY_REG_ACC_MASK) == PHY_REG_ACC_AER_IBLK) {
                r = phy_aer_iblk_write(pc_temp, addr, data);
            } else if ((addr & PHY_REG_ACC_MASK) == PHY_REG_ACC_BRCM_XE) {
                r = phy_brcm_xe_write(pc_temp, addr, data);
            } else if ((addr & PHY_REG_ACC_MASK) == PHY_REG_ACC_TSC_IBLK) {
                if (sal_strcmp(pc_temp->drv->drv_name, "bcmi_tsc_xgxs") == 0) {
                    r = phy_tsc_iblk_write(pc_temp, addr, data);
                } else {
                    r = PHY_CONFIG_SET(pc_temp, PhyConfig_RegisterAccess, addr, (void *)&data);
                }
            } else if ((addr & PHY_REG_ACC_MASK) == PHY_REG_ACC_BRCM_RDB) {
                r = phy_brcm_rdb_write(pc_temp, addr, data);
            } else {
                r = PHY_BUS_WRITE(pc_temp, addr, data);
            }

            if (r != SYS_OK) {
                sal_printf("Error!\n");
                return;
            }
            sal_printf("PHY(Port %d) : Reg:0x%X  Value:0x%08X\n", uport, addr, data);
          }
    }
}

#endif /* !CFG_PCM_SUPPORT_INCLUDED */

APISTATIC void
APIFUNC(cli_cmd_switch_reg_get)(CLI_CMD_OP op) REENTRANT
{
    if (op == CLI_CMD_OP_HELP) {
        sal_printf("Command to get Robo switch register value.\n"
                   "Page, offset and length are required.\n");
    } else if (op == CLI_CMD_OP_DESC) {
        sal_printf("Get value of switch register");
    } else {
        uint8 page, offset, len;
        uint8 unit = 0;
        uint32 addr, val;
        soc_switch_t *soc;

#if (BOARD_NUM_OF_UNITS > 1)
        sal_printf("Unit(0~%bu): ", (uint8)BOARD_NUM_OF_UNITS - 1);
        if (ui_get_byte(&unit, NULL) != UI_RET_OK) {
            return;
        }
        if (unit >= board_unit_count()) {
            sal_printf("Invalid unit!\n");
            return;
        }
#endif

        soc = board_get_soc_by_unit(unit);
        SAL_ASSERT(soc != NULL);
        if (soc == NULL) {
            return;
        }

        if ((*soc->chip_type)() == SOC_TYPE_SWITCH_ROBO) {
            if (ui_get_byte(&page, "Page: ") == UI_RET_OK &&
                ui_get_byte(&offset, "Offset: ") == UI_RET_OK &&
                ui_get_byte(&len, "Length: ") == UI_RET_OK) {

                uint8 buf[8], i;
                sys_error_t r;
                r = (*soc->robo_switch_reg_get)(unit, page, offset, buf, len);
                if (r != SYS_OK) {
                    sal_printf("Error!\n");
                    return;
                }
                sal_printf("Byte [0-%bu]: ", len - 1);
                for(i=0; i<len; i++) {
                    sal_printf("%02bX ", buf[i]);
                }
                sal_putchar('\n');
            }
        } else {
#ifdef CFG_SWITCH_XGS_NEW_SBUS_FORMAT_INCLUDED
            uint8 block;
            if (ui_get_byte(&block, "Block: ") == UI_RET_OK &&
                ui_get_dword(&addr, "Addr: ") == UI_RET_OK ) {
                sys_error_t r;
                r = (*soc->xgs_switch_reg_get)(unit, block, addr, &val);
                if (r != SYS_OK) {
                    sal_printf("Error!\n");
                    return;
                }
                sal_printf("Value: 0x%08X\n", val);
            }
#else
            if (ui_get_dword(&addr, "Addr: ") == UI_RET_OK) {
                sys_error_t r;
                r = (*soc->xgs_switch_reg_get)(unit, addr, &val);
                if (r != SYS_OK) {
                    sal_printf("Error!\n");
                    return;
                }
                sal_printf("Value: 0x%08X\n", val);
            }
#endif /* CFG_SWITCH_XGS_NEW_SBUS_FORMAT_INCLUDED */
        }
    }
}

APISTATIC void
APIFUNC(cli_cmd_switch_reg_set)(CLI_CMD_OP op) REENTRANT
{
    if (op == CLI_CMD_OP_HELP) {
        sal_printf("Command to set Robo switch register.\n"
                   "Page, offset, length and data are required.\n");
    } else if (op == CLI_CMD_OP_DESC) {
        sal_printf("Set value of switch register");
    } else {
        uint8 page, offset, len;
        uint8 unit = 0;
        uint32 addr, val;
        soc_switch_t *soc;

#if (BOARD_NUM_OF_UNITS > 1)
        sal_printf("Unit(0~%bu): ", (uint8)BOARD_NUM_OF_UNITS - 1);
        if (ui_get_byte(&unit, NULL) != UI_RET_OK) {
            return;
        }
        if (unit >= board_unit_count()) {
            sal_printf("Invalid unit!\n");
            return;
        }
#endif

        soc = board_get_soc_by_unit(unit);
        SAL_ASSERT(soc != NULL);
        if (soc == NULL) {
            return;
        }

        if ((*soc->chip_type)() == SOC_TYPE_SWITCH_ROBO) {
            if (ui_get_byte(&page, "Page: ") == UI_RET_OK &&
                ui_get_byte(&offset, "Offset: ") == UI_RET_OK &&
                ui_get_byte(&len, "Length: ") == UI_RET_OK) {
                uint8 buf[8], i;
                sys_error_t r;


                for(i=0; i<len; i++) {
                    sal_printf("Byte [%bu]: ", i);
                    if (ui_get_byte(&buf[i], NULL) != UI_RET_OK) {
                        break;
                    }
                }
                if (i != len) {
                    sal_printf("Cancelled.\n");
                    return;
                }

                r = (*soc->robo_switch_reg_set)(unit, page, offset, buf, len);
                if (r != SYS_OK) {
                    sal_printf("Error!\n");
                    return;
                }
                sal_printf("Done.\n");
            }
        } else {
#ifdef CFG_SWITCH_XGS_NEW_SBUS_FORMAT_INCLUDED
            uint8 block;
            if (ui_get_byte(&block, "Block: ") == UI_RET_OK &&
                ui_get_dword(&addr, "Addr: ") == UI_RET_OK &&
                ui_get_dword(&val, "Value: ") == UI_RET_OK) {

                sys_error_t r;
                r = (*soc->xgs_switch_reg_set)(unit, block, addr, val);
                if (r != SYS_OK) {
                    sal_printf("Error!\n");
                    return;
                }
                sal_printf("Done.\n");
            }
#else
            if (ui_get_dword(&addr, "Addr: ") == UI_RET_OK &&
                ui_get_dword(&val, "Value: ") == UI_RET_OK ) {
                sys_error_t r;

                r = (*soc->xgs_switch_reg_set)(unit, addr, val);
                if (r != SYS_OK) {
                    sal_printf("Error!\n");
                    return;
                }
                sal_printf("Done.\n");
            }
#endif /* CFG_SWITCH_XGS_NEW_SBUS_FORMAT_INCLUDED */
        }
    }
}

#if CFG_XGS_CHIP
void
APISTATIC APIFUNC(cli_cmd_switch_mem_get)(CLI_CMD_OP op) REENTRANT
{
    if (op == CLI_CMD_OP_HELP) {
        sal_printf("This command displays data from table as dwords in hex.\n"
                    "To stop dumping, press ESC or Ctrl-C.\n");
    } else if (op == CLI_CMD_OP_DESC) {
        sal_printf("Dump table");
    } else {
        uint8 *addr;
        uint16 i, index, len;
        soc_switch_t *soc;
        uint32 buf[16];

#ifdef CFG_SWITCH_XGS_NEW_SBUS_FORMAT_INCLUDED
        uint8 block;
       if (ui_get_byte(&block, "Block: ") == UI_RET_OK &&
            ui_get_address(&addr, "Address: ") == UI_RET_OK &&
            ui_get_word(&index, "Index: ") == UI_RET_OK &&
            ui_get_word(&len, "Length: ") == UI_RET_OK &&
            len > 0 && len < 16) {

            soc = board_get_soc_by_unit(0);
            SAL_ASSERT(soc != NULL);

            (*soc->xgs_switch_mem_get)(0, block, DATAPTR2MSADDR(addr+index), buf, len);

            sal_printf("Value: ");
            for (i = 0; i < len; i++) {
                sal_printf(" 0x%08X ", buf[i]);
            }
        }
#else
        if (ui_get_address(&addr, "Address: ") == UI_RET_OK &&
            ui_get_word(&index, "Index: ") == UI_RET_OK &&
            ui_get_word(&len, "Length: ") == UI_RET_OK &&
            len > 0 && len < 16) {

            soc = board_get_soc_by_unit(0);
            SAL_ASSERT(soc != NULL);

            (*soc->xgs_switch_mem_get)(0, DATAPTR2MSADDR(addr+index), buf, len);

            sal_printf("Value: ");
            for (i = 0; i < len; i++) {
                sal_printf(" 0x%08X ", buf[i]);
            }
            sal_printf("\nDone\n");
        }
#endif /* CFG_SWITCH_XGS_NEW_SBUS_FORMAT_INCLUDED */
    }
}

void
APISTATIC APIFUNC(cli_cmd_switch_mem_set)(CLI_CMD_OP op) REENTRANT
{
    if (op == CLI_CMD_OP_HELP) {
        sal_printf("This command edits the contents of table in a dword "
                    "by dword manner.\n"
                    "Press ESC or Ctrl-C to quit editing.\n");
    } else if (op == CLI_CMD_OP_DESC) {
        sal_printf("Edit contents of table");
    } else {
        ui_ret_t r;
        uint8 *addr;
        uint16 i, index, len;
        soc_switch_t *soc;
        uint32 val, buf[16];
#ifdef CFG_SWITCH_XGS_NEW_SBUS_FORMAT_INCLUDED
        uint8 block;
       if (ui_get_byte(&block, "Block: ") == UI_RET_OK &&
            ui_get_address(&addr, "Address: ") == UI_RET_OK &&
            ui_get_word(&index, "Index: ") == UI_RET_OK &&
            ui_get_word(&len, "Length: ") == UI_RET_OK &&
            len > 0 && len < 16) {

            soc = board_get_soc_by_unit(0);
            SAL_ASSERT(soc != NULL);

            (*soc->xgs_switch_mem_get)(0, block, DATAPTR2MSADDR(addr+index), buf, len);

             for(i=0; i<len; i++) {
                  sal_printf("DWord [%bu:0x%08x]: ", i, buf[i]);
                  r = ui_get_dword(&val, NULL);
                  if (r == UI_RET_OK || UI_RET_EMPTY) {
                      /* Keep original value if empty */
                      if (r == UI_RET_OK) {
                          buf[i] = val;
                      }
                      continue;
                  }
                  break;
             }
             if (i != len) {
                  sal_printf("Cancelled.\n");
                  return;
             }
             (*soc->xgs_switch_mem_set)(0, block, DATAPTR2MSADDR(addr+index), buf, len);
             sal_printf("Done\n");
        }
#else
        if (ui_get_address(&addr, "Address: ") == UI_RET_OK &&
            ui_get_word(&index, "Index: ") == UI_RET_OK &&
            ui_get_word(&len, "Length: ") == UI_RET_OK &&
            len > 0 && len < 16) {

            soc = board_get_soc_by_unit(0);
            SAL_ASSERT(soc != NULL);

            (*soc->xgs_switch_mem_get)(0, DATAPTR2MSADDR(addr+index), buf, len);

             for(i=0; i<len; i++) {
                  sal_printf("DWord [%bu:0x%08x]: ", i, buf[i]);
                  r = ui_get_dword(&val, NULL);
                  if (r == UI_RET_OK || UI_RET_EMPTY) {
                      /* Keep original value if empty */
                      if (r == UI_RET_OK) {
                          buf[i] = val;
                      }
                      continue;
                  }
                  break;
             }
             if (i != len) {
                  sal_printf("Cancelled.\n");
                  return;
             }
             (*soc->xgs_switch_mem_set)(0, DATAPTR2MSADDR(addr+index), buf, len);
             sal_printf("Done\n");
        }
#endif /* CFG_SWITCH_XGS_NEW_SBUS_FORMAT_INCLUDED */
    }
}
#endif /* CFG_XGS_CHIP */

#ifdef CFG_SWITCH_SNAKETEST_INCLUDED
APISTATIC void
APIFUNC(cli_cmd_snake)(CLI_CMD_OP op) REENTRANT
{
    if (op == CLI_CMD_OP_HELP) {
        sal_printf("Command to do snake test.\n");
    } else if (op == CLI_CMD_OP_DESC) {
        sal_printf("Snake Test");
    } else {
        ui_ret_t r;
        uint32 val32 = 0, min_uport = SAL_NZUPORT_TO_UPORT(1), max_uport = SAL_NZUPORT_TO_UPORT(board_uport_count());
        uint32 duration;
        int i;
        char str[32];

        sal_printf("Run mode : \n"
                   " 0: Internal MAC loopback\n"
                   "    - Must disconnect cable from all testing ports\n"
                   "    - Total testing ports must be larger than or equal to 2\n");
        sal_printf(" 1: Internal PHY loopback\n"
                   "    - Must disconnect cable from all testing ports\n"
                   "    - Total testing ports must be larger than or equal to 2\n");
        sal_printf(" 2: External PHY loopback\n"
                   "    - Must connect loopback cable from all testing ports\n"
                   "    - Total testing ports must be larger than or equal to 2\n");
        sal_printf(" 3: Port pair loopback\n"
                   "    - Min port must start from an odd number, such as 1\n"
                   "    - Total testing ports must be larger than or equal to 2\n"
                   "    - Total testing ports must be the multiple of 2\n");
        sal_printf(" 4: Do snake setting for packet generator device\n"
                   "    - Min port must start from an odd number, such as 1\n"
                   "    - Total testing ports must be larger than or equal to 4\n"
                   "    - Total testing ports must be the multiple of 2\n");
        r = ui_get_decimal(&val32, "Enter your choice: [0] ");
        if (r == UI_RET_EMPTY || r == UI_RET_OK) {
            if (r == UI_RET_EMPTY) {
                val32 = SNAKETEST_TYPE_INT_MAC;
            } else if (val32 >= SNAKETEST_TYPE_COUNT) {
                sal_printf("Invalid choice.\n");
                return;
            }
        }

        if (val32 < SNAKETEST_TYPE_COUNT) {
            sal_sprintf(str, "Enter min port: [%d] ", min_uport);
            r = ui_get_decimal(&min_uport, str);
            if (r == UI_RET_EMPTY || r == UI_RET_OK) {
                if (r == UI_RET_EMPTY) {
                    min_uport = SAL_NZUPORT_TO_UPORT(1);
                } else if (SAL_UPORT_IS_NOT_VALID(min_uport)) {
                    sal_printf("Invalid port.\n");
                    return;
                }
            }
            sal_sprintf(str, "Enter max port: [%d] ", max_uport);
            r = ui_get_decimal(&max_uport, str);
            if (r == UI_RET_EMPTY || r == UI_RET_OK) {
                if (r == UI_RET_EMPTY) {
                    max_uport = SAL_NZUPORT_TO_UPORT(board_uport_count());
                } else if (SAL_UPORT_IS_NOT_VALID(max_uport)) {
                    sal_printf("Invalid port.\n");
                    return;
                }
            }

            if (val32 <= SNAKETEST_TYPE_PORT_PAIR) {
                r = ui_get_decimal(&duration, "Enter duration (seconds) - must be the multiple of 10: [60] ");
                if (r == UI_RET_EMPTY || r == UI_RET_OK) {
                    if (r == UI_RET_EMPTY) {
                        duration = 60;
                    }
                }
            }

            if (min_uport > max_uport) {
                sal_printf("Invalid port.\n");
                return;
            }

            if ((val32 == SNAKETEST_TYPE_INT_MAC) || 
                (val32 == SNAKETEST_TYPE_INT_PHY) ||
                (val32 == SNAKETEST_TYPE_EXT)) {
                if ((max_uport - min_uport + 1) < 2 ) {
                    sal_printf("Total testing ports must be larger than or equal to 2.\n");
                    return;
                }
            } else if ((val32 == SNAKETEST_TYPE_PORT_PAIR) || (val32 == SNAKETEST_TYPE_PKT_GEN)) {
                if (!(min_uport % 2)) {
                    sal_printf("Min port must start from an odd number.\n");
                    return;
                }
                
                if ((max_uport - min_uport + 1) % 2) {
                    sal_printf("Total testing ports must be the multiple of 2.\n");
                    return;            
                } else {
                    if (val32 == SNAKETEST_TYPE_PORT_PAIR) {
                        sal_printf("Please connect cable for port pair");
                        for (i = min_uport ; i < max_uport ; i = i + 2) {
                            sal_printf(" (%d, %d)", i, i+1);
                        }
                        sal_printf(".\n");
                    } else {
                        if ((max_uport - min_uport + 1) >= 4 ) {
                            sal_printf("Please connect cable for port pair");
                            for (i = min_uport + 1 ; i < (max_uport - 1) ; i = i + 2) {
                                sal_printf(" (%d, %d)", i, i+1);
                            }
                            sal_printf(".\n");
                            sal_printf("\nPlease connect cable with packet generator device for ports %d and %d.\n", min_uport, max_uport);
                        } else {
                            sal_printf("Total testing ports must be larger than or equal to 4.\n");
                            return;
                        }
                    }                
                }
            } 

            snaketest((uint8)val32, (uint8)min_uport, (uint8)max_uport, (int)duration);
            sal_printf("\nPlease reboot system to recover setting from snake test !\n");
        } else {
            sal_printf("Invalid mode.\n");
        }
    }
}
#endif
uint8 board_linkdown_message = 0;
uint8 board_upload_vc = 0;

APISTATIC void
APIFUNC(ui_switch_misc)(CLI_CMD_OP op) REENTRANT {

        uint8 select = 0;
        if (op == CLI_CMD_OP_HELP) {
            sal_printf("Command to configure switch misc feature\n");
        } else if (op == CLI_CMD_OP_DESC) {
            sal_printf("Command to configure switch misc feature");
        } else {
            sal_printf("0: Link scan enable/disable\n");
            sal_printf("1: Link down message enable/disable\n");
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
            sal_printf("2: Upload vendor config enable/disable\n");
#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */
            if (ui_get_byte(&select, "Please select: ") == UI_RET_OK) {
                 switch (select) {
                    case 0:
                         sal_printf("  0: Enable link scan \n"
                                    "  1: Disable link scan \n");
                         if (ui_get_byte(&select, "select: ") == UI_RET_OK) { 
                             if (select == 0) {
                                board_linkscan_disable = 0;
                             } else if (select == 1) {
                                board_linkscan_disable = 1;
                             }
                         }
                    break;
                    case 1:
                         sal_printf("  0: Disable link down message \n"
                                    "  1: Enable link down message \n");
                         if (ui_get_byte(&select, "select: ") == UI_RET_OK) { 
                             if (select == 0) {
                                board_linkdown_message = 0;
                             } else if (select == 1) {
                                board_linkdown_message = 1;
                             }
                         }
                    break;
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
                    case 2:
                         sal_printf("  0: Disable vendor config upload \n"
                                    "  1: Enable vendor config upload \n");
                         if (ui_get_byte(&select, "select: ") == UI_RET_OK) { 
                             if (select == 0) {
                                board_upload_vc = 0;
                             } else if (select == 1) {
                                board_upload_vc = 1;
                             }
                         }
                    break;
#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */
                    default:
                    break;
                 }
            }
        }


}

#if defined(CFG_SWITCH_L2_ADDR_INCLUDED)

const char STRING_UPORT[] = "UPORT";
const char STRING_MCINDEX[] = "MCIDX";
const char STRING_TGID[] = "TGID ";
const char STRING_STATIC[] = "S";
const char STRING_EMPTY[] = "";

APISTATIC void
APIFUNC(cli_cmd_l2)(CLI_CMD_OP op) REENTRANT
{
    if (op == CLI_CMD_OP_HELP) {
        sal_printf("Command to operate L2 address.\n");
    } else if (op == CLI_CMD_OP_DESC) {
        sal_printf("L2 address commands");
    } else {
        int rv = SYS_OK;
        ui_ret_t r;
        uint32 val32 = 0, i = 0;
        board_l2_addr_t l2addr;
        uint16 index;
        char name[32];
        char c;
        uint8 multicast_bit;

        sal_memset(&l2addr, 0x0, sizeof(board_l2_addr_t));

        sal_printf("Operation mode : \n"
                   " 0: Add L2 address\n");
        sal_printf(" 1: Delete L2 address\n");
        sal_printf(" 2: Delete L2 addresses by port\n");
        sal_printf(" 3: Delete L2 addresses by VLAN\n");
        sal_printf(" 4: Delete L2 addresses by TRUNK\n");
        sal_printf(" 5: Delete all L2 addresses\n");
        sal_printf(" 6: Lookup L2 address\n");
        sal_printf(" 7: Get first L2 address\n");
        sal_printf(" 8: Get next L2 address\n");
        sal_printf(" 9: Get last L2 address\n");
        sal_printf(" 10: Get all L2 addresses\n");
        r = ui_get_decimal(&val32, "Enter your choice: [0] ");
        if (r == UI_RET_EMPTY || r == UI_RET_OK) {
            if (r == UI_RET_EMPTY) {
                val32 = L2_CMD_ADD;
            } else if (val32 >= L2_CMD_COUNT) {
                sal_printf("Invalid choice.\n");
                return;
            }
        }

        if (val32 == L2_CMD_ADD) {
            if (ui_get_string(name, 32, "Mac: ") == UI_RET_OK) {
                if (parse_mac_address((const char*)name, l2addr.mac) == SYS_OK) {
                    r = ui_get_decimal(&val32, "Enter VLAN ID : [1] ");
                    if (r == UI_RET_EMPTY || r == UI_RET_OK) {
                        if (r == UI_RET_EMPTY) {
                            val32 = 1;
                        } else if ((val32 > 4094) || (val32 < 1)) {
                            sal_printf("Invalid VLAN ID.\n");
                            return;
                        }
                    }
                    l2addr.vid = (uint16) val32;
                    r = ui_get_decimal(&val32, "Enter user port number: [1] ");
                    if (r == UI_RET_EMPTY || r == UI_RET_OK) {
                        if (r == UI_RET_EMPTY) {
                            val32 = SAL_NZUPORT_TO_UPORT(1);
                        } else if (SAL_UPORT_IS_NOT_VALID(val32)) {
                            sal_printf("Invalid port.\n");
                            return;
                        }
                    }
                    l2addr.uport = (uint16) val32;
                    sal_printf("Static (y or n): ");
                    c = sal_getchar();
                    if ((c == 'y') || (c == 'Y')) {
                        l2addr.flags |= BOARD_L2_STATIC;
                    }
                    rv = board_l2_addr_add(&l2addr);
                    if (rv == SYS_ERR_EXISTS) {
                        sal_printf("\nThe L2 address %s with vid %d already existed\n", name, l2addr.vid);
                    } else {
                        sal_printf("\nAdd L2 address %s with vid %d\n", name, l2addr.vid);
                    }
                } else {
                     sal_printf("Invalid MAC address or format. (For example, 00-00-00-01-02-03)\n");
                }
            } else {
                sal_printf("Invalid MAC address or format. (For example, 00-00-00-01-02-03)\n");
                return;
            }
        } else if (val32 == L2_CMD_DELETE) {
            if (ui_get_string(name, 32, "Mac: ") == UI_RET_OK) {
                if (parse_mac_address((const char*)name, l2addr.mac) == SYS_OK) {
                    r = ui_get_decimal(&val32, "Enter VLAN ID : [1] ");
                    if (r == UI_RET_EMPTY || r == UI_RET_OK) {
                        if (r == UI_RET_EMPTY) {
                            val32 = 1;
                        } else if ((val32 > 4094) || (val32 < 1)) {
                            sal_printf("Invalid VLAN ID.\n");
                            return;
                        }
                    }
                    l2addr.vid = (uint16) val32;
                } else {
                     sal_printf("Invalid MAC address or format. (For example, 00-00-00-01-02-03)\n");
                }
            } else {
                sal_printf("Invalid MAC address or format. (For example, 00-00-00-01-02-03)\n");
                return;
            }
            rv = board_l2_addr_delete(&l2addr);
            if (rv == SYS_ERR_NOT_FOUND) {
                sal_printf("L2 address  %s with vid %d not found !\n", name, val32);
            } else {
                sal_printf("Delete L2 addresses %s with vid %d\n", name, val32);
            }
        } else if (val32 == L2_CMD_DELETE_BY_PORT) {
            r = ui_get_decimal(&val32, "Enter user port number: [1] ");
            if (r == UI_RET_EMPTY || r == UI_RET_OK) {
                if (r == UI_RET_EMPTY) {
                    val32 = SAL_NZUPORT_TO_UPORT(1);
                } else if (SAL_UPORT_IS_NOT_VALID(val32)) {
                    sal_printf("Invalid port.\n");
                    return;
                }
            }

            rv = board_l2_addr_delete_by_port((uint8)val32);
            if (rv == SYS_ERR_TIMEOUT) {
                sal_printf("Delete L2 addresses by port timeout !\n");
            } else {
                sal_printf("Delete L2 addresses by port %d\n", val32);
            }
        } else if (val32 == L2_CMD_DELETE_BY_VLAN) {
            r = ui_get_decimal(&val32, "Enter VLAN ID : [1] ");
            if (r == UI_RET_EMPTY || r == UI_RET_OK) {
                if (r == UI_RET_EMPTY) {
                    val32 = 1;
                } else if ((val32 > 4094) || (val32 < 1)) {
                    sal_printf("Invalid VLAN ID.\n");
                    return;
                }
            }

            rv = board_l2_addr_delete_by_vlan((uint16)val32);
            if (rv == SYS_ERR_TIMEOUT) {
                sal_printf("Delete L2 addresses by VLAN ID timeout !\n");
            } else {
                sal_printf("Delete L2 addresses by VLAN ID %d\n", val32);
            }
        } else if (val32 == L2_CMD_DELETE_BY_TRUNK) {
            r = ui_get_decimal(&val32, "Enter TRUNK ID: [1] ");
            if (r == UI_RET_EMPTY || r == UI_RET_OK) {
                if (r == UI_RET_EMPTY) {
                    val32 = 1;
                } else if ((val32 > BOARD_MAX_NUM_OF_LAG) || (val32 < 1)) {
                    sal_printf("Invalid TRUNK ID.\n");
                    return;
                }
            }

            rv = board_l2_addr_delete_by_trunk((uint8)val32);
            if (rv == SYS_ERR_TIMEOUT) {
                sal_printf("Delete L2 addresses by TRUNK ID timeout !\n");
            } else {
                sal_printf("Delete L2 addresses by TRUNK ID %d\n", val32);
            }
        } else if (val32 == L2_CMD_DELETE_ALL) {
            rv = board_l2_addr_delete_all();
            if (rv == SYS_ERR_TIMEOUT) {
                sal_printf("Delete all L2 addresses timeout !\n");
            } else {
                sal_printf("Delete all L2 addresses\n");
            }
        } else if (val32 == L2_CMD_LOOKUP) {
            if (ui_get_string(name, 32, "Mac: ") == UI_RET_OK) {
                if (parse_mac_address((const char*)name, l2addr.mac) == SYS_OK) {
                    r = ui_get_decimal(&val32, "Enter VLAN ID : [1] ");
                    if (r == UI_RET_EMPTY || r == UI_RET_OK) {
                        if (r == UI_RET_EMPTY) {
                            val32 = 1;
                        } else if ((val32 > 4094) || (val32 < 1)) {
                            sal_printf("Invalid VLAN ID.\n");
                            return;
                        }
                    }
                    l2addr.vid = (uint16) val32;
                } else {
                    sal_printf("Invalid MAC address or format. (For example, 00-00-00-01-02-03)\n");
                    return;
                }
            } else {
                sal_printf("Invalid MAC address or format. (For example, 00-00-00-01-02-03)\n");
                return;
            }
            rv = board_l2_addr_lookup(&l2addr, &index);
            if (rv == SYS_ERR_NOT_FOUND) {
                sal_printf("L2 address not found !\n");
            } else {
                sal_printf("L2 address found at index 0x%4x\n", index);
            }
        } else if (val32 == L2_CMD_GET_FIRST) {
            rv = board_l2_addr_get_first(&l2addr, &index);
            if (rv == SYS_ERR_NOT_FOUND) {
                sal_printf("L2 address not found !\n");
            } else if (rv == SYS_ERR) {
                sal_printf("board_l2_addr_get_first return failed !\n");
            } else {
                multicast_bit = l2addr.mac[0] & 0x1;
                sal_printf("First L2 address found at index 0x%x\n", index);
                sal_printf("     MAC 0x%2x-%2x-%2x-%2x-%2x-%2x\n", 
                    l2addr.mac[0], l2addr.mac[1], l2addr.mac[2], 
                    l2addr.mac[3], l2addr.mac[4], l2addr.mac[5]);
                sal_printf("     VID %d\n", l2addr.vid);
                sal_printf("     %s %d\n", multicast_bit ? STRING_MCINDEX : (l2addr.is_trunk ? STRING_TGID : STRING_UPORT),
                    multicast_bit ? l2addr.mcidx : (l2addr.is_trunk ? l2addr.tgid : l2addr.uport));
            }
        } else if (val32 == L2_CMD_GET_NEXT) {
            rv = board_l2_addr_get_next(&l2addr, &index);
            if (rv == SYS_ERR_NOT_FOUND) {
                sal_printf("L2 address not found !\n");
            } else if (rv == SYS_ERR) {
                sal_printf("board_l2_addr_get_next return failed !\n");
            } else {
                multicast_bit = l2addr.mac[0] & 0x1;
                sal_printf("Next L2 address found at index 0x%x\n", index);
                sal_printf("     MAC 0x%2x-%2x-%2x-%2x-%2x-%2x\n", 
                    l2addr.mac[0], l2addr.mac[1], l2addr.mac[2], 
                    l2addr.mac[3], l2addr.mac[4], l2addr.mac[5]);
                sal_printf("     VID %d\n", l2addr.vid);
                sal_printf("     %s %d\n", multicast_bit ? STRING_MCINDEX : (l2addr.is_trunk ? STRING_TGID : STRING_UPORT),
                    multicast_bit ? l2addr.mcidx : (l2addr.is_trunk ? l2addr.tgid : l2addr.uport));
            }
        } else if (val32 == L2_CMD_GET_LAST) {
            rv = board_l2_addr_get_last(&l2addr, &index);
            if (rv == SYS_ERR_NOT_FOUND) {
                sal_printf("L2 address not found !\n");
            } else if (rv == SYS_ERR) {
                sal_printf("board_l2_addr_get_last return failed !\n");
            } else {
                multicast_bit = l2addr.mac[0] & 0x1;
                sal_printf("Last L2 address found at index 0x%x\n", index);
                sal_printf("     MAC 0x%2x-%2x-%2x-%2x-%2x-%2x\n", 
                    l2addr.mac[0], l2addr.mac[1], l2addr.mac[2], 
                    l2addr.mac[3], l2addr.mac[4], l2addr.mac[5]);
                sal_printf("     VID %d\n", l2addr.vid);
                sal_printf("     %s %d\n", multicast_bit ? STRING_MCINDEX : (l2addr.is_trunk ? STRING_TGID : STRING_UPORT),
                    multicast_bit ? l2addr.mcidx : (l2addr.is_trunk ? l2addr.tgid : l2addr.uport));
            }
        } else if (val32 == L2_CMD_GET_ALL) {
            rv = board_l2_addr_get_first(&l2addr, &index);
            if (rv == SYS_ERR_NOT_FOUND) {
                sal_printf("L2 address not found !\n");
            } else if (rv == SYS_ERR) {
                sal_printf("board_l2_addr_get_first return failed !\n");
            } else {
                i++;
                sal_printf("Below L2 addresses found at L2 ENTRY table\n");
                multicast_bit = l2addr.mac[0] & 0x1;
                sal_printf(" [%5d]   Index 0x%4x   MAC 0x%2x-%2x-%2x-%2x-%2x-%2x   VID %4d   %s %4d   %s\n", 
                    i, index, l2addr.mac[0], l2addr.mac[1], l2addr.mac[2], 
                    l2addr.mac[3], l2addr.mac[4], l2addr.mac[5], l2addr.vid, 
                    multicast_bit ? STRING_MCINDEX : (l2addr.is_trunk ? STRING_TGID : STRING_UPORT),
                    multicast_bit ? l2addr.mcidx : (l2addr.is_trunk ? l2addr.tgid : l2addr.uport),
                    l2addr.flags & BOARD_L2_STATIC ? STRING_STATIC : STRING_EMPTY);
                while (1) {
                    rv = board_l2_addr_get_next(&l2addr, &index);
                    if (rv == SYS_ERR_NOT_FOUND) {
                        break;
                    } else if (rv == SYS_ERR) {
                        sal_printf("board_l2_addr_get_next return failed !\n");
                        break;
                    } else {
                        i++;
                        multicast_bit = l2addr.mac[0] & 0x1;
                        sal_printf(" [%5d]   Index 0x%4x   MAC 0x%2x-%2x-%2x-%2x-%2x-%2x   VID %4d   %s %4d   %s\n", 
                            i, index, l2addr.mac[0], l2addr.mac[1], l2addr.mac[2], 
                            l2addr.mac[3], l2addr.mac[4], l2addr.mac[5], l2addr.vid, 
                            multicast_bit ? STRING_MCINDEX : (l2addr.is_trunk ? STRING_TGID : STRING_UPORT),
                            multicast_bit ? l2addr.mcidx : (l2addr.is_trunk ? l2addr.tgid : l2addr.uport),
                            l2addr.flags & BOARD_L2_STATIC ? STRING_STATIC : STRING_EMPTY);
                    }
                }
            }
        }
    }
}
#endif /* CFG_SWITCH_L2_ADDR_INCLUDED */

void
APIFUNC(ui_switch_init)(void) REENTRANT
{
#ifndef CFG_PCM_SUPPORT_INCLUDED
    /* Phy register get/set */
    cli_add_cmd('p', cli_cmd_phy_reg_get);
    cli_add_cmd('q', cli_cmd_phy_reg_set);
#endif
    /* Switch register get/set */
    cli_add_cmd('g', cli_cmd_switch_reg_get);
    cli_add_cmd('s', cli_cmd_switch_reg_set);

#if CFG_XGS_CHIP
    /* Switch table read/write */
    cli_add_cmd('r', cli_cmd_switch_mem_get);
    cli_add_cmd('w', cli_cmd_switch_mem_set);
#endif /* CFG_XGS_CHIP */

#if defined(CFG_SWITCH_SNAKETEST_INCLUDED)
    cli_add_cmd('k', cli_cmd_snake);
#endif

#ifdef BRD_VLAN_DEBUG
    cli_add_cmd('v', cli_cmd_switch_vlan_info_dump);
#endif /* BRD_VLAN_DEBUG */

#ifdef CFG_XCOMMAND_INCLUDED
    cli_add_cmd('x', cli_cmd_switch_xcmd_shell);
#ifdef CFG_XCOMMAND_BUILDER_CLI_INCLUDED
    cli_add_cmd('b', cli_cmd_switch_xcmd_builder);
#endif /* CFG_XCOMMAND_BUILDER_CLI_INCLUDED */
#endif /* CFG_XCOMMAND_INCLUDED */

     cli_add_cmd('l', ui_switch_misc);
#if defined(CFG_SWITCH_L2_ADDR_INCLUDED)
     cli_add_cmd('L', cli_cmd_l2);
#endif /* CFG_SWITCH_L2_ADDR_INCLUDED */
}

#endif
