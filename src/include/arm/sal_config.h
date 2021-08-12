/*
 * $Id: sal_config.h,v 1.31 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED

extern sys_error_t sal_config_init(void);
extern void sal_config_show(void);

/**
 * sal_config_set: set ascii value associated with name
 *
 * @param name (IN)- name of config item
 * @param value(IN)- value of config item
 * @return  address of
 *     SYS_OK : config set success
 *     else config get fail
 */

extern sys_error_t sal_config_set(const char *name, const char *value);

/**
 * sal_config_get: get ascii value associated with name
 *
 * @param name (IN)- name of config item
 * @return  address of
 *     NULL : config get fail
 *     else config get success
 */
extern const char *sal_config_get(const char *name);

/**
 * sal_config_xy_get: get ascii value associated with name
 *
 * @param name (IN)- name of config item
 * @return  address of
 *     NULL : config get fail
 *     else config get success
 */
extern const char *sal_config_xy_get(const char *name, int x, int y);

/**
 * sal_config_pbmp_get: get pbmp_t associated with name
 *
 * @param name (IN)- name of config item
 * @param x    (IN)- if x < 0 , x value is invalid, if x >= 0, x is valid and used as index
 * @param y    (IN)- if y < 0 , y value is invalid, if y >= 0, y is valid and used as index
 * @param p (IN)- address of ouput port bit map
 * @return
 *     SYS_ERR_FALSE : config get fail
 *     SYS_OK: config get success
 */

extern sys_error_t sal_config_pbmp_get(const char *name, pbmp_t *p);

/**
 * sal_config_pbmp_set: set pbmp_t associated with name
 *
 * @param name (IN)- name of config item
 * @param p (IN)- address of ouput port bit map
 * @return
 *     SYS_ERR_FALSE : config set fail
 *     SYS_OK: config set success
 */

extern sys_error_t sal_config_pbmp_set(const char *name, pbmp_t p);

/**
 * sal_config_bytes_get: get bytes buffer associated with name
 *
 * @param name (IN)- name of config item
 * @param buf (IN) - output word buffer address
 * @param len (IN) - output word buffer length
 * @return byte count of output result
 *    0 : means no output
 */

extern int sal_config_bytes_get(const char*name, uint8* buf, int len);


/**
 * sal_config_words_get: get words buffer associated with name
 *
 * @param name (IN)- name of config item
 * @param buf (IN) - output word buffer address
 * @param len (IN) - output word buffer length
 * @return byte count of output result
 *    0 : means no output
 */

extern int sal_config_words_get(const char*name, uint32* buf, int word_len);


/**
 * sal_config_uint8_get: get a uint8 variable associated with name
 *
 * @param name (IN)- name of config item
 * @param byte (IN) - output byte address
 * @param len (IN) - output word buffer length
 * @return word count of output result
 *          0 : means no output
 */

extern sys_error_t sal_config_uint8_get(const char*name, uint8* byte);


/**
 * sal_config_uint8_xy_get: get a uint8 variable associated with name, x, y
 *                        the config value could be "name", "name_x" and "name_x_y"
 *
 * @param name (IN)- name of config item
 * @param x    (IN)- if x < 0 , x value is invalid, if x >= 0, x is valid and used as index
 * @param y    (IN)- if y < 0 , y value is invalid, if y >= 0, y is valid and used as index
 * @param byte (IN) - output byte address
 * @param len (IN) - output word buffer length
 * @return word count of output result
 *          0 : means no output
 */

extern sys_error_t sal_config_uint8_xy_get(const char* name, int x, int y, uint8* byte);


/**
 * sal_config_uint16_get: get a uint16 variable associated with name
 *
 * @param name (IN)- name of config item
 * @param hword (IN) - output hword address
 * @param len (IN) - output word buffer length
 * @return word count of output result
 *          0 : means no output
 */

extern sys_error_t sal_config_uint16_get(const char*name, uint16* hword);


/**
 * sal_config_uint32_get: get a uint32 variable associated with name
 *
 * @param name (IN)- name of config item
 * @param word (IN) - output word buffer address
 * @param len (IN) - output word buffer length
 * @return word count of output result
 *          0 : means no output
 */

extern sys_error_t sal_config_uint32_get(const char*name, uint32* word);



/*

     To enumerate all possible config items

*/
#define SAL_CONFIG_BOARD_NAME                  "board_name"
#define SAL_CONFIG_SKU_DEVID                   "sku_devid"
#define SAL_CONFIG_SKU_OPTION                  "sku_option"

