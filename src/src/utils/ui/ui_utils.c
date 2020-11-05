/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifdef __C51__
#ifdef CODE_USERCLASS
#pragma userclass (code = uutl)
#endif /* CODE_USERCLASS */
#endif /* __C51__ */

#include "system.h"
#include "utils/ui.h"

#if CFG_CONSOLE_ENABLED

void
APIFUNC(ui_backspace)(void) REENTRANT
{
    sal_putchar(UI_KB_BS);
}

void
APIFUNC(ui_dump_memory)(uint8 *addr, uint16 len) REENTRANT
{
    uint8 p;
    uint8 c, buf[17];
    
    p = 0;
    for(;;) {
        
        if (len == 0) {
            break;
        }
        
        if (p == 0) {
            sal_printf("\n %08lX: ", (uint32)DATAPTR2MSADDR(addr));
        }
        
        c = *addr;
        if (p == 8) {
            sal_putchar(' ');
        }
        sal_printf("%02bX ", c);
        if (c < ' ' || c > '~') {
            c = '.';
        }
        
        buf[p] = c;
        
        p++;
        if (p == 16 || len == 1) {
            buf[p] = 0;
            sal_printf("   %s", buf);
            p = 0;
        }
        
        addr++;
        len--;

        /* Cancel per user request */        
        while (sal_char_avail()) {
            char ch = sal_getchar();
            ui_backspace();
            if (ch == UI_KB_ESC || ch == UI_KB_CTRL_C) {
                sal_printf("\n");
                return;
            }
        }
    }
    
    sal_printf("\n");
}

#endif /* CFG_CONSOLE_ENABLED */

#if CFG_CLI_ENABLED

ui_ret_t
APIFUNC(ui_get_hex)(uint32 *value, uint8 size) REENTRANT
{
    uint8 len;
    char ch;
    
    if (value == NULL) {
        return UI_RET_ERROR;
    }

    len = 0;
    *value = 0;
    sal_printf("0x");
    while(len <= size) {
        ch = sal_getchar();
        if (ch == UI_KB_LF || ch == UI_KB_CR) {
            break;
        } else if (ch == UI_KB_ESC || ch == UI_KB_CTRL_C) {
            /* Cancelled */
            ui_backspace();
            if (ch == UI_KB_ESC) {
                sal_putchar(UI_KB_CR);
            }
            sal_putchar('\n');
            return UI_RET_CANCEL;
        } else if (ch == UI_KB_BS || ch == UI_KB_DEL) {
            if (len > 0) {
                len--;
                *value >>= 4;
#ifdef __C51__
                sal_putchar(' ');
#endif
                ui_backspace();

            } else {
                sal_putchar('x');
            }
        } else if (len == size) {
            /* Can only accept Backspace or ENTER */
            ui_backspace();
            sal_putchar(' ');
            ui_backspace();
        } else if (ch >= '0' && ch <= '9') {
            len++;
            ch -= '0';
            *value <<= 4;
            *value += ch;
        } else if (ch >= 'A' && ch <= 'F') {
            len++;
            ch -= 'A';
            *value <<= 4;
            *value += ch + 10;
        } else if (ch >= 'a' && ch <= 'f') {
            len++;
            ch -= 'a';
            *value <<= 4;
            *value += ch + 10;
        } else {
            ui_backspace();
            sal_putchar(' ');
            ui_backspace();
        }
    }
    
    if (len == 0) {
        return UI_RET_EMPTY;
    }
    
    return UI_RET_OK;
}

ui_ret_t
APIFUNC(ui_get_byte)(uint8 *val, const char *str) REENTRANT
{
    uint32 value;
    ui_ret_t r;
    
    if (val == NULL) {
        return UI_RET_ERROR;
    }

    if (str != NULL) {
        sal_printf(str);
    }
    r = ui_get_hex(&value, 2);
    *val = (uint8)(value & 0xFF);
    return r;
}

