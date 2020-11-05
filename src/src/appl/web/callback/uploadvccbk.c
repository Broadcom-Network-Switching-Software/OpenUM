/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#include "appl/httpd.h"
#include "appl/ssp.h"
#include "utils/nvram.h"

/* Utility: show web page of error messages.
 *          Used only in SSP handlers. 
 *          Must return SSP_HANDLER_RET_MODIFIED after calling this.
 */
void webutil_show_error(SSP_HANDLER_CONTEXT *cxt, 
           SSP_PSMH psmem,
           const char *title,
           const char *message,
           const char *button,
           const char *action
           );
           
/* Common string used in webutil_show_error() */
extern const char err_button_retry[];

/*

      Callback Function of vendor configure upload page

*/ 

#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
static uint8 restore_in_progress = 0;
static uint32 restore_total_size;
static uint32 restore_current_received;

#ifdef HTTPD_TIMER_SUPPORT
STATICCBK void
restore_timer_reboot(void *in_data) REENTRANT
{
    httpd_delete_timers_by_callback(restore_timer_reboot);

    board_reset(in_data);
    
}
#endif /* HTTPD_TIMER_SUPPORT */
#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */

SSP_HANDLER_RETVAL 
ssphandler_vendor_config_upload_cgi(SSP_HANDLER_CONTEXT *cxt, SSP_PSMH psmem) REENTRANT
{
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
    SSP_HANDLER_CONTEXT_EXT *cxte = (SSP_HANDLER_CONTEXT_EXT *)cxt;
    unsigned char *pbuff = NULL;
    static BOOL    hard_reset;
    nvram_header_t *header;

    if (cxt->type == SSP_HANDLER_CLOSE) {
            
            hard_reset = TRUE;
            if (restore_in_progress == 2) {
#ifdef HTTPD_TIMER_SUPPORT
            httpd_create_timer(3, restore_timer_reboot, (void *)&hard_reset);
#else /* HTTPD_TIMER_SUPPORT */
                /* For no HTTPD timer case, the specific design must be added here 
                 *  to reset device 
                 */
#endif /* HTTPD_TIMER_SUPPORT */
            }
            
            restore_in_progress = 0;
            return SSP_HANDLER_RET_INTACT;
    }
    
    if (cxt->type != SSP_HANDLER_FILE_UPLOAD) {
            return SSP_HANDLER_RET_INTACT;
    }
    
        /* The first call to this handler is the indication of file length */
    if (cxte->url_data.upload.buf == NULL && cxte->url_data.upload.index == -1) {
               
            /* Check if someone has already started */
            if (restore_in_progress) {
              //  return show_error(cxt, psmem, ERROR_ALLOC_MUTEX);
              return SSP_HANDLER_RET_INTACT;
            }

            restore_total_size = cxte->url_data.upload.length;
             
            /* Allocate sector buffer */
            pbuff = (unsigned char *)ssputil_psmem_alloc(
                        psmem, 
                        ssphandler_vendor_config_upload_cgi, 
                        restore_total_size + 4
                        );
            if (pbuff == NULL) {
                sal_printf("Out of memory \n");
                webutil_show_error(
                     cxt, psmem,
                     "System",
                     "Out of memory. ",
                     err_button_retry,
                     "window.location.assign('restore.htm')");
                     /* We don't want to process it more */
                     /* cxt->flags = 0; */
                 return SSP_HANDLER_RET_MODIFIED;    
            }
            
            restore_in_progress = 1;

            restore_current_received = 0;

            return SSP_HANDLER_RET_INTACT;
            
     } else if (cxte->url_data.upload.buf == NULL && cxte->url_data.upload.length == 0) {

            /* Check if someone has already started */
            if (restore_in_progress == 0) {
              //  return show_error(cxt, psmem, ERROR_ALLOC_MUTEX);
              return SSP_HANDLER_RET_MODIFIED;
            }

            /* Get sector buffer previous allocated/updated */
            pbuff = (unsigned char *)ssputil_psmem_get(psmem, ssphandler_vendor_config_upload_cgi);
            
            /* Check whether it starts with NVRAM_MAGIC */
            header = (nvram_header_t *)pbuff;

            if (header->magic != NVRAM_MAGIC) {
                webutil_show_error(
                    cxt, psmem,
                    "System",
                    "Wrong file format.",
                    err_button_retry,
                    "window.location.assign('upload_vc.htm')");
                    /* We don't want to process it more */
                    /* cxt->flags = 0; */
                    return SSP_HANDLER_RET_MODIFIED;    
            }

            /* Write it to the flash */
            if (flash_erase((hsaddr_t)NVRAM_BASE , NVRAM_SPACE) < 0) {
                ssputil_psmem_free(psmem, ssphandler_vendor_config_upload_cgi);
                //return show_error(cxt, psmem, ERROR_WRITE_FAIL);

                restore_in_progress = 0;                
                return SSP_HANDLER_RET_MODIFIED;
            }
            if (flash_program((hsaddr_t)NVRAM_BASE, (const void*)pbuff, NVRAM_SPACE) < 0) {
                ssputil_psmem_free(psmem, ssphandler_vendor_config_upload_cgi);
                //return show_error(cxt, psmem, ERROR_WRITE_FAIL);

                restore_in_progress = 0;                
                return SSP_HANDLER_RET_MODIFIED;
            }

            if (flash_erase((hsaddr_t)MEDIUM_FLASH_START_ADDRESS , MEDIUM_FLASH_SIZE) < 0) {
                ssputil_psmem_free(psmem, ssphandler_vendor_config_upload_cgi);
                //return show_error(cxt, psmem, ERROR_WRITE_FAIL);

                restore_in_progress = 0;                
                return SSP_HANDLER_RET_MODIFIED;
            }

            ssputil_psmem_free(psmem, ssphandler_vendor_config_upload_cgi);
                
            /* Mark it as it'd completed successfully */
            restore_in_progress = 2;

            return SSP_HANDLER_RET_INTACT;   
      } else {
    
             /* Get sector buffer previous allocated/updated */
             pbuff = (unsigned char *)ssputil_psmem_get(psmem, ssphandler_vendor_config_upload_cgi);

             /* Check if someone has already started */
             if (restore_in_progress == 0) {
                 return SSP_HANDLER_RET_MODIFIED;
             }

             sal_memcpy(pbuff + cxte->url_data.upload.index, cxte->url_data.upload.buf, (size_t)cxte->url_data.upload.length);              
    
             restore_current_received += cxte->url_data.upload.length;

             pbuff[restore_current_received] = 0;
     }       
#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED*/
    
     return SSP_HANDLER_RET_INTACT;

}


