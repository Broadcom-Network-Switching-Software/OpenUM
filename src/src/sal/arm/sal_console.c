/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <stdarg.h>

#include "system.h"
#include "bsp_config.h"
#include "ns16550.h"

#define UART_READREG(r)    SYS_REG_READ8((CFG_UART_BASE+(r)))
#define UART_WRITEREG(r,v) SYS_REG_WRITE8((CFG_UART_BASE+(r)), v)

#if CFG_CONSOLE_ENABLED
STATIC char lastchar;
#endif /* CFG_CONSOLE_ENABLED */

extern void sal_console_init(void);
extern void sal_debugf(const char *fmt, ...);

#if CFG_CONSOLE_ENABLED

/*  *********************************************************************
    *  C library platform denpent uart driver 
    *
    *  put_char and get_char 
 */

char
put_char(char c) {

   while ((UART_READREG(R_UART_LSR) & LSR_TXRDY) == 0) {
         ;
   };
   UART_WRITEREG(R_UART_DATA, c);

   return c;   
}

char get_char(void){

  char c;

  do {
     POLL();
  } while (!(UART_READREG(R_UART_LSR) & LSR_RXRDY));

  c = (char)(UART_READREG(R_UART_DATA) & 0xFF);

  return c;
}

/*  *********************************************************************
    *  um_console_write()
    *
    *  Console output function
 */

int 
um_console_write(const char *buffer,int length)
{
    int blen = length;
    const char *bptr = buffer;
    while (blen > 0) {            
        put_char(*bptr);
        bptr++;
        blen--;
    }
    return 0;
}



/*  *********************************************************************
    *  um_console_print()
    *
    *  Console output function
 */
int
um_console_print(const char *str)
{
    int count = 0;
    int len;
    char *p;

    /* Convert CR to CRLF as we write things out */
    while ((p = sal_strchr(str,'\n'))) {
        um_console_write(str,p-str);
        um_console_write("\r\n",2);
        count += (p-str);
        str = p + 1;
    }

    len = sal_strlen(str);
    um_console_write(str, len);
    count += len;

    return count;
}

void
sal_console_init(void)
{

}

#endif /* CFG_CONSOLE_ENABLED */

void
sal_printf(const char *fmt, ...)
{
#if CFG_CONSOLE_ENABLED
    va_list arg_ptr;
    char buf[256];

    va_start(arg_ptr, fmt);
    vsprintf(buf, fmt, arg_ptr);
    va_end(arg_ptr);

    um_console_print(buf);
#else
    UNREFERENCED_PARAMETER(fmt);
#endif
}


int
cdk_printf(const char *fmt, ...)
{
#if CFG_CONSOLE_ENABLED
    va_list arg_ptr;
    char buf[256];

    va_start(arg_ptr, fmt);
    vsprintf(buf, fmt, arg_ptr);
    va_end(arg_ptr);

    um_console_print(buf);
#else
    UNREFERENCED_PARAMETER(fmt);
#endif
    return SYS_OK;
}

void
sal_debugf(const char *fmt, ...)
{
#if CFG_CONSOLE_ENABLED
    va_list arg_ptr;
    char buf[256];

    va_start(arg_ptr, fmt);
    vsprintf(buf, fmt, arg_ptr);
    va_end(arg_ptr);

    um_console_print(buf);
#endif /* CFG_CONSOLE_ENABLED */
}
void
sal_assert(const char *expr, const char *file, uint16 line) REENTRANT
{
#if CFG_CONSOLE_ENABLED
    sal_printf("ERROR: Assertion failed: (%s) at %s:%u\n", expr, file, line);
#endif /* CFG_CONSOLE_ENABLED */
    for(;;);
}
BOOL
sal_char_avail(void)
{
#if CFG_CONSOLE_ENABLED
    return (UART_READREG(R_UART_LSR) & LSR_RXRDY) ? 1 : 0;
#else
    return FALSE;
#endif
}

char
sal_getchar(void)
{
#if CFG_CONSOLE_ENABLED

    lastchar = get_char();
    switch (lastchar) {
        case 0x7f:
        case '\b':
            break;
        case '\r':
        case '\n':
            um_console_write("\r\n",2);
            break;
        default:
            if (lastchar >= ' ') {
                um_console_write(&lastchar,1);
            }
            break;
    }

    return lastchar;
#else
    for(;;) {
        POLL();
    }
    return 0;
#endif
}


char
sal_get_last_char(void)
{
#if CFG_CONSOLE_ENABLED
    return lastchar;
#else
    return 0;
#endif
}

char
sal_putchar(char c)
{
#if CFG_CONSOLE_ENABLED
    switch (c) {
        case '\b':
            um_console_write("\b \b",3);
        break;
        case '\r':
        case '\n':
            um_console_write("\r\n",2);
        break;
        default:
            um_console_write(&c,1);
        break;
    }
#endif
    return c;
}

