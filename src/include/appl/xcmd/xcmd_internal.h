/*
 * $Id: xcmd_internal.h,v 1.4 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _XCMD_INTERNAL_H_
#define _XCMD_INTERNAL_H_

#include "system.h"

#if defined(CFG_XCOMMAND_INCLUDED) || defined(CFG_VENDOR_CONFIG_SUPPORT_INCLUDED)

#include "xcmd_public.h"

/****************************************************************
 *                                                              *
 * Configurations                                               *
 */
 
#define DEFAULT_CFGFILE_COMMENT_BEGIN       '!'
#define DEFAULT_CFGFILE_DEPTH_CHAR          ' '


#define MAX_PARAM_STACK_SIZE            (32)
#define DEFAULT_OUTPUT_INVALID_PARAM    "<param>"
#define DEFAULT_COMMENT_LINE_PREFIX     '!'
#define DEFAULT_MARKED_LINE_PREFIX      '?'

/********************************************************
*
*
*  Displaying page
 */

typedef enum {

  XCMD_DISP_PAGES_STATE_DISABLE, 
  XCMD_DISP_PAGES_STATE_NORMAL_OUTPUT, 
  XCMD_DISP_PAGES_STATE_BUFFER_OUTPUT, 
  XCMD_DISP_PAGES_STATE_SEEK, 
  XCMD_DISP_PAGES_STATE_DRAIN,
  XCMD_DISP_PAGES_STATE_CANCEL,

} XCMD_DISP_PAGES_STATE;

#define XCMD_DISP_PAGE_COLS_PER_ROW 80
#define XCMD_DISP_PAGE_ROWS_PER_PAGE 30
#define XCMD_DISP_PAGE_SIZE (XCMD_DISP_PAGE_COLS_PER_ROW*XCMD_DISP_PAGE_ROWS_PER_PAGE)

typedef struct XCMD_DISP_BUFFER {
    char content [XCMD_DISP_PAGE_SIZE];
    int  cur_pos;
    struct XCMD_DISP_BUFFER *next;
    struct XCMD_DISP_BUFFER *prev; 
} XCMD_DISP_BUFFER;


typedef struct XCMD_DISP_PAGE {
  char *page_buffer;
  char *page_start;
  int  remain;
  int  last_pos;
  struct XCMD_DISP_PAGE *next;
  struct XCMD_DISP_PAGE *prev;
  XCMD_DISP_BUFFER *start;
  XCMD_DISP_BUFFER *end;  
} XCMD_DISP_PAGE;



  
  typedef struct {
     XCMD_DISP_PAGES_STATE state;
     XCMD_DISP_PAGE *pages;
     XCMD_DISP_BUFFER *buf_head;
     XCMD_DISP_BUFFER buf;
     int max_cols_per_row;
     int max_rows;
     int cols; /* start from 1 */
     int rows;
     int has_cr;
     int non_stop;
     int page_no;
     int target_page_no;
     void *disp_start;
     XCMD_DISP_PAGE *page;
     
  } XCMD_DISP_PAGES_CONTEXT;





/****************************************************************
 *                                                              *
 * Per Connection-wide                                                 *
 */
typedef enum {
        XCFLAG_ACCESS_GUEST       = 0x00000001,
        XCFLAG_ACCESS_NORMAL      = 0x00000002,
        XCFLAG_ACCESS_PRIVILEGED  = 0x00000004,
        XCFLAG_CMDTYPE_CONFIG     = 0x00000010,
        XCFLAG_CMDTYPE_SHOW       = 0x00000020,
        XCFLAG_CMDTYPE_UPDATE     = 0x00000040,
        XCFLAG_CMDTYPE_OPERATE    = 0x00000080,
        XCFLAG_BEHAVIOR_MARKED    = 0x00010000,
        XCFLAG_OPTIONAL_TAG_GENERAL_MARKED      = 0x00100000,
        XCFLAG_OPTIONAL_TAG_WO_TK_MATCH_CHECK   = 0x00200000,
} XCMD_CMDFLAGS;


