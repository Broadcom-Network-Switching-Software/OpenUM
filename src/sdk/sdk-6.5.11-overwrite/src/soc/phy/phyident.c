/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * File:        phyident.c
 * Purpose:     These routines and structures are related to
 *              figuring out phy identification and correlating
 *              addresses to drivers
 */
#include <sal/types.h>
#include <sal/core/boot.h>
#include <shared/bsl.h>
#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/error.h>
#include <soc/phyreg.h>
#include <soc/phy.h>
#include <soc/phyctrl.h>
#include <soc/phy/phyctrl.h>
#include <soc/phy/drv.h>

#ifdef BCM_SBX_SUPPORT
#include <soc/sbx/sbx_drv.h>
#ifdef BCM_CALADAN3_SUPPORT
#include <soc/sbx/caladan3/port.h>
#endif
#endif


#ifdef BCM_HURRICANE3_SUPPORT
#include <soc/wolfhound2.h>
#endif

#include "phydefs.h"  /* Must include before other phy related includes */
#include "phyident.h"
#include "phyaddr.h"
#include "physr.h"

#define LOCAL_DEBUG 0

static int _chk_phy(int unit, soc_port_t port, soc_phy_table_t *my_entry,
                    uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);

static int _chk_null(int unit, soc_port_t port, soc_phy_table_t *my_entry,
                     uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);

static int _chk_sfp_phy(int unit, soc_port_t port, soc_phy_table_t *my_entry,
                    uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);

