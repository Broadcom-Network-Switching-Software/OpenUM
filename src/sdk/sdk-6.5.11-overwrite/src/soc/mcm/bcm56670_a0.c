#include <sal/core/libc.h>
#include <soc/defs.h>
#include <soc/mem.h>
#include <soc/mcm/driver.h>
#include <soc/mcm/allenum.h>
#include <soc/mcm/intenum.h>
#include <soc/drv.h>
#include <shared/bsl.h>
#ifdef BCM_56670_A0
static soc_block_info_t soc_blocks_bcm56670_a0[] = {
    { SOC_BLK_AVS,          0,   33,  33   }, /* 0 A0 */
    { SOC_BLK_BCPORT,       0,   35,  35   }, /* 1 B0 */
    { SOC_BLK_BCPORT,       1,   36,  36   }, /* 2 B1 */
    { SOC_BLK_CLPORT,       0,   9,   9    }, /* 3 C0 */
    { SOC_BLK_CLPORT,       1,   11,  11   }, /* 4 C1 */
    { SOC_BLK_CLPORT,       2,   13,  13   }, /* 5 C2 */
    { SOC_BLK_CLPORT,       3,   14,  14   }, /* 6 C3 */
    { SOC_BLK_CLPORT,       4,   15,  15   }, /* 7 C4 */
    { SOC_BLK_CLPORT,       5,   22,  22   }, /* 8 C5 */
    { SOC_BLK_CLPORT,       6,   23,  23   }, /* 9 C6 */
    { SOC_BLK_CLPORT,       7,   25,  25   }, /* 10 C7 */
    { SOC_BLK_CLPORT,       8,   27,  27   }, /* 11 C8 */
    { SOC_BLK_CLPORT,       9,   29,  29   }, /* 12 C9 */
    { SOC_BLK_CMIC,         0,   5,   5    }, /* 13 C0 */
    { SOC_BLK_CPRI,         0,   8,   8    }, /* 14 C0 */
    { SOC_BLK_CPRI,         1,   10,  10   }, /* 15 C1 */
    { SOC_BLK_CPRI,         2,   12,  12   }, /* 16 C2 */
    { SOC_BLK_CPRI,         3,   24,  24   }, /* 17 C3 */
    { SOC_BLK_CPRI,         4,   26,  26   }, /* 18 C4 */
    { SOC_BLK_CPRI,         5,   28,  28   }, /* 19 C5 */
    { SOC_BLK_EPIPE,        0,   2,   2    }, /* 20 E0 */
    { SOC_BLK_IPIPE,        0,   1,   1    }, /* 21 I0 */
    { SOC_BLK_IPROC,        0,   34,  34   }, /* 22 I0 */
    { SOC_BLK_LBPORT,       0,   30,  30   }, /* 23 L0 */
    { SOC_BLK_MACSEC,       0,   37,  37   }, /* 24 M0 */
    { SOC_BLK_MMU,          0,   3,   3    }, /* 25 M0 */
    { SOC_BLK_OTPC,         0,   4,   4    }, /* 26 O0 */
    { SOC_BLK_PGW_CL,       0,   6,   6    }, /* 27 P0 */
    { SOC_BLK_PGW_CL,       1,   7,   7    }, /* 28 P1 */
    { SOC_BLK_SER,          0,   32,  32   }, /* 29 S0 */
    { SOC_BLK_TOP,          0,   31,  31   }, /* 30 T0 */
    { SOC_BLK_TSCF,         0,   63,  63   }, /* 31 T0 */
    { SOC_BLK_XLPORT,       0,   16,  16   }, /* 32 X0 */
    { SOC_BLK_XLPORT,       1,   17,  17   }, /* 33 X1 */
    { SOC_BLK_XLPORT,       2,   18,  18   }, /* 34 X2 */
    { SOC_BLK_XLPORT,       3,   19,  19   }, /* 35 X3 */
    { SOC_BLK_XLPORT,       4,   20,  20   }, /* 36 X4 */
    { SOC_BLK_XLPORT,       5,   21,  21   }, /* 37 X5 */
    { -1,                   -1,  -1,  -1   }  /* end */
};

