#include <sal/core/libc.h>
#include <soc/defs.h>
#include <soc/mem.h>
#include <soc/mcm/driver.h>
#include <soc/mcm/allenum.h>
#include <soc/mcm/intenum.h>
#include <soc/drv.h>
#include <shared/bsl.h>
#ifdef BCM_53570_A0
static const soc_block_info_t soc_blocks_bcm53570_a0[] = {
    { SOC_BLK_PGW_GE8P,     0,   28,  28   }, /* 0 P0 */
    { SOC_BLK_PGW_GE8P,     1,   32,  32   }, /* 1 P1 */
    { SOC_BLK_PGW_GE8P,     2,   36,  36   }, /* 2 P2 */
    { SOC_BLK_AVS,          0,   17,  17   }, /* 3 A0 */
    { SOC_BLK_CLPORT,       0,   2,   2    }, /* 4 C0 */
    { SOC_BLK_CMIC,         0,   14,  14   }, /* 5 C0 */
    { SOC_BLK_CRYPTO,       0,   1,   15   }, /* 6 C0 */
    { SOC_BLK_EPIPE,        0,   11,  11   }, /* 7 E0 */
    { SOC_BLK_GPORT,        0,   29,  29   }, /* 8 G0 */
    { SOC_BLK_GPORT,        1,   33,  33   }, /* 9 G1 */
    { SOC_BLK_GPORT,        2,   37,  37   }, /* 10 G2 */
    { SOC_BLK_GPORT,        3,   41,  41   }, /* 11 G3 */
    { SOC_BLK_GPORT,        4,   42,  42   }, /* 12 G4 */
    { SOC_BLK_GPORT,        5,   45,  45   }, /* 13 G5 */
    { SOC_BLK_GPORT,        6,   46,  46   }, /* 14 G6 */
    { SOC_BLK_IPIPE,        0,   10,  10   }, /* 15 I0 */
    { SOC_BLK_IPROC,        0,   15,  15   }, /* 16 I0 */
    { SOC_BLK_MMU,          0,   12,  12   }, /* 17 M0 */
    { SOC_BLK_OTPC,         0,   13,  13   }, /* 18 O0 */
    { SOC_BLK_PGW_GE,       0,   40,  40   }, /* 19 P0 */
    { SOC_BLK_PGW_GE,       1,   44,  44   }, /* 20 P1 */
    { SOC_BLK_PMQ,          0,   30,  30   }, /* 21 P0 */
    { SOC_BLK_PMQ,          1,   34,  34   }, /* 22 P1 */
    { SOC_BLK_PMQ,          2,   38,  38   }, /* 23 P2 */
    { SOC_BLK_PMQ,          3,   43,  43   }, /* 24 P3 */
    { SOC_BLK_PMQ,          4,   47,  47   }, /* 25 P4 */
    { SOC_BLK_SER,          0,   19,  19   }, /* 26 S0 */
    { SOC_BLK_TOP,          0,   16,  16   }, /* 27 T0 */
    { SOC_BLK_XLPORT,       0,   4,   4    }, /* 28 X0 */
    { SOC_BLK_XLPORT,       1,   5,   5    }, /* 29 X1 */
    { SOC_BLK_XLPORT,       2,   6,   6    }, /* 30 X2 */
    { SOC_BLK_XLPORT,       3,   7,   7    }, /* 31 X3 */
    { SOC_BLK_XLPORT,       4,   24,  24   }, /* 32 X4 */
    { SOC_BLK_XLPORT,       5,   25,  25   }, /* 33 X5 */
    { SOC_BLK_XLPORT,       6,   26,  26   }, /* 34 X6 */
    { -1,                   -1,  -1,  -1   }  /* end */
};

