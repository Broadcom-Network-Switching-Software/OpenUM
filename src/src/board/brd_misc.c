/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifdef __C51__
#ifdef CODE_USERCLASS
#pragma userclass (code = brdimpl)
#endif /* CODE_USERCLASS */
#endif /* __C51__ */

#include "system.h"
#include "utils/ports.h"
#include "appl/persistence.h"

#ifdef _BCM95333X_
#include "soc/bcm5333x.h"
#endif

/*
 * Typical implementation of certain board functions
 *
 * Note: Only applicable for single-unit board.
 */

#if defined(CFG_HW_CABLE_DIAG_INCLUDED)
/* Cable diag result */
#define     CD_NO_FAULT         0x0000 /* Good cable/PCB signal paths, but no Gigabit link */
#define     CD_PIN_SHORT_OR_XT  0x0001 /* Pin-short or cross-talk along 2 or more cable/PCB signal paths */
#define     CD_OPEN             0x0002 /* One or both pins are open for a twisted pair */
#define     CD_SHORT            0x0004 /* Two pins from the same twisted pair are shorted together */
#define     CD_FORCED           0x0008 /* Persistent noise present (most likely caused by Forced 10/100) */
#define     CD_GLINK            0x0010 /* Gigabit link is up and running */
#define     CD_NO_CABLE         0x0020 /* No cable connected */
#endif /* CFG_HW_CABLE_DIAG_INCLUDED */


#ifdef CFG_PCM_SUPPORT_INCLUDED
sys_error_t
_port_cable_diag(uint8 unit, uint8 lport, int16 *fault, int16 *length)
{
#if defined(CFG_HW_CABLE_DIAG_INCLUDED)

    int i, max_pair = 4;
    pcm_port_cable_diag_t cd_state;
    int duplex;
    int speed;
#ifdef CFG_SWITCH_EEE_INCLUDED
    int mode;
#endif 

    *fault = 0;
    *length = 0;

    /* Disable EEE before cable diag */
#ifdef CFG_SWITCH_EEE_INCLUDED
	pcm_port_eee_enable_set(unit, lport, 0, &mode);
#endif
    /* ECD_CTRL : break link */
    pcm_port_cable_diag(unit,lport, &cd_state);
    /* Recover EEE enabled after cable diag */
#ifdef CFG_SWITCH_EEE_INCLUDED
	pcm_port_eee_enable_set(unit, lport, 1, &mode);
#endif

    /* Check NO cable case */
    if ((cd_state.pair_state[0] == SOC_PORT_CABLE_STATE_OPEN) &&
        (0 == cd_state.pair_len[0])) {
        for (i = 1; i < max_pair; i++) {
            if (cd_state.pair_state[i] != SOC_PORT_CABLE_STATE_OPEN ||
                cd_state.pair_len[i] != 0) {
                break;
            }
        }
        if (i == max_pair) {
            *fault = CD_NO_CABLE;
            return SYS_OK;
        }
    }

    SOC_IF_ERROR_RETURN(pcm_port_speed_get(unit, lport, &speed));
    SOC_IF_ERROR_RETURN(pcm_port_duplex_get(unit, lport, &duplex));

    if ((speed != 1000) && (speed != 0)){
        max_pair = 2;
    }

    for (i = 0; i < max_pair; i++) {
        *fault |= cd_state.pair_state[i];
        *length = cd_state.pair_len[i] > *length ? cd_state.pair_len[i] : *length;
    }

    if ((*fault & SOC_PORT_CABLE_STATE_SHORT) ||
        (*fault & SOC_PORT_CABLE_STATE_CROSSTALK)) {
        *fault = CD_PIN_SHORT_OR_XT;
    } else if ((*fault & SOC_PORT_CABLE_STATE_OPEN) && (*fault & SOC_PORT_CABLE_STATE_SHORT)) {
        *fault = CD_OPEN | CD_SHORT;
    } else if (*fault & SOC_PORT_CABLE_STATE_OPEN) {
        *fault = CD_OPEN;
    } else if (*fault & SOC_PORT_CABLE_STATE_SHORT) {
        *fault = CD_SHORT;
    } else {
        *fault = CD_NO_FAULT;
    }

    *length *= 100;
    sal_printf(" fault = 0x%x, length = %d\n", *fault, *length);
#endif /* CFG_HW_CABLE_DIAG_INCLUDED */
    return SYS_OK;
}