static soc_port_info_t soc_ports_bcm56670_a0[] = {
    { 13,  0   }, /* 0 C0.0 */
    { -1,  0   }, /* 0 - */
    { -1,  0   }, /* 0 - */
    { 14,  0   }, /* 1 C0.0 */
    { 3,   0   }, /* 1 C0.0 */
    { 27,  0   }, /* 1 P0.0 */
    { 14,  1   }, /* 2 C0.1 */
    { 3,   1   }, /* 2 C0.1 */
    { 27,  1   }, /* 2 P0.1 */
    { 14,  2   }, /* 3 C0.2 */
    { 3,   2   }, /* 3 C0.2 */
    { 27,  2   }, /* 3 P0.2 */
    { 14,  3   }, /* 4 C0.3 */
    { 3,   3   }, /* 4 C0.3 */
    { 27,  3   }, /* 4 P0.3 */
    { 15,  0   }, /* 5 C1.0 */
    { 4,   0   }, /* 5 C1.0 */
    { 27,  4   }, /* 5 P0.4 */
    { 15,  1   }, /* 6 C1.1 */
    { 4,   1   }, /* 6 C1.1 */
    { 27,  5   }, /* 6 P0.5 */
    { 15,  2   }, /* 7 C1.2 */
    { 4,   2   }, /* 7 C1.2 */
    { 27,  6   }, /* 7 P0.6 */
    { 15,  3   }, /* 8 C1.3 */
    { 4,   3   }, /* 8 C1.3 */
    { 27,  7   }, /* 8 P0.7 */
    { 16,  0   }, /* 9 C2.0 */
    { 5,   0   }, /* 9 C2.0 */
    { 27,  8   }, /* 9 P0.8 */
    { 16,  1   }, /* 10 C2.1 */
    { 5,   1   }, /* 10 C2.1 */
    { 27,  9   }, /* 10 P0.9 */
    { 16,  2   }, /* 11 C2.2 */
    { 5,   2   }, /* 11 C2.2 */
    { 27,  10  }, /* 11 P0.10 */
    { 16,  3   }, /* 12 C2.3 */
    { 5,   3   }, /* 12 C2.3 */
    { 27,  11  }, /* 12 P0.11 */
    { 6,   0   }, /* 13 C3.0 */
    { 27,  12  }, /* 13 P0.12 */
    { -1,  0   }, /* 13 - */
    { 6,   1   }, /* 14 C3.1 */
    { 27,  13  }, /* 14 P0.13 */
    { -1,  0   }, /* 14 - */
    { 6,   2   }, /* 15 C3.2 */
    { 27,  14  }, /* 15 P0.14 */
    { -1,  0   }, /* 15 - */
    { 6,   3   }, /* 16 C3.3 */
    { 27,  15  }, /* 16 P0.15 */
    { -1,  0   }, /* 16 - */
    { 7,   0   }, /* 17 C4.0 */
    { 27,  16  }, /* 17 P0.16 */
    { -1,  0   }, /* 17 - */
    { 7,   1   }, /* 18 C4.1 */
    { 27,  17  }, /* 18 P0.17 */
    { -1,  0   }, /* 18 - */
    { 7,   2   }, /* 19 C4.2 */
    { 27,  18  }, /* 19 P0.18 */
    { -1,  0   }, /* 19 - */
    { 7,   3   }, /* 20 C4.3 */
    { 27,  19  }, /* 20 P0.19 */
    { -1,  0   }, /* 20 - */
    { 32,  0   }, /* 21 X0.0 */
    { 27,  20  }, /* 21 P0.20 */
    { -1,  0   }, /* 21 - */
    { 32,  1   }, /* 22 X0.1 */
    { 27,  21  }, /* 22 P0.21 */
    { -1,  0   }, /* 22 - */
    { 32,  2   }, /* 23 X0.2 */
    { 27,  22  }, /* 23 P0.22 */
    { -1,  0   }, /* 23 - */
    { 32,  3   }, /* 24 X0.3 */
    { 27,  23  }, /* 24 P0.23 */
    { -1,  0   }, /* 24 - */
    { 33,  0   }, /* 25 X1.0 */
    { 27,  24  }, /* 25 P0.24 */
    { -1,  0   }, /* 25 - */
    { 33,  1   }, /* 26 X1.1 */
    { 27,  25  }, /* 26 P0.25 */
    { -1,  0   }, /* 26 - */
    { 33,  2   }, /* 27 X1.2 */
    { 27,  26  }, /* 27 P0.26 */
    { -1,  0   }, /* 27 - */
    { 33,  3   }, /* 28 X1.3 */
    { 27,  27  }, /* 28 P0.27 */
    { -1,  0   }, /* 28 - */
    { 34,  0   }, /* 29 X2.0 */
    { 27,  28  }, /* 29 P0.28 */
    { -1,  0   }, /* 29 - */
    { 34,  1   }, /* 30 X2.1 */
    { 27,  29  }, /* 30 P0.29 */
    { -1,  0   }, /* 30 - */
    { 34,  2   }, /* 31 X2.2 */
    { 27,  30  }, /* 31 P0.30 */
    { -1,  0   }, /* 31 - */
    { 34,  3   }, /* 32 X2.3 */
    { 27,  31  }, /* 32 P0.31 */
    { -1,  0   }, /* 32 - */
    { 35,  0   }, /* 33 X3.0 */
    { 28,  0   }, /* 33 P1.0 */
    { -1,  0   }, /* 33 - */
    { 35,  1   }, /* 34 X3.1 */
    { 28,  1   }, /* 34 P1.1 */
    { -1,  0   }, /* 34 - */
    { 35,  2   }, /* 35 X3.2 */
    { 28,  2   }, /* 35 P1.2 */
    { -1,  0   }, /* 35 - */
    { 35,  3   }, /* 36 X3.3 */
    { 28,  3   }, /* 36 P1.3 */
    { -1,  0   }, /* 36 - */
    { 36,  0   }, /* 37 X4.0 */
    { 28,  4   }, /* 37 P1.4 */
    { -1,  0   }, /* 37 - */
    { 36,  1   }, /* 38 X4.1 */
    { 28,  5   }, /* 38 P1.5 */
    { -1,  0   }, /* 38 - */
    { 36,  2   }, /* 39 X4.2 */
    { 28,  6   }, /* 39 P1.6 */
    { -1,  0   }, /* 39 - */
    { 36,  3   }, /* 40 X4.3 */
    { 28,  7   }, /* 40 P1.7 */
    { -1,  0   }, /* 40 - */
    { 37,  0   }, /* 41 X5.0 */
    { 28,  8   }, /* 41 P1.8 */
    { -1,  0   }, /* 41 - */
    { 37,  1   }, /* 42 X5.1 */
    { 28,  9   }, /* 42 P1.9 */
    { -1,  0   }, /* 42 - */
    { 37,  2   }, /* 43 X5.2 */
    { 28,  10  }, /* 43 P1.10 */
    { -1,  0   }, /* 43 - */
    { 37,  3   }, /* 44 X5.3 */
    { 28,  11  }, /* 44 P1.11 */
    { -1,  0   }, /* 44 - */
    { 8,   0   }, /* 45 C5.0 */
    { 28,  12  }, /* 45 P1.12 */
    { -1,  0   }, /* 45 - */
    { 8,   1   }, /* 46 C5.1 */
    { 28,  13  }, /* 46 P1.13 */
    { -1,  0   }, /* 46 - */
    { 8,   2   }, /* 47 C5.2 */
    { 28,  14  }, /* 47 P1.14 */
    { -1,  0   }, /* 47 - */
    { 8,   3   }, /* 48 C5.3 */
    { 28,  15  }, /* 48 P1.15 */
    { -1,  0   }, /* 48 - */
    { 9,   0   }, /* 49 C6.0 */
    { 28,  16  }, /* 49 P1.16 */
    { -1,  0   }, /* 49 - */
    { 9,   1   }, /* 50 C6.1 */
    { 28,  17  }, /* 50 P1.17 */
    { -1,  0   }, /* 50 - */
    { 9,   2   }, /* 51 C6.2 */
    { 28,  18  }, /* 51 P1.18 */
    { -1,  0   }, /* 51 - */
    { 9,   3   }, /* 52 C6.3 */
    { 28,  19  }, /* 52 P1.19 */
    { -1,  0   }, /* 52 - */
    { 17,  0   }, /* 53 C3.0 */
    { 10,  0   }, /* 53 C7.0 */
    { 28,  20  }, /* 53 P1.20 */
    { 17,  1   }, /* 54 C3.1 */
    { 10,  1   }, /* 54 C7.1 */
    { 28,  21  }, /* 54 P1.21 */
    { 17,  2   }, /* 55 C3.2 */
    { 10,  2   }, /* 55 C7.2 */
    { 28,  22  }, /* 55 P1.22 */
    { 17,  3   }, /* 56 C3.3 */
    { 10,  3   }, /* 56 C7.3 */
    { 28,  23  }, /* 56 P1.23 */
    { 18,  0   }, /* 57 C4.0 */
    { 11,  0   }, /* 57 C8.0 */
    { 28,  24  }, /* 57 P1.24 */
    { 18,  1   }, /* 58 C4.1 */
    { 11,  1   }, /* 58 C8.1 */
    { 28,  25  }, /* 58 P1.25 */
    { 18,  2   }, /* 59 C4.2 */
    { 11,  2   }, /* 59 C8.2 */
    { 28,  26  }, /* 59 P1.26 */
    { 18,  3   }, /* 60 C4.3 */
    { 11,  3   }, /* 60 C8.3 */
    { 28,  27  }, /* 60 P1.27 */
    { 19,  0   }, /* 61 C5.0 */
    { 12,  0   }, /* 61 C9.0 */
    { 28,  28  }, /* 61 P1.28 */
    { 19,  1   }, /* 62 C5.1 */
    { 12,  1   }, /* 62 C9.1 */
    { 28,  29  }, /* 62 P1.29 */
    { 19,  2   }, /* 63 C5.2 */
    { 12,  2   }, /* 63 C9.2 */
    { 28,  30  }, /* 63 P1.30 */
    { 19,  3   }, /* 64 C5.3 */
    { 12,  3   }, /* 64 C9.3 */
    { 28,  31  }, /* 64 P1.31 */
    { 23,  0   }, /* 65 L0.0 */
    { -1,  0   }, /* 65 - */
    { -1,  0   }, /* 65 - */
    { 24,  0   }, /* 66 M0.0 */
    { -1,  0   }, /* 66 - */
    { -1,  0   }, /* 66 - */
    { -1,  -1  }  /* end */
};

