/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _UTILS_UI_H_
#define _UTILS_UI_H_

/* Keyboard codes */
#define UI_KB_ESC       0x1B
#define UI_KB_LF        0x0A
#define UI_KB_CR        0x0D
#define UI_KB_BS        0x08
#define UI_KB_CTRL_C    0x03
#define UI_KB_DEL       0x7F

typedef enum {
    UI_RET_OK,     /* OK with input data */
    UI_RET_EMPTY,  /* ENTER pressed w/o data (i.e., using suggested value) */
    UI_RET_CANCEL, /* Cancelled by ESC or Ctrl-C */
    UI_RET_ERROR   /* Error occurs, usually invalid parameters */
} ui_ret_t;


/*  platform dependent C library IO function */
extern char put_char(char c);
extern char get_char(void);


/* Input hex number: byte (8bit), word (16bit), dword (32bit) with prompt */
extern ui_ret_t ui_get_byte(uint8 *val, const char *str) REENTRANT;
extern ui_ret_t ui_get_word(uint16 *val, const char *str) REENTRANT;
extern ui_ret_t ui_get_dword(uint32 *val, const char *str) REENTRANT;

/* Input address in hex */
extern ui_ret_t ui_get_address(uint8 **paddr, const char *str) REENTRANT;

/* Input a uint32 in decimal (str must not be NULL or "") */
extern ui_ret_t ui_get_decimal(uint32 *pvalue, const char *str) REENTRANT;

/* Dump memory as bytes in hex (can be cancelled using ESC or Ctrl-C) */
extern void ui_dump_memory(uint8 *addr, uint16 len) REENTRANT;

/* Backspace (go back and delete one character) */
extern void ui_backspace(void) REENTRANT;

/* Input heximal number */
extern ui_ret_t ui_get_hex(uint32 *value, uint8 size) REENTRANT;

/* Input a fixed number of bytes. 
 * Set show_org to TRUE to show original data as default.
 * Note: Even if user cancels, previous inputed bytes are still updated.
 */
extern ui_ret_t ui_get_bytes(uint8 *pbytes, 
                             uint8 len, 
                             const char *str, 
                             BOOL show_org) REENTRANT;

/* Input yes or no. 
   Suggested is the default answer when pressing ENTER:
     0: default No
     1: default Yes
     2: No default answer (must enter 'y' or 'n') 
   Return value: TRUE - yes, FALSE - no
 */
extern BOOL ui_yes_or_no(const char *str, uint8 suggested) REENTRANT;

/* Input a string with maximum number of bytes. */
extern ui_ret_t ui_get_string(char *pbytes, uint8 len, const char *str) REENTRANT;

/* Input a secure string with maximum number of bytes. */
extern ui_ret_t ui_get_secure_string(char *pbytes, uint8 len, const char *str) REENTRANT;

#endif /* _UTILS_UI_H_ */