#else  /* CFG_PCM_SUPPORT_INCLUDED */
sys_error_t
_port_cable_diag(uint8 unit, uint8 lport, int16 *fault, int16 *length)
{
#if defined(CFG_HW_CABLE_DIAG_INCLUDED)

    int i, max_pair = 4;
    phy_port_cable_diag_t cd_state;
    int duplex;
    uint32 speed;

    *fault = 0;
    *length = 0;

    /* Disable EEE before cable diag */
#ifdef CFG_SWITCH_EEE_INCLUDED
        bmd_phy_eee_set(unit, lport, PHY_EEE_NONE);
#endif
    /* ECD_CTRL : break link */
    SOC_IF_ERROR_RETURN(PHY_CABLE_DIAG(BMD_PORT_PHY_CTRL(unit, lport), &cd_state));
    /* Recover EEE enabled after cable diag */
#ifdef CFG_SWITCH_EEE_INCLUDED
        bmd_phy_eee_set(unit, lport, PHY_EEE_802_3);
#endif

    /* Check NO cable case */
    if ((cd_state.pair_state[0] == PhyPortCableState_Open) &&
        (0 == cd_state.pair_len[0])) {
        for (i = 1; i < max_pair; i++) {
            if (cd_state.pair_state[i] != PhyPortCableState_Open ||
                cd_state.pair_len[i] != 0) {
                break;
            }
        }
        if (i == max_pair) {
            *fault = CD_NO_CABLE;
            return SYS_OK;
        }
    }

    SOC_IF_ERROR_RETURN(PHY_SPEED_GET(BMD_PORT_PHY_CTRL(unit, lport), &speed));
    SOC_IF_ERROR_RETURN(PHY_DUPLEX_GET(BMD_PORT_PHY_CTRL(unit, lport), &duplex));
    if ((speed != 1000) && (speed != 0)){
        max_pair = 2;
    }

    for (i = 0; i < max_pair; i++) {
        *fault |= cd_state.pair_state[i];
        *length = MAX(cd_state.pair_len[i], *length);
    }

    if ((*fault & PhyPortCableState_Short) ||
        (*fault & PhyPortCableState_Crosstalk)) {
        *fault = CD_PIN_SHORT_OR_XT;
    } else if ((*fault & PhyPortCableState_Open) && (*fault & PhyPortCableState_Short)) {
        *fault = CD_OPEN | CD_SHORT;
    } else if (*fault & PhyPortCableState_Open) {
        *fault = CD_OPEN;
    } else if (*fault & PhyPortCableState_Short) {
        *fault = CD_SHORT;
    } else {
        *fault = CD_NO_FAULT;
    }

    *length *= 100;
    SAL_DEBUGF((" fault = 0x%x, length = %d\n", *fault, *length));

    /* Re-initialize JUMBO frame support */
    SOC_IF_ERROR_RETURN(PHY_INIT(BMD_PORT_PHY_CTRL(unit, lport)));

#endif /* CFG_HW_CABLE_DIAG_INCLUDED */
    return SYS_OK;
}
#endif /* CFG_PCM_SUPPORT_INCLUDED */

/*
 * Cable Diagnostics
 */
sys_error_t
brdimpl_port_cable_diag(uint16 uport, port_cable_diag_t *status)
{
    sys_error_t rv = SYS_OK;

#if defined(CFG_HW_CABLE_DIAG_INCLUDED)

    uint8 unit, lport;
    int16 fault, length;

    SOC_IF_ERROR_RETURN(board_uport_to_lport(uport, &unit, &lport));
#ifdef _BCM95333X_    
    /* Get chip local port for BMD_PORT_PHY_CTRL to cable diag */
    lport = SOC_PORT_P2L_MAPPING(lport);
#endif    

    SOC_IF_ERROR_RETURN(
        _port_cable_diag(unit, lport, &fault, &length));

    status->length = (int32)(length/100);

    switch (fault) {
        case CD_NO_CABLE:
            status->state = PORT_CABLE_STATE_NO_CABLE;
            break;
        case (CD_OPEN|CD_SHORT):
            status->state = PORT_CABLE_STATE_OPENSHORT;
            break;
        case CD_OPEN:
            status->state = PORT_CABLE_STATE_OPEN;
            break;
        case CD_SHORT:
            status->state = PORT_CABLE_STATE_SHORT;
            break;
        case CD_PIN_SHORT_OR_XT:
            status->state = PORT_CABLE_STATE_CROSSTALK;
            break;
        default:
            status->state = PORT_CABLE_STATE_OK;
        break;
    }
#endif
    return rv;
}

#ifdef CFG_RESET_BUTTON_INCLUDED
static BOOL reset_button_detect = FALSE;

void
brdimpl_reset_button_detect(void *param) REENTRANT
{
    BOOL hard_reset;

    UNREFERENCED_PARAMETER(param);
    if (reset_button_detect == TRUE) {
        if (board_reset_button_get() == TRUE) {
#if CFG_PERSISTENCE_SUPPORT_ENABLED
            sal_printf("Load factory and Reset......\n");
            sal_usleep(500000);
            persistence_restore_factory_defaults();
            persistence_save_all_current_settings();
            board_reset((void *)&hard_reset);
#endif            
        } else {
            sal_printf("Reset......\n");
            sal_usleep(500000);
            board_reset((void *)&hard_reset);
        }
    } else {
        if (board_reset_button_get() == TRUE) {
            reset_button_detect = TRUE;
        }
    }    
} 
#endif /* CFG_RESET_BUTTON_INCLUDED */
