/*
 * $Id: intr.c,v 1.4 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifdef __C51__
#ifdef CODE_USERCLASS
#pragma userclass (code = krnintr)
#endif /* CODE_USERCLASS */
#endif /* __C51__ */

#include <system.h>

#ifdef CFG_INTR_INCLUDED

/*****************************************
 *   Local variable.
 */
static sys_intr_drv_t *intr_drv = NULL;

/*****************************************
 *   Public Function.
 */
sys_error_t
sys_intr_drv_init(sys_intr_drv_t *drv)
{
    intr_drv = drv;
    return SYS_OK;
}

sys_error_t
sys_intr_entry(void)
{

   /* Check if device-specific interrupt callback is installed. */
   if (intr_drv == NULL ||
       intr_drv->intr_entry == NULL)
   {
       return SYS_ERR;
   }

   return intr_drv->intr_entry(0);
}

sys_error_t
sys_intr_func_set(int intrnum, sys_intr_f fn, uint32 param)
{
    /* Check if device-specific interrupt callback is installed. */
    if (intr_drv == NULL ||
        intr_drv->intr_func_set == NULL)
    {
        return SYS_ERR;
    }

    return intr_drv->intr_func_set(0, intrnum, fn, param);
}

sys_error_t
sys_intr_enable_set(int intrnum, int enable)
{
    /* Check if device-specific interrupt callback is installed. */
    if (intr_drv == NULL ||
        intr_drv->intr_enable_set == NULL)
    {
        return SYS_ERR;
    }

    return intr_drv->intr_enable_set(0, intrnum, enable);
}

sys_error_t
sys_intr_enable_get(int intrnum, int *enable) {

    /* Check if device-specific interrupt callback is installed. */
    if (intr_drv == NULL ||
        intr_drv->intr_enable_get == NULL ||
        enable == NULL)
    {
        return SYS_ERR;
    }

    return intr_drv->intr_enable_get(0, intrnum, enable);
}

sys_error_t
sys_intr_handling_save_disable(uint32 *status)
{
    /* Check if device-specific interrupt callback is installed. */
    if (intr_drv == NULL ||
        intr_drv->intr_handling_save_disable == NULL)
    {
        return SYS_ERR;
    }

    return intr_drv->intr_handling_save_disable(0, status);
}

sys_error_t
sys_intr_handling_restore_enable(uint32 *status)
{
    /* Check if device-specific interrupt callback is installed. */
    if (intr_drv == NULL ||
        intr_drv->intr_handling_restore_enable == NULL)
    {
        return SYS_ERR;
    }

    return intr_drv->intr_handling_restore_enable(0, status);
}

#endif /* CFG_INTR_INCLUDED */
