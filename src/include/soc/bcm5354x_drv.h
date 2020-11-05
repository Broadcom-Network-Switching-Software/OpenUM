#ifndef _BCM5354X_DRV_H_
#define _BCM5354X_DRV_H_

/* move from drv.h */
typedef int (*soc_misc_init_f)(int);
typedef int (*soc_mmu_init_f)(int);
typedef int (*soc_age_timer_get_f)(int, int *, int *);
typedef int (*soc_age_timer_max_get_f)(int, int *);
typedef int (*soc_age_timer_set_f)(int, int, int);
typedef int (*soc_phy_firmware_load_f)(int, int, uint8 *, int);
typedef int (*soc_sbus_mdio_read_f)(int, uint32, uint32, uint32 *);
typedef int (*soc_sbus_mdio_write_f)(int, uint32, uint32, uint32);
typedef int (*soc_bond_options_init_f)(int);

/*
 * Typedef: soc_functions_t
 * Purpose: Chip driver functions that are not automatically generated.
 */
typedef struct soc_functions_s {
    /* Get/set age timer value and enable in device, if supported */
    soc_misc_init_f                    soc_misc_init;
    soc_mmu_init_f                     soc_mmu_init;
    soc_age_timer_get_f                soc_age_timer_get;
    soc_age_timer_max_get_f            soc_age_timer_max_get;
    soc_age_timer_set_f                soc_age_timer_set;
    soc_phy_firmware_load_f            soc_phy_firmware_load;
    soc_sbus_mdio_read_f               soc_sbus_mdio_read;
    soc_sbus_mdio_write_f              soc_sbus_mdio_write;
    soc_bond_options_init_f            soc_bond_options_init;
} soc_functions_t;


typedef int (*soc_portctrl_pm_init_f)(int);
typedef int (*soc_portctrl_pm_deinit_f)(int);
typedef int (*soc_portctrl_pm_port_config_get_f)(int, soc_port_t, void*);
typedef int (*soc_portctrl_pm_port_phyaddr_get_f)(int, soc_port_t);

typedef struct soc_portctrl_functions_s {
    soc_portctrl_pm_init_f             soc_portctrl_pm_init;
    soc_portctrl_pm_deinit_f           soc_portctrl_pm_deinit;
    soc_portctrl_pm_port_config_get_f    soc_portctrl_pm_port_config_get;
    soc_portctrl_pm_port_phyaddr_get_f   soc_portctrl_pm_port_phyaddr_get;
} soc_portctrl_functions_t;

/*Type used to define the distribution of 10G lanes for a 100G port using
 * a TSC-12 which provides up to 12 10G lanes*/
typedef enum soc_100g_lane_config_s {
    SOC_LANE_CONFIG_100G_4_4_2 = 0,
    SOC_LANE_CONFIG_100G_3_4_3 = 1,
    SOC_LANE_CONFIG_100G_2_4_4 = 2,
    SOC_INVALID_LANE_CONFIG    = -1
} soc_100g_lane_config_t;

/*
 * soc_info_t is part of the per-unit soc_control_t.
 * It gets filled in at run time based on the port_info and block_info
 * from the driver structure.
 */
typedef struct {
    int     num;                    /* number of entries used in port[] */
    int     min;                    /* minimum bit on */
    int     max;                    /* maximum bit on */
    pbmp_t  bitmap;
    pbmp_t  disabled_bitmap;        /* ports that are forced to be disabled */
} soc_ptype_t;


