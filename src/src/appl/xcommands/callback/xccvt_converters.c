/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <system.h>
#if defined(CFG_XCOMMAND_INCLUDED) || defined(CFG_VENDOR_CONFIG_SUPPORT_INCLUDED)


#include "xccvt_converters.h"



/*
 * Function:
 *      xc_uplist_from_string
 * Purpose:
 *      Convert logical port bitmap array into string, ASCII port number start from 1. Logical port bitmap starts from bit0.
 * Parameters:
 *      string : port list , etc 1, 3, 5-9
 *         len : length of string 
 *      prop: for XCMD_CORE
 *      puplist: user port bitmap list, each bit represents a port 
 * Returns:
 *      XCMD_ERR_xxx
 */

XCMD_ERROR 
xc_uplist_from_string(const char *string,unsigned int len, void *prop, uplist_t *puplist)
{
    unsigned int i;
    const char *endptr;
    int prev;

    uplist_t uplist;
        
    if (prop == NULL || string == NULL) {
        return XCMD_ERR_INTERNAL_INVALID_FUNC_PARAM;
    }

    sal_memset(&uplist, 0, sizeof(uplist));
    
    /* 
         *   Parse string to local upbmp_t 
         */
    prev = -1;

    for(i=0; i<len; i++) {
        
        unsigned int uport;
        
        /* skip spaces */
        while (i < len && string[i] == ' ') {
            i++;
        }
        if (i >= len) {
            break;
        }

        /* Get the port number */
        uport = SAL_NZUPORT_TO_UPORT(sal_strtoul(&string[i], &endptr, 10));

        if (endptr == &string[i]) {
            return XCMD_ERR_INVALID_COMMAND_INPUT;
        }

        
        if (SAL_UPORT_IS_NOT_VALID(uport)) {
            return XCMD_ERR_INVALID_COMMAND_INPUT;
        }

        /* Advance pointer to the first non-number one */
        i += (endptr - &string[i]);
        if (i > len) {
            return XCMD_ERR_INTERNAL_INVALID_ENGINE_ROUTE;
        }
        
        /* skip spaces */
        while (i < len && string[i] == ' ') {
            i++;
        }
        
        if (i == len || string[i] == ',') {

            if (prev != -1) {
                
                /* In continuos mode */
                int p;
                if ((int)uport < prev) {
                    return XCMD_ERR_INVALID_COMMAND_INPUT;
                }
                
                for(p = prev; p<=(int)uport; p++) {
                    uplist_port_add(uplist.bytes, p);
                }
                
                prev = -1;

            } else {
                
                /* Single port */
                uplist_port_add(uplist.bytes, uport);
            }
            
            if (i == len) {
                break;
            }
            
        } else if (string[i] == '-') {
            
            if (prev != -1) {
                /* already in continuous mode */
                return XCMD_ERR_INVALID_COMMAND_INPUT;
            }
            
            /* start continuous mode */
            prev = (int)uport;
            
        } else {
            return XCMD_ERR_INVALID_COMMAND_INPUT;
        }
    }
    
    if (prev != -1) {
        /* End in continuous mode */
        return XCMD_ERR_INVALID_COMMAND_INPUT;
    }

    if (puplist == NULL) {
         /* Means to do validation only */
        return XCMD_ERR_OK;
    }

    /* Copy data for user */
    sal_memcpy((void *)puplist, &uplist, sizeof(uplist));
    
    return XCMD_ERR_OK;
}

/*
 * Function:
 *      xc_uplist_to_string
 * Purpose:
 *      Convert port list string to logical port bitmap array, ASCII port number start from 1. Logical port bitmap starts from bit0.
 * Parameters:
 *      pstring : port list , etc 1, 3, 5-9
 *      prop: 
 *      puplist: user port bitmap list, each bit represents a port 
 * Returns:
 *      XCMD_ERR_xxx
 */

XCMD_ERROR 
xc_uplist_to_string(const uplist_t *puplist, void *prop,char **pstring)
{
    uint16 uport, prev;
    char *p;
    int cont;
    
    if (puplist == NULL) {
        return XCMD_ERR_INTERNAL_INVALID_FUNC_PARAM;
    }
    if (pstring == NULL) {
        return XCMD_ERR_OK;
    }

    *pstring = xcmd_alloc(64, "xc_uplist_to_string");

    if (*pstring == NULL) {
        return XCMD_ERR_OUT_OF_RESOURCE;
    }

    /* Convert to port list string */
    p = *pstring;
    cont = 0;
    prev = -1;
    SAL_UPORT_ITER(uport) {
        if (uplist_port_matched(puplist->bytes, uport) != SYS_OK) {
            continue;
        }
        if (prev == -1) {
            p += sal_sprintf(p, "%d", SAL_NZUPORT_TO_UPORT(uport));
        } else {
            if (uport == prev + 1) {
                if (!cont) {
                    p += sal_sprintf(p, "-");
                    cont = 1;
                }
            } else {
                if (cont) {
                    p += sal_sprintf(p, "%d", prev);
                    cont = 0;
                }
                p += sal_sprintf(p, ",%d", SAL_NZUPORT_TO_UPORT(uport));
            }
        }
        prev  = uport;
    }
    if (cont) {
        p += sprintf(p, "%d", SAL_NZUPORT_TO_UPORT(prev));
        cont = 0;
    }
    *p = 0;

    return XCMD_ERR_OK;
}

/*
 * Function:
 *      xc_pbmp_from_string
 * Purpose:
 *      Convert logical port bitmap array from string, ASCII port number start from 1. Logical port bitmap starts from bit0.
 * Parameters:
 *      string : port list , etc 1, 3, 5-9 is null terminated string
 *      prop: for XCMD_CORE
 *      puplist: user port bitmap list, each bit represents a port 
 * Returns:
 *      sys_error_t
 */

XCMD_ERROR xc_pbmp_from_string(const char *string, int len, void *prop, pbmp_t *pbmp) {


   int prev, i;
   pbmp_t local_pbmp;
   const char *endptr;

  
   PBMP_CLEAR(local_pbmp);

   endptr = &string[len];
   
   /* 
       *   Parse string to local upbmp_t 
       */
   prev = -1;

   for(i=0; i<len; i++) {
    
    unsigned int port;
    
    /* skip spaces */
    while (i < len && string[i] == ' ') {
         i++;
    }
    if (i >= len) {
        break;
    }

    /* Get the port number */
    port = sal_strtoul(&string[i], &endptr, 10);

    if (endptr == &string[i]) {
        return SYS_ERR_PARAMETER;
    }

    /* Advance pointer to the first non-number one */
    i += (endptr - &string[i]);
    if (i > len) {
        return XCMD_ERR_INTERNAL_INVALID_ENGINE_ROUTE;
    }
    
    /* skip spaces */
    while (i < len && string[i] == ' ') {
        i++;
    }
    
    if (i == len || string[i] == ',') {

        if (prev != -1) {
            
            /* In continuos mode */
            int p;
            if ((int)port < prev) {
                return XCMD_ERR_INVALID_COMMAND_INPUT;
            }
            
            for(p = prev; p<=(int)port; p++) {
                if (p < _SHR_PBMP_PORT_MAX) {
                    PBMP_PORT_ADD(local_pbmp, p);
                }
            }
            
            prev = -1;

        } else {
            
            if (port < _SHR_PBMP_PORT_MAX) {
                PBMP_PORT_ADD(local_pbmp, port);
            }

        }
        
        if (i == len) {
            break;
        }
        
    } else if (string[i] == '-') {
        
        if (prev != -1) {
            /* already in continuous mode */
            return XCMD_ERR_INVALID_COMMAND_INPUT;
        }
        
        /* start continuous mode */
        prev = (int)port;
        
    } else {
        return XCMD_ERR_INVALID_COMMAND_INPUT;
    }
}

if (prev != -1) {
    /* End in continuous mode */
    return XCMD_ERR_INVALID_COMMAND_INPUT;
}

   /* Copy data for user */
   sal_memcpy((void *)pbmp, &local_pbmp, sizeof(pbmp_t));

   return XCMD_ERR_OK;
}

/*
 * Function:
 *      pbmp_to_string
 * Purpose:
 *      Convert logical port bitmap array from string, ASCII port number start from 1. Logical port bitmap starts from bit0.
 * Parameters:
 *      string : port list , etc 1, 3, 5-9 is null terminated string
 *      prop: for XCMD_CORE
 *      puplist: user port bitmap list, each bit represents a port 
 * Returns:
 *      sys_error_t
 */
XCMD_ERROR xc_pbmp_to_string(const pbmp_t pbmp, void *prop, char **pstring) {


   int prev;

   char local_string[256];

   int port;
   char *p = local_string;
   int cont = 0;
    
   /* 
       *   Parse string to local upbmp_t 
       */
   prev = -1;

   PBMP_ITER(pbmp, port) {

       if (prev == -1) {
            p += sal_sprintf(p, "%d", port);
       } else {
            if (port == prev + 1) {
                if (!cont) {
                    p += sal_sprintf(p, "-");
                    cont = 1;
                }
            } else {
                if (cont) {
                    p += sal_sprintf(p, "%d", prev);
                    cont = 0;
                }
                p += sal_sprintf(p, ",%d", port);
            }
        }
        prev  = port;

   }
   if (cont) {
       if (prev < port) {
           p += sal_sprintf(p, "%d", prev);
       } else {
           p += sal_sprintf(p, "%d", port);
       }
	   cont = 0;
   }

   *p = 0;
    
       
   if (pstring != NULL && *pstring != NULL) {       
        /* Copy data for user */
        sal_strcpy(*pstring, local_string);       
   }
   return XCMD_ERR_OK;
}


/*
 * Port list converters
 */
const XCMD_TYPE_CONVERTER xccvt_port_list_converters = {
    (XCCONV_FROM_STRING) xc_uplist_from_string,
    (XCCONV_TO_STRING) xc_uplist_to_string,
    NULL,
    xccvt_common_free_string,
};

const XCMD_TYPE_CONVERTER xccvt_pbmp_converters = {
    (XCCONV_FROM_STRING) xc_pbmp_from_string,
    (XCCONV_TO_STRING) xc_pbmp_to_string,
    NULL,
    NULL,
};

#endif