static const soc_port_info_t soc_ports_bcm53570_a0[] = {
    { 5,   0   }, /* 0 C0.0 */
    { -1,  0   }, /* 0 - */
    { -1,  0   }, /* 1 - */
    { -1,  0   }, /* 1 - */
    { 8,   0   }, /* 2 G0.0 */
    { 0,   0   }, /* 2  0.0 */
    { 8,   1   }, /* 3 G0.1 */
    { 0,   1   }, /* 3  0.1 */
    { 8,   2   }, /* 4 G0.2 */
    { 0,   2   }, /* 4  0.2 */
    { 8,   3   }, /* 5 G0.3 */
    { 0,   3   }, /* 5  0.3 */
    { 8,   4   }, /* 6 G0.4 */
    { 0,   4   }, /* 6  0.4 */
    { 8,   5   }, /* 7 G0.5 */
    { 0,   5   }, /* 7  0.5 */
    { 8,   6   }, /* 8 G0.6 */
    { 0,   6   }, /* 8  0.6 */
    { 8,   7   }, /* 9 G0.7 */
    { 0,   7   }, /* 9  0.7 */
    { 9,   0   }, /* 10 G1.0 */
    { 1,   0   }, /* 10  0.0 */
    { 9,   1   }, /* 11 G1.1 */
    { 1,   1   }, /* 11  0.1 */
    { 9,   2   }, /* 12 G1.2 */
    { 1,   2   }, /* 12  0.2 */
    { 9,   3   }, /* 13 G1.3 */
    { 1,   3   }, /* 13  0.3 */
    { 9,   4   }, /* 14 G1.4 */
    { 1,   4   }, /* 14  0.4 */
    { 9,   5   }, /* 15 G1.5 */
    { 1,   5   }, /* 15  0.5 */
    { 9,   6   }, /* 16 G1.6 */
    { 1,   6   }, /* 16  0.6 */
    { 9,   7   }, /* 17 G1.7 */
    { 1,   7   }, /* 17  0.7 */
    { 10,  0   }, /* 18 G2.0 */
    { 2,   0   }, /* 18  0.0 */
    { 10,  1   }, /* 19 G2.1 */
    { 2,   1   }, /* 19  0.1 */
    { 10,  2   }, /* 20 G2.2 */
    { 2,   2   }, /* 20  0.2 */
    { 10,  3   }, /* 21 G2.3 */
    { 2,   3   }, /* 21  0.3 */
    { 10,  4   }, /* 22 G2.4 */
    { 2,   4   }, /* 22  0.4 */
    { 10,  5   }, /* 23 G2.5 */
    { 2,   5   }, /* 23  0.5 */
    { 10,  6   }, /* 24 G2.6 */
    { 2,   6   }, /* 24  0.6 */
    { 10,  7   }, /* 25 G2.7 */
    { 2,   7   }, /* 25  0.7 */
    { 11,  0   }, /* 26 G3.0 */
    { 19,  0   }, /* 26 P0.0 */
    { 11,  1   }, /* 27 G3.1 */
    { 19,  1   }, /* 27 P0.1 */
    { 11,  2   }, /* 28 G3.2 */
    { 19,  2   }, /* 28 P0.2 */
    { 11,  3   }, /* 29 G3.3 */
    { 19,  3   }, /* 29 P0.3 */
    { 11,  4   }, /* 30 G3.4 */
    { 19,  4   }, /* 30 P0.4 */
    { 11,  5   }, /* 31 G3.5 */
    { 19,  5   }, /* 31 P0.5 */
    { 11,  6   }, /* 32 G3.6 */
    { 19,  6   }, /* 32 P0.6 */
    { 11,  7   }, /* 33 G3.7 */
    { 19,  7   }, /* 33 P0.7 */
    { 12,  0   }, /* 34 G4.0 */
    { 19,  8   }, /* 34 P0.8 */
    { 12,  1   }, /* 35 G4.1 */
    { 19,  9   }, /* 35 P0.9 */
    { 12,  2   }, /* 36 G4.2 */
    { 19,  10  }, /* 36 P0.10 */
    { 12,  3   }, /* 37 G4.3 */
    { 19,  11  }, /* 37 P0.11 */
    { 12,  4   }, /* 38 G4.4 */
    { 19,  12  }, /* 38 P0.12 */
    { 12,  5   }, /* 39 G4.5 */
    { 19,  13  }, /* 39 P0.13 */
    { 12,  6   }, /* 40 G4.6 */
    { 19,  14  }, /* 40 P0.14 */
    { 12,  7   }, /* 41 G4.7 */
    { 19,  15  }, /* 41 P0.15 */
    { 13,  0   }, /* 42 G5.0 */
    { 20,  0   }, /* 42 P1.0 */
    { 13,  1   }, /* 43 G5.1 */
    { 20,  1   }, /* 43 P1.1 */
    { 13,  2   }, /* 44 G5.2 */
    { 20,  2   }, /* 44 P1.2 */
    { 13,  3   }, /* 45 G5.3 */
    { 20,  3   }, /* 45 P1.3 */
    { 13,  4   }, /* 46 G5.4 */
    { 20,  4   }, /* 46 P1.4 */
    { 13,  5   }, /* 47 G5.5 */
    { 20,  5   }, /* 47 P1.5 */
    { 13,  6   }, /* 48 G5.6 */
    { 20,  6   }, /* 48 P1.6 */
    { 13,  7   }, /* 49 G5.7 */
    { 20,  7   }, /* 49 P1.7 */
    { 14,  0   }, /* 50 G6.0 */
    { 20,  8   }, /* 50 P1.8 */
    { 14,  1   }, /* 51 G6.1 */
    { 20,  9   }, /* 51 P1.9 */
    { 14,  2   }, /* 52 G6.2 */
    { 20,  10  }, /* 52 P1.10 */
    { 14,  3   }, /* 53 G6.3 */
    { 20,  11  }, /* 53 P1.11 */
    { 14,  4   }, /* 54 G6.4 */
    { 20,  12  }, /* 54 P1.12 */
    { 14,  5   }, /* 55 G6.5 */
    { 20,  13  }, /* 55 P1.13 */
    { 14,  6   }, /* 56 G6.6 */
    { 20,  14  }, /* 56 P1.14 */
    { 14,  7   }, /* 57 G6.7 */
    { 20,  15  }, /* 57 P1.15 */
    { 28,  0   }, /* 58 X0.0 */
    { -1,  0   }, /* 58 - */
    { 28,  1   }, /* 59 X0.1 */
    { -1,  0   }, /* 59 - */
    { 28,  2   }, /* 60 X0.2 */
    { -1,  0   }, /* 60 - */
    { 28,  3   }, /* 61 X0.3 */
    { -1,  0   }, /* 61 - */
    { 29,  0   }, /* 62 X1.0 */
    { -1,  0   }, /* 62 - */
    { 29,  1   }, /* 63 X1.1 */
    { -1,  0   }, /* 63 - */
    { 29,  2   }, /* 64 X1.2 */
    { -1,  0   }, /* 64 - */
    { 29,  3   }, /* 65 X1.3 */
    { -1,  0   }, /* 65 - */
    { 30,  0   }, /* 66 X2.0 */
    { -1,  0   }, /* 66 - */
    { 30,  1   }, /* 67 X2.1 */
    { -1,  0   }, /* 67 - */
    { 30,  2   }, /* 68 X2.2 */
    { -1,  0   }, /* 68 - */
    { 30,  3   }, /* 69 X2.3 */
    { -1,  0   }, /* 69 - */
    { 31,  0   }, /* 70 X3.0 */
    { -1,  0   }, /* 70 - */
    { 31,  1   }, /* 71 X3.1 */
    { -1,  0   }, /* 71 - */
    { 31,  2   }, /* 72 X3.2 */
    { -1,  0   }, /* 72 - */
    { 31,  3   }, /* 73 X3.3 */
    { -1,  0   }, /* 73 - */
    { 32,  0   }, /* 74 X4.0 */
    { -1,  0   }, /* 74 - */
    { 32,  1   }, /* 75 X4.1 */
    { -1,  0   }, /* 75 - */
    { 32,  2   }, /* 76 X4.2 */
    { -1,  0   }, /* 76 - */
    { 32,  3   }, /* 77 X4.3 */
    { -1,  0   }, /* 77 - */
    { 33,  0   }, /* 78 X5.0 */
    { -1,  0   }, /* 78 - */
    { 33,  1   }, /* 79 X5.1 */
    { -1,  0   }, /* 79 - */
    { 33,  2   }, /* 80 X5.2 */
    { -1,  0   }, /* 80 - */
    { 33,  3   }, /* 81 X5.3 */
    { -1,  0   }, /* 81 - */
    { 34,  0   }, /* 82 X6.0 */
    { -1,  0   }, /* 82 - */
    { 34,  1   }, /* 83 X6.1 */
    { -1,  0   }, /* 83 - */
    { 34,  2   }, /* 84 X6.2 */
    { -1,  0   }, /* 84 - */
    { 34,  3   }, /* 85 X6.3 */
    { -1,  0   }, /* 85 - */
    { 4,   0   }, /* 86 C0.0 */
    { -1,  0   }, /* 86 - */
    { 4,   1   }, /* 87 C0.1 */
    { -1,  0   }, /* 87 - */
    { 4,   2   }, /* 88 C0.2 */
    { -1,  0   }, /* 88 - */
    { 4,   3   }, /* 89 C0.3 */
    { -1,  0   }, /* 89 - */
    { -1,  -1  }  /* end */
};