typedef struct {
    soc_chip_e      chip_type;            /* chip type enum - used instead of 'chip'
                                             for newer devices as the bitmap was exhausted */
    uint32          spi_device;
    uint32          chip;                  /* chip id bits */
    int             block_num;              /* count of entries in block_info */
    soc_ptype_t     fe;    
    soc_ptype_t     ge;
    soc_ptype_t     ce;
    soc_ptype_t     il;                     /* Interlaken :phy driver need this*/
    soc_ptype_t     xe;
    soc_ptype_t     xl;
    soc_ptype_t     hg;
    soc_ptype_t     cl;                     /* cport and xport cores based port */
    soc_ptype_t     port;                   /* fe|ge|xe|hg|spi|fl */
    soc_ptype_t     all;                    /* fe|ge|xe|hg|cmic|fl */

    pbmp_t          lmd_pbm;                /* LMD port bitmap */
    pbmp_t          gmii_pbm;               /* GMII port bitmap */
    pbmp_t          hg2_pbm;                /* HiGig2 encap port bitmap */
    pbmp_t          oversub_pbm;                /* Over sub */
    int             port_offset[SOC_MAX_NUM_PORTS];       /* 3 for ge3 */
    int             port_init_speed[SOC_MAX_NUM_PORTS];   /* ports initial speed */
    int             port_speed_max[SOC_MAX_NUM_PORTS];    /* max port speed */
    char            port_name[SOC_MAX_NUM_PORTS][11];
    int             port_type[SOC_MAX_NUM_PORTS];   /* used by NULL phy driver */
    int             port_p2l_mapping[SOC_MAX_NUM_PORTS];  /* phy to logic */
    int             port_l2p_mapping[SOC_MAX_NUM_PORTS];  /* logic to phy */
    int             port_l2p_mapping_valid[SOC_MAX_NUM_PORTS];  /* logic to phy */
    int             port_p2m_mapping[SOC_MAX_NUM_PORTS];             /* phy to mmu */
    int             port_m2p_mapping[SOC_MAX_NUM_MMU_PORTS];         /* mmu to phy */    
    int             port_u2l_mapping[SOC_MAX_NUM_PORTS];
    int             port_l2u_mapping[SOC_MAX_NUM_PORTS];
    int             port_num_lanes[SOC_MAX_NUM_PORTS];               /* number of lanes */
    int             port_refclk_int[SOC_MAX_NUM_PORTS];              /*internal phy refclk*/
    soc_100g_lane_config_t port_100g_lane_config[SOC_MAX_NUM_PORTS]; /*Phy active lane assignment*/
    int             port_fallback_lane[SOC_MAX_NUM_PORTS];           /*lane used when 100G port Auto Negotiates down for TSCE 12X10*/
    pbmp_t          block_bitmap[SOC_MAX_NUM_BLKS];
} soc_info_t;

/*
 * Typedef: soc_control_t
 * Purpose: SOC Control Structure.  All info about a device instance.
 */

typedef struct soc_control_s {
    uint32      soc_flags; /* Flags for this device */
#define   SOC_F_ATTACHED           0x01    /* Device attached */
#define   SOC_F_INITED             0x02    /* Device inited */
#define   SOC_F_LSE                0x04    /* Link SCAN enabled in SW */
#define   SOC_F_SL_MODE            0x08    /* Device is in SL stack mode */
#define   SOC_F_POLLED             0x10    /* Polled IRQ mode */
#define   SOC_F_URPF_ENABLED       0x20    /* Unicast rpf enabled on device */
#define   SOC_F_IPMCREPLSHR        0x40    /* IPMC replication with lists */
#define   SOC_F_BUSY               0x80    /* Device is not fully out of Reset */
#define   SOC_F_MEM_CLEAR_USE_DMA  0x100   /* Device should use table dma
                                              for memory clear operations */
#define   SOC_F_GPORT              0x200   /* Device should use GPORT for
                                              source/destination notation */
#define   SOC_F_RCPU_SCHAN         0x400   /* Indicates that SCHAN operations will
                                              be sent over RCPU mechanism */
#define   SOC_F_RCPU_ONLY          0x800   /* Indicates that all access to the device
                                              will be done through RCPU */
#define   SOC_F_RE_INITED          0x1000  /* Device re-inited while running */
#define   SOC_F_PORT_CLASS_BLOCKED 0x2000  /* Device is not allowed to change port classes */
#define   SOC_F_XLATE_EGR_BLOCKED  0x4000  /* Device is not allowed to setup old
                                              style vlan egress translate */
#define   SOC_F_KNET_MODE          0x8000  /* Device is in kernel networking mode */
#define   SOC_F_REMOTE_ENCAP       0x10000 /* Device is uses remote multicast encapsulation */
#define   SOC_F_MEM_CLEAR_HW_ACC   0x20000 /* Device should use h/w pipe clear
                                              for memory clear operations */
#define   SOC_F_ALL_MODULES_INITED 0x40000 /* System Level Init Flag */
#define   SOC_F_HW_RESETING        0x80000 /* System is processing cold boot */

    /* Mask of flags that are persistent across init */
#define   SOC_F_RESET              (SOC_F_ATTACHED|SOC_F_INITED|SOC_F_POLLED|SOC_F_HW_RESETING|SOC_F_KNET_MODE)

    /* Port and block information filled in when attached or inited */
    soc_info_t  info;

    /* Chip driver pointers */
    soc_driver_t
                *chip_driver;
    soc_functions_t *soc_functions;

    soc_portctrl_functions_t
                *soc_portctrl_functions;
    /* Feature cache */
    uint32    features[64];
} soc_control_t;


#define SOC_UNIT_NAME(unit) "BCM53540_A0"

#define SOC_UNIT_GROUP(unit) "BCM53540"

#define SOC_UNIT_FAMILY(unit) ""

/* For NULL PHY */
#undef SOC_PORT_VALID
#define SOC_PORT_VALID(unit, port)    (1)

extern void soc_phy_addr_default(int unit, int lport, uint16 *phy_addr, uint16 *phy_addr_int);
extern soc_functions_t *soc_chip_drv_funs_find(uint16 dev_id, uint8 rev_id);
#endif /* _BCM5354X_DRV_H_ */