typedef enum {
    XCMDI_OP_EXECUTE = 0,
    XCMDI_OP_FINDMATCH,
    XCMDI_OP_COMPLETE,
    XCMDI_OP_BUILD,
    XCMDI_OP_COUNT, /* Must be the last */
} XCMDI_OP;


typedef struct {
    XCMD_ERROR (*open)(void *);
    XCMD_ERROR (*write_str)(void *, const char *, unsigned int);
    XCMD_ERROR (*write_cr)(void *);
    XCMD_ERROR (*report_error)(void *, XCMD_ERROR, const char *);
    XCMD_ERROR (*close)(void *);
} XCMD_OUTPUT;

typedef struct {
    XCMD_ERROR (*open)(void *, const char *);
    XCMD_ERROR (*get_line)(void *, const char **, unsigned int *, XCMDI_OP *);
    XCMD_ERROR (*report_error)(void *, XCMD_ERROR, const char *, unsigned int);
    XCMD_ERROR (*close)(void *);
} XCMD_INPUT;


/****************************************************************
 *                                                              *
 * Context-wide                                                 *
 */
 
typedef void *XCMD_CXT_HANDLE;


/****************************************************************
 *                                                              *
 * Node type handlers                                           *
 */
 
typedef struct {
    int id;
    const char *string;
    unsigned int len;
    const XCMD_TYPE_CONVERTER *converter;
    void *properties;
    void *value;
    int visited;
} XCEXE_PARAM;

typedef void *XCEXE_PARAM_STACK;

typedef struct {
    
    
    int type;
    XCMD_CMDFLAGS optional_tag_flag;
    
    /* command line */
    int op;
    char *line;
    unsigned int maxlen;
    unsigned int curpos;
    unsigned int buflen;
    
    /* Param stack */
    XCEXE_PARAM_STACK params;
    
    /* Error */
    XCMD_ERROR error_code;
    char *error_desc;
    unsigned int free_err_desc;   /* whether to free it or not */
    unsigned int errpos;
    
    /* Context scope */
    XCMD_CXT_HANDLE context;

    /* Display page context */
    XCMD_DISP_PAGES_CONTEXT *disp;

    /* Per connection(console) properties */
    XCMD_INPUT * input;
    
} XCEXE_SESSION;


typedef struct {
    int id;
    char *string;
    XCMD_TYPE_CONVERTER *converter;
    void *properties;
} XCGEN_PARAM;

typedef void *XCGEN_PARAM_STACK;

typedef struct {

    
    int type;
    
    /* Output stream to write commands */
    void *pstream;
    
    /* Param stack */
    XCGEN_PARAM_STACK params;
    
    /* Error */
    XCMD_ERROR error_code;
    
    /* Context scope */
    XCMD_CXT_HANDLE context;

} XCGEN_SESSION;

typedef XCMD_ERROR (*XCHANDLER_EXECUTE)(XCEXE_SESSION *ps, void *properties);
typedef XCMD_ERROR (*XCHANDLER_COMPLETE)(XCEXE_SESSION *ps, void *properties);
typedef XCMD_ERROR (*XCHANDLER_LIST)(XCEXE_SESSION *ps, void *properties);
typedef XCMD_ERROR (*XCHANDLER_BUILD)(XCGEN_SESSION *ps, void *properties);
typedef XCMD_ERROR (*XCHANDLER_AUTH)(XCGEN_SESSION *ps, void *properties); 
typedef struct {
    XCHANDLER_EXECUTE execute;
    XCHANDLER_COMPLETE complete;
    XCHANDLER_LIST list;
    XCHANDLER_BUILD build;
    XCHANDLER_AUTH auth;
} XCMD_NODE_HANDLERS;

extern XCMD_NODE_HANDLERS xcnode_handler_select;
extern XCMD_NODE_HANDLERS xcnode_handler_optional;
extern XCMD_NODE_HANDLERS xcnode_handler_cmd_eol;
extern XCMD_NODE_HANDLERS xcnode_handler_cmd_keyword;
extern XCMD_NODE_HANDLERS xcnode_handler_param_eol;
extern XCMD_NODE_HANDLERS xcnode_handler_param_keyword;
extern XCMD_NODE_HANDLERS xcnode_handler_param_word;
extern XCMD_NODE_HANDLERS xcnode_handler_param_line;
extern XCMD_NODE_HANDLERS xcnode_handler_param_integer;
extern XCMD_NODE_HANDLERS xcnode_handler_param_custom;
extern XCMD_NODE_HANDLERS xcnode_handler_param_options;
extern XCMD_NODE_HANDLERS xcnode_handler_option;