#if defined(INCLUDE_PHY_SIMUL)
static int _chk_simul(int unit, soc_port_t port, soc_phy_table_t *my_entry,
                      uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_SIMUL */

#if defined(INCLUDE_PHY_5690)
static int _chk_fiber5690(int unit, soc_port_t port,
                          soc_phy_table_t *my_entry,
                          uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_5690 */

#if defined(INCLUDE_XGXS_QSGMII65)
static int _chk_qsgmii53314(int unit, soc_port_t port,
                          soc_phy_table_t *my_entry,
                          uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_XGXS_QSGMII65 */

#if defined(INCLUDE_PHY_54680)
static int _chk_qgphy_5332x(int unit, soc_port_t port, 
                          soc_phy_table_t *my_entry,
                          uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_54680 */

#if defined(INCLUDE_PHY_56XXX)
static int _chk_fiber56xxx(int unit, soc_port_t port,
                          soc_phy_table_t *my_entry,
                          uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_56XXX */

#if defined(INCLUDE_PHY_56XXX)
static int _chk_fiber56xxx_5601x(int unit, soc_port_t port,
                          soc_phy_table_t *my_entry,
                          uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_56XXX */

#if defined(INCLUDE_SERDES_COMBO)
static int _chk_serdes_combo_5601x(int unit, soc_port_t port,
                          soc_phy_table_t *my_entry,
                          uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_SERDES_COMBO */

#if defined(INCLUDE_PHY_XGXS6)
static int _chk_unicore(int unit, soc_port_t port,
                        soc_phy_table_t *my_entry,
                        uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_XGXS6 */

#if defined(INCLUDE_PHY_8706)
static int _chk_8706(int unit, soc_port_t port, soc_phy_table_t *my_entry,
                     uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_8706 */

#if defined(INCLUDE_PHY_8072)
static int _chk_8072(int unit, soc_port_t port, soc_phy_table_t *my_entry,
                     uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_8072 */

#if defined(INCLUDE_PHY_8040)
static int _chk_8040(int unit, soc_port_t port, soc_phy_table_t *my_entry,
                     uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_8040 */

#if defined(INCLUDE_PHY_8481)
static int _chk_8481(int unit, soc_port_t port, soc_phy_table_t *my_entry,
                     uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_8481 */

#if defined(INCLUDE_SERDES_COMBO65)
static int
_chk_serdescombo65(int unit, soc_port_t port, soc_phy_table_t *my_entry,
         uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);

#endif /* INCLUDE_SERDES_COMBO65 */

#if defined(INCLUDE_XGXS_16G)
static int
_chk_xgxs16g1l(int unit, soc_port_t port, soc_phy_table_t *my_entry,
         uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_XGXS_16G */
#if defined(INCLUDE_XGXS_VIPER)
static int
_chk_viper(int unit, soc_port_t port, soc_phy_table_t *my_entry,
         uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif

#if defined(INCLUDE_XGXS_WCMOD)
static int
_chk_wcmod(int unit, soc_port_t port, soc_phy_table_t *my_entry,
         uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif  /* INCLUDE_XGXS_WCMOD */

#if defined(INCLUDE_PHY_53XXX)
static int _chk_fiber53xxx(int unit, soc_port_t port,
                          soc_phy_table_t *my_entry,
                          uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);
#endif /* INCLUDE_PHY_53XXX */

static int _chk_default(int unit, soc_port_t port, soc_phy_table_t *my_entry,
                        uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi);

static soc_known_phy_t
    _phy_ident_type_get(uint16 phy_id0, uint16 phy_id1);

static soc_phy_table_t _null_phy_entry =
    {_chk_null, _phy_id_NULL,   "Null",    &phy_null,          NULL};

#if defined(INCLUDE_PHY_SIMUL)
static soc_phy_table_t _simul_phy_entry =
    {_chk_simul, _phy_id_SIMUL, "Simulation", &phy_simul,      NULL};
#endif /* INCLUDE_PHY_SIMUL */

#if defined(INCLUDE_PHY_5690)
static soc_phy_table_t _fiber5690_phy_entry =
    {_chk_fiber5690, _phy_id_NULL, "Internal SERDES", &phy_5690drv_ge, NULL };
#endif /* INCLUDE_PHY_5690 */

#if defined(INCLUDE_PHY_56XXX)
static soc_phy_table_t _fiber56xxx_phy_entry =
    {_chk_fiber56xxx, _phy_id_NULL, "Internal SERDES", &phy_56xxxdrv_ge, NULL };
#endif /* INCLUDE_PHY_56XXX */

#if defined(INCLUDE_PHY_53XXX)
static soc_phy_table_t _fiber53xxx_phy_entry =
    {_chk_fiber53xxx, _phy_id_NULL, "Internal  SERDES", &phy_53xxxdrv_ge, NULL};
#endif /* INCLUDE_PHY_53XXX */

static soc_phy_table_t _default_phy_entry =
    {_chk_default, _phy_id_NULL, "Unknown", &phy_drv_fe, NULL};

#ifdef BCM_ROBO_SUPPORT
static soc_phy_table_t _default_phy_entry_ge =
    {_chk_default, _phy_id_NULL, "Unknown", &phy_drv_ge, NULL};
#endif /* BCM_ROBO_SUPPORT */

/*
 * Variable:
 *      _standard_phy_table
 * Purpose:
 *      Defines the standard supported Broadcom PHYs, and the corresponding
 *      driver.
 */

static const soc_phy_table_t _standard_phy_table[] = {

#ifdef INCLUDE_PHY_522X
    {_chk_phy, _phy_id_BCM5218, "BCM5218",     &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM5220, "BCM5220/21",  &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM5226, "BCM5226",     &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM5228, "BCM5228",     &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM5238, "BCM5238",     &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM5248, "BCM5248",     &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM5324, "BCM5324/FE",  &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM5348, "BCM5348/FE",  &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM53242, "BCM53242/FE",  &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM53262, "BCM53262/FE",  &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM53101, "BCM53101/FE",  &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM53280, "BCM53280/FE",  &phy_522xdrv_fe, NULL},
    {_chk_phy, _phy_id_BCM53600, "BCM53600/FE",  &phy_522xdrv_fe, NULL},
#endif /* INCLUDE_PHY_522X */

    {_chk_phy, _phy_id_BCM5400, "BCM5400",     &phy_drv_ge, NULL},

#ifdef INCLUDE_PHY_54XX
    {_chk_phy, _phy_id_BCM5401, "BCM5401",     &phy_5401drv_ge, NULL},
    {_chk_phy, _phy_id_BCM5402, "BCM5402",     &phy_5402drv_ge, NULL},
    {_chk_phy, _phy_id_BCM5404, "BCM5404",     &phy_5404drv_ge, NULL},
    {_chk_phy, _phy_id_BCM5424, "BCM5424/34",  &phy_5424drv_ge, NULL},
    {_chk_phy, _phy_id_BCM5411, "BCM5411",     &phy_5411drv_ge, NULL},
#endif /* INCLUDE_PHY_54XX */
#if defined(INCLUDE_PHY_5464_ROBO)
    {_chk_phy, _phy_id_BCM5461, "BCM5461",     &phy_5464robodrv_ge, NULL},
    {_chk_phy, _phy_id_BCM5464, "BCM5464",     &phy_5464robodrv_ge, NULL},
    {_chk_phy, _phy_id_BCM5466, "BCM5466",     &phy_5464robodrv_ge, NULL},
    {_chk_phy, _phy_id_BCM5478, "BCM5478",     &phy_5464robodrv_ge, NULL},
    {_chk_phy, _phy_id_BCM5488, "BCM5488",     &phy_5464robodrv_ge, NULL},
#endif /* INCLUDE_PHY_5464_ROBO */

#if defined(INCLUDE_PHY_5482_ROBO)
    {_chk_phy, _phy_id_BCM5482, "BCM5482",     &phy_5482robodrv_ge, NULL},
    {_chk_phy, _phy_id_BCM5481, "BCM5481",     &phy_5482robodrv_ge, NULL},
#endif /* INCLUDE_PHY_5482_ROBO */

#if defined(INCLUDE_PHY_5464_ESW)
    {_chk_phy, _phy_id_BCM5461, "BCM5461",     &phy_5464drv_ge, NULL},
    {_chk_phy, _phy_id_BCM5464, "BCM5464",     &phy_5464drv_ge, NULL},
    {_chk_phy, _phy_id_BCM5466, "BCM5466",     &phy_5464drv_ge, NULL},
    {_chk_phy, _phy_id_BCM5478, "BCM5478",     &phy_5464drv_ge, NULL},
    {_chk_phy, _phy_id_BCM5488, "BCM5488",     &phy_5464drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54980, "BCM54980",   &phy_5464drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54980C, "BCM54980",  &phy_5464drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54980V, "BCM54980",  &phy_5464drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54980VC, "BCM54980", &phy_5464drv_ge, NULL},
    {_chk_phy, _phy_id_BCM53314, "BCM53314",   &phy_5464drv_ge, NULL},
#endif /* INCLUDE_PHY_5464 */

#if defined(INCLUDE_PHY_5464_ROBO)
    {_chk_phy, _phy_id_BCM5398, "BCM5398",     &phy_5464robodrv_ge, NULL},
    {_chk_phy, _phy_id_BCM5395, "BCM5395",     &phy_5464robodrv_ge, NULL},
    {_chk_phy, _phy_id_BCM53115, "BCM53115",     &phy_5464robodrv_ge, NULL},
    {_chk_phy, _phy_id_BCM53118, "BCM53118",     &phy_5464robodrv_ge, NULL},
#endif /* BCM_ROBO_SUPPORT && INCLUDE_PHY_5464 */

#if defined(INCLUDE_PHY_5482_ESW)
    {_chk_phy, _phy_id_BCM5482, "BCM5482/801x",     &phy_5482drv_ge, NULL},
#endif /* INCLUDE_PHY_5482 */

#if defined(INCLUDE_PHY_54684)
    {_chk_phy, _phy_id_BCM54684, "BCM54684", &phy_54684drv_ge, NULL},
#endif /* defined(INCLUDE_PHY_54684) */

#if defined(INCLUDE_PHY_54640)
    {_chk_phy, _phy_id_BCM54640, "BCM54640", &phy_54640drv_ge, NULL},
#endif /* defined(INCLUDE_PHY_54640) */

#if defined(INCLUDE_PHY_54682)
    {_chk_phy, _phy_id_BCM54682, "BCM54682E", &phy_54682drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54684E, "BCM54684E", &phy_54682drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54685, "BCM54685", &phy_54682drv_ge, NULL},
#endif /* defined(INCLUDE_PHY_54682) */

#ifdef INCLUDE_PHY_54616
    {_chk_phy, _phy_id_BCM54616  , "BCM54616"  , &phy_54616drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54618E , "BCM54618E" , &phy_54616drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54618SE, "BCM54618SE", &phy_54616drv_ge, NULL},
#endif /* INCLUDE_PHY_54616 */

#ifdef INCLUDE_PHY_84728 
    {_chk_phy, _phy_id_BCM84707, "BCM84707",  &phy_84728drv_xe,  NULL}, 
    {_chk_phy, _phy_id_BCM84073, "BCM84073",  &phy_84728drv_xe,  NULL}, 
    {_chk_phy, _phy_id_BCM84074, "BCM84074",  &phy_84728drv_xe,  NULL}, 
    {_chk_phy, _phy_id_BCM84728, "BCM84728",  &phy_84728drv_xe,  NULL}, 
    {_chk_phy, _phy_id_BCM84748, "BCM84748",  &phy_84728drv_xe,  NULL}, 
    {_chk_phy, _phy_id_BCM84727, "BCM84727",  &phy_84728drv_xe,  NULL}, 
    {_chk_phy, _phy_id_BCM84747, "BCM84747",  &phy_84728drv_xe,  NULL}, 
    {_chk_phy, _phy_id_BCM84762, "BCM84762",  &phy_84728drv_xe,  NULL}, 
    {_chk_phy, _phy_id_BCM84764, "BCM84764",  &phy_84728drv_xe,  NULL}, 
    {_chk_phy, _phy_id_BCM84042, "BCM84042",  &phy_84728drv_xe,  NULL}, 
    {_chk_phy, _phy_id_BCM84044, "BCM84044",  &phy_84728drv_xe,  NULL}, 
#endif /* INCLUDE_PHY_84728 */ 

#ifdef INCLUDE_PHY_8806X
    {_chk_phy, _phy_id_BCM8806x, "BCM8806X",  &phy_8806xdrv,  NULL},
#endif /* INCLUDE_PHY_8806X */

#ifdef INCLUDE_MACSEC
#if defined(INCLUDE_PHY_54580)
    {_chk_phy, _phy_id_BCM54584, "BCM54584", &phy_54580drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54580, "BCM54580", &phy_54580drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54540, "BCM54540", &phy_54580drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54585, "BCM54584", &phy_54580drv_ge, NULL},
#endif /* defined(INCLUDE_PHY_54580) */
#if defined(INCLUDE_PHY_54380)
    {_chk_phy, _phy_id_BCM54380, "BCM54380", &phy_54380drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54382, "BCM54382", &phy_54380drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54340, "BCM54340", &phy_54380drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54385, "BCM54385", &phy_54380drv_ge, NULL},
#endif /* defined(INCLUDE_PHY_54380) */
#ifdef INCLUDE_PHY_8729
    {_chk_phy, _phy_id_BCM8729, "BCM8729",  &phy_8729drv_gexe,  NULL},
#endif /* INCLUDE_PHY_8729 */
#ifdef INCLUDE_PHY_84756 
    {_chk_phy, _phy_id_BCM84756, "BCM84756",  &phy_84756drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84757, "BCM84757",  &phy_84756drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84759, "BCM84759",  &phy_84756drv_xe,  NULL},
#endif /* INCLUDE_PHY_84756 */
#ifdef INCLUDE_PHY_84334
    {_chk_phy, _phy_id_BCM84334, "BCM84334",  &phy_84334drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84333, "BCM84333",  &phy_84334drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84336, "BCM84336",  &phy_84334drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84844, "BCM84844",  &phy_84334drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84846, "BCM84846",  &phy_84334drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84848, "BCM84848",  &phy_84334drv_xe,  NULL},
#endif /* INCLUDE_PHY_84334 */
#ifdef INCLUDE_PHY_84749 
    {_chk_phy, _phy_id_BCM84749, "BCM84749",  &phy_84749drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84729, "BCM84729",  &phy_84749drv_xe,  NULL},
#endif /* INCLUDE_PHY_84749 */
#endif  /* INCLUDE_MACSEC */
#if defined(INCLUDE_PHY_542XX)
    {_chk_phy, _phy_id_BCM54182, "BCM54182", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54185, "BCM54185", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54180, "BCM54180", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54140, "BCM54140", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54192, "BCM54192", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54195, "BCM54195", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54190, "BCM54190", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54194, "BCM54194", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54210, "BCM54210", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54220, "BCM54220", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54280, "BCM54280", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54282, "BCM54282", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54240, "BCM54240", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54285, "BCM54285", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM5428x, "BCM5428X", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54290, "BCM54290", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54292, "BCM54292", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54294, "BCM54294", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54295, "BCM54295", &phy_542xxdrv_ge, NULL},
    {_chk_phy, _phy_id_BCM54296, "BCM54296", &phy_542xxdrv_ge, NULL},
#endif /* defined(INCLUDE_PHY_542XX) */
#ifdef INCLUDE_PHY_84756
#if defined(INCLUDE_FCMAP) || defined(INCLUDE_MACSEC)
    {_chk_phy, _phy_id_BCM84756, "BCM84756",  &phy_84756drv_fcmap_xe,  NULL},
    {_chk_phy, _phy_id_BCM84757, "BCM84757",  &phy_84756drv_fcmap_xe,  NULL},
    {_chk_phy, _phy_id_BCM84759, "BCM84759",  &phy_84756drv_fcmap_xe,  NULL},
#endif /* INCLUDE_FCMAP  || INCLUDE_MACSEC */
#endif /* INCLUDE_PHY_84756 */

#ifdef INCLUDE_PHY_5421S
    {_chk_phy, _phy_id_BCM5421, "BCM5421S",    &phy_5421Sdrv_ge, NULL},
#endif /* INCLUDE_PHY_5421S */

#ifdef INCLUDE_PHY_54680
    {_chk_phy, _phy_id_BCM54680, "BCM54680", &phy_54680drv_ge, NULL},
    {_chk_qgphy_5332x, _phy_id_BCM53324, "BCM53324", &phy_54680drv_ge, NULL},
    {_chk_phy, _phy_id_BCM53125, "BCM53125", &phy_54680drv_ge, NULL},
    {_chk_phy, _phy_id_BCM53128, "BCM53128", &phy_54680drv_ge, NULL},
    {_chk_phy, _phy_id_BCM53010, "BCM53010", &phy_54680drv_ge, NULL},
    {_chk_phy, _phy_id_BCM53018, "BCM53018", &phy_54680drv_ge, NULL},
    {_chk_phy, _phy_id_BCM53020, "BCM5302X", &phy_54680drv_ge, NULL},
#endif /* INCLUDE_PHY_54680 */

#ifdef INCLUDE_PHY_EGPHY28
    {_chk_phy, _phy_id_BCM53134, "BCM53134", &phy_egphy28drv_ge, NULL},
    {_chk_phy, _phy_id_BCM56160_GPHY, "BCM56160-GPHY", &phy_egphy28drv_ge, NULL},
    {_chk_phy, _phy_id_BCM53540, "BCM53540-GPHY", &phy_egphy28drv_ge, NULL},
#endif /* INCLUDE_PHY_EGPHY28 */

#ifdef INCLUDE_PHY_54880
    {_chk_phy, _phy_id_BCM54880, "BCM54880", &phy_54880drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54881, "BCM54881", &phy_54880drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54810, "BCM54810", &phy_54880drv_ge, NULL},
    {_chk_phy, _phy_id_BCM54811, "BCM54811", &phy_54880drv_ge, NULL},
    {_chk_phy, _phy_id_BCM89500, "BCM89500", &phy_54880drv_ge, NULL},
#endif /* INCLUDE_PHY_54880 */

#ifdef INCLUDE_PHY_54640E
    {_chk_phy, _phy_id_BCM54680E, "BCM54640E", &phy_54640drv_ge, NULL},
#endif /* INCLUDE_PHY_54640E */

#ifdef INCLUDE_PHY_54880E
    {_chk_phy, _phy_id_BCM54880E, "BCM54880E", &phy_54680drv_ge, NULL},
#endif /* INCLUDE_PHY_54880E */

#ifdef INCLUDE_PHY_54680E
    {_chk_phy, _phy_id_BCM54680E, "BCM54680E", &phy_54680drv_ge, NULL},
#endif /* INCLUDE_PHY_54680E */

#ifdef INCLUDE_PHY_52681E
    {_chk_phy, _phy_id_BCM52681E, "BCM52681E", &phy_54680drv_ge, NULL},
#endif /* INCLUDE_PHY_52681E */

#ifdef INCLUDE_PHY_8703
    {_chk_phy, _phy_id_BCM8703, "BCM8703",  &phy_8703drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM8704, "BCM8704",  &phy_8703drv_xe,  NULL},
#endif /* INCLUDE_PHY_8703 */
#ifdef INCLUDE_PHY_8705
    {_chk_phy, _phy_id_BCM8705, "BCM8705/24/25",  &phy_8705drv_xe,  NULL},
#endif /* INCLUDE_PHY_8705 */
#if defined(INCLUDE_PHY_8706)
    /* BCM8706_A0 and BCM8705 has the same device ID. Therefore, the probe must
     * check for 8706 before 8705 to correctly attach BCM8706. For 8706,
     * phy_8706 config must be set.
     */
    {_chk_8706, _phy_id_BCM8706, "BCM8706/8726", &phy_8706drv_xe, NULL},
    {_chk_8706, _phy_id_BCM8727, "BCM8727", &phy_8706drv_xe, NULL},
    {_chk_8706, _phy_id_BCM8747, "BCM8728/8747", &phy_8706drv_xe, NULL},
#endif /* INCLUDE_PHY_8706 */
#if defined(INCLUDE_PHY_8072)
    {_chk_8072, _phy_id_BCM8072, "BCM8072", &phy_8072drv_xe, NULL},
    {_chk_8072, _phy_id_BCM8073, "BCM8073", &phy_8072drv_xe, NULL},
    {_chk_8072, _phy_id_BCM8074, "BCM8074", &phy_8074drv_xe, NULL},
#endif /* INCLUDE_PHY_8072 */

#if defined(INCLUDE_PHY_8040)
    {_chk_8040, _phy_id_BCM8040, "BCM8040", &phy_8040drv_xe, NULL},
#endif /* INCLUDE_PHY_8040 */

#if defined(INCLUDE_PHY_8481)
    {_chk_8481, _phy_id_BCM8481x, "BCM8481X", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84812ce, "BCM84812", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84821, "BCM84821", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84822, "BCM84822", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84823, "BCM84823", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84833, "BCM84833", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84834, "BCM84834", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84835, "BCM84835", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84836, "BCM84836", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84844, "BCM84844", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84846, "BCM84846", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84848, "BCM84848", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84858, "BCM84858", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84856, "BCM84856", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84860, "BCM84860", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84861, "BCM84861", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84864, "BCM84864", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84868, "BCM84868", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84888, "BCM84888", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84884, "BCM84884", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84888E, "BCM84888E", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84884E, "BCM84884E", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84881, "BCM84881", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84880, "BCM84880", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84888S, "BCM84888S", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84887, "BCM84887", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84886, "BCM84886", &phy_8481drv_xe, NULL},
    {_chk_8481, _phy_id_BCM84885, "BCM84885", &phy_8481drv_xe, NULL},
#endif /* INCLUDE_PHY_8481 */
#ifdef INCLUDE_PHY_8750
    {_chk_phy, _phy_id_BCM8750, "BCM8750",  &phy_8750drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM8752, "BCM8752",  &phy_8750drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM8754, "BCM8754",  &phy_8750drv_xe,  NULL},
#endif /* INCLUDE_PHY_8750 */
#ifdef INCLUDE_PHY_84740
    {_chk_phy, _phy_id_BCM84740, "BCM84740",  &phy_84740drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84164, "BCM84164",  &phy_84740drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84758, "BCM84758",  &phy_84740drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84780, "BCM84780",  &phy_84740drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84784, "BCM84784",  &phy_84740drv_xe,  NULL},
    {_chk_phy, _phy_id_BCM84318, "BCM84318",  &phy_84740drv_xe,  NULL},
#endif /* INCLUDE_PHY_84740 */
#ifdef INCLUDE_PHY_84328
    {_chk_phy, _phy_id_BCM84328, "BCM84328",  &phy_84328drv_xe,  NULL},
#endif /* INCLUDE_PHY_84328 */
#ifdef INCLUDE_PHY_84793
    {_chk_phy, _phy_id_BCM84793, "BCM84793",  &phy_84793drv_ce,  NULL},
#endif
#ifdef INCLUDE_PHY_82328
    {_chk_phy, _phy_id_BCM82328, "BCM82328",  &phy_82328drv_xe,  NULL},
#endif
#if defined(INCLUDE_PHY_82381)
    {_chk_phy, _phy_id_BCM82381, "Furia", &phy82381_drv, NULL},
#endif
#if defined(INCLUDE_PHY_82764)
    {_chk_phy, _phy_id_BCM82764, "Sesto", &phy82764_drv, NULL},
#endif
#if defined(INCLUDE_PHY_82864)
    {_chk_phy, _phy_id_BCM82864, "Madura", &phy82864_drv, NULL},
#endif

#ifdef INCLUDE_PHY_82780
    {_chk_phy, _phy_id_BCM82780, "BCM82780", &phy82780_drv, NULL}, 
#endif

    {_chk_sfp_phy, _phy_id_numberKnown+1, "copper sfp",&phy_copper_sfp_drv,NULL},
    
};

/* Internal PHY table */
static soc_phy_table_t _int_phy_table[] = {
#if defined(INCLUDE_PHY_56XXX)
    {_chk_fiber56xxx, _phy_id_NULL, "Internal SERDES",
     &phy_56xxxdrv_ge, NULL},
#endif /* INCLUDE_PHY_56XXX */

#if defined(INCLUDE_PHY_53XXX)
    {_chk_fiber53xxx, _phy_id_NULL, "Internal SERDES",
     &phy_53xxxdrv_ge, NULL},
#endif /* INCLUDE_PHY_53XXX */

#if defined(INCLUDE_SERDES_COMBO)
     {_chk_phy, _phy_id_SERDESCOMBO, "COMBO", &phy_serdescombo_ge, NULL},
#endif /* INCLUDE_SERDES_COMBO */

#if defined(INCLUDE_SERDES_100FX)
    {_chk_phy, _phy_id_SERDES100FX, "1000X/100FX", &phy_serdes100fx_ge, NULL},
#endif /* INCLUDE_SERDES_100FX */

#if defined(INCLUDE_SERDES_65LP)
    {_chk_phy, _phy_id_SERDES65LP, "65LP", &phy_serdes65lp_ge, NULL},
#endif /* INCLUDE_SERDES_65LP */

#ifdef INCLUDE_XGXS_QSGMII65
    {_chk_qsgmii53314, _phy_id_NULL, "QSGMII65", &phy_qsgmii65_ge,
     NULL },
#endif /* INCLUDE_XGXS_QSGMII65 */

#if defined(INCLUDE_SERDES_COMBO65)
    {_chk_serdescombo65, _phy_id_SERDESCOMBO65, "COMBO65", 
     &phy_serdescombo65_ge, NULL},
#endif /* INCLUDE_SERDES_COMBO65 */

#if defined(INCLUDE_PHY_XGXS1)
    {_chk_phy, _phy_id_BCMXGXS1, "XGXS1",      &phy_xgxs1_hg, NULL},
    {_chk_phy, _phy_id_BCMXGXS2, "XGXS2",      &phy_xgxs1_hg, NULL},
#endif /* INCLUDE_PHY_XGXS1 */

#if defined(INCLUDE_PHY_XGXS5)
    {_chk_phy, _phy_id_BCMXGXS5, "XGXS5",      &phy_xgxs5_hg, NULL},
#endif /* INCLUDE_PHY_XGXS5 */

#if defined(INCLUDE_PHY_XGXS6)
    {_chk_phy,     _phy_id_BCMXGXS6, "XGXS6",  &phy_xgxs6_hg, NULL},
    {_chk_unicore, _phy_id_BCMXGXS2, "XGXS6",  &phy_xgxs6_hg, NULL},
#endif /* INCLUDE_PHY_XGXS6 */

    /* Must probe for newer internal SerDes/XAUI first before probing for
     * older devices. Newer devices reuse the same device ID and introduce
     * a new mechanism to differentiate betwee devices. Therefore, newer
     * PHY drivers implement probe funtion to check for correct device.
     */
#if defined(INCLUDE_XGXS_WC40)
    {_chk_phy, _phy_id_XGXS_WC40, "WC40/4",    &phy_wc40_hg, NULL},
#endif /* INCLUDE_XGXS_HL65 */

#if defined(INCLUDE_XGXS_HL65)
    {_chk_phy, _phy_id_XGXS_HL65, "HL65/4",    &phy_hl65_hg, NULL},
#endif /* INCLUDE_XGXS_HL65 */

#if defined(INCLUDE_PHY_56XXX)
    {_chk_fiber56xxx_5601x, _phy_id_NULL, "Internal SERDES",
     &phy_56xxx_5601x_drv_ge, NULL},
#endif /* INCLUDE_PHY_56XXX */

#if defined(INCLUDE_SERDES_COMBO)
    {_chk_serdes_combo_5601x, _phy_id_NULL, "COMBO SERDES",
     &phy_serdescombo_5601x_ge, NULL},
#endif /* INCLUDE_SERDES_COMBO */

#if defined(INCLUDE_XGXS_16G)
    {_chk_phy, _phy_id_XGXS_16G, "XGXS16G",    &phy_xgxs16g_hg, NULL},
    {_chk_xgxs16g1l, _phy_id_XGXS_16G, "XGXS16G/1", &phy_xgxs16g1l_ge, NULL},
#endif /* INCLUDE_XGXS_16G */

#if defined(INCLUDE_XGXS_WCMOD)
    {_chk_wcmod, _phy_id_XGXS_WL, "WCMOD/4",    &phy_wcmod_hg, NULL},
#endif  /* INCLUDE_XGXS_WL */
#if defined(INCLUDE_XGXS_TSCMOD)
    {_chk_phy, _phy_id_XGXS_TSC, "TSCMOD/4",    &phy_tscmod_hg, NULL},
#endif  /* INCLUDE_XGXS_TSC */

#if defined(INCLUDE_XGXS_TSCE)
    {_chk_phy, _phy_id_XGXS_TSC, "TSCE", &phy_tsce_drv, NULL},
#endif /* INCLUDE_XGXS_TSCE */

#if defined(INCLUDE_XGXS_TSCF)
    {_chk_phy, _phy_id_XGXS_TSC, "TSCF", &phy_tscf_drv, NULL},
#endif /* INCLUDE_XGXS_TSCF */

#if defined(INCLUDE_SERDES_QSGMIIE)
    {_chk_phy, _phy_id_SERDES65LP, "QSGMIIE",   &phy_qsgmiie_drv, NULL},
#endif

#if defined(INCLUDE_XGXS_VIPER)
    {_chk_viper, _phy_id_XGXS_VIPER, "VIPER", &phy_viper_drv, NULL},
#endif /* INCLUDE_XGXS_VIPER */

#if defined(INCLUDE_SERDES_QTCE)
    {_chk_phy, _phy_id_XGXS_TSC, "QTCE", &phy_qtce_drv, NULL},
#endif /* INCLUDE_XGXS_TSCF */


};
#if defined (INCLUDE_PHY_56XXX) || defined (INCLUDE_SERDES_COMBO)
/*
 * Check corrupted registers by writing zeroes
 * to block address register and making sure zeroes
 * are read back.
 */
STATIC INLINE int 
_is_corrupted_reg(int unit, uint8 phy_addr)
{
    int         rv;
    uint16      data;

    rv = soc_miim_write(unit, phy_addr, 0x1f, 0);
    if (rv != SOC_E_NONE) {
        return FALSE;
    }
    rv = soc_miim_read(unit, phy_addr, 0x1f, &data);
    if (rv != SOC_E_NONE) {
        return FALSE;
    }

    return (data != 0);
}
#endif /* (INCLUDE_PHY_56XXX) || (INCLUDE_SERDES_COMBO)*/


/*
 * Function:
 *      _soc_phy_addr_default
 * Purpose:
 *      Return the default PHY addresses used to initialize the PHY map
 *      for a port.
 * Parameters:
 *      unit - StrataSwitch unit number
 *      phy_addr - (OUT) Outer PHY address
 *      phy_addr_int - (OUT) Intermediate PHY address, 0xff if none
 */

static void
_soc_phy_addr_default(int unit, int port,
                      uint16 *phy_addr, uint16 *phy_addr_int)
{


    soc_phy_addr_default(unit, port, phy_addr, phy_addr_int);
    /*
     * Override the calculated address(es) with the per-port properties
     */
   *phy_addr = soc_property_port_get(unit, port,
                                      spn_PORT_PHY_ADDR,
                                      *phy_addr);
}

int
soc_phy_deinit(int unit)
{
    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_phy_init
 * Purpose:
 *      Initialize PHY software subsystem.
 * Parameters:
 *      unit - StrataSwitch unit number
 * Returns:
 *      SOC_E_XXX
 */
int
soc_phy_init(int unit)
{
    uint16              phy_addr=0, phy_addr_int=0;
    soc_port_t          port;

    PBMP_PORT_ITER(unit, port) { /* To get all ports Address */
        _soc_phy_addr_default(unit, port, &phy_addr, &phy_addr_int);

/*
In inlcude/soc/phyctrl.h
 #define PHY_ADDR(unit, port)         (SOC_PHY_INFO(unit, port).phy_addr) 
 #define PHY_ADDR_INT(unit, port)     (SOC_PHY_INFO(unit, port).phy_addr_int)

*/
        PHY_ADDR(unit, port)     = phy_addr;
        PHY_ADDR_INT(unit, port) = phy_addr_int;
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_phy_port_init
 * Purpose:
 *      Initialize PHY software for a single port.
 * Parameters:
 *      unit - unit number
 *      port - port number
 * Returns:
 *      SOC_E_XXX
 */
int
soc_phy_port_init(int unit, soc_port_t port)
{
    uint16              phy_addr=0, phy_addr_int=0;

    soc_phy_addr_default(unit, port, &phy_addr, &phy_addr_int);

    PHY_ADDR(unit, port)     = phy_addr;
    PHY_ADDR_INT(unit, port) = phy_addr_int;
    

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_phy_add_entry
 * Purpose:
 *      Add an entry to the PHY table
 * Parameters:
 *      entry - pointer to the entry
 * Returns:
 *      SOC_E_NONE - no error
 *      SOC_E_INIT - not initialized
 *      SOC_E_MEMORY - not no more space in table.
 */
#if 0
int
soc_phy_add_entry(soc_phy_table_t *entry)
{
    assert(_phys_in_table >= 0);        /* Fatal if not already inited */

    if (_phys_in_table >= _MAX_PHYS) {
        return SOC_E_MEMORY;
    }

    phy_table[_phys_in_table++] = entry;

    return SOC_E_NONE;
}
#endif
#if defined(INCLUDE_SERDES_COMBO65)
/*
 * Function:
 *      _chk_serdescombo65
 * Purpose:
 *      Check function for Raven Combo SerDes PHYs
 * Parameters:
 *      See soc_phy_ident_f
 *      *pi is an output parameter filled with PHY info.
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      Sets the phy_info structure for this port.
 */

static int
_chk_serdescombo65(int unit, soc_port_t port, soc_phy_table_t *my_entry,
         uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    COMPILER_REFERENCE(unit);

    if (SOC_IS_RAVEN(unit)) {
        if (port == 1 || port == 2 || port == 4 || port == 5) {
            pi->phy_name = my_entry->phy_name;
            return TRUE;
        }
    }

#ifdef BCM_ROBO_SUPPORT
    if (SOC_IS_ROBO(unit)) {
        if(SOC_IS_TB(unit) && IS_S_PORT(unit, port)){
            pi->phy_name = my_entry->phy_name;
            return TRUE;
        }
    }
#endif /* BCM_ROBO_SUPPORT */

    return FALSE;
}

#endif /* INCLUDE_SERDES_COMBO65 */

#if defined(INCLUDE_XGXS_16G)
/*
 * Function:
 *      _chk_xgxs16g1l
 * Purpose:
 *      Standard check function for PHYs (see soc_phy_ident_f)
 * Parameters:
 *      See soc_phy_ident_f
 *      *pi is an output parameter filled with PHY info.
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      Sets the phy_info structure for this port.
 */

static int
_chk_xgxs16g1l(int unit, soc_port_t port, soc_phy_table_t *my_entry,
         uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    if (my_entry->myNum == _phy_ident_type_get(phy_id0, phy_id1)) {
#ifdef BCM_KATANA_SUPPORT
         if (SOC_IS_KATANAX(unit) && IS_GE_PORT(unit,port)) {
             uint32 rval;
             int port_mode = 2;

             if (IS_MXQ_PORT(unit,port)) {
                 SOC_IF_ERROR_RETURN(READ_XPORT_MODE_REGr(unit, port, &rval));
                 port_mode = soc_reg_field_get(unit, XPORT_MODE_REGr, rval, PHY_PORT_MODEf);
             }
             if (port_mode) {
                 return TRUE;
             } else {
                 return FALSE;
             }
         } else
#endif /* BCM_KATANA_SUPPORT */

#ifdef BCM_TRIUMPH3_SUPPORT
        if (SOC_IS_TRIUMPH3(unit) && IS_GE_PORT(unit, port)) {
            pi->phy_name = my_entry->phy_name;
            return TRUE;
        }
#endif

        if (SOC_IS_SCORPION(unit) && !IS_GX_PORT(unit, port)) {
            pi->phy_name = my_entry->phy_name;
            return TRUE;
        }

#ifdef BCM_ENDURO_SUPPORT
    if (SOC_IS_ENDURO(unit) && (port > 25) && IS_GE_PORT(unit,port)) {
            pi->phy_name = my_entry->phy_name;
            return TRUE;      
    }
#endif /* BCM_ENDURO_SUPPORT */

#ifdef BCM_NORTHSTARPLUS_SUPPORT
        if (SOC_IS_NORTHSTARPLUS(unit) && IS_GE_PORT(unit,port)) {
                pi->phy_name = my_entry->phy_name;
                return TRUE;      
        }
#endif /* BCM_NORTHSTARPLUS_SUPPORT */


    }
    return FALSE;
}
#endif /* INCLUDE_XGXS_16G */
#if defined(INCLUDE_XGXS_VIPER)
/*
 * Function:
 *      _chk_viper
 * Purpose:
 *      Standard check function for VIPER PHYs (see soc_phy_ident_f)
 * Parameters:
 *      See soc_phy_ident_f
 *      *pi is an output parameter filled with PHY info.
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      Sets the phy_info structure for this port.
 */
static int
_chk_viper(int unit, soc_port_t port, soc_phy_table_t *my_entry,
         uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    
    
   if (SOC_IS_SABER2(unit) || SOC_IS_GREYHOUND2(unit) ||
       soc_feature(unit, soc_feature_wh2)) {
       return TRUE;
   }

   return _chk_phy(unit, port, my_entry, phy_id0, phy_id1, pi);
}
#endif /* INCLUDE_XGXS_VIPER */

#if defined(INCLUDE_XGXS_WCMOD)
/*
 * Function:
 *      _chk_wcmod
 * Purpose:
 *      Standard check function for PHYs (see soc_phy_ident_f)
 * Parameters:
 *      See soc_phy_ident_f
 *      *pi is an output parameter filled with PHY info.
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      Sets the phy_info structure for this port.
 */
static int
_chk_wcmod(int unit, soc_port_t port, soc_phy_table_t *my_entry,
           uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    if (SOC_IS_TRIUMPH3(unit)) {
        if (IS_GE_PORT(unit, port) &&
            !soc_property_port_get(unit, port, "phy_wcmod", 1)) {
            /* Allow debug fallback to legacy XGXS16g1l PHY driver */
            return FALSE;
        }
    }

    return _chk_phy(unit, port, my_entry, phy_id0, phy_id1, pi);
}
#endif  /* INCLUDE_XGXS_WCMOD */
/*
 * Function:
 *      _chk_phy
 * Purpose:
 *      Standard check function for PHYs (see soc_phy_ident_f)
 * Parameters:
 *      See soc_phy_ident_f
 *      *pi is an output parameter filled with PHY info.
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      Sets the phy_info structure for this port.
 */

static int
_chk_phy(int unit, soc_port_t port, soc_phy_table_t *my_entry,
         uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    COMPILER_REFERENCE(unit);

    if (my_entry->myNum == _phy_ident_type_get(phy_id0, phy_id1)) {
        pi->phy_name = my_entry->phy_name;
        return TRUE;
    }

    return FALSE;
}

#if defined(INCLUDE_PHY_SIMUL)
#if defined(SIM_ALL_PHYS)
#define USE_SIMULATION_PHY(unit, port)  (TRUE)
#else
#define USE_SIMULATION_PHY(unit, port) \
     (soc_property_port_get(unit, port, spn_PHY_SIMUL, 0))
#endif
#else
#define USE_SIMULATION_PHY(unit, port)  (FALSE)
#endif

/*
 * Function:
 *      _chk_null
 * Purpose:
 *      Check function for NULL phys.
 * Returns:
 *      True if this phy matches.
 *      False otherwise.
 */

static int
_chk_null(int unit, soc_port_t port,  soc_phy_table_t *my_entry,
          uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
#if defined(INCLUDE_CES)
    uint16              dev_id;
    uint8               rev_id;
#endif

    if ((SAL_BOOT_PLISIM && (!SAL_BOOT_RTLSIM &&
        !USE_SIMULATION_PHY(unit, port))) ||
        !soc_property_get(unit, spn_PHY_ENABLE, 1) ||
        soc_property_port_get(unit, port, spn_PHY_NULL, 0) 
#ifdef BCM_TRIUMPH3_SUPPORT
        || ((SOC_IS_TRIUMPH3(unit) 
#ifdef BCM_TRIDENT2_SUPPORT
          || SOC_IS_TD2_TT2(unit)
#endif
        ) && SOC_PBMP_MEMBER(SOC_PORT_DISABLED_BITMAP(unit, all), port))
#endif
        ) {
        pi->phy_name = my_entry->phy_name;

        return TRUE;
    }

#if defined(INCLUDE_CES)
/*
 * For CES on the 56441/2/3 where the CES mii port 
 * is internal and there is no physical phy set the
 * phy device to the null phy.
 */
    if (soc_feature(unit, soc_feature_ces) && port == 1) {
    soc_cm_get_id(unit, &dev_id, &rev_id);

    if (dev_id == BCM56441_DEVICE_ID ||
        dev_id == BCM56442_DEVICE_ID ||
        dev_id == BCM56443_DEVICE_ID) {
        pi->phy_name = my_entry->phy_name;
        return TRUE;
    }
    }
#endif

#if defined(INCLUDE_RCPU)
    if (SOC_IS_RCPU_ONLY(unit)) {
        pi->phy_name = my_entry->phy_name;
        return TRUE;
    }
#endif /* INCLUDE_RCPU */

    return FALSE;
}

#if defined(BCM_XGS3_SWITCH_SUPPORT) || defined(BCM_ROBO_SUPPORT)
/*
 * Function:
 *      _chk_gmii
 * Purpose:
 *      Check function for GMII port.
 * Returns:
 *      True if this phy matches.
 *      False otherwise.
 */

static int
_chk_gmii(int unit, soc_port_t port,  soc_phy_table_t *my_entry,
          uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    if (IS_GMII_PORT(unit, port)) {
        pi->phy_name = my_entry->phy_name;

        return TRUE;
    }

    return FALSE;
}
#endif /* BCM_XGS3_SWITCH_SUPPORT || BCM_ROBO_SUPPORT */

#if defined(INCLUDE_PHY_SIMUL)

/*
 * Function:
 *      _chk_simul
 * Purpose:
 *      Check function for simulation phys.
 * Returns:
 *      True if this phy matches.
 *      False otherwise.
 */

static int
_chk_simul(int unit, soc_port_t port,  soc_phy_table_t *my_entry,
           uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    if (USE_SIMULATION_PHY(unit, port) && !SOC_IS_ARDON(unit)) {
        pi->phy_name = my_entry->phy_name;

        return TRUE;
    }

    return FALSE;
}
#endif /* include phy sim */

#undef USE_SIMULATION_PHY

#ifdef INCLUDE_XGXS_QSGMII65
/*
 * Function:
 *      _chk_qsgmii53314
 * Purpose:
 *      Check for using Internal 53314 SERDES Device 
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      This routine should be called after checking for external
 *      PHYs because it will default the PHY driver to the internal
 *      SERDES in the absense of any other PHY.
 */

static int
_chk_qsgmii53314(int unit, soc_port_t port, soc_phy_table_t *my_entry,
               uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    COMPILER_REFERENCE(my_entry);
    COMPILER_REFERENCE(phy_id0);
    COMPILER_REFERENCE(phy_id1);
    COMPILER_REFERENCE(pi);

    if (SOC_IS_HAWKEYE(unit) && port >= 9) {
        pi->phy_name = "Phy53314";
        return TRUE;
    }

    if (IS_GE_PORT(unit, port) && (SOC_IS_HURRICANE(unit) && (port >= 2) && (port <= 25))) {
        pi->phy_name = "Phy53314";
        return TRUE;
    }

    return FALSE;
}
#endif /* INCLUDE_XGXS_QSGMII65 */

#ifdef INCLUDE_PHY_54680
/*
 * Function:
 *      _chk_qgphy_5332x
 * Purpose:
 *      Check for using 5332x QGPHY Device 
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      Because The Id of QGPHY device of HKEEE is same as BCM54682E,
 *      the _chk_phy can be used for QGPHY device of HKEEE
 */

static int
_chk_qgphy_5332x(int unit, soc_port_t port, soc_phy_table_t *my_entry,
               uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    COMPILER_REFERENCE(my_entry);
    COMPILER_REFERENCE(phy_id0);
    COMPILER_REFERENCE(phy_id1);
    COMPILER_REFERENCE(pi);

    if (SOC_IS_HAWKEYE(unit) && 
        soc_feature (unit, soc_feature_eee) && 
        (port <= 8) && 
        (port != 0)) {
        pi->phy_name = "BCM53324";
        return TRUE;
    }

    return FALSE;
}
#endif /* INCLUDE_PHY_54680 */

#if defined(INCLUDE_PHY_56XXX)
/*
 * Function:
 *      _chk_fiber56xxx
 * Purpose:
 *      Check for using Internal 56XXX SERDES Device for fiber
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      This routine should be called after checking for external
 *      PHYs because it will default the PHY driver to the internal
 *      SERDES in the absense of any other PHY.
 */

static int
_chk_fiber56xxx(int unit, soc_port_t port, soc_phy_table_t *my_entry,
               uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    COMPILER_REFERENCE(my_entry);
    COMPILER_REFERENCE(phy_id0);
    COMPILER_REFERENCE(phy_id1);

    if (IS_GE_PORT(unit, port) && !IS_GMII_PORT(unit, port) &&
        !SOC_IS_HAWKEYE(unit) && !SOC_IS_HURRICANE2(unit) &&
        soc_feature(unit, soc_feature_dodeca_serdes)) {
        pi->phy_name = "Phy56XXX";

        return TRUE;
    }

    return FALSE;
}
#endif /* INCLUDE_PHY_56XXX */

#if defined(INCLUDE_PHY_XGXS6)
/*
 * Function:
 *      _chk_unicore
 * Purpose:
 *      Check for Unicore, which may return two different PHY IDs
 *      depending on the current IEEE register mapping. One of
 *      these PHY IDs conflicts with the BCM5673/74 PHY ID, so
 *      we need to check the port type here as well.
 * Parameters:
 *      See soc_phy_ident_f
 *      *pi is an output parameter filled with PHY info.
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      Sets the phy_info structure for this port.
 */

static int
_chk_unicore(int unit, soc_port_t port, soc_phy_table_t *my_entry,
             uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    COMPILER_REFERENCE(unit);

    if (my_entry->myNum == _phy_ident_type_get(phy_id0, phy_id1) &&
        soc_feature(unit, soc_feature_xgxs_v6)) {
        pi->phy_name = my_entry->phy_name;
        return TRUE;
    }

    return FALSE;
}
#endif /* INCLUDE_PHY_XGXS6 */

static int
_chk_sfp_phy(int unit, soc_port_t port, soc_phy_table_t *my_entry,
         uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    COMPILER_REFERENCE(unit);

    if (soc_property_port_get(unit, port, spn_PHY_COPPER_SFP, 0)) {
        if (!(phy_id0 == (uint16)0xFFFF && phy_id1 == (uint16)0xFFFF)) {
            LOG_INFO(BSL_LS_SOC_PHY,
                     (BSL_META_U(unit,
                                 "_chk_sfp_phy: u=%d p=%d id0=0x%x, id1=0x%x,"
                                 " oui=0x%x,model=0x%x,rev=0x%x\n"),
                      unit, port,phy_id0,phy_id1,PHY_OUI(phy_id0, phy_id1),
                      PHY_MODEL(phy_id0, phy_id1),PHY_REV(phy_id0, phy_id1)));
            pi->phy_name = my_entry->phy_name;
            return TRUE;
        }
    }
    return FALSE;
}

#if defined(INCLUDE_PHY_8706)
static int
_chk_8706(int unit, soc_port_t port, soc_phy_table_t *my_entry,
             uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    if (my_entry->myNum == _phy_ident_type_get(phy_id0, phy_id1) ||
        soc_property_port_get(unit, port, spn_PHY_8706, FALSE)) {
        pi->phy_name = my_entry->phy_name;
        return TRUE;
    }
    return FALSE;
}

#endif /* INCLUDE_PHY_8706 */

#if defined(INCLUDE_PHY_8040)
static int
_chk_8040(int unit, soc_port_t port, soc_phy_table_t *my_entry,
             uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    if (my_entry->myNum == _phy_ident_type_get(phy_id0, phy_id1)) {
        pi->phy_name = my_entry->phy_name;
        return TRUE;
    }
    return FALSE;
}
#endif /* INCLUDE_PHY_8040 */

#if defined(INCLUDE_PHY_8072)
static int
_chk_8072(int unit, soc_port_t port, soc_phy_table_t *my_entry,
             uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    if (my_entry->myNum == _phy_ident_type_get(phy_id0, phy_id1) ||
        soc_property_port_get(unit, port, spn_PHY_8072, FALSE)) {
        pi->phy_name = my_entry->phy_name;
        return TRUE;
    }
    return FALSE;
}

#endif /* INCLUDE_PHY_8072 */

#if defined(INCLUDE_PHY_8481)
static int
_chk_8481(int unit, soc_port_t port, soc_phy_table_t *my_entry,
             uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    /* for GE/XE/HG ports only */
    if ( ! (IS_XE_PORT(unit, port) || IS_HG_PORT(unit, port)
                                   || IS_GE_PORT(unit, port)) ) {
        return FALSE;
    }

    if (my_entry->myNum == _phy_ident_type_get(phy_id0, phy_id1)) {
        pi->phy_name = my_entry->phy_name;
        return TRUE;
    }

    return FALSE;

}
#endif /* INCLUDE_PHY_8481 */

#if defined(INCLUDE_PHY_56XXX)
/*
 * Function:
 *      _chk_fiber56xxx_5601x
 * Purpose:
 *      Check for using Internal SERDES Device for fiber with shadow registers
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      This routine should be called after checking for external
 *      PHYs because it will default the PHY driver to the internal
 *      SERDES in the absense of any other PHY.
 */

static int
_chk_fiber56xxx_5601x(int unit, soc_port_t port, soc_phy_table_t *my_entry,
               uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    uint8       phy_addr;

    COMPILER_REFERENCE(my_entry);
    COMPILER_REFERENCE(phy_id0);
    COMPILER_REFERENCE(phy_id1);

    phy_addr = PORT_TO_PHY_ADDR_INT(unit, port);

    if (SOC_IS_RAPTOR(unit) && 
        soc_feature(unit, soc_feature_fe_ports) &&
        ((port == 4) || (port == 5)) &&
        (soc_property_port_get(unit, port, spn_SERDES_SHADOW_DRIVER, FALSE) ||
         _is_corrupted_reg(unit, phy_addr))) {
        pi->phy_name = "Phy56XXX5601x";
        return TRUE;
    }

    return FALSE;
}
#endif /* INCLUDE_PHY_56XXX */

#if defined(INCLUDE_SERDES_COMBO)
/*
 * Function:
 *      _chk_serdes_combo_5601x
 * Purpose:
 *      Check for using Internal SERDES Device for fiber with shadow registers
 * Returns:q
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      This routine should be called after checking for external
 *      PHYs because it will default the PHY driver to the internal
 *      SERDES in the absense of any other PHY.
 */

static int
_chk_serdes_combo_5601x(int unit, soc_port_t port, soc_phy_table_t *my_entry,
               uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    uint8       phy_addr;

    COMPILER_REFERENCE(my_entry);
    COMPILER_REFERENCE(phy_id0);
    COMPILER_REFERENCE(phy_id1);

    phy_addr = PORT_TO_PHY_ADDR_INT(unit, port);

    if (SOC_IS_RAPTOR(unit) && 
        soc_feature(unit, soc_feature_fe_ports) &&
        ((port == 1) || (port == 2)) &&
        (soc_property_port_get(unit, port, spn_SERDES_SHADOW_DRIVER, FALSE) ||
         _is_corrupted_reg(unit, phy_addr))) {
        pi->phy_name = "COMBO5601x";
        return TRUE;
    }    

    return FALSE;
}
#endif /* INCLUDE_SERDES_COMBO */

#if defined(INCLUDE_PHY_53XXX)
/*
 * Function:
 *      _chk_fiber53xxx
 * Purpose:
 *      Check for using Internal 53XXX SERDES Device for fiber
 * Returns:
 *      TRUE if this PHY matches.
 *      FALSE otherwise.
 * Notes:
 *      This routine should be called after checking for external
 *      PHYs because it will default the PHY driver to the internal
 *      SERDES in the absense of any other PHY.
 */

static int
_chk_fiber53xxx(int unit, soc_port_t port, soc_phy_table_t *my_entry,
               uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    COMPILER_REFERENCE(my_entry);
    COMPILER_REFERENCE(phy_id0);
    COMPILER_REFERENCE(phy_id1);

    if (IS_GE_PORT(unit, port) && !IS_GMII_PORT(unit, port) &&
        soc_feature(unit, soc_feature_dodeca_serdes) && 
        SOC_IS_ROBO(unit)) {
        pi->phy_name = "Phy53XXX";

        return TRUE;
    }

    return FALSE;
}
#endif /* INCLUDE_PHY_53XXX */
/*
 * Function:
 *      _chk_default
 * Purpose:
 *      Select a default PHY driver.
 * Returns:
 *      TRUE
 * Notes:
 *      This routine always "finds" a default PHY driver and can
 *      be the last entry in the PHY table (or called explicitly).
 */

static int
_chk_default(int unit, soc_port_t port, soc_phy_table_t *my_entry,
             uint16 phy_id0, uint16 phy_id1, soc_phy_info_t *pi)
{
    pi->phy_name = my_entry->phy_name;

    return TRUE;
}

STATIC int
_forced_phy_probe(int unit, soc_port_t port,
                  soc_phy_info_t *pi, phy_ctrl_t *ext_pc)
{
#if defined(INCLUDE_PHY_SIMUL)
    /* Similarly, check for simulation driver */
    if (phyd == NULL &&
        _chk_simul(unit, port, &_simul_phy_entry, 0xffff, 0xffff, pi)) {
        ext_pc->pd  = _simul_phy_entry.driver;
        pi->phy_id0 = 0xffff;
        pi->phy_id1 = 0xffff;
    }
#endif

    return SOC_E_NONE;
}

STATIC int
_int_phy_probe(int unit, soc_port_t port,
               soc_phy_info_t *pi, phy_ctrl_t *int_pc)
{
    uint16               phy_addr;
    uint16               phy_id0, phy_id1;
    int                  i;
    int                  rv;
    phy_driver_t         *int_phyd;
#if defined(BCM_KATANA2_SUPPORT)
    uint8 lane_num = 0;
    uint8 phy_mode = 0;
    uint8 chip_num = 0;
#endif

    phy_addr = int_pc->phy_id;
    int_phyd = NULL;
    i = sizeof(_int_phy_table) / sizeof(_int_phy_table[0]);

    /* Make sure page 0 is mapped before reading for PHY dev ID */

#if defined(BCM_TRIUMPH3_SUPPORT) 
    if (SOC_IS_TRIUMPH3(unit) &&
        (SOC_PBMP_MEMBER(SOC_PORT_DISABLED_BITMAP(unit,all), port))) {
        
        phy_id0 = phy_id1 = 0;
    } else 
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        if (SOC_IS_ARDON(unit)) {
            phy_id0 = 0x600d;
            phy_id1 = 0x8770;
        } else {

            if(SOC_IS_SABER2(unit)) {
                /* For saber2, the Sbus mdio access is expected along 
                 * with AER information 
                 */
                (void)int_pc->write(unit, phy_addr, 0x1f, 0xFFD0); 
                (void)int_pc->write(unit, phy_addr, 0x1e, 0); 
            }

            (void)int_pc->write(unit, phy_addr, 0x1f, 0); 

            (void)int_pc->read(unit, phy_addr, MII_PHY_ID0_REG, &phy_id0);
            (void)int_pc->read(unit, phy_addr, MII_PHY_ID1_REG, &phy_id1);

        }

    }

    
    pi->phy_id0       = phy_id0;
    pi->phy_id1       = phy_id1;
    pi->phy_addr_int  = phy_addr;
    int_pc->phy_id0   = phy_id0;
    int_pc->phy_id1   = phy_id1;
    int_pc->phy_oui   = PHY_OUI(phy_id0, phy_id1);
    int_pc->phy_model = PHY_MODEL(phy_id0, phy_id1);
    int_pc->phy_rev   = PHY_REV(phy_id0, phy_id1);
#if LOCAL_DEBUG
    bsl_printf("internal PHY: port=%d phy_id0 = %x phy_id1 %x addr = %x \n",
              port, phy_id0, phy_id1, phy_addr);
#endif
    for (i = i - 1; i >= 0; i--) {
        if ((_int_phy_table[i].checkphy)(unit, port, &_int_phy_table[i],
                                         phy_id0, phy_id1, pi)) {
            /* Device ID matches. Calls driver probe routine to confirm
             * that the driver is the appropriate one.
             * Many PHY devices has the same device ID but they are
             * actually different.
             */

            rv = PHY_PROBE(_int_phy_table[i].driver, unit, int_pc);
            if ((rv == SOC_E_NONE) || (rv == SOC_E_UNAVAIL)) {
#if LOCAL_DEBUG
                bsl_printf("<%d> int Index = %d Mynum = %d %s\n", rv, i, _int_phy_table[i].myNum, _int_phy_table[i].phy_name);
#endif
                int_phyd = _int_phy_table[i].driver;
                break;
            }
        }
    }

#if defined(INCLUDE_XGXS_16G)
    if (IS_GE_PORT(unit, port) && (&phy_xgxs16g1l_ge == int_phyd)) {
        /* using XGXS16G in independent lane mode on GE port.  */
        PHY_FLAGS_SET(unit, port, PHY_FLAGS_INDEPENDENT_LANE);
#if defined(BCM_SCORPION_SUPPORT)
        if (SOC_IS_SCORPION(unit)) {
            switch(port) {
            case 25:
                pi->phy_name = "XGXS16G/1/0";
                int_pc->lane_num = 0;
                break;
            case 26:
                pi->phy_name = "XGXS16G/1/1";
                int_pc->lane_num = 1;      
                break;
            case 27:
                pi->phy_name = "XGXS16G/1/2";
                int_pc->lane_num = 2;
                break;
            case 28:
                pi->phy_name = "XGXS16G/1/3";
                int_pc->lane_num = 3;
                break;
            default:
                break;
            }
        }
#endif
    }
#endif

#if defined(INCLUDE_XGXS_HL65)
    if (IS_GE_PORT(unit, port) && (&phy_hl65_hg == int_phyd)) {
        /* If using HyperLite on GE port, use the HyperLite in independent
         * lane mode.
         */
        PHY_FLAGS_SET(unit, port, PHY_FLAGS_INDEPENDENT_LANE);
        pi->phy_name = "HL65/1";
#if defined(BCM_HURRICANE_SUPPORT)
        if (SOC_IS_HURRICANE(unit)) {
            int_pc->phy_mode = PHYCTRL_DUAL_LANE_PORT;
            int_pc->flags |= PHYCTRL_MDIO_ADDR_SHARE;
            switch (port) {
                case 26:
                    pi->phy_name = "HL65/0/0";
                    int_pc->lane_num = 0;
                    break;
                case 27:
                    pi->phy_name = "HL65/0/2";
                    int_pc->lane_num = 2;
                    break;
                case 28:
                    pi->phy_name = "HL65/1/0";
                    int_pc->lane_num = 0;
                    break;
                case 29:
                    pi->phy_name = "HL65/1/2";
                    int_pc->lane_num = 2;
                    break;
                default:
                    break;
            }
        }
#endif
#if defined(BCM_VALKYRIE_SUPPORT) || defined(BCM_TRIUMPH_SUPPORT)
        if (SOC_IS_VALKYRIE(unit) || SOC_IS_TRIUMPH(unit)) {
            switch(port) {
            case 2:
            case 6:
            case 14:
            case 26:
            case 27:
            case 35:
                pi->phy_name = "HL65/1/0";
                int_pc->lane_num = 0;
                break;

            case 3:
            case 7:
            case 15:
            case 32:
            case 36:
            case 43:
                pi->phy_name = "HL65/1/1";
                int_pc->lane_num = 1;
                break;

            case 4:
            case 16:
            case 18:
            case 33:
            case 44:
            case 46:
                pi->phy_name = "HL65/1/2";
                int_pc->lane_num = 2;
                break;

            case 5:
            case 17:
            case 19:
            case 34:
            case 45:
            case 47:
                pi->phy_name = "HL65/1/3";
                int_pc->lane_num = 3;
                break;

            default:
                break;

            }
        }
#endif /* BCM_VALKYRIE_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT) || defined(BCM_APOLLO_SUPPORT) || \
    defined(BCM_VALKYRIE2_SUPPORT)
        if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
            SOC_IS_VALKYRIE2(unit)) {
            switch(port) {
            case 30:
            case 34:
            case 38:
            case 42:
            case 46:
            case 50:
                pi->phy_name = "HL65/1/0";
                int_pc->lane_num = 0;
                break;

            case 31:
            case 35:
            case 39:
            case 43:
            case 47:
            case 51:
                pi->phy_name = "HL65/1/1";
                int_pc->lane_num = 1;
                break;

            case 32:
            case 36:
            case 40:
            case 44:
            case 48:
            case 52:
                pi->phy_name = "HL65/1/2";
                int_pc->lane_num = 2;
                break;

            case 33:
            case 37:
            case 41:
            case 45:
            case 49:
            case 53:
                pi->phy_name = "HL65/1/3";
                int_pc->lane_num = 3;
                break;

            default:
                break;

            }
        }
#endif /* BCM_TRIUMPH2_SUPPORT */
    }

#if defined(BCM_HURRICANE_SUPPORT)
        if ((&phy_hl65_hg == int_phyd) && SOC_IS_HURRICANE(unit) && ((IS_HG_PORT(unit, port)) || IS_XE_PORT(unit, port))) {
            if ((port == 26) && (!SOC_PORT_VALID(unit, 27))) {
                LOG_INFO(BSL_LS_SOC_PHY,
                         (BSL_META_U(unit,
                                     "Port 26 in combo mode\n")));
                pi->phy_name = "HL65/0";
            } else if ((port == 28) && (!SOC_PORT_VALID(unit, 29))) {
                LOG_INFO(BSL_LS_SOC_PHY,
                         (BSL_META_U(unit,
                                     "Port 28 in combo mode\n")));
                pi->phy_name = "HL65/1";
            } else {
                LOG_INFO(BSL_LS_SOC_PHY,
                         (BSL_META_U(unit,
                                     "Port %d in HGd mode\n"), port));
                PHY_FLAGS_SET(unit, port, PHY_FLAGS_INDEPENDENT_LANE);
                int_pc->phy_mode = PHYCTRL_DUAL_LANE_PORT;
                int_pc->flags |= PHYCTRL_MDIO_ADDR_SHARE;
                switch (port) {
                    case 26:
                        pi->phy_name = "HL65/0/0";
                        int_pc->lane_num = 0;
                        break;
                    case 27:
                        pi->phy_name = "HL65/0/2";
                        int_pc->lane_num = 2;
                        break;
                    case 28:
                        pi->phy_name = "HL65/1/0";
                        int_pc->lane_num = 0;
                        break;
                    case 29:
                        pi->phy_name = "HL65/1/2";
                        int_pc->lane_num = 2;
                        break;
                    default:
                        break;
                }
            }
        }
#endif /* BCM_HURRICANE_SUPPORT */

#if defined(BCM_BM9600_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)
    if (&phy_hl65_hg == int_phyd) {
        if (SOC_IS_SBX_BM9600(unit)) {
            static char *bm9600_phy_names[] = {
                "HC65/0/0",  "HC65/0/1",  "HC65/0/2",  "HC65/0/3",  
                "HC65/1/0",  "HC65/1/1",  "HC65/1/2",  "HC65/1/3",  
                "HC65/2/0",  "HC65/2/1",  "HC65/2/2",  "HC65/2/3",  
                "HC65/3/0",  "HC65/3/1",  "HC65/3/2",  "HC65/3/3",  
                "HC65/4/0",  "HC65/4/1",  "HC65/4/2",  "HC65/4/3",  
                "HC65/5/0",  "HC65/5/1",  "HC65/5/2",  "HC65/5/3",  
                "HC65/6/0",  "HC65/6/1",  "HC65/6/2",  "HC65/6/3",  
                "HC65/7/0",  "HC65/7/1",  "HC65/7/2",  "HC65/7/3",  
                "HC65/8/0",  "HC65/8/1",  "HC65/8/2",  "HC65/8/3",  
                "HC65/9/0",  "HC65/9/1",  "HC65/9/2",  "HC65/9/3",  
                "HC65/10/0", "HC65/10/1", "HC65/10/2", "HC65/10/3", 
                "HC65/11/0", "HC65/11/1", "HC65/11/2", "HC65/11/3", 
                "HC65/12/0", "HC65/12/1", "HC65/12/2", "HC65/12/3", 
                "HC65/13/0", "HC65/13/1", "HC65/13/2", "HC65/13/3", 
                "HC65/14/0", "HC65/14/1", "HC65/14/2", "HC65/14/3", 
                "HC65/15/0", "HC65/15/1", "HC65/15/2", "HC65/15/3", 
                "HC65/16/0", "HC65/16/1", "HC65/16/2", "HC65/16/3", 
                "HC65/17/0", "HC65/17/1", "HC65/17/2", "HC65/17/3", 
                "HC65/18/0", "HC65/18/1", "HC65/18/2", "HC65/18/3", 
                "HC65/19/0", "HC65/19/1", "HC65/19/2", "HC65/19/3", 
                "HC65/20/0", "HC65/20/1", "HC65/20/2", "HC65/20/3", 
                "HC65/21/0", "HC65/21/1", "HC65/21/2", "HC65/21/3", 
                "HC65/22/0", "HC65/22/1", "HC65/22/2", "HC65/22/3", 
                "HC65/23/0", "HC65/23/1", "HC65/23/2", "HC65/23/3" 
            }; 
            pi->phy_name = bm9600_phy_names[port];
            int_pc->lane_num = port % 4;
            int_pc->flags |= PHYCTRL_MDIO_ADDR_SHARE;
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_HC65_FABRIC);
        } else {
            if (SOC_IS_SIRIUS(unit)) {
                static struct {
                    char  *name;
                    int    lane;
                } _sirius_phy_port_info[] = {
                    { "HC65/4/2", 2 },  /* port 9 , sfi0,  lane 2 */
                    { "HC65/4/3", 3 },  /* port 10, sfi1,  lane 3 */
                    { "HC65/5/0", 0 },  /* port 11, sfi2,  lane 0 */
                    { "HC65/5/1", 1 },  /* port 12, sfi3,  lane 1 */
                    { "HC65/5/2", 2 },  /* port 13, sfi4,  lane 2 */
                    { "HC65/5/3", 3 },  /* port 14, sfi5,  lane 3 */
                    { "HC65/6/0", 0 },  /* port 15, sfi6,  lane 0 */
                    { "HC65/6/1", 1 },  /* port 16, sfi7,  lane 1 */
                    { "HC65/6/2", 2 },  /* port 17, sfi8,  lane 2 */
                    { "HC65/6/3", 3 },  /* port 18, sfi9,  lane 3 */
                    { "HC65/7/0", 0 },  /* port 19, sfi10, lane 0 */
                    { "HC65/7/1", 1 },  /* port 20, sfi11, lane 1 */
                    { "HC65/7/2", 2 },  /* port 21, sfi12, lane 2 */
                    { "HC65/7/3", 3 },  /* port 22, sfi13, lane 3 */
                    { "HC65/8/0", 0 },  /* port 23, sfi14, lane 0 */
                    { "HC65/8/1", 1 },  /* port 24, sfi15, lane 1 */
                    { "HC65/8/2", 2 },  /* port 25, sfi16, lane 2 */
                    { "HC65/8/3", 3 },  /* port 26, sfi17, lane 3 */
                    { "HC65/9/0", 0 },  /* port 27, sfi18, lane 0 */
                    { "HC65/9/1", 1 },  /* port 28, sfi19, lane 1 */
                    { "HC65/9/2", 2 },  /* port 29, sfi20, lane 2 */
                    { "HC65/9/3", 3 },  /* port 30, sfi21, lane 3 */
                    { "HC65/4/0", 0 },  /* port 31, sci0,  lane 0 */
                    { "HC65/4/1", 1 }   /* port 32, sci1,  lane 1 */
                };
                /*
                 * Make sure port is SFI or SCI. 
                 *  0..7  : Higig 
                 *  8     : CPU
                 *  9..30 : SFI
                 *  31-32 : SCI
                 */
                if (port >= 9 && port <= 32) {
                    int port_idx = port - 9;
                    pi->phy_name = _sirius_phy_port_info[port_idx].name;
                    int_pc->lane_num = _sirius_phy_port_info[port_idx].lane;
                    int_pc->flags |= PHYCTRL_MDIO_ADDR_SHARE;
                    PHY_FLAGS_SET(unit, port, PHY_FLAGS_HC65_FABRIC);
                }
            }
        }
    }
#endif /* BCM_BM9600_SUPPORT || BCM_SIRIUS_SUPPORT */
#endif /* INCLUDE_XGXS_HL65 */

#if defined(INCLUDE_XGXS_QSGMII65)
    if (IS_GE_PORT(unit, port) && (&phy_qsgmii65_ge == int_phyd)) {
        pi->phy_name = "QSGMII65";
#if defined(BCM_HAWKEYE_SUPPORT)
        if (SOC_IS_HAWKEYE(unit)) {
            switch(port) {
            case 9:
            case 17:
                pi->phy_name = "QSGMII65/0";
                int_pc->lane_num = 0;
                break;

            case 10:
            case 18:
                pi->phy_name = "QSGMII65/1";
                int_pc->lane_num = 1;
                break;

            case 11:
            case 19:
                pi->phy_name = "QSGMII65/2";
                int_pc->lane_num = 2;
                break;

            case 12:
            case 20:
                pi->phy_name = "QSGMII65/3";
                int_pc->lane_num = 3;
                break;

            case 13:
            case 21:
                pi->phy_name = "QSGMII65/4";
                int_pc->lane_num = 4;
                break;

            case 14:
            case 22:
                pi->phy_name = "QSGMII65/5";
                int_pc->lane_num = 5;
                break;

            case 15:
            case 23:
                pi->phy_name = "QSGMII65/6";
                int_pc->lane_num = 6;
                break;

            case 16:
            case 24:
                pi->phy_name = "QSGMII65/7";
                int_pc->lane_num = 7;
                break;

            default:
                break;

            }
        }
#endif /* SOC_IS_HAWKEYE */
#if defined(BCM_HURRICANE_SUPPORT)
        if (SOC_IS_HURRICANE(unit)) {
            switch(port) {
            case 2:
            case 10:
            case 18:
                pi->phy_name = "QSGMII65/0";
                int_pc->lane_num = 0;
                break;

            case 3:
            case 11:
            case 19:
                pi->phy_name = "QSGMII65/1";
                int_pc->lane_num = 1;
                break;

            case 4:
            case 12:
            case 20:
                pi->phy_name = "QSGMII65/2";
                int_pc->lane_num = 2;
                break;

            case 5:
            case 13:
            case 21:
                pi->phy_name = "QSGMII65/3";
                int_pc->lane_num = 3;
                break;

            case 6:
            case 14:
            case 22:
                pi->phy_name = "QSGMII65/4";
                int_pc->lane_num = 4;
                break;

            case 7:
            case 15:
            case 23:
                pi->phy_name = "QSGMII65/5";
                int_pc->lane_num = 5;
                break;

            case 8:
            case 16:
            case 24:
                pi->phy_name = "QSGMII65/6";
                int_pc->lane_num = 6;
                break;

            case 9:
            case 17:
            case 25:
                pi->phy_name = "QSGMII65/7";
                int_pc->lane_num = 7;
                break;

            default:
                break;

            }
        }
#endif /* BCM_HURRICANE_SUPPORT */
    }
#endif /* INCLUDE_XGXS_QSGMII65 */

#if defined(BCM_GREYHOUND_SUPPORT)
    if (SOC_IS_GREYHOUND(unit) && 
            (IS_GE_PORT(unit, port) && !IS_XL_PORT(unit, port))) {
        int_pc->lane_num = (port - 2) % 8;  /* lane_num in {0~7} */
        int_pc->chip_num = (port - 2) / 8;  /* chip_num in {0,1}*/ 
        int_pc->phy_mode = PHYCTRL_QSGMII_CORE_PORT;
        int_pc->flags |= PHYCTRL_MDIO_ADDR_SHARE; 
    }
#endif  /* BCM_GREYHOUND_SUPPORT */

#if defined(BCM_KATANA2_SUPPORT)
    if ((SOC_IS_KATANA2(unit)) && (!SOC_IS_SABER2(unit)) &&
        (port <= KT2_MAX_PHYSICAL_PORTS)) {
        if (katana2_get_wc_phy_info(unit,port,&lane_num, &phy_mode, &chip_num) == SOC_E_NONE) {
            int_pc->lane_num = lane_num;
            int_pc->phy_mode = phy_mode;
            int_pc->chip_num = chip_num;
            int_pc->flags |= PHYCTRL_MDIO_ADDR_SHARE;
            /* PHY_FLAGS_SET(unit, port, PHY_FLAGS_INDEPENDENT_LANE); */
        }
        if ((SOC_INFO(unit).olp_port[0]) && (port == KT2_OLP_PORT)) {
            int_pc->lane_num = 2;
        }
    }

#endif

#if defined(INCLUDE_PHY_56XXX) && defined(INCLUDE_PHY_XGXS6)
    /* If we detetcted a Unicore driver for a GE port, attach internal SerDes
     * driver.
     * Current Unicore driver does not support external GE PHY.
     */
    if (IS_GE_PORT(unit, port) &&  (&phy_xgxs6_hg == int_phyd)) {
        if (_chk_fiber56xxx(unit, port, &_fiber56xxx_phy_entry,
                       phy_id0, phy_id1, pi)) {
            int_phyd = &phy_56xxxdrv_ge;
        }
    }
#endif /* INCLUDE_PHY_56XXX && INCLUDE_PHY_XGXS6 */


#if defined(INCLUDE_PHY_56XXX) 
    /* If we detected a shadow register driver, allocate driver data */
    if (&phy_56xxx_5601x_drv_ge == int_phyd) {
        serdes_5601x_sregs_t *sr;

        /* Allocate shadow registers */
        sr = sal_alloc(sizeof(serdes_5601x_sregs_t), 
                       "SERDES_COMBO shadow regs");
        if (sr == NULL) {
            return SOC_E_MEMORY;
        }
        int_pc->driver_data = sr;
    }
#endif /* INCLUDE_PHY_56XXX */

#if defined(INCLUDE_SERDES_COMBO)
    /* If we detected a shadow register driver, allocate driver data */
    if (&phy_serdescombo_5601x_ge == int_phyd) {
        serdescombo_5601x_sregs_t *sr;

        /* Allocate shadow registers */
        sr = sal_alloc(sizeof(serdescombo_5601x_sregs_t), 
                       "SERDES_COMBO shadow regs");
        if (sr == NULL) {
            return SOC_E_MEMORY;
        }
        int_pc->driver_data = sr;
    }
#endif /* INCLUDE_SERDES_COMBO */

#if defined(INCLUDE_PHY_53XXX)
    /* If we detetcted a Unicore driver for a GE port, attach internal SerDes
     * driver.
     * Current Unicore driver does not support external GE PHY.
     */
    if (IS_GE_PORT(unit, port)) {
        if (_chk_fiber53xxx(unit, port, &_fiber53xxx_phy_entry,
                       phy_id0, phy_id1, pi)) {
            int_phyd = &phy_53xxxdrv_ge;
        }
    }
#endif /* INCLUDE_PHY_53XXX */

#ifdef BCM_XGS_SUPPORT
    if (int_phyd == NULL) {
        if (IS_XE_PORT(unit, port) || IS_HG_PORT(unit, port)) {
            /* If no appropriate driver is installed in the phy driver table
             * use a default higig driver for XE port */
#if defined (INCLUDE_PHY_XGXS6)
            if (soc_feature(unit, soc_feature_xgxs_v6)) {
                int_phyd = &phy_xgxs6_hg;
            } else
#endif /* INCLUDE_PHY_XGXS6*/
#if defined(INCLUDE_PHY_XGXS5)
            if (soc_feature(unit, soc_feature_xgxs_v5)) {
                int_phyd = &phy_xgxs5_hg;
            } else
#endif /* INCLUDE_PHY_XGXS5 */
            {
              //  int_phyd = &phy_xgxs1_hg;  
            }
        }
    }
#endif /* BCM_XGS_SUPPORT */

#if defined(BCM_XGS3_SWITCH_SUPPORT) || defined(BCM_ROBO_SUPPORT)
    /* If GMII port, attach NULL PHY driver to
     * internal PHY driver. GMII port does not have SerDes.
     */
    if (_chk_gmii(unit, port, &_null_phy_entry, 0xffff, 0xffff, pi)) {
        int_phyd = _null_phy_entry.driver;
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT || BCM_ROBO_SUPPORT */

    int_pc->pd = int_phyd;

    return SOC_E_NONE;
}



int
_ext_phy_probe(int unit, soc_port_t port,
               soc_phy_info_t *pi, phy_ctrl_t *ext_pc)
{
    uint16               phy_addr;
    uint32               id0_addr, id1_addr;
    uint16               phy_id0=0, phy_id1=0;
    int                  i;
    phy_driver_t        *phyd;
    int                  rv;
    int                  cl45_override = 0;
    char                 *propval;
#ifdef BCM_NORTHSTAR_SUPPORT
    uint16               dev_id = 0;
    uint8                rev_id = 0;
#endif /* BCM_NORTHSTAR_SUPPORT */

    phy_addr = ext_pc->phy_id;
    phyd     = NULL;

    if (SOC_IS_ARDON(unit)) {
        ext_pc->pd = NULL;
        return SOC_E_NONE;
    }
    /* Clause 45 instead of Clause 22 MDIO access */
    if (soc_property_port_get(unit, port, spn_PORT_PHY_CLAUSE, 22) == 45) {
        cl45_override = 1;
    }

    if (IS_XE_PORT(unit, port) || IS_HG_PORT(unit, port) || IS_CE_PORT(unit, port) || SOC_IS_ARAD(unit)) {
        if (SOC_IS_CALADAN3(unit) || SOC_IS_ARAD(unit)) {
            if (cl45_override) {
                id0_addr = SOC_PHY_CLAUSE45_ADDR(PHY_C45_DEV_PMA_PMD,
                                         MII_PHY_ID0_REG);
                id1_addr = SOC_PHY_CLAUSE45_ADDR(PHY_C45_DEV_PMA_PMD,
                                         MII_PHY_ID1_REG);
            } else {
                id0_addr = MII_PHY_ID0_REG;
                id1_addr = MII_PHY_ID1_REG;
            }
        } else {
            id0_addr = SOC_PHY_CLAUSE45_ADDR(PHY_C45_DEV_PMA_PMD,
                                         MII_PHY_ID0_REG);
            id1_addr = SOC_PHY_CLAUSE45_ADDR(PHY_C45_DEV_PMA_PMD,
                                         MII_PHY_ID1_REG);
        }
    } else if ( IS_GE_PORT(unit, port) ) {
        if (cl45_override) {
            id0_addr = SOC_PHY_CLAUSE45_ADDR(PHY_C45_DEV_PMA_PMD,
                                     MII_PHY_ID0_REG);
            id1_addr = SOC_PHY_CLAUSE45_ADDR(PHY_C45_DEV_PMA_PMD,
                                     MII_PHY_ID1_REG);
        } else {
            id0_addr = MII_PHY_ID0_REG;
            id1_addr = MII_PHY_ID1_REG;
        }
    } else {
#ifdef BCM_SIRIUS_SUPPORT
        if (SOC_IS_SIRIUS(unit)) {
            /* Do not probe GE ports in Sirius */
            ext_pc->pd  = NULL;
            return SOC_E_NONE;
        }
#endif /* BCM_SIRIUS_SUPPORT */
        id0_addr = MII_PHY_ID0_REG;
        id1_addr = MII_PHY_ID1_REG;
    }

    if (soc_property_port_get(unit, port,
                              spn_PORT_PHY_PRECONDITION_BEFORE_PROBE, 0)) {
       for (i = (sizeof(_standard_phy_table)/sizeof(_standard_phy_table[0]) - 1) ; i >= 0; i--) {
           rv = PHY_PRECONDITION_BEFORE_PROBE(_standard_phy_table[i].driver,
                                               unit, ext_pc);
           if (rv == SOC_E_NONE) {
               break;
           }
       }
    }

    propval = soc_property_port_get_str(unit, port, spn_PORT_PHY_ID0);
    if (propval != NULL){
        phy_id0 = soc_property_port_get(unit, port, spn_PORT_PHY_ID0, 0xFFFF);
    }
    else{
        (void)ext_pc->read(unit, phy_addr, id0_addr, &phy_id0);
    }
    propval = soc_property_port_get_str(unit, port, spn_PORT_PHY_ID1);
    if (propval != NULL){
        phy_id1 = soc_property_port_get(unit, port, spn_PORT_PHY_ID1, 0xFFFF);
    }
    else{
        (void)ext_pc->read(unit, phy_addr, id1_addr, &phy_id1);
    }
#if LOCAL_DEBUG    
    bsl_printf("enternal PHY: port=%d phy_id0 = %x phy_id1 %x addr = %x \n",
              port, phy_id0, phy_id1, phy_addr);
#endif
    /* Look through table for match */
    for (i = (sizeof(_standard_phy_table)/sizeof(soc_phy_table_t) - 1); i >= 0; i--) {
        if ((_standard_phy_table[i].checkphy)(unit, port, (soc_phy_table_t *) &_standard_phy_table[i],
                                     phy_id0, phy_id1, pi)) {
            /* Device ID matches. Calls driver probe routine to confirm
             * that the driver is the appropriate one.
             * Many PHY devices has the same device ID but they are
             * actually different.
             */
            rv = PHY_PROBE(_standard_phy_table[i].driver, unit, ext_pc);
            if ((rv == SOC_E_NONE) || (rv == SOC_E_UNAVAIL)) {
#if LOCAL_DEBUG    
                bsl_printf("<%d> ext Index = %d Mynum = %d %s\n", rv, i, _standard_phy_table[i].myNum, _standard_phy_table[i].phy_name);
#endif
                phyd = _standard_phy_table[i].driver;
                pi->phy_id0       = phy_id0;
                pi->phy_id1       = phy_id1;
                pi->phy_addr      = phy_addr;
                if (ext_pc->dev_name) {
                    pi->phy_name      = ext_pc->dev_name;
                }
                ext_pc->phy_id0   = phy_id0;
                ext_pc->phy_id1   = phy_id1;
                ext_pc->phy_oui   = PHY_OUI(phy_id0, phy_id1);
                ext_pc->phy_model = PHY_MODEL(phy_id0, phy_id1);
                ext_pc->phy_rev   = PHY_REV(phy_id0, phy_id1);

#ifdef BCM_NORTHSTAR_SUPPORT
                if (SOC_IS_NORTHSTAR(unit)) {
                    soc_cm_get_id(unit, &dev_id, &rev_id);
                    ext_pc->phy_rev = rev_id;
                }
#endif /* BCM_NORTHSTAR_SUPPORT */

                PHY_FLAGS_SET(unit, port, PHY_FLAGS_EXTERNAL_PHY);

                break;
            }
        }
    }

#if defined(INCLUDE_PHY_5464_ESW) && defined(INCLUDE_PHY_5464_ROBO)
    
    if (SOC_IS_ROBO(unit) && (phyd == &phy_5464drv_ge)) {
        phyd = &phy_5464robodrv_ge;
    }
#endif /* INCLUDE_PHY_5464_ESW && INCLUDE_PHY_5464_ROBO */

#if defined(INCLUDE_PHY_5482_ESW) && defined(INCLUDE_PHY_5482_ROBO)
    
    if (SOC_IS_ROBO(unit) && (phyd == &phy_5482drv_ge)) {
        phyd = &phy_5482robodrv_ge;
    }
#endif /* INCLUDE_PHY_5464_ESW && INCLUDE_PHY_5464_ROBO */

#if defined(INCLUDE_PHY_54684)
    if (IS_GE_PORT(unit, port) && (phyd == &phy_54684drv_ge)) {
#if defined(BCM_HAWKEYE_SUPPORT)
        if (SOC_IS_HAWKEYE(unit)) {
            switch(port) {
            case 9:
            case 17:
                ext_pc->lane_num = 0;
                break;

            case 10:
            case 18:
                ext_pc->lane_num = 1;
                break;

            case 11:
            case 19:
                ext_pc->lane_num = 2;
                break;

            case 12:
            case 20:
                ext_pc->lane_num = 3;
                break;

            case 13:
            case 21:
                ext_pc->lane_num = 4;
                break;

            case 14:
            case 22:
                ext_pc->lane_num = 5;
                break;

            case 15:
            case 23:
                ext_pc->lane_num = 6;
                break;

            case 16:
            case 24:
                ext_pc->lane_num = 7;
                break;

            default:
                break;

            }
        }
#endif /* SOC_IS_HAWKEYE */
#if defined(BCM_HURRICANE_SUPPORT)
        if (SOC_IS_HURRICANE(unit)) {
            switch(port) {
            case 2:
            case 10:
            case 18:
                ext_pc->lane_num = 0;
                break;

            case 3:
            case 11:
            case 19:
                ext_pc->lane_num = 1;
                break;

            case 4:
            case 12:
            case 20:
                ext_pc->lane_num = 2;
                break;

            case 5:
            case 13:
            case 21:
                ext_pc->lane_num = 3;
                break;

            case 6:
            case 14:
            case 22:
                ext_pc->lane_num = 4;
                break;

            case 7:
            case 15:
            case 23:
                ext_pc->lane_num = 5;
                break;

            case 8:
            case 16:
            case 24:
                ext_pc->lane_num = 6;
                break;

            case 9:
            case 17:
            case 25:
                ext_pc->lane_num = 7;
                break;

            default:
                break;

            }
        }
#endif /* BCM_HURRICANE_SUPPORT */
    }
#endif /* INCLUDE_PHY_54684 */

    ext_pc->pd = phyd;

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_phy_probe
 * Purpose:
 *      Probe the PHY on the specified port and return a pointer to the
 *      drivers for the device found.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      phyd_ptr - (OUT) Pointer to PHY driver (NULL on error)
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Loop thru table making callback for each known PHY.
 *      We loop from the table from top to bottom so that user additions
 *      take precedence over default values.  The first checkphy function
 *      returning TRUE is used as the driver.
 */
int
soc_phy_probe(int unit, soc_port_t port, phy_ctrl_t *ext_pc,
              phy_ctrl_t *int_pc)
{
    soc_phy_info_t      *pi;
#ifdef BCM_ROBO_SUPPORT
    uint16               phy_id0 = 0, phy_id1 = 0;
    uint32               id0_addr = 0, id1_addr = 0;
#endif /* BCM_ROBO_SUPPORT */

    /* Always use default addresses for probing.
     * This make sure that the external PHY probe works correctly even
     * when the device is hot plugged or the external PHY address is
     * overriden from previous probe.
     */
    int_pc->phy_id = PHY_ADDR_INT(unit, port);
    ext_pc->phy_id = PHY_ADDR(unit, port);

    /*
     * Characterize PHY by reading MII registers.
     */
    pi       = &SOC_PHY_INFO(unit, port);

    /* Probe for null PHY configuration first to avoid MII timeouts */

    if (_chk_null(unit, port, &_null_phy_entry, 0xffff, 0xffff, pi)) {
        ext_pc->pd     = _null_phy_entry.driver;
        int_pc->pd     = _null_phy_entry.driver;
    }

#ifdef PORTMOD_SUPPORT    
    if (soc_feature(unit, soc_feature_portmod)) {
        /* If portmod is enabled all the phymod compliant 
         * phy probes would be handled by portmod. All the 
         * internal phys are phymod compliant. We just need 
         * to probe for legacy external phys */
        
        /* Assign catch all extphy_to_phymod pd table to intpc
         * to handle all the notifications for internal phy
         * from external phy */

        int_pc->pd     = &phy_ext_to_int_drv; 

    } else {
#endif /* PORTMOD_SUPPORT */
    /* Search for internal phy */
    if (NULL == int_pc->pd) {
        SOC_IF_ERROR_RETURN
            (_int_phy_probe(unit, port, pi, int_pc));
    }

#ifdef PORTMOD_SUPPORT
    }
#endif
    /* Search for external PHY */
    if (NULL == ext_pc->pd) {
        SOC_IF_ERROR_RETURN
            (_ext_phy_probe(unit, port, pi, ext_pc));
    }


    /* Override external PHY driver according to config settings */
    SOC_IF_ERROR_RETURN
        (_forced_phy_probe(unit, port, pi, ext_pc));

#ifdef BCM_ROBO_SUPPORT
    if (SOC_IS_ROBO(unit)) {
        if (IS_GMII_PORT(unit, port) && (ext_pc->pd == NULL)) {
            id0_addr = MII_PHY_ID0_REG;
            id1_addr = MII_PHY_ID1_REG;
            phy_id0 = 0xFFFF;
            phy_id1 = 0xFFFF;
    
            (void)ext_pc->read(unit, phy_addr, id0_addr, &phy_id0);
            (void)ext_pc->read(unit, phy_addr, id1_addr, &phy_id1);

            if ((phy_id0 != 0) && (phy_id0 != 0xFFFF) &&
                (phy_id1 != 0) && (phy_id1 != 0xFFFF) &&
                _chk_default(unit, port, &_default_phy_entry_ge, 0xffff, 0xffff, pi)) {
                ext_pc->pd = _default_phy_entry_ge.driver;
            }
        }
    }
#endif /* BCM_ROBO_SUPPORT */

    if (ext_pc->pd != NULL) {
        if (IS_GMII_PORT(unit, port)) {
            /* If GMII port has external PHY, remove the NULL PHY driver
             * attached to internal PHY in _int_phy_probe().
             */
            int_pc->pd = NULL;
        }
        if ((int_pc->pd == _null_phy_entry.driver) &&
            (ext_pc->pd == _null_phy_entry.driver)) {
            /* Attach NULL PHY driver as external PHY driver */
            int_pc->pd = NULL;
        }
    }

   if ((ext_pc->pd == NULL) && (int_pc->pd == NULL) &&
        _chk_default(unit, port, &_default_phy_entry, 0xffff, 0xffff, pi)) {
        ext_pc->pd = _default_phy_entry.driver;
    }

    assert((ext_pc->pd != NULL) || (int_pc->pd != NULL));

    if (ext_pc->pd == NULL ||        /* No external PHY */
        ext_pc->pd == int_pc->pd) {  /* Forced PHY */
        /* Use internal address when application trying to access
         * external PHY.
         */
        pi->phy_addr = pi->phy_addr_int;

        /* If there is no external PHY, the internal PHY must be in
         * fiber mode.
         */
        if (soc_property_port_get(unit, port,
                                      spn_SERDES_FIBER_PREF, 1)) {
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_FIBER);
        } else {
            PHY_FLAGS_CLR(unit, port, PHY_FLAGS_FIBER);
        }
    }

    /*
     * The property if_tbi_port<X> can be used to force TBI mode on a
     * port.  The individual PHY drivers should key off this flag.
     */
    if (soc_property_port_get(unit, port, spn_IF_TBI, 0)) {
        PHY_FLAGS_SET(unit, port, PHY_FLAGS_10B);
    }

    pi->an_timeout =
        soc_property_port_get(unit, port,
                              spn_PHY_AUTONEG_TIMEOUT, 250000);

    LOG_INFO(BSL_LS_SOC_PHY,
             (BSL_META_U(unit,
                         "soc_phy_probe: port=%d addr=0x%x "
                         "id1=0x%x id0=0x%x flg=0x%x driver=\"%s\"\n"),
              port,
              pi->phy_addr, pi->phy_id0, pi->phy_id1,
              pi->phy_flags, pi->phy_name));

    return SOC_E_NONE;
}

int
soc_phy_check_sim(int unit, int port, phy_ctrl_t *pc)
{
    return SOC_E_NONE;
}

#ifdef BCM_CALADAN3_SUPPORT
/*
 * Function:
 *      soc_caladan3_phy_addr_multi_get
 * Purpose:
 *      Provide a list of internal MDIO addresses corresponding to a port.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      array_max - maximum number of elements in mdio_addr array.
 *      array_size - (OUT) number of valid elements returned in mdio_addr.
 *      mdio_addr - (OUT) list of internal mdio addresses for the port.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Return the list of relevant internal MDIO addresses connected
 *      to a given port.
 *      Currently, only 100G+ ports require multiple MDIO addresses.
 */
STATIC int
soc_sbx_caladan3_phy_addr_multi_get(int unit, soc_port_t port, int array_max,
                       int *array_size, phyident_core_info_t *core_info)
{
    soc_info_t *si;
    int idx = 0;

    si = &SOC_INFO(unit);

    if ((si->port_num_lanes[port] >= 10) || (si->port_type[port] == SOC_BLK_IL)) {
        if (array_max < 3) {
            return SOC_E_PARAM;
        }
        if (soc_sbx_caladan3_is_line_port(unit, port)) {
            core_info[idx].core_type = phyident_core_type_wc;
            core_info[idx].mld_index = 0;
            core_info[idx].index_in_mld = 0;
            core_info[idx].mdio_addr = 0x81;
            idx++;
           
            core_info[idx].core_type = phyident_core_type_wc;
            core_info[idx].mld_index = 0;
            core_info[idx].index_in_mld = 1;
            core_info[idx].mdio_addr = 0xA1;
            idx++;

            core_info[idx].core_type = phyident_core_type_wc;
            core_info[idx].mld_index = 0;
            core_info[idx].index_in_mld = 2;
            core_info[idx].mdio_addr = 0xC1;
            idx++;

            core_info[idx].core_type = phyident_core_type_mld;
            core_info[idx].mld_index = 0;
            core_info[idx].index_in_mld = 0;
            core_info[idx].mdio_addr = 0xD5;
            idx++;
        } else {
            core_info[idx].core_type = phyident_core_type_wc;
            core_info[idx].mld_index = 1;
            core_info[idx].index_in_mld = 0;
            core_info[idx].mdio_addr = 0x95;
            idx++;

            core_info[idx].core_type = phyident_core_type_wc;
            core_info[idx].mld_index = 1;
            core_info[idx].index_in_mld = 1;
            core_info[idx].mdio_addr = 0x99;
            idx++;

            if (((si->port_num_lanes[port] > 8) && (si->port_type[port] == SOC_BLK_IL)) ||
                (si->port_num_lanes[port] >= 10)) {
                /* IL narrow band mode doesn't need to config 3 wc */
                core_info[idx].core_type = phyident_core_type_wc;
                core_info[idx].mld_index = 1;
                core_info[idx].index_in_mld = 2;
                core_info[idx].mdio_addr = 0xB5;
                idx++;
            }

            core_info[idx].core_type = phyident_core_type_mld;
            core_info[idx].mld_index = 1;
            core_info[idx].index_in_mld = 0;
            core_info[idx].mdio_addr = 0xD6;
            idx++;
        }
    } else if (si->port_speed_max[port] == 1000) {
        /* QGMII mode, format is QSMII mdio, WC mdio, QSGMII lane */
        core_info[idx].core_type = phyident_core_type_qsmii;
        core_info[idx].mdio_addr = PHY_ADDR_INT(unit, port);
        core_info[idx].qsmii_lane = (port % 16) >> 3;
        idx++;

        core_info[idx].core_type = phyident_core_type_wc;
        _bcm88030_wc_phy_addr(unit, port, &(core_info[idx].mdio_addr));
        idx++;

    } else {
        core_info[idx].core_type = phyident_core_type_wc;
        core_info[idx].mdio_addr = PHY_ADDR_INT(unit, port);
        idx++;
    }

    *array_size = idx;
    return SOC_E_NONE;
}
#endif

void 
phyident_core_info_t_init(phyident_core_info_t* core_info) {
    core_info->mdio_addr = 0xFFFF;
    core_info->core_type = phyident_core_types_count;
    core_info->mld_index = PHYIDENT_INFO_NOT_SET;
    core_info->index_in_mld = PHYIDENT_INFO_NOT_SET;
    core_info->qsmii_lane = PHYIDENT_INFO_NOT_SET;
    core_info->first_phy_in_core = 0;
    core_info->nof_lanes_in_core = 4;
}

/*
 * Function:
 *      soc_phy_addr_multi_get
 * Purpose:
 *      Provide a list of internal MDIO addresses corresponding to a port.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      array_max - maximum number of elements in mdio_addr array.
 *      array_size - (OUT) number of valid elements returned in mdio_addr.
 *      mdio_addr - (OUT) list of internal mdio addresses for the port.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Return the list of relevant internal MDIO addresses connected
 *      to a given port.
 *      Currently, only 100G+ ports require multiple MDIO addresses.
 */
int
soc_phy_addr_multi_get(int unit, soc_port_t port, int array_max,
                       int *array_size, phyident_core_info_t *core_info)
{
    int addr_num = 0;
    int i;

    if ((0 >= array_max) || (NULL == array_size) || (NULL == core_info)) {
        return SOC_E_PARAM;
    }

    for(i=0 ; i<array_max ; i++) {
        phyident_core_info_t_init(&(core_info[i]));
    }

#ifdef BCM_PETRA_SUPPORT
    if(SOC_IS_DPP(unit)) {
        return _dpp_phy_addr_multi_get(unit, port, 1, array_max, array_size, core_info);
    }
#endif
#ifdef BCM_CALADAN3_SUPPORT
    if (SOC_IS_CALADAN3(unit)) {
        return soc_sbx_caladan3_phy_addr_multi_get(unit, port, array_max, array_size, core_info);
    }
#endif

    if (IS_CL_PORT(unit, port)) {
        if (SOC_IS_TRIUMPH3(unit)) {
            uint32 mld_index = (0 == SOC_BLOCK_NUMBER(unit, SOC_PORT_BLOCK(unit, 
                          SOC_INFO(unit).port_l2p_mapping[port]))) ? 0 : 1;
            if (array_max > 1) {
                core_info[addr_num].mdio_addr = PHY_ADDR_INT(unit, port);
                core_info[addr_num].core_type = phyident_core_type_wc;
                core_info[addr_num].mld_index = mld_index;
                core_info[addr_num].index_in_mld = 0;
                addr_num++;
            }
            if (array_max > 2) {
                core_info[addr_num].mdio_addr = core_info[addr_num - 1].mdio_addr + 4;
                core_info[addr_num].core_type = phyident_core_type_wc;
                core_info[addr_num].mld_index = mld_index;
                core_info[addr_num].index_in_mld = 1;
                addr_num++;
            }
            if (array_max > 3) {
                core_info[addr_num].mdio_addr = core_info[addr_num - 1].mdio_addr + 4;
                core_info[addr_num].core_type = phyident_core_type_wc;
                core_info[addr_num].mld_index = mld_index;
                core_info[addr_num].index_in_mld = 2;
                addr_num++;
            }
            if (array_max > 4) {
                core_info[addr_num].mdio_addr = mld_index ? 0xde : 0xdd;
                core_info[addr_num].core_type = phyident_core_type_mld;
                core_info[addr_num].mld_index = mld_index;
                core_info[addr_num].index_in_mld = 0;
                addr_num++;
            }
            *array_size = addr_num;
        } else {
            
            return SOC_E_UNAVAIL;
        }
    } else {
        core_info[0].mdio_addr = PHY_ADDR_INT(unit, port);
        core_info[0].core_type = phyident_core_type_wc;
        *array_size = 1;
    }
    return SOC_E_NONE;
}

/*
 * Variable:
 *      phy_id_map
 * Purpose:
 *      Map the PHY identifier register (OUI and device ID) into
 *      enumerated PHY type for prototypical devices.
 */

typedef struct phy_id_map_s {
    soc_known_phy_t     phy_num;        /* Enumerated PHY type */
    uint32              oui;            /* Device OUI */
    uint16              model;          /* Device Model */
    uint16              rev_map;        /* Device Revision */
} phy_id_map_t;

#define PHY_REV_ALL   (0xffff)
#define PHY_REV_0_3   (0x000f)
#define PHY_REV_4_7   (PHY_REV_0_3 << 4)
#define PHY_REV_8_11  (PHY_REV_0_3 << 8)
#define PHY_REV_12_15 (PHY_REV_0_3 << 12)
#define PHY_REV_0     (1 << 0)
#define PHY_REV_1     (1 << 1)
#define PHY_REV_2     (1 << 2)
#define PHY_REV_3     (1 << 3)
#define PHY_REV_4     (1 << 4)
#define PHY_REV_5     (1 << 5)
#define PHY_REV_6     (1 << 6)
#define PHY_REV_7     (1 << 7)
#define PHY_REV_8     (1 << 8)
#define PHY_REV_9     (1 << 9)
#define PHY_REV_10    (1 << 10)
#define PHY_REV_11    (1 << 11)
#define PHY_REV_12    (1 << 12)

STATIC const phy_id_map_t phy_id_map[] = {
    { _phy_id_BCM5218,      PHY_BCM5218_OUI,        PHY_BCM5218_MODEL,
      PHY_REV_ALL },
    { _phy_id_BCM5220,      PHY_BCM5220_OUI,        PHY_BCM5220_MODEL,
      PHY_REV_ALL }, /* & 5221 */
    { _phy_id_BCM5226,      PHY_BCM5226_OUI,        PHY_BCM5226_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5228,      PHY_BCM5228_OUI,        PHY_BCM5228_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5238,      PHY_BCM5238_OUI,        PHY_BCM5238_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5248,      PHY_BCM5248_OUI,        PHY_BCM5248_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5324,      PHY_BCM5324_OUI,        PHY_BCM5324_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5400,      PHY_BCM5400_OUI,        PHY_BCM5400_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5401,      PHY_BCM5401_OUI,        PHY_BCM5401_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5402,      PHY_BCM5402_OUI,        PHY_BCM5402_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5404,      PHY_BCM5404_OUI,        PHY_BCM5404_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5411,      PHY_BCM5411_OUI,        PHY_BCM5411_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5421,      PHY_BCM5421_OUI,        PHY_BCM5421_MODEL,
      PHY_REV_ALL}, /* & 5421S */
    { _phy_id_BCM5424,      PHY_BCM5424_OUI,        PHY_BCM5424_MODEL,
      PHY_REV_ALL}, /* & 5434 */
    { _phy_id_BCM5464,      PHY_BCM5464_OUI,        PHY_BCM5464_MODEL,
      PHY_REV_ALL}, /* & 5464S */
    { _phy_id_BCM5466,       PHY_BCM5466_OUI,       PHY_BCM5466_MODEL,
      PHY_REV_ALL}, /* & 5466S */
    { _phy_id_BCM5461,       PHY_BCM5461_OUI,       PHY_BCM5461_MODEL,
      PHY_REV_ALL}, /* & 5461S */
    { _phy_id_BCM5461,       PHY_BCM5462_OUI,       PHY_BCM5462_MODEL,
      PHY_REV_ALL}, /* & 5461D */
    { _phy_id_BCM5478,       PHY_BCM5478_OUI,       PHY_BCM5478_MODEL,
      PHY_REV_ALL}, /*   5478 */
    { _phy_id_BCM5488,       PHY_BCM5488_OUI,       PHY_BCM5488_MODEL,
      PHY_REV_ALL}, /*   5488 */
    { _phy_id_BCM5482,       PHY_BCM5482_OUI,       PHY_BCM5482_MODEL,
      PHY_REV_ALL}, /* & 5482S */
    { _phy_id_BCM5481,       PHY_BCM5481_OUI,       PHY_BCM5481_MODEL,
      PHY_REV_ALL}, /* & 5481 */
    { _phy_id_BCM54980,      PHY_BCM54980_OUI,      PHY_BCM54980_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54980C,     PHY_BCM54980C_OUI,     PHY_BCM54980C_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54980V,     PHY_BCM54980V_OUI,     PHY_BCM54980V_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54980VC,    PHY_BCM54980VC_OUI,    PHY_BCM54980VC_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54680,      PHY_BCM54680_OUI,      PHY_BCM54680_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54880,      PHY_BCM54880_OUI,      PHY_BCM54880_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54880E,     PHY_BCM54880E_OUI,     PHY_BCM54880E_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54680E,     PHY_BCM54680E_OUI,     PHY_BCM54680E_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM52681E,     PHY_BCM52681E_OUI,     PHY_BCM52681E_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54881,      PHY_BCM54881_OUI,      PHY_BCM54881_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54810,      PHY_BCM54810_OUI,      PHY_BCM54810_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54811,      PHY_BCM54811_OUI,      PHY_BCM54811_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54684,      PHY_BCM54684_OUI,      PHY_BCM54684_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54684E,     PHY_BCM54684E_OUI,     PHY_BCM54684E_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54682,      PHY_BCM54682E_OUI,     PHY_BCM54682E_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54685,      PHY_BCM54685_OUI,      PHY_BCM54685_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54640,      PHY_BCM54640_OUI,      PHY_BCM54640_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54616,      PHY_BCM54616_OUI,      PHY_BCM54616_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54618E,     PHY_BCM54618E_OUI,     PHY_BCM54618E_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54618SE,    PHY_BCM54618SE_OUI,    PHY_BCM54618SE_MODEL,
      PHY_REV_ALL},

#ifdef INCLUDE_MACSEC
#if defined(INCLUDE_PHY_54380)
    { _phy_id_BCM54380,      PHY_BCM54380_OUI,      PHY_BCM54380_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54382,      PHY_BCM54380_OUI,      PHY_BCM54382_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54340,      PHY_BCM54380_OUI,      PHY_BCM54340_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54385,      PHY_BCM54380_OUI,      PHY_BCM54385_MODEL,
      PHY_REV_ALL},
#endif
#endif    

#if defined(INCLUDE_PHY_542XX)
    { _phy_id_BCM54182,      PHY_BCM5418X_OUI,      PHY_BCM54182_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54185,      PHY_BCM5418X_OUI,      PHY_BCM54185_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54180,      PHY_BCM5418X_OUI,      PHY_BCM54180_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54140,      PHY_BCM5418X_OUI,      PHY_BCM54140_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54192,      PHY_BCM5419X_OUI,      PHY_BCM54192_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54195,      PHY_BCM5419X_OUI,      PHY_BCM54195_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54190,      PHY_BCM5419X_OUI,      PHY_BCM54190_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54194,      PHY_BCM5419X_OUI,      PHY_BCM54194_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54210,      PHY_BCM542XX_OUI,      PHY_BCM54210_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54220,      PHY_BCM542XX_OUI,      PHY_BCM54220_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54280,      PHY_BCM542XX_OUI,      PHY_BCM54280_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54282,      PHY_BCM542XX_OUI,      PHY_BCM54282_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54240,      PHY_BCM542XX_OUI,      PHY_BCM54240_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54285,      PHY_BCM542XX_OUI,      PHY_BCM54285_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54290,      PHY_BCM542XX_OUI,      PHY_BCM54290_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54292,      PHY_BCM542XX_OUI,      PHY_BCM54292_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54294,      PHY_BCM542XX_OUI,      PHY_BCM54294_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54295,      PHY_BCM542XX_OUI,      PHY_BCM54295_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54296,      PHY_BCM542XX_OUI,      PHY_BCM54296_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5428x,      PHY_BCM542XX_OUI,      PHY_BCM5428X_MODEL,
      PHY_REV_ALL},
#endif

#if defined(INCLUDE_PHY_8806X)
    { _phy_id_BCM8806x,      PHY_BCM8806X_OUI,      PHY_BCM8806X_MODEL,
      PHY_REV_ALL},
#endif /* INCLUDE_PHY_8806X */

#ifdef INCLUDE_MACSEC
#if defined(INCLUDE_PHY_54580)
    { _phy_id_BCM54584,      PHY_BCM54580_OUI,      PHY_BCM54584_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54580,      PHY_BCM54580_OUI,      PHY_BCM54580_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54540,      PHY_BCM54580_OUI,      PHY_BCM54540_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM54585,      PHY_BCM54580_OUI,      PHY_BCM54585_MODEL,
      PHY_REV_ALL},
#endif
#endif /* INCLUDE_MACSEC */
    { _phy_id_BCM8011,       PHY_BCM8011_OUI,       PHY_BCM8011_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM8040,       PHY_BCM8040_OUI,       PHY_BCM8040_MODEL,
      PHY_REV_0 | PHY_REV_1 | PHY_REV_2},
    { _phy_id_BCM8703,       PHY_BCM8703_OUI,       PHY_BCM8703_MODEL,
      PHY_REV_0 | PHY_REV_1 | PHY_REV_2},
    { _phy_id_BCM8704,       PHY_BCM8704_OUI,       PHY_BCM8704_MODEL,
      PHY_REV_3},
    { _phy_id_BCM8705,       PHY_BCM8705_OUI,       PHY_BCM8705_MODEL,
      PHY_REV_4},
    { _phy_id_BCM8706,       PHY_BCM8706_OUI,       PHY_BCM8706_MODEL,
      PHY_REV_5},
    { _phy_id_BCM8750,       PHY_BCM8750_OUI,       PHY_BCM8750_MODEL,
      PHY_REV_0},
    { _phy_id_BCM8752,       PHY_BCM8752_OUI,       PHY_BCM8752_MODEL,
      PHY_REV_0},
    { _phy_id_BCM8754,       PHY_BCM8754_OUI,       PHY_BCM8754_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM8072,       PHY_BCM8072_OUI,       PHY_BCM8072_MODEL,
      PHY_REV_5},
    { _phy_id_BCM8727,       PHY_BCM8727_OUI,       PHY_BCM8727_MODEL,
      PHY_REV_6},
    { _phy_id_BCM8073,       PHY_BCM8073_OUI,       PHY_BCM8073_MODEL,
      PHY_REV_6},
    { _phy_id_BCM8747,       PHY_BCM8747_OUI,       PHY_BCM8747_MODEL,
      PHY_REV_7},
    { _phy_id_BCM84740,      PHY_BCM84740_OUI,      PHY_BCM84740_MODEL,
      PHY_REV_0},
    { _phy_id_BCM84164,      PHY_BCM84164_OUI,      PHY_BCM84164_MODEL,
      PHY_REV_0_3},
    { _phy_id_BCM84758,      PHY_BCM84758_OUI,      PHY_BCM84758_MODEL,
      PHY_REV_0_3},
    { _phy_id_BCM84780,      PHY_BCM84780_OUI,      PHY_BCM84780_MODEL,
      PHY_REV_4_7},
    { _phy_id_BCM84784,      PHY_BCM84784_OUI,      PHY_BCM84784_MODEL,
      PHY_REV_8_11},
#ifdef INCLUDE_MACSEC
#ifdef INCLUDE_PHY_8729
    { _phy_id_BCM8729,       PHY_BCM5927_OUI,       PHY_BCM5927_MODEL,
      PHY_REV_4},
    { _phy_id_BCM8729,       PHY_BCM8729_OUI,       PHY_BCM8729_MODEL,
      PHY_REV_12},
#endif
#ifdef INCLUDE_PHY_84756
    { _phy_id_BCM84756,      PHY_BCM84756_OUI,      PHY_BCM84756_MODEL,
      PHY_REV_0_3},
    { _phy_id_BCM84757,      PHY_BCM84756_OUI,      PHY_BCM84756_MODEL,
      PHY_REV_8_11},
    { _phy_id_BCM84759,      PHY_BCM84756_OUI,      PHY_BCM84756_MODEL,
      PHY_REV_4_7},
#endif
#ifdef INCLUDE_PHY_84334
    { _phy_id_BCM84334,      PHY_BCM84334_OUI,      PHY_BCM84334_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84333,      PHY_BCM84333_OUI,      PHY_BCM84333_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84336,      PHY_BCM84336_OUI,      PHY_BCM84336_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84844,      PHY_BCM84844_OUI,      PHY_BCM84844_MODEL,
      PHY_REV_ALL},
#endif
#ifdef INCLUDE_PHY_84749
    { _phy_id_BCM84749,      PHY_BCM84749_OUI,      PHY_BCM84749_MODEL,
      PHY_REV_0},
    { _phy_id_BCM84729,      PHY_BCM84729_OUI,      PHY_BCM84729_MODEL,
      PHY_REV_0},
#endif
#endif  /* INCLUDE_MACSEC */
#if defined(INCLUDE_FCMAP) || defined(INCLUDE_MACSEC)
#ifdef INCLUDE_PHY_84756
    { _phy_id_BCM84756,      PHY_BCM84756_OUI,      PHY_BCM84756_MODEL,
      PHY_REV_0_3},
    { _phy_id_BCM84757,      PHY_BCM84756_OUI,      PHY_BCM84756_MODEL,
      PHY_REV_8_11},
    { _phy_id_BCM84759,      PHY_BCM84756_OUI,      PHY_BCM84756_MODEL,
      PHY_REV_4_7},
#endif  /*INCLUDE_PHY_84756 */
#endif  /* INCLUDE_FCMAP || INCLUDE_MACSEC */
#ifdef INCLUDE_PHY_84728 
    { _phy_id_BCM84707,      PHY_BCM84707_OUI,      PHY_BCM84707_MODEL, 
      PHY_REV_4_7}, 
    { _phy_id_BCM84073,      PHY_BCM84073_OUI,      PHY_BCM84073_MODEL, 
      PHY_REV_8_11}, 
    { _phy_id_BCM84074,      PHY_BCM84074_OUI,      PHY_BCM84074_MODEL, 
      PHY_REV_12_15}, 
    { _phy_id_BCM84728,      PHY_BCM84728_OUI,      PHY_BCM84728_MODEL, 
      PHY_REV_0_3}, 
    { _phy_id_BCM84748,      PHY_BCM84748_OUI,      PHY_BCM84748_MODEL, 
      PHY_REV_4_7}, 
    { _phy_id_BCM84727,      PHY_BCM84727_OUI,      PHY_BCM84727_MODEL, 
      PHY_REV_8_11}, 
    { _phy_id_BCM84747,      PHY_BCM84747_OUI,      PHY_BCM84747_MODEL, 
      PHY_REV_12_15}, 
    { _phy_id_BCM84762,      PHY_BCM84762_OUI,      PHY_BCM84762_MODEL, 
      PHY_REV_0_3}, 
    { _phy_id_BCM84764,      PHY_BCM84764_OUI,      PHY_BCM84764_MODEL, 
      PHY_REV_4_7}, 
    { _phy_id_BCM84042,      PHY_BCM84042_OUI,      PHY_BCM84042_MODEL, 
      PHY_REV_8_11}, 
    { _phy_id_BCM84044,      PHY_BCM84044_OUI,      PHY_BCM84044_MODEL, 
      PHY_REV_12_15}, 
#endif 
    { _phy_id_BCM8481x,      PHY_BCM8481X_OUI,      PHY_BCM8481X_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84812ce,    PHY_BCM84812CE_OUI,    PHY_BCM84812CE_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84821,      PHY_BCM84821_OUI,      PHY_BCM84821_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84822,      PHY_BCM84822_OUI,      PHY_BCM84822_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84823,      PHY_BCM84823_OUI,      PHY_BCM84823_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84833,      PHY_BCM84833_OUI,      PHY_BCM84833_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84834,      PHY_BCM84834_OUI,      PHY_BCM84834_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84835,      PHY_BCM84835_OUI,      PHY_BCM84835_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84836,      PHY_BCM84836_OUI,      PHY_BCM84836_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84844,      PHY_BCM84844_OUI,      PHY_BCM84844_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84846,      PHY_BCM84846_OUI,      PHY_BCM84846_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84848,      PHY_BCM84848_OUI,      PHY_BCM84848_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84858,      PHY_BCM84858_OUI,      PHY_BCM84858_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84856,      PHY_BCM84856_OUI,      PHY_BCM84856_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84860,      PHY_BCM84860_OUI,      PHY_BCM84860_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84861,      PHY_BCM84861_OUI,      PHY_BCM84861_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84864,      PHY_BCM84864_OUI,      PHY_BCM84864_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84868,      PHY_BCM84868_OUI,      PHY_BCM84868_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84888,      PHY_BCM84888_OUI,      PHY_BCM84888_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84884,      PHY_BCM84884_OUI,      PHY_BCM84884_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84888E,     PHY_BCM84888E_OUI,     PHY_BCM84888E_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84884E,     PHY_BCM84884E_OUI,     PHY_BCM84884E_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84881,      PHY_BCM84881_OUI,      PHY_BCM84881_MODEL,
       PHY_REV_ALL},
    { _phy_id_BCM84880,      PHY_BCM84880_OUI,      PHY_BCM84880_MODEL,
       PHY_REV_ALL},
    { _phy_id_BCM84888S,     PHY_BCM84888S_OUI,     PHY_BCM84888S_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84885,      PHY_BCM84885_OUI,      PHY_BCM84885_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM84886,      PHY_BCM84886_OUI,      PHY_BCM84886_MODEL,
       PHY_REV_ALL},
    { _phy_id_BCM84887,      PHY_BCM84887_OUI,      PHY_BCM84887_MODEL,
       PHY_REV_ALL},
#ifdef INCLUDE_PHY_84328
    { _phy_id_BCM84328,      PHY_BCM84328_OUI,      PHY_BCM84328_MODEL,
      PHY_REV_ALL},
#endif
#ifdef INCLUDE_PHY_84793
    { _phy_id_BCM84793,      PHY_BCM84793_OUI,      PHY_BCM84793_MODEL,
      PHY_REV_ALL},
#endif
#ifdef INCLUDE_PHY_82328
    { _phy_id_BCM82328,      PHY_BCM82328_OUI,      PHY_BCM82328_MODEL,
      PHY_REV_ALL},
#endif
#ifdef INCLUDE_PHY_82780
    { _phy_id_BCM82780,      PHY_BCM82780_OUI,      PHY_BCM82780_MODEL,
      PHY_REV_ALL},
#endif

#ifdef INCLUDE_PHY_84740
    { _phy_id_BCM84318,      PHY_BCM84318_OUI,      PHY_BCM84318_MODEL,
      PHY_REV_ALL},
#endif
    { _phy_id_BCMXGXS1,      PHY_BCMXGXS1_OUI,      PHY_BCMXGXS1_MODEL,
      PHY_REV_ALL},
    /*
     * HL65 has the same device ID as XGXS1.
     *{ _phy_id_XGXS_HL65,   PHY_XGXS_HL65_OUI,     PHY_XGXS_HL65_MODEL,
     * PHY_REV_ALL }
     */
    { _phy_id_BCMXGXS2,      PHY_BCMXGXS2_OUI,      PHY_BCMXGXS2_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCMXGXS5,      PHY_BCMXGXS5_OUI,      PHY_BCMXGXS5_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCMXGXS6,      PHY_BCMXGXS6_OUI,      PHY_BCMXGXS6_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5398,       PHY_BCM5398_OUI,       PHY_BCM5398_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5348,       PHY_BCM5348_OUI,       PHY_BCM5348_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM5395,       PHY_BCM5395_OUI,       PHY_BCM5395_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53242,      PHY_BCM53242_OUI,     PHY_BCM53242_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53262,      PHY_BCM53262_OUI,     PHY_BCM53262_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53115,      PHY_BCM53115_OUI,     PHY_BCM53115_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53118,      PHY_BCM53118_OUI,     PHY_BCM53118_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53280,      PHY_BCM53280_OUI,     PHY_BCM53280_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53101,      PHY_BCM53101_OUI,     PHY_BCM53101_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53314,      PHY_BCM53314_OUI,     PHY_BCM53314_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53324,      PHY_BCM53324_OUI,     PHY_BCM53324_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53125,      PHY_BCM53125_OUI,     PHY_BCM53125_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53128,      PHY_BCM53128_OUI,     PHY_BCM53128_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53134,      PHY_BCM53134_OUI,     PHY_BCM53134_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53600,      PHY_BCM53600_OUI,     PHY_BCM53600_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM89500,      PHY_BCM89500_OUI,     PHY_BCM89500_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53010,      PHY_BCM53010_OUI,     PHY_BCM53010_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53018,      PHY_BCM53018_OUI,     PHY_BCM53018_MODEL,
      PHY_REV_ALL},
    { _phy_id_BCM53020,      PHY_BCM53020_OUI,     PHY_BCM53020_MODEL,
      PHY_REV_ALL},  
    { _phy_id_BCM56160_GPHY, PHY_BCM56160GPHY_OUI, PHY_BCM56160GPHY_MODEL,
      PHY_REV_ALL},  
    { _phy_id_BCM53540,      PHY_BCM53540_OUI,     PHY_BCM53540_MODEL,
      PHY_REV_ALL},
    { _phy_id_SERDES100FX,   PHY_SERDES100FX_OUI,   PHY_SERDES100FX_MODEL,
      PHY_REV_4 | PHY_REV_5},
    { _phy_id_SERDES65LP,    PHY_SERDES65LP_OUI,    PHY_SERDES65LP_MODEL,
      PHY_REV_ALL},
    { _phy_id_SERDESCOMBO,   PHY_SERDESCOMBO_OUI,   PHY_SERDESCOMBO_MODEL,
      PHY_REV_8 },
    { _phy_id_XGXS_16G,      PHY_XGXS_16G_OUI,      PHY_XGXS_16G_MODEL,
      PHY_REV_0 },
    { _phy_id_XGXS_TSC,      PHY_XGXS_TSC_OUI,      PHY_XGXS_TSC_MODEL,
      PHY_REV_0 },
    { _phy_id_XGXS_VIPER,    PHY_XGXS_VIPER_OUI,    PHY_XGXS_VIPER_MODEL,
      PHY_REV_9 },
#ifdef INCLUDE_PHY_82381
    { _phy_id_BCM82381,      PHY_BCM82381_OUI,      PHY_BCM82381_MODEL,
      PHY_REV_ALL},
#endif
#ifdef INCLUDE_PHY_82764
    { _phy_id_BCM82764,      PHY_BCM82764_OUI,      PHY_BCM82764_MODEL,
      PHY_REV_ALL},
#endif
#ifdef INCLUDE_PHY_82864
    { _phy_id_BCM82864,      PHY_BCM82864_OUI,      PHY_BCM82864_MODEL,
PHY_REV_ALL},
#endif


};

/*
 * Function:
 *      _phy_ident_type_get
 * Purpose:
 *      Check the PHY ID and return an enumerated value indicating
 *      the PHY.  This looks very redundant, but in the future, more
 *      complicated PHY detection may be necessary.  In addition, the
 *      enum value could be used as an index.
 * Parameters:
 *      phy_id0 - PHY ID register 0 (MII register 2)
 *      phy_id1 - PHY ID register 1 (MII register 3)
 */

static soc_known_phy_t
_phy_ident_type_get(uint16 phy_id0, uint16 phy_id1)
{
    int                 i;
    const phy_id_map_t        *pm;
    uint32              oui;
    uint16              model, rev_map;

    oui       = PHY_OUI(phy_id0, phy_id1);
    model     = PHY_MODEL(phy_id0, phy_id1);
    rev_map   = 1 << PHY_REV(phy_id0, phy_id1);

    LOG_INFO(BSL_LS_SOC_PHY,
             (BSL_META("phy_id0 = %04x phy_id1 %04x oui = %04x model = %04x rev_map = %04x\n"),
              phy_id0, phy_id1, oui, model, rev_map));
    for (i = 0; i < COUNTOF(phy_id_map); i++) {
        pm = &phy_id_map[i];
        if ((pm->oui == oui) && (pm->model == model)) {
            if (pm->rev_map & rev_map) {
                return pm->phy_num;
            }
        }
    }

    return _phy_id_unknown;
}

/*
 * Function:
 *      soc_phy_nocxn
 * Purpose:
 *      Return the no_cxn PHY driver
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      phyd_ptr - (OUT) Pointer to PHY driver.
 * Returns:
 *      SOC_E_XXX
 */

int
soc_phy_nocxn(int unit, phy_driver_t **phyd_ptr)
{
    *phyd_ptr = &phy_nocxn;

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_phy_info_get
 * Purpose:
 *      Accessor function to copy out PHY info structure
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      pi - (OUT) Pointer to output structure.
 * Returns:
 *      SOC_E_XXX
 */

int
soc_phy_info_get(int unit, soc_port_t port, soc_phy_info_t *pi)
{
    soc_phy_info_t *source;

    source = &SOC_PHY_INFO(unit, port);

    sal_memcpy(pi, source, sizeof(soc_phy_info_t));
    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_phy_an_timeout_get
 * Purpose:
 *      Return autonegotiation timeout for a specific port
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      Timeout in usec
 */

sal_usecs_t
soc_phy_an_timeout_get(int unit, soc_port_t port)
{
    return PHY_AN_TIMEOUT(unit, port);
}

/*
 * Function:
 *      soc_phy_addr_of_port
 * Purpose:
 *      Return PHY ID of the PHY attached to a specified port.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      PHY ID
 */

uint16
soc_phy_addr_of_port(int unit, soc_port_t port)
{
    return PHY_ADDR(unit, port);
}

/*
 * Function:
 *      soc_phy_addr_int_of_port
 * Purpose:
 *      Return PHY ID of a intermediate PHY on specified port.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      PHY ID
 * Notes:
 *      Only applies to chip ports that have an intermediate PHY.
 */

uint16
soc_phy_addr_int_of_port(int unit, soc_port_t port)
{
    return PHY_ADDR_INT(unit, port);
}


/*
 * Function:
 *      soc_phy_id1reg_get
 * Purpose:
 *      Return the PHY ID1 field from the PHY on a specified port.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      PHY ID
 */

uint16
soc_phy_id1reg_get(int unit, soc_port_t port)
{
    return PHY_ID1_REG(unit, port);
}

/*
 * Function:
 *      soc_phy_id1reg_get
 * Purpose:
 *      Return the PHY ID0 field from the PHY on a specified port.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      PHY ID
 */

uint16
soc_phy_id0reg_get(int unit, soc_port_t port)
{
    return PHY_ID0_REG(unit, port);
}

/*
 * Function:
 *      soc_phy_is_c45_miim
 * Purpose:
 *      Return TRUE  if Phy uses Clause 45 MIIM, FALSE otherwise
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      Return TRUE  if Phy uses Clause 45 MIIM, FALSE otherwise
 */

int
soc_phy_is_c45_miim(int unit, soc_port_t port)
{
    return PHY_CLAUSE45_MODE(unit, port);
}

/*
 * Function:
 *      soc_phy_name_get
 * Purpose:
 *      Return name of PHY driver corresponding to specified port.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      Static pointer to string.
 */

char *
soc_phy_name_get(int unit, soc_port_t port)
{
    if (PHY_NAME(unit, port) != NULL) {
        return PHY_NAME(unit, port);
    } else {
        return "<Unnamed PHY>";
    }
}

/*
 * Function:
 *      soc_phy_addr_to_port
 * Purpose:
 *      Return the port to which a given PHY ID corresponds.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      Port number
 */

soc_port_t 
soc_phy_addr_to_port(int unit, uint16 phy_addr)
{
    int port;
    PBMP_PORT_ITER(unit, port) { /* To get all ports Address */
        if (PHY_ADDR(unit, port) == phy_addr || PHY_ADDR_INT(unit, port) == phy_addr) {
            return port;
        } 
    }
    return -1;

}
  