soc_driver_t soc_driver_bcm53570_a0 = {
    /* type                   */ SOC_CHIP_BCM53570_A0,
    /* chip_string            */ "greyhound2",
    /* origin                 */ "Id: //depot/greyhound2/regsfile/chip_top.regs#1 ",
    /* pci_vendor             */ BROADCOM_VENDOR_ID,
    /* pci_device             */ BCM53570_DEVICE_ID,
    /* pci_revision           */ BCM53570_A0_REV_ID,
    /* num_cos                */ 8,
    /* reg_info               */ NULL,
    /* reg_unique_acc         */ NULL,
    /* reg_above_64_info      */ NULL,
    /* reg_array_info         */ NULL,
    /* mem_info               */ NULL,
    /* mem_unique_acc         */ NULL,
    /* mem_aggr               */ NULL,
    /* mem_array_info         */ NULL,
    /* block_info             */ (soc_block_info_t *) soc_blocks_bcm53570_a0,
    /* port_info              */ (soc_port_info_t *) soc_ports_bcm53570_a0,
    /* counter_maps           */ NULL,
    /* features               */ NULL,
    /* init                   */ NULL,
    /* services               */ NULL,
    /* port_num_blktype       */ 2,
    /* cmicd_base             */ 0x03200000
};  /* soc_driver             */