soc_driver_t soc_driver_bcm56670_a0 = {
    /* type                   */ SOC_CHIP_BCM56670_A0,
    /* chip_string            */ "monterey",
    /* origin                 */ "Id: //depot/monterey/regsfile/chip_top.regs#32 ",
    /* pci_vendor             */ BROADCOM_VENDOR_ID,
    /* pci_device             */ BCM56670_DEVICE_ID,
    /* pci_revision           */ BCM56670_A0_REV_ID,
    /* num_cos                */ 48,
    /* reg_info               */ NULL,
    /* reg_unique_acc         */ NULL,
    /* reg_above_64_info      */ NULL,
    /* reg_array_info         */ NULL,
    /* mem_info               */ NULL,
    /* mem_unique_acc         */ NULL,
    /* mem_aggr               */ NULL,
    /* mem_array_info         */ NULL,
    /* block_info             */ soc_blocks_bcm56670_a0,
    /* port_info              */ soc_ports_bcm56670_a0,
    /* counter_maps           */ NULL,
    /* features               */ NULL,
    /* init                   */ NULL,
    /* services               */ NULL,
    /* port_num_blktype       */ 3,
    /* cmicd_base             */ 0x03200000,
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
      return &soc_driver_bcm56670_a0;
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
soc_monterey_info_config(int unit, soc_control_t *soc)
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
    si->chip_type = SOC_INFO_CHIP_TYPE_APACHE;
    
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

#endif /* BCM_56560_A0 */