ui_ret_t
APIFUNC(ui_get_word)(uint16 *val, const char *str) REENTRANT
{
    uint32 value;
    ui_ret_t r;

    if (val == NULL) {
        return UI_RET_ERROR;
    }

    if (str != NULL) {
        sal_printf(str);
    }
    r = ui_get_hex(&value, 4);
    *val = (uint16)(value & 0xFFFF);
    return r;
}

ui_ret_t
APIFUNC(ui_get_dword)(uint32 *val, const char *str) REENTRANT
{
    uint32 value;
    ui_ret_t r;

    if (val == NULL) {
        return UI_RET_ERROR;
    }

    if (str != NULL) {
        sal_printf(str);
    }
    r = ui_get_hex(&value, 8);
    *val = value;
    return r;
}

ui_ret_t
APIFUNC(ui_get_address)(uint8 **paddr, const char *str) REENTRANT
{
    uint32 value;
    uint8 len = sizeof(msaddr_t) * 2;  /* 2 digits per byte */    
    ui_ret_t r;

    if (paddr == NULL) {
        return UI_RET_ERROR;
    }

    if (str != NULL) {
        sal_printf(str);
    }
    r = ui_get_hex(&value, len);
    *paddr = MSADDR2DATAPTR(value);
    sal_putchar(' ');
    return r;
}

#if CFG_SAL_LIB_SUPPORT_ENABLED
ui_ret_t
APIFUNC(ui_get_decimal)(uint32 *pvalue, const char *str) REENTRANT
{
    char ch, last;
    uint32 value = 0;
    uint8 len;
    
    if (pvalue == NULL) {
        return UI_RET_ERROR;
    }

    if (str != NULL) {
        sal_printf(str);
        last = str[sal_strlen(str) - 1];
    } else {
        last = ' ';
    }
    len = 0;
    while(len <= 10) {
        ch = sal_getchar();
        if (ch == UI_KB_LF || ch == UI_KB_CR) {
            break;
        } else if (ch == UI_KB_ESC || ch == UI_KB_CTRL_C) {
            /* Cancelled */
            ui_backspace();
            if (ch == UI_KB_ESC) {
                sal_putchar(UI_KB_CR);
            }
            sal_putchar('\n');
            return UI_RET_CANCEL;
        } else if (ch == UI_KB_BS || ch == UI_KB_DEL) {
            if (len > 0) {
                len--;
                value /= 10;
#ifdef __C51__
                sal_putchar(' ');
#endif
                ui_backspace();
            } else {
                sal_putchar(last);
            }
        } else if (len == 10) {
            /* Can only accept Backspace or ENTER */
            ui_backspace();
            sal_putchar(' ');
            ui_backspace();
        } else if (ch >= '0' && ch <= '9') {
            len++;
            ch -= '0';
            value *= 10;
            value += ch;
        } else {
            ui_backspace();
            sal_putchar(' ');
            ui_backspace();
        }
    }

    if (len == 0) {
        return UI_RET_EMPTY;
    }    
    
    *pvalue = value;
    return UI_RET_OK;
}

BOOL
APIFUNC(ui_yes_or_no)(const char *str, uint8 suggested) REENTRANT
{
    char ch;
    
    for(;;) {
        if (str != NULL) {
            sal_printf(str);
        }
        if (suggested == 0) {
            sal_printf(" [N/y] ");
        } else if (suggested == 1) {
            sal_printf(" [Y/n] ");
        } else if (suggested == 2) {
            sal_printf(" [y/n] ");
        }
    
        ch = sal_getchar();
        if (ch == UI_KB_LF || ch == UI_KB_CR) {
            if (suggested == 0) {
                return FALSE;
            }
            if (suggested == 1) {
                return TRUE;
            }
            continue;
        } 
        if (ch == UI_KB_ESC) {
            ui_backspace();
            sal_putchar(UI_KB_CR);
        }
        sal_putchar('\n');
        if (ch == 'Y' || ch == 'y') {
            return TRUE;
        } else if (ch == 'N' || ch == 'n') {
            return FALSE;
        }
    }
}
#endif /* CFG_SAL_LIB_SUPPORT_ENABLED */

