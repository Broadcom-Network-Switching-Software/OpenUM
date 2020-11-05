#ifdef __C51__

#ifdef CODE_USERCLASS

#pragma userclass (code = uisw)

#endif /* CODE_USERCLASS */

#endif /* __C51__ */

#include "system.h"
#include "appl/cli.h"
#include "utils/ui.h"
#include "utils/net.h"
#include "board.h"

APISTATIC void
APIFUNC(cli_cmd_qt_utils)(CLI_CMD_OP op) REENTRANT
{
    uint32 link, uport, sel;
    uint8 lport, unit;
    extern unsigned int dtcm_endaddr_ref;
    unsigned int addr = 0x50000000;
    unsigned int size = 128 *1024;

    if (op == CLI_CMD_OP_HELP) {
        sal_printf("Supporting utils for QT verification.\n");
    } else if (op == CLI_CMD_OP_DESC) {
        sal_printf("Supporting utils for QT verification\n");
    } else {
        extern int link_qt[];
        sel = 0;
        sal_printf("Run case: \n"
                    "0. Link down and up MAC \n"
                    "1. L2CRAM testing \n");
		sal_printf( "2. SRAM testing \n");
		
        if (ui_get_dword(&sel, "[0]: ") != UI_RET_OK) {
            return;
        }
        if (sel > 1)  {
            return;
        }

        if (sel == 0) {
            sal_printf("memory compare from %x\n", dtcm_endaddr_ref);
            for (addr = dtcm_endaddr_ref; addr < (0x50000000 + size); addr += 4) {
                  if ((*((unsigned int *) addr)) != (0xA5A5A5A5 + ((addr & (size-1)) >> 5))) {
                      sal_printf("memory compare error %x\n", addr);
                  }
            }
        } else if (sel == 1) {
          if ((ui_get_dword(&link, "Link:") == UI_RET_OK) && 
              (ui_get_dword(&uport, "Port:") == UI_RET_OK)) {
               if (SAL_UPORT_IS_NOT_VALID(uport) == 0) { 
                  if (board_uport_to_lport(uport, &unit, &lport) == SYS_OK) {
                      link_qt[lport] = ((link != 0)? 1 : 0);
                  }
               } else {
                  SAL_UPORT_ITER(uport) {
                       if (board_uport_to_lport(uport, &unit, &lport) == SYS_OK) {
                           link_qt[lport] = ((link != 0)? 1 : 0);
                       }
                  }
               }
          }
        } else if (sel == 2) {
            for (addr = dtcm_endaddr_ref; addr < (0x50000000 + size); addr += 4) {
                  if ((*((unsigned int *) addr)) != (0xA5A5A5A5 + ((addr & (size-1)) >> 5))) {
                      sal_printf("memory compare error %x\n", addr);
                  }
            }
        }
    }
}

void
APIFUNC(ui_qt_init)(void) REENTRANT
{
   cli_add_cmd('u', cli_cmd_qt_utils);
    
}
