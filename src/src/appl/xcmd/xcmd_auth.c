/*
 * $Id: xcmd_auth.c,v 1.4 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"

#if defined(CFG_XCOMMAND_INCLUDED) || defined(CFG_VENDOR_CONFIG_SUPPORT_INCLUDED)

#include "appl/xcmd/xcmd_internal.h"
#include "xcmd_core.h"
#include "xc_input_cli.h"


#define XCMD_USER_NAME_MAX 16
#define XCMD_PASSWORD_MAX  16
#define XCMD_MAX_USER      4

typedef struct {
    char user[XCMD_USER_NAME_MAX+1];    
    char password[XCMD_PASSWORD_MAX+1];
    unsigned int flags;
} XCAUTH_ENTRY;

/*
*
*   Variables
 */
static const XCAUTH_ENTRY xcli_auth_guest = {
       "guest",
       "",
       (XCFLAG_ACCESS_GUEST)       
};

static const XCAUTH_ENTRY xcli_auth_admin = {
       "admin",
       "password",
       (XCFLAG_ACCESS_PRIVILEGED)
};

    
static XCAUTH_ENTRY *xcli_auth_table[XCMD_MAX_USER+1];
static char auth_table_init = 0;


/*
 * Function:
 *      xcli_auth_table_init
 * Purpose:
 *       XCMD INTERNAL : default user and password table initialization 
 * Parameters:
 *      
 *      
 * Returns:
 *      XCMD_ERR_OK: successful. 
 */


static XCMD_ERROR xcli_auth_table_init(void) {

  int i;

  if (auth_table_init) return XCMD_ERR_OK;
  
  xcli_auth_table[0] = (XCAUTH_ENTRY *) &xcli_auth_guest;
  xcli_auth_table[1] = (XCAUTH_ENTRY *) &xcli_auth_admin;


  // TO-DO: this may get from serializer  
  for (i=2 ; i< XCMD_MAX_USER; i++) {
       xcli_auth_table[i] = NULL;
  }

  auth_table_init = 1;
  
  return XCMD_ERR_OK;
}



/*
 * Function:
 *      xcli_auth_check
 * Purpose:
 *       XCMD INTERNAL : Authority check between user permission level (flags) and session permission level 
 * Parameters:
 *      ps: session control structure
 *      flags : the permission level of user
 * Returns:
 *      XCMD_ERR_OK: auth check successful. 
 *      XCMD_ERR_XXX: fail 
 */
 
unsigned int   
xcli_auth_login(const char *user, const char* password) {

  int i;
  unsigned int flags;

  xcli_auth_table_init();

  flags = XCFLAG_ACCESS_GUEST;
    
  for (i=0; i<XCMD_MAX_USER; i++) {
    
        if (xcli_auth_table[i] == NULL) {
            break;
        }
        
        if (sal_strcmp(user, xcli_auth_table[i]->user) == 0) {

            if (sal_strlen(xcli_auth_table[i]->password) == 0) {
                flags = xcli_auth_table[i]->flags;
                return flags;
            }

            if (sal_strncmp(password, xcli_auth_table[i]->password, sal_strlen(xcli_auth_table[i]->password)) == 0){
                flags = xcli_auth_table[i]->flags;
                return flags;
            }
        }

  }  
  return XCMD_ERR_OK;
}


/*
 * Function:
 *      xcli_auth_check
 * Purpose:
 *       XCMD INTERNAL : Authority check between user permission level (flags) and session permission level 
 * Parameters:
 *      ps: session control structure
 *      flags : the permission level of user
 * Returns:
 *      XCMD_ERR_OK: auth check successful. 
 *      XCMD_ERR_XXX: fail 
 */
 
XCMD_ERROR
xcli_auth_check(XCEXE_SESSION *ps, unsigned int flags) {

   XCIN_CLI *in = (XCIN_CLI *) ps->input;

   uint32 session_flags = in->flags;
   
   if (session_flags &  XCFLAG_ACCESS_PRIVILEGED) {
       return XCMD_ERR_OK;
   }
   
   if ((session_flags &  XCFLAG_ACCESS_NORMAL) && ((flags & XCFLAG_ACCESS_PRIVILEGED) == 0)) {       
       return XCMD_ERR_OK;
   }
    
    if ((session_flags &  XCFLAG_ACCESS_GUEST) && ((flags & (XCFLAG_ACCESS_PRIVILEGED | XCFLAG_ACCESS_NORMAL)) == 0)) {       
        return XCMD_ERR_OK;
    }

    return XCMD_ERR_INTERNAL_INVALID_PRIVILEGE_LEVEL;
}

/*
 * Function:
 *      xcli_auth_adduser
 * Purpose:
 *      XCMD API:  add user in privilege context 
 * Parameters:
 *      user : exist user name 
 *      password : password of corresponding user 
 *      access : the permission level of user, "privilege", "normal" or "guest"
 * Returns:
 *      XCMD_ERR_OK: add user successful. 
 *      XCMD_ERR_XXX: fail 
 */