ui_ret_t
APIFUNC(ui_get_bytes)(uint8 *pbytes, uint8 len, const char *str, BOOL show_org) REENTRANT
{
    uint8 i;
    uint32 value;
    ui_ret_t r;
    
    if (pbytes == NULL) {
        return UI_RET_ERROR;
    }

    for(i=0; i<len; i++) {
        if (str != NULL && str[0] != 0) {
            sal_printf(" %s", str);
        }
        sal_printf(" byte %bu: ", i);
        if (show_org) {
            sal_printf("[0x%02bX] ", pbytes[i]);
        }

        r = ui_get_hex(&value, 2);
        if (r == UI_RET_OK) {
            pbytes[i] = (uint8)(value & 0xFF);
        } else if (r == UI_RET_EMPTY) {
            continue;
        } else {
            return r;
        }
    }
    return UI_RET_OK;    
}

#if CFG_SAL_LIB_SUPPORT_ENABLED
ui_ret_t
APIFUNC(ui_get_string)(char *pbytes, uint8 len, const char *str) REENTRANT
{
    char ch, last;
    uint8 ch_len = 0;
    if (pbytes == NULL) {
        return UI_RET_ERROR;
    }

    if (str != NULL) {
        sal_printf(str);
        last = str[sal_strlen(str) - 1];
    } else {
        last = ' ';
    }

    for(;;) {
        ch = sal_getchar();
        if (ch == UI_KB_LF || ch == UI_KB_CR) {
            if (!ch_len) {
                return UI_RET_EMPTY;
            }
            *(pbytes+ch_len) = '\0';
            break;
        } else if (ch == UI_KB_BS || ch == UI_KB_DEL) {
            if (ch_len > 0) {
                ch_len--;
                ui_backspace();
                sal_putchar(' ');
                ui_backspace();
            } else {
                ui_backspace();
                sal_putchar(last);
            }
            continue;
        } else if (ch_len == len) {
            /* Can only accept Backspace or ENTER */
            ui_backspace();
            sal_putchar(' ');
            ui_backspace();
            continue;
        }
        *(pbytes+ch_len) = ch;
        ch_len++;
    }
    return UI_RET_OK;
}


#ifdef CFG_XCOMMAND_INCLUDED
ui_ret_t
APIFUNC(ui_get_secure_string)(char *pbytes, uint8 len, const char *str) REENTRANT
{
    char ch, last;
    uint8 ch_len = 0;
    if (pbytes == NULL) {
        return UI_RET_ERROR;
    }

    if (str != NULL) {
        sal_printf(str);
        last = str[sal_strlen(str) - 1];
    } else {
        last = ' ';
    }

    for(;;) {
        ch = get_char();
        if (ch == UI_KB_LF || ch == UI_KB_CR) {
            sal_printf("\r\n");
            if (!ch_len) {
                return UI_RET_EMPTY;
            }
            
            *(pbytes+ch_len) = '\0';
            break;
        } else if (ch == UI_KB_BS || ch == UI_KB_DEL) {
            if (ch_len > 0) {
                ch_len--;
                ui_backspace();
                sal_putchar(' ');
                ui_backspace();
            } else {
                ui_backspace();
                sal_putchar(last);
            }
            continue;
        } else if (ch_len == len) {
            /* Can only accept Backspace or ENTER */
            ui_backspace();
            sal_putchar(' ');
            ui_backspace();
            continue;
        }
        put_char('*');
        *(pbytes+ch_len) = ch;
        ch_len++;
    }
    return UI_RET_OK;
}
#endif
#endif /* CFG_SAL_LIB_SUPPORT_ENABLED */
#endif /* CFG_CLI_ENABLED */