/****************************************************************
 *                                                              *
 * Callbacks                                                    *
 */
 
typedef struct {
    XCMD_HANDLER handler;
    XCMD_BUILDER builder;
} XCMD_CALLBACKS;


/****************************************************************
 *                                                              *
 * Node types                                                   *
 */
 
typedef struct {
    const XCMD_NODE_HANDLERS * const handlers;
    const void * const properties;
} XCMD_NODE_PTR;
 
typedef struct {
    const char * const name;
    const XCMD_NODE_PTR next_node;
} XCNODE_CONTEXT;

typedef struct {
    const XCMD_NODE_PTR *children;
    const unsigned int count;
} XCNODE_SELECT;

typedef struct {
    const XCMD_NODE_PTR *children;
    const unsigned int count;
} XCNODE_OPTIONAL;

typedef struct {
    const char * const name;
    const char * const desc;
    const XCMD_CMDFLAGS flags;
    const XCMD_NODE_PTR next_node;
} XCNODE_CMD_KEYWORD;

typedef struct {
    const int path;
    const XCMD_CMDFLAGS flags;
    const XCMD_CALLBACKS callbacks;
    const XCNODE_CONTEXT * const sub_context;
} XCNODE_CMD_EOL;

typedef struct {
    const char * const name;
    const char * const desc;
    const XCMD_NODE_PTR next_node;
} XCNODE_PARAM_KEYWORD;

typedef struct {
    const int id;
    const char * const name;
    const char * const desc;
    unsigned int minlen;
    unsigned int maxlen;
    const int memtype; /* 1 for stack; 0 for allocated */
    const XCMD_NODE_PTR next_node;
} XCNODE_PARAM_WORD;

typedef struct {
    const int id;
    const char * const name;
    const char * const desc;
    unsigned int minlen;
    unsigned int maxlen;
    const int memtype; /* 1 for stack; 0 for allocated */
    const XCMD_NODE_PTR next_node;
} XCNODE_PARAM_LINE;

typedef struct {
    const int id;
    const char * const name;
    const char * const desc;
    unsigned int min;
    unsigned int max;
    int bsigned;
    unsigned int radix;
    const XCMD_NODE_PTR next_node;
} XCNODE_PARAM_INTEGER;

typedef struct {
    const int id;
    const XCMD_NODE_PTR *options;
    const unsigned int count;
    const XCMD_NODE_PTR next_node;
} XCNODE_PARAM_OPTIONS;

typedef struct {
    const char * const name;
    const char * const desc;
    const int value;
} XCNODE_OPTION;

typedef struct {
    const int id;
    const char * const name;
    const char * const desc;
    const XCMD_TYPE_CONVERTER *converters;
    const int format; /* 1 for LINE format; 0 for WORD */
    const XCMD_NODE_PTR next_node;
} XCNODE_PARAM_CUSTOM;

typedef struct {
    const int path;
    const XCMD_CMDFLAGS flags;
    const XCMD_CALLBACKS callbacks;
    const XCNODE_CONTEXT * const sub_context;
} XCNODE_PARAM_EOL;

/******************************************************
*
*
*  Authetication
 */
extern unsigned int xcli_auth_login(const char *user, const char* password);
extern XCMD_ERROR xcli_auth_check(XCEXE_SESSION *ps, unsigned int flags);

/******************************************************
*
*
*  platform dependent C library IO function 
 */

extern char put_char(char c);
extern char get_char(void);

/******************************************************
*
*
*  lagcy platform dependent C library IO function 
 */

extern int um_console_print(const char *str);                                            
extern int um_console_write(unsigned char *buffer,int length);

#endif /* CFG_XCOMMAND_INCLUDED */

#endif /* _XCMD_INTERNAL_H_ */