/*
 * Function:    soc_feature_init
 * Purpose:     initialize features into the SOC_CONTROL cache
 * Parameters:  unit    - the device
 * Returns:     void
 */
void
soc_feature_init(int unit)
{

    sal_memset(SOC_CONTROL(unit)->features, 0, sizeof(SOC_CONTROL(unit)->features));

    SOC_FEATURE_SET(unit, soc_feature_logical_port_num);

}


soc_driver_t *soc_chip_driver_find(uint16 dev_id, uint8 rev_id) {	    
      return &soc_driver_bcm53570_a0;
}

/*
 * Function:
 *      soc_info_config
 * Parameters:
 *      unit - Unit number
 *      soc - soc_control_t associated with this unit
 * Purpose:
 *      Fill in soc_info structure for a newly attached unit.
 *      Generates bitmaps and various arrays based on block and
 *      ports that the hardware has enabled.
 *
 *      This isn't STATIC so that pcid can get at it.
 */

int
soc_greyhound2_info_config(int unit, soc_control_t *soc)
{
    soc_info_t          *si;
    
    uint16              dev_id;
    uint8               rev_id;
    int                 port, phy_port, blk, bindex;

    
    int                 port_idx;

    SOC_CONTROL(unit) = soc;
    si = &soc->info;
    
    soc_cm_get_id(unit, &dev_id, &rev_id);

    /*
     * Instantiate the driver -- Verify chip revision matches driver
     * compilation revision.
     */
    soc->chip_driver = soc_chip_driver_find(dev_id, rev_id);

    soc->soc_functions = soc_chip_drv_funs_find(dev_id, rev_id);

    /* Set feature cache, since used by mutex creation */
    soc_feature_init(unit);

    /*
     * Attached flag must be true during initialization.
     * If initialization fails, the flag is cleared by soc_detach (below).
     */
    soc->soc_flags |= SOC_F_ATTACHED;

    /*
     * Used to implement the SOC_IS_*(unit) macros
     */    
    si->chip_type = SOC_INFO_CHIP_TYPE_GREYHOUND2;
    
    for (phy_port = 0; ; phy_port++) {
        blk = SOC_PORT_IDX_BLOCK(unit, phy_port, 0);
        bindex = SOC_PORT_IDX_BINDEX(unit, phy_port, 0);

        if (blk < 0 && bindex < 0) { /* end of regsfile port list */
            break;
        }
        port = si->port_p2l_mapping[phy_port];
        if (port < 0) { /* not used in user config */
            continue;
        }
		    LOG_ERROR(BSL_LS_SOC_COMMON, 
		             (BSL_META_U(unit, " %d port\n"), port));

        if (blk < 0) { /* disabled port */
            continue;
        }

        for (port_idx = 0; port_idx < SOC_DRIVER(unit)->port_num_blktype;
             port_idx++) {

            blk = SOC_PORT_IDX_BLOCK(unit, phy_port, port_idx);
            if (blk < 0) { /* end of block list of each port */
                break;
            }            
            SOC_PBMP_PORT_ADD(si->block_bitmap[blk], port);

        }
    }
    return SOC_E_NONE;
}
#endif /* BCM_53570_A0 */