XCMD_ERROR
xcli_auth_adduser(const char *user, const char* password, const char *access) {

     int i;
     const unsigned int mask = XCFLAG_ACCESS_PRIVILEGED | XCFLAG_ACCESS_NORMAL | XCFLAG_ACCESS_GUEST;

     XCAUTH_ENTRY *entry = NULL;
     unsigned int flags = 0;
     xcli_auth_table_init();

     if (sal_strcmp(access, "privilege") == 0) {

         flags = XCFLAG_ACCESS_PRIVILEGED;

     } else if (sal_strcmp(access, "normal") == 0) {

         flags = XCFLAG_ACCESS_NORMAL;

     } else if (sal_strcmp(access, "guest") == 0) {

         flags = XCFLAG_ACCESS_GUEST; 

     } else {
     
        return XCMD_ERR_FALSE;
     }

     /* search for free account */   
     for (i=0; i<XCMD_MAX_USER; i++) {

         if ((sal_strcmp(xcli_auth_table[i]->user, user) == 0)) {
             sal_printf("XCMD add user fail: %s existed\n", user);
             return XCMD_ERR_FALSE;
         }
         
         if (xcli_auth_table[i] == NULL) {
             break;
         }

     }

     /* There is no free accont to add*/
     if (i == XCMD_MAX_USER) {
         return XCMD_ERR_FALSE;
     }

     /* Sanity check */ 
     if ((flags & mask) == 0) {
         return XCMD_ERR_FALSE;
     }

     if (user == NULL || password == NULL) {
         return XCMD_ERR_FALSE;
     }
     if (sal_strlen(user) > 0 && sal_strlen(user) < XCMD_USER_NAME_MAX && sal_strlen(password) < XCMD_PASSWORD_MAX) {
         sal_printf("Length check error: user name ( 1~16 characters ) , password (0~16 character)\n");
         return XCMD_ERR_FALSE;
     }

     entry = xcmd_alloc(sizeof(XCAUTH_ENTRY), "auth entry");

     if (entry == NULL) {
         sal_printf("Out of memory \n");
         return XCMD_ERR_FALSE;
     }

     /* add user name and password*/
     entry->flags = (flags & mask);
     sal_strcpy(entry->password, password);
     sal_strcpy(entry->user, user);
     
     xcli_auth_table[i] = entry;
     
     return XCMD_ERR_OK;

}


/*
 * Function:
 *      xcli_auth_deluser
 * Purpose:
 *      delete exist user in privilege context
 * Parameters:
 *      user : exist user name 
 *      password : password of corresponding user 
 *      
 * Returns:
 *      XCMD_ERR_OK: delete user successful. 
 *      XCMD_ERR_XXX: fail 
 */

XCMD_ERROR
xcli_auth_deluser(const char *user, const char* password) {

     int i;

     xcli_auth_table_init();

     /* search for free account */   
     for (i=0; i<XCMD_MAX_USER; i++) {

         if (xcli_auth_table[i] == NULL) {
             return XCMD_ERR_FALSE;
         }

         if ((sal_strcmp(xcli_auth_table[i]->user, user) == 0) && 
             ((sal_strcmp(xcli_auth_table[i]->password, password) == 0) || 
              (sal_strlen(xcli_auth_table[i]->password) == 0))) {

              if (i < 2) {
                  sal_printf("default user: admin and guest can not be delete\n");
                  return XCMD_ERR_FALSE;
              }

              /* pop up the entry from stack */ 
              for (;i<(XCMD_MAX_USER-1); i++) {
                   xcli_auth_table[i] = xcli_auth_table[i+1];
              }
              return XCMD_ERR_OK;

         }

     }

     return XCMD_ERR_FALSE;

}

/*
 * Function:
 *      xcli_auth_change_password
 * Purpose:
 *      renew password of exist user in privilege context
 * Parameters:
 *      user : exist user name 
 *      old_password : old password
 *      new_password: new password 
 * Returns:
 *      XCMD_ERR_OK: change password successful. 
 *      XCMD_ERR_XXX: fail 
 */

XCMD_ERROR
xcli_auth_change_password(const char *user, const char* old_password, const char *new_password) {

     int i;

     xcli_auth_table_init();

     /* search for free account */   
     for (i=0; i<XCMD_MAX_USER; i++) {

         if (xcli_auth_table[i] == NULL) {
             return XCMD_ERR_FALSE;
         }

         if ((sal_strcmp(xcli_auth_table[i]->user, user) == 0) && 
             ((sal_strcmp(xcli_auth_table[i]->password, old_password) == 0) || 
              (sal_strlen(xcli_auth_table[i]->password) == 0))) {

              if (sal_strlen(new_password) >= XCMD_PASSWORD_MAX) {
                  return XCMD_ERR_FALSE;
              }
              
              sal_strcpy(xcli_auth_table[i]->password, new_password);

              return XCMD_ERR_OK;
         }
     }

     return XCMD_ERR_FALSE;

}

#endif /* CFG_XCOMMAND_INCLUDED */