#define SAL_CONFIG_LED_OPTION                  "led_option"
#define SAL_CONFIG_LED_PROGRAM                 "led_program"
#define SAL_CONFIG_LED_1_PROGRAM               "led_1_program"


#define SAL_CONFIG_RESET_BUTTON_ENABLE         "reset_button_enable"
#define SAL_CONFIG_RESET_BUTTON_GPIO_BIT       "reset_button_gpio_bit"
#define SAL_CONFIG_RESET_BUTTON_POLARITY       "reset_button_polarity"

#define SAL_CONFIG_PHY_LED1_MODE               "phy_led1_mode"
#define SAL_CONFIG_PHY_LED2_MODE               "phy_led2_mode"
#define SAL_CONFIG_PHY_LED3_MODE               "phy_led3_mode"
#define SAL_CONFIG_PHY_LED4_MODE               "phy_led4_mode"
#define SAL_CONFIG_PHY_LED_CTRL                "phy_led_ctrl"
#define SAL_CONFIG_PHY_LED_SELECT              "phy_led_select"

#define SAL_CONFIG_VALID_PORTS                 "valid_logical_ports"

#define SAL_CONFIG_SPEED_1000_PORTS            "speed_1000_logical_ports"
#define SAL_CONFIG_SPEED_2500_PORTS            "speed_2500_logical_ports"
#define SAL_CONFIG_SPEED_5000_PORTS            "speed_5000_logical_ports"
#define SAL_CONFIG_SPEED_10000_PORTS           "speed_10000_logical_ports"
#define SAL_CONFIG_SPEED_25000_PORTS           "speed_25000_logical_ports"
#define SAL_CONFIG_SPEED_40000_PORTS           "speed_40000_logical_ports"
#define SAL_CONFIG_SPEED_50000_PORTS           "speed_50000_logical_ports"

#define SAL_CONFIG_PHY_AN_PORTS                "phy_an_logical_ports"
#define SAL_CONFIG_PHY_CL73_PORTS              "phy_cl73_logical_ports"
#define SAL_CONFIG_PHY_CL37_PORTS              "phy_cl37_logical_ports"

#define SAL_CONFIG_QTC_INTERFACE               "qtc_interface"
#define SAL_CONFIG_TSCE_INTERFACE              "tsce_interface"
#define SAL_CONFIG_TSCF_INTERFACE              "tscf_interface"
#define SAL_CONFIG_TSC_INTERFACE               "tsc_interface"
#define SAL_CONFIG_SGMIIPX4_INTERFACE          "sgmiip4_interface"
#define SAL_CONFIG_VIPER_INTERFACE             "viper_interface"

#define SAL_CONFIG_UIP_IFCONFIG                "ifconfig"

#define SAL_CONFIG_MMU_LOSSLESS                "mmu_lossless"
#define SAL_CONFIG_MMU_LOSSLESS_PORTS          "mmu_lossless_logical_ports"

#define SAL_CONFIG_OVERSUBSCRIBE_PORTS               "oversubscribe_ports"

#define SAL_CONFIG_COE_SCENARIO     "coe_scenario"

/*
 * 0: Linerate only (default).
 * 1: Oversubscribe mode (all ports will be oversub).
 * 2: Mixed mode. Check device specification for applicability. Port bitmap specified via pbmp_oversubscribe.
 */
#define SAL_CONFIG_OVERSUBSCRIBE_MODE               "oversubscribe_mode"
/*
 * "portmap" is used to defines a fundamental port configuration which complies with PRD,
 *  by specifying the logical to physical port mapping and port maximun speed.
 *
 *    portmap_<logical_port_number>=<physical port number>:<max_speed in Gb>
 *
 *   Example: portmap_2 = 4:10
 *          portmap_1 = 4:40
 */

#define SAL_CONFIG_PORTMAP                          "portmap"

/*
 * Select mmu mode.
 */
#define SAL_CONFIG_MMU_MODE  "mmu_mode"

/*
 * Specifying non-zero port bitmap for the ports connected to the backplane.
 */
#define SAL_CONFIG_UPLINK_LOGICAL_PORTS  "uplink_logical_ports"

/*
 * Specifying non-zero port bitmap for the standby uplink ports.
 */
#define SAL_CONFIG_STANDBY_LOGICAL_PORTS  "standby_logical_ports"

/* Enable 1000X at 25G vco. default is running at 20G vco  1 enable */
#define SAL_CONFIG_SERDES_1000X_AT_25G_VCO  "serdes_1000x_at_25g_vco"

#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */
