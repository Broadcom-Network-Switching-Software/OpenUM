/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * StrataSwitch driver
 */

#include <soc/drv.h>
#ifdef _SHR_BSL_H_
#error Header file soc/drv.h must not include bsl.h directly or indirectly
#endif

#include <shared/bsl.h>

#include <sal/core/libc.h>
#include <shared/alloc.h>
#include <shared/switch.h>
#include <shared/bitop.h>

#if defined(PORTMOD_SUPPORT) && defined(BCM_ESW_SUPPORT)
#include <soc/esw/portctrl.h>
#endif

/*
 * Driver global variables
 *
 *   soc_control: per-unit driver control structure
 *   soc_persist: persistent per-unit driver control structure
 *   soc_family_suffix: a family suffix, configured by soc property soc_family.
 *     Used what parsing soc properties.
 *     If configured, and property.unit, property.chip and property.group was not
 *     found, property.family is searched for.
 */
soc_control_t   *soc_control[SOC_MAX_NUM_DEVICES];
char            *soc_family_suffix[SOC_MAX_NUM_DEVICES] = {0};




/*
 * Function:
 *    soc_dev_name
 * Purpose:
 *    Return official name (or optionally nickname) of attached chip.
 * Parameters:
 *      unit - unit number
 * Returns:
 *    Pointer to static string
 */

const char *
soc_dev_name(int unit)
{
    const char *name = soc_cm_get_name(unit);
    return (name == NULL ? "<UNKNOWN>" : name);
}

/*
 * Function:
 *    soc_attached
 * Purpose:
 *    Check if a specified SOC device is probed and attached.
 * Returns:
 *    TRUE if attached, FALSE otherwise.
 */
int
soc_attached(int unit)
{
    return 1;
}

/*
 * Function:
 *      _str_to_val
 * Purpose:
 *      Convert string to value
 * Parameters:
 *      str - (IN) value string
 *      val - (OUT) converted value
 *      scale - (IN) scale up factor (for decimal only)
 *                   1 means to multiply by 10 before return
 *                   2 means to multiply by 100 before return
 *                   ...
 *      suffix - (OUT) the unit specified in the property (for decimal only)
 *                     'b' for 100b, or 'c' for 500c, ...
 * Notes:
 *      The string is parsed as a C-style numeric constant.
 */
STATIC char *
_str_to_val(const char *str, int *val, int scale, char *suffix)
{
    uint32 abs_val;
    int neg, base, shift;
    unsigned char symbol;

    if (*str == '0' && str[1] != '.' && str[1] != ',') {
        str++;

        symbol = *str | 0x20;
        if (symbol == 'x') { /* hexadecimal */
            str++;
            for (abs_val = 0; *str != '\0'; str++) {
                symbol = *str - '0';
                if (symbol < 10) {
                    abs_val = (abs_val << 4) | symbol;
                    continue;
                }
                symbol = (*str | 0x20) - 'a';
                if (symbol < 6) {
                    abs_val = (abs_val << 4) | (symbol + 10);
                    continue;
                }
                break;
            }
        } else {
            if (symbol == 'b') { /* binary */
                base = 2;
                shift = 1;
                str++;
            } else { /* octal */
                base = 8;
                shift = 3;
            }
            for (abs_val = 0; *str != '\0'; str++) {
                symbol = *str - '0';
                if (symbol < base) {
                    abs_val = (abs_val << shift) | symbol;
                    continue;
                }
                break;
            }
        }
        *val = abs_val;
        *suffix = (*str == ',') ? ',' : '\0';
    } else {
        if (*str == '-') {
            str++;
            neg = TRUE;
        } else {
            neg = FALSE;
        }
        for (abs_val = 0; *str != '\0'; str++) {
            symbol = *str - '0';
            if (symbol < 10) {
                abs_val = abs_val * 10 + symbol;
                continue;
            }
            break;
        }
        if (*str == '.') {
            str++;
            for (; scale > 0 && *str != '\0'; scale--, str++) {
                symbol = *str - '0';
                if (symbol < 10) {
                    abs_val = abs_val * 10 + symbol;
                    continue;
                }
                break;
            }
        }
        for (; scale > 0; scale--) {
            abs_val *= 10;
        }

        *val = neg ? -(int)abs_val : (int)abs_val;
        *suffix = *str;
    }

    return (char *)str;
}

/*
 * Function:
 *  soc_property_get_str
 * Purpose:
 *  Retrieve a global configuration property string.
 * Parameters:
 *  unit - unit number
 *  name - base name of property to look up
 * Notes:
 *  Each property requested is looked up with one suffix after
 *  another until it is found.  Suffixes are tried in the following
 *  order:
 *  .<unit-num>     (e.g. ".0")
 *  .<CHIP_TYPE>    (e.g. BCM5680_B0)
 *  .<CHIP_GROUP>   (e.g. BCM5680)
 *  <nothing>
 */
char *
soc_property_get_str(int unit, const char *name)
{
    if (unit >= 0) {
        /* length of name + \0 */
        int     l = sal_strlen(name) + 1;
        int     ltemp;
        char    name_exp[SOC_PROPERTY_NAME_MAX], *s;

        if (sal_snprintf(name_exp, SOC_PROPERTY_NAME_MAX,  
                         "%s.%d", name, unit) >= SOC_PROPERTY_NAME_MAX) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "Unsupported soc_property length for %s.%d. "
                          "Max soc property length:%d\n"), 
               name, unit, SOC_PROPERTY_NAME_MAX));
            return NULL;
        }
        if ((s = soc_cm_config_var_get(unit, name_exp)) != NULL) {
            return s;
        }

        /* Remaining operations require that the unit is attached */
        if (soc_attached(unit)) {

            ltemp = sal_strlen(SOC_UNIT_NAME(unit));

            /* check the length of <name>.<unit_name>\0 */
            if ((l + 1 + ltemp) > SOC_PROPERTY_NAME_MAX) {
                return NULL;
            }
            sal_strncpy(name_exp + l, SOC_UNIT_NAME(unit), ltemp);
            if (ltemp)
                *(name_exp + l + ltemp) = '\0';
            if ((s = soc_cm_config_var_get(unit, name_exp)) != NULL) {
                return s;
            }

            ltemp = sal_strlen(SOC_UNIT_GROUP(unit));

            /* check the length of <name>.<unit_group> */
            if ((l + 1 + ltemp) > SOC_PROPERTY_NAME_MAX) {
                return NULL;
            }
            sal_strncpy(name_exp + l, SOC_UNIT_GROUP(unit), ltemp);
            if (ltemp)
                *(name_exp + l + ltemp) = '\0';
            if ((s = soc_cm_config_var_get(unit, name_exp)) != NULL) {
                return s;
            }

            if (soc_family_suffix[unit]) {
                ltemp = sal_strlen(soc_family_suffix[unit]);

                /* check the length of <name>.<soc_family_suffix> */
                if ((l + 1 + ltemp) > SOC_PROPERTY_NAME_MAX) {
                    return NULL;
                }
                sal_strncpy(name_exp + l, soc_family_suffix[unit], ltemp);
                if (ltemp)
                    *(name_exp + l + ltemp) = '\0';
                if ((s = soc_cm_config_var_get(unit, name_exp)) != NULL) {
                    return s;
                }
            }
        }
    }

    return soc_cm_config_var_get(unit, name);
}

/*
 * Function:
 *      soc_property_get
 * Purpose:
 *      Retrieve a global configuration numeric value.
 * Parameters:
 *      unit - unit number
 *      name - base name of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      See soc_property_get_str for suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */

uint32
soc_property_get(int unit, const char *name, uint32 defl)
{
    char        *s;

    if ((s = soc_property_get_str(unit, name)) == NULL) {
        return defl;
    }

    return _shr_ctoi(s);
}

/*
 * Function:
 *      soc_property_obj_attr_get
 * Purpose:
 *      Retrieve a global configuration numeric value and its unit for the
 *      specified prefix, object and attribute.
 * Parameters:
 *      unit - unit number
 *      prefix - prefix name of the numeric property to look up
 *      obj - object name of the numeric property to look up
 *      index - object index of the numeric property to look up
 *      attr - attribute name of the numeric property to look up
 *      scale - scale up factor (see _str_to_val)
 *      suffix - the unit specified in the property (see _str_to_val)
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of object suffixes
 *      until it is found.
 *              prefix.obj1.attr    attribute of a particular object instance
 *              prefix.obj.attr     attribute of the object type
 *      If none are found then the default value in defl is returned.
 *      See soc_property_get_str for other suffix handling in property name.
 *      See _str_to_val for value string handling.
 *      The string is parsed as a C-style numeric constant.
 */

uint32
soc_property_obj_attr_get(int unit, const char *prefix, const char *obj,
                          int index, const char *attr, int scale,
                          char *suffix, uint32 defl)
{
    char        *str;
    int         val;
    char        prop[SOC_PROPERTY_NAME_MAX];

    str = NULL;


    if (index != -1) {
        if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                         "%s.%s%d.%s", prefix, obj, index, attr) >= SOC_PROPERTY_NAME_MAX) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "Unsupported soc_property length for %s.%s%d.%s. "
                          "Max soc property length:%d\n"), 
               prefix, obj, index, attr, SOC_PROPERTY_NAME_MAX));
            return defl; 
        }
        str = soc_property_get_str(unit, prop);
    }

    if (str == NULL) {
        if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                         "%s.%s.%s", prefix, obj, attr) >= SOC_PROPERTY_NAME_MAX) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "Unsupported soc_property length for %s.%s.%s. "
                          "Max soc property length:%d\n"), 
               prefix, obj, attr, SOC_PROPERTY_NAME_MAX));
            return defl; 
        }
        str = soc_property_get_str(unit, prop);
    }

    if (str == NULL) {
        return defl;
    }

    (void)_str_to_val(str, &val, scale, suffix);
    return (uint32)val;
}

/*
 * Function:
 *      soc_property_get_pbmp
 * Purpose:
 *      Retrieve a global configuration bitmap value.
 * Parameters:
 *      unit - unit number
 *      name - base name of numeric property to look up
 *      defneg - should default bitmap be all 1's?
 * Notes:
 *      See soc_property_get_str for suffix handling.
 *      The string can be more than 32 bits worth of
 *      data if it is in hex format (0x...).  If not
 *      hex, it is treated as a 32 bit value.
 */

pbmp_t
soc_property_get_pbmp(int unit, const char *name, int defneg)
{
    pbmp_t      bm;
    char        *s;

    if ((s = soc_property_get_str(unit, name)) == NULL) {
        SOC_PBMP_CLEAR(bm);
        if (defneg) {
            SOC_PBMP_NEGATE(bm, bm);
        }
        return bm;
    }
    if (_shr_pbmp_decode(s, &bm) < 0) {
        SOC_PBMP_CLEAR(bm);
    }
    return bm;
}

/*
 * Function:
 *      soc_property_get_pbmp_default
 * Purpose:
 *      Retrieve a global configuration bitmap value with specific default.
 * Parameters:
 *      unit - unit number
 *      name - base name of numeric property to look up
 *      def_pbmp - default bitmap
 * Notes:
 *      See soc_property_get_str for suffix handling.
 *      The string can be more than 32 bits worth of
 *      data if it is in hex format (0x...).  If not
 *      hex, it is treated as a 32 bit value.
 */

pbmp_t
soc_property_get_pbmp_default(int unit, const char *name,
                              pbmp_t def_pbmp)
{
    pbmp_t      bm;
    char        *s;

    if ((s = soc_property_get_str(unit, name)) == NULL) {
        SOC_PBMP_ASSIGN(bm, def_pbmp);
        return bm;
    }
    if (_shr_pbmp_decode(s, &bm) < 0) {
        SOC_PBMP_CLEAR(bm);
    }
    return bm;
}

/*
 * Function:
 *      soc_property_suffix_num_str_get
 * Purpose:
 *      Retrieve a per-enumerated suffix global configuration bitmap value.
 * Parameters:
 *      unit - unit number
 *      num - enumerated suffix for which property is applicable
 *      name - base name of numeric property to look up
 *      suffix - suffix of numeric property to look up
 *      pbmp_def - default value of bitmap property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"0  enumerated suffix name
 *              name            plain name
 *      If none are found then the default value in pbmp_def is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as an hexadecimal value
 */

pbmp_t
soc_property_suffix_num_pbmp_get(int unit, int num, const char *name,
                            const char *suffix, soc_pbmp_t pbmp_def)
{
    pbmp_t      bm;
    char        *s;

    if ((s = soc_property_suffix_num_str_get(unit, num, name, suffix)) == NULL) {
        SOC_PBMP_ASSIGN(bm, pbmp_def);
        return bm;
    }
    if (_shr_pbmp_decode(s, &bm) < 0) {
        SOC_PBMP_ASSIGN(bm, pbmp_def);
    }
    return bm;
}


/*
 * Function:
 *      soc_property_get_bitmap_default
 * Purpose:
 *      Retrieve a global configuration bitmap value with specific default.
 * Parameters:
 *      unit - unit number
 *      name - base name of numeric property to look up
 *      def_bitmap - default bitmap
 *      bitmap - reterived bitmap
 * Notes:
 *      See soc_property_get_str for suffix handling.
 *      The string can be more than 32 bits worth of
 *      data if it is in hex format (0x...).  If not
 *      hex, it is treated as a 32 bit value.
 */

void
soc_property_get_bitmap_default(int unit, const char *name,
                              uint32 *bitmap, int max_words, uint32 *def_bitmap)
{
    char        *s;

    if ((s = soc_property_get_str(unit, name)) == NULL) {
        sal_memcpy(bitmap, def_bitmap, sizeof(uint32) * max_words);
        return;
    }
    if (shr_bitop_str_decode(s, bitmap, max_words) < 0) {
        sal_memset(bitmap, 0, sizeof(uint32) * max_words);
        return;
    }
}

/*
 * Function:
 *      soc_property_get_csv
 * Purpose:
 *      Retrieve a global configuration comma-separated value in an array.
 * Parameters:
 *      unit - unit number
 *      name - base name of numeric property to look up
 *      val_array - Integer array to put the values
 *      val_max - Maximum number of elements to put into the array
 * Notes:
 *      Returns the count of values read.
 */

int
soc_property_get_csv(int unit, const char *name,
                          int val_max, int *val_array)
{
    char *str, suffix;
    int count;

    str = soc_property_get_str(unit, name);
    if (str == NULL) {
        return 0;
    }

    count = 0;
    for (count = 0; count < val_max; count++) {
        str = _str_to_val(str, &val_array[count], 0, &suffix);
        if (suffix == ',') {
            str++;
        } else {
            count++;
            break;
        }
    }

    return count;
}

/*
 * Function:
 *      soc_property_port_get
 * Purpose:
 *      Retrieve a per-port global configuration value
 * Parameters:
 *      unit - unit number
 *      port - Zero-based port number.
 *      name - base name of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_fe0        port name
 *              name_fe         port type
 *              name_port1      logical port number (one based)
 *              name.port1      physical port number (one based)
 *              name_1          BCM API port number (untranslated)
 *              name            plain name
 *      If none are found then NULL is returned
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */

char *
soc_property_port_get_str(int unit, soc_port_t port,
                          const char *name)
{
    char        *s, *p;
    pbmp_t      lpbmp;
    char prop[SOC_PROPERTY_NAME_MAX];
#if 0
    char prop_alter[SOC_PROPERTY_NAME_MAX];
    int         pno;
#endif
    /*
     * Try "name_fe0" if in valid range (can be out of range for
     * subport / internal)
     */
    if (port < SOC_MAX_NUM_PORTS) {        
        
        if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                         "%s_%s", name, SOC_PORT_NAME(unit, port)) >= SOC_PROPERTY_NAME_MAX) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "Unsupported soc_property length for %s_%s. "
                          "Max soc property length:%d\n"), 
               name, SOC_PORT_NAME(unit, port), SOC_PROPERTY_NAME_MAX));
            return NULL; 
        }

        if ((s = soc_property_get_str(unit, prop)) != NULL) {
            return s;
        }
#if 0
        /* Try alternative name if available */
        if (SOC_PORT_NAME_ALTER_VALID(unit, port))
        {
            if (sal_snprintf(prop_alter, SOC_PROPERTY_NAME_MAX, 
                             "%s_%s", name, SOC_PORT_NAME_ALTER(unit, port)) >= SOC_PROPERTY_NAME_MAX) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Unsupported soc_property length for %s_%s. "
                              "Max soc property length:%d\n"), 
                   name, SOC_PORT_NAME_ALTER(unit, port), SOC_PROPERTY_NAME_MAX));
                return NULL; 
            }

            if ((s = soc_property_get_str(unit, prop_alter)) != NULL) {
                return s;
            }
        }
#endif
        /* Try "name_fe" (strip off trailing digits) */
        p = &prop[sal_strlen(prop)-1];
        while (*p >= '0' && *p <= '9') {
          --p;
        }
        *++p = '\0';
        if ((s = soc_property_get_str(unit, prop)) != NULL) {
          return s;
        }
#if 0
        /* Try alternative name if available */
        if (SOC_PORT_NAME_ALTER_VALID(unit, port))
        {
            p = &prop_alter[sal_strlen(prop_alter)-1];
            while (*p >= '0' && *p <= '9') {
              --p;
            }
            *++p = '\0';
            if ((s = soc_property_get_str(unit, prop_alter)) != NULL) {
              return s;
            }
        }
#endif
    }

    /* Try "name.port%d" for explicit match first */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s.port%d", name, port+1) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "Unsupported soc_property length for %s.port%d. "
                      "Max soc property length:%d\n"), 
           name, port+1, SOC_PROPERTY_NAME_MAX));
        return NULL; 
    }
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return s;
    }
#if 0
    /*
     * Try "name_port1": search for matching port number.
     * In the case of the SOC_PORT macros, "port" is being
     * passed in as a ptype, not a variable.
     */
    for (pno = 0; pno < SOC_PORT_NUM(unit, port); pno++) {
        if (SOC_PORT(unit, port, pno) == port) {
            if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                             "%s_port%d", name, pno+1) >= SOC_PROPERTY_NAME_MAX) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Unsupported soc_property length for %s.port%d. "
                              "Max soc property length:%d\n"), 
                   name, pno+1, SOC_PROPERTY_NAME_MAX));
                return NULL; 
            }
            if ((s = soc_property_get_str(unit, prop)) != NULL) {
                return s;
            }            
            break;
        }
    }
#endif
    /* Try "name_%d" for raw logical port match */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s_%d", name, port) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "Unsupported soc_property length for %s_%d. "
                      "Max soc property length:%d\n"), 
           name, port, SOC_PROPERTY_NAME_MAX));
        return NULL; 
    }
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return s;
    }

    /* Try "name_logical_ports" for UM Vendor Config Compatiblility */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s_logical_ports", name) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "Unsupported soc_property length for %s_logical_ports. "
                      "Max soc property length:%d\n"), 
           name, SOC_PROPERTY_NAME_MAX));
        return NULL; 
    }
    extern char sal_config_pbmp_get(const char *name, pbmp_t *p);
    if (sal_config_pbmp_get(prop, &lpbmp) == 0) {       
           if (PBMP_MEMBER(lpbmp, port)) {
               if ((sal_strcmp(name, spn_PHY_AN_C37) == 0) || 
                   (sal_strcmp(name, spn_PHY_AN_C73) == 0)) 
               {
                    return "2";
               } else {
                    return "1";
               }
           } else {
               return "0";
           }
    }
    /* try plain "name" */
    if ((s = soc_property_get_str(unit, name)) != NULL) {
        return s;
    }

    return NULL;
}

/*
 * Function:
 *      soc_property_port_get
 * Purpose:
 *      Retrieve a per-port global configuration numeric value.
 * Parameters:
 *      unit - unit number
 *      port - Zero-based port number.
 *      name - base name of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      See soc_property_port_get_str for parsing details.
 */

uint32
soc_property_port_get(int unit, soc_port_t port,
                      const char *name, uint32 defl)
{
    char        *s;

    s = soc_property_port_get_str(unit, port, name);
    if (s) {
      return _shr_ctoi(s);
    }

    return defl;
}

/*
 * Function:
 *      soc_property_phy_get_str
 * Purpose:
 *      Retrieve a per-phy global configuration value
 * Parameters:
 *      unit - unit number
 *      pport - physical port number
 *      phy_num - PHY number in chain (1: innermost external PHY)
 *      phy_port - PHY port (0:line, 1:system)
 *      lane - PHY lane number on selected PHY port
 *      name - base name of numeric property to look up
 * Notes:
 *      The variable name is looked up up with a number of suffixes
 *      until a match is found:
 *              name{1.2}       physical port number + PHY number
 *              name{1}         physical port number (match any PHY number)
 *              name{1.2.0.1}   physical port number + PHY number/port/lane
 *      If no match is found, then the physical port number is
 *      converted to a logical port number and sent through
 *      soc_property_port_get_str for additional matching.
 */

char *
soc_property_phy_get_str(int unit, int pport,
                         int phy_num, int phy_port, int lane,
                         const char *name)
{
    char        *s;
    char        prop[SOC_PROPERTY_NAME_MAX];
    soc_port_t  port;
    int         len;

    if (phy_num >= 0) {
        /* Try "name{%d.%d.%d.%d}" for exact PHY lane match */
        len = sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, "%s{%d.%d.%d.%d}",
                           name, pport, phy_num, phy_port, lane);
        if (len >= SOC_PROPERTY_NAME_MAX) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "SOC property %s{%d.%d.%d.%d} too long. "
                                  "Max soc property length: %d\n"), 
                       name, pport, phy_num, phy_port, lane,
                       SOC_PROPERTY_NAME_MAX));
            return NULL; 
        }
        if ((s = soc_property_get_str(unit, prop)) != NULL) {
            return s;
        }
        /* Try "name{%d.%d.%d}" for exact PHY port match */
        len = sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, "%s{%d.%d.%d}",
                           name, pport, phy_num, phy_port);
        if (len >= SOC_PROPERTY_NAME_MAX) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "SOC property %s{%d.%d.%d} too long. "
                                  "Max soc property length: %d\n"), 
                       name, pport, phy_num, phy_port,
                       SOC_PROPERTY_NAME_MAX));
            return NULL; 
        }
        if ((s = soc_property_get_str(unit, prop)) != NULL) {
            return s;
        }
        /* Try "name{%d.%d}" for exact PHY match */
        len = sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, "%s{%d.%d}",
                           name, pport, phy_num);
        if (len >= SOC_PROPERTY_NAME_MAX) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "SOC property %s{%d.%d} too long. "
                                  "Max soc property length: %d\n"), 
                       name, pport, phy_num,
                       SOC_PROPERTY_NAME_MAX));
            return NULL; 
        }
        if ((s = soc_property_get_str(unit, prop)) != NULL) {
            return s;
        }
    }

    /* Try "name{%d}" for specific physical port match */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s{%d}", name, pport) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "SOC property %s{%d} too long. "
                      "Max soc property length: %d\n"), 
           name, pport, SOC_PROPERTY_NAME_MAX));
        return NULL; 
    }
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return s;
    }

    if (SOC_PORT_VALID_RANGE(unit, pport)) {
        port = pport;
        if (soc_feature(unit, soc_feature_logical_port_num)) {
            port = SOC_INFO(unit).port_p2l_mapping[pport];
        }
        return soc_property_port_get_str(unit, port, name);
    }

    return NULL;
}

/*
 * Function:
 *      soc_property_phy_get
 * Purpose:
 *      Retrieve a per-PHY global configuration numeric value.
 * Parameters:
 *      unit - unit number
 *      port - physical port number
 *      phy_num - PHY number in chain
 *      phy_port - PHY port
 *      lane - PHY lane number on selected PHY port
 *      name - base name of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      See soc_property_phy_get_str for parsing details.
 */

uint32
soc_property_phy_get(int unit, int port,
                     int phy_num, int phy_port, int lane,
                     const char *name, uint32 defl)
{
    char        *s;

    s = soc_property_phy_get_str(unit, port, phy_num, phy_port, lane, name);
    if (s) {
        return _shr_ctoi(s);
    }

    return defl;
}

/*
 * Function:
 *      soc_property_phys_port_get
 * Purpose:
 *      Retrieve a per-physical port global configuration numeric value.
 * Parameters:
 *      unit - unit number
 *      pport - physical port number
 *      name - base name of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      See soc_property_phy_get_str for parsing details.
 */

uint32
soc_property_phys_port_get(int unit, int pport,
                           const char *name, uint32 defl)
{
    char        *s;

    s = soc_property_phy_get_str(unit, pport, -1, -1, -1, name);
    if (s) {
        return _shr_ctoi(s);
    }

    return defl;
}

/*
 * Function:
 *      soc_property_port_obj_attr_get
 * Purpose:
 *      Retrieve a global configuration numeric value and its unit for the
 *      specified prefix, object and attribute.
 * Parameters:
 *      unit - unit number
 *      port - Zero-based port number.
 *      prefix - prefix name of the numeric property to look up
 *      obj - object name of the numeric property to look up
 *      index - object index of the numeric property to look up
 *      attr - attribute name of the numeric property to look up
 *      scale - scale up factor (see _str_to_val)
 *      suffix - the unit specified in the property (see _str_to_val)
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of object suffixes
 *      until it is found.
 *              prefix.obj1.attr    attribute of a particular object instance
 *              prefix.obj.attr     attribute of the object type
 *      If none are found then the default value in defl is returned.
 *      See soc_property_port_get_str for other suffix handling in property
 *      name.
 *      See _str_to_val for value string handling.
 *      The string is parsed as a C-style numeric constant.
 */

uint32
soc_property_port_obj_attr_get(int unit, soc_port_t port, const char *prefix,
                               const char *obj, int index, const char *attr,
                               int scale, char *suffix, uint32 defl)
{
    char        *str;
    int         val;
    char        prop[SOC_PROPERTY_NAME_MAX];

    str = NULL;

    if (index != -1) {
        if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                         "%s.%s%d.%s", prefix, obj, index, attr) >= SOC_PROPERTY_NAME_MAX) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "Unsupported soc_property length for %s.%s%d.%s. "
                          "Max soc property length:%d\n"), 
               prefix, obj, index, attr, SOC_PROPERTY_NAME_MAX));
            return defl; 

        }
        str = soc_property_port_get_str(unit, port, prop);
    }

    if (str == NULL) {
        if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                         "%s.%s.%s", prefix, obj, attr) >= SOC_PROPERTY_NAME_MAX) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "Unsupported soc_property length for %s.%s.%s. "
                          "Max soc property length:%d\n"), 
               prefix, obj, attr, SOC_PROPERTY_NAME_MAX));
            return defl; 

        }
        str = soc_property_port_get_str(unit, port, prop);
    }

    if (str == NULL) {
        return defl;
    }

    (void)_str_to_val(str, &val, scale, suffix);
    return (uint32)val;
}

int
soc_property_port_get_csv(int unit, soc_port_t port, const char *name,
                          int val_max, int *val_array)
{
    char *str, suffix;
    int count;

    str = soc_property_port_get_str(unit, port, name);
    if (str == NULL) {
        return 0;
    }

    count = 0;
    for (count = 0; count < val_max; count++) {
        str = _str_to_val(str, &val_array[count], 0, &suffix);
        if (suffix == ',') {
            str++;
        } else {
            count++;
            break;
        }
    }

    return count;
}

/*
 * Function:
 *      soc_property_cos_get
 * Purpose:
 *      Retrieve a per-port global configuration value
 * Parameters:
 *      unit - unit number
 *      cos - Zero-based cos number.
 *      name - base name of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name.cos1       cos number (one based)
 *              name            plain name
 *      If none are found then NULL is returned
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */

char *
soc_property_cos_get_str(int unit, soc_cos_t cos,
                          const char *name)
{
    char        *s;
    char        prop[SOC_PROPERTY_NAME_MAX];

    /* try "name.cos%d" for explicit match first */
    /* check length of <name>.cos<cos>\0 */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s.cos%d", name, (cos + 1)) >= SOC_PROPERTY_NAME_MAX) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "Unsupported soc_property length for %s.cos%d. "
                          "Max soc property length:%d\n"), 
               name, (cos + 1), SOC_PROPERTY_NAME_MAX));
            return NULL; 
    }
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return(s);
    }
    /* try plain "name" */
    if ((s = soc_property_get_str(unit, name)) != NULL) {
        return(s);
    }

    return(NULL);
}


/*
 * Function:
 *      soc_property_cos_get
 * Purpose:
 *      Retrieve a per-cos global configuration numeric value.
 * Parameters:
 *      unit - unit number
 *      cos - Zero-based cos number.
 *      name - base name of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_cos1       cos number (one based)
 *              name            plain name
 *      If none are found then the default value in defl is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */
uint32
soc_property_cos_get(int unit, soc_cos_t cos,
                                    const char *name, uint32 defl)
{
    char        *s;

    s = soc_property_cos_get_str(unit, cos, name);
    if (s) {
      return _shr_ctoi(s);
    }

    return defl;
}


/*
 * Function:
 *      soc_property_suffix_num_get
 * Purpose:
 *      Retrieve a per-enumerated suffix global configuration numeric value.
 * Parameters:
 *      unit - unit number
 *      num - enumerated suffix for which property is applicable
 *      name - base name of numeric property to look up
 *      suffix - suffix of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"0  enumerated suffix name
 *              name            plain name
 *      If none are found then the default value in defl is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */

uint32
soc_property_suffix_num_get(int unit, int num, const char *name,
                            const char *suffix, uint32 defl)
{
    char        *s;
    char        prop[SOC_PROPERTY_NAME_MAX];

    /* try "name_'suffix'0" */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s_%s%1d", name, suffix, num) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "Unsupported soc_property length for %s_%s%1d. "
                      "Max soc property length:%d\n"), 
           name, suffix, num, SOC_PROPERTY_NAME_MAX));
        return defl; 
    }
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return _shr_ctoi(s);
    }

    /* try "name.port%d.suffix" for explicit match first */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s.port%d.%s", name, num+1, suffix) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "Unsupported soc_property length for %s.port%d.%s. "
                      "Max soc property length:%d\n"),  
           name, num+1, suffix, SOC_PROPERTY_NAME_MAX));
        return defl; 
    }
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return _shr_ctoi(s);
    }

    /* try "name_'suffix'" */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s_%s", name, suffix) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "Unsupported soc_property length for %s_%s. "
                      "Max soc property length:%d\n"),  
           name, suffix, SOC_PROPERTY_NAME_MAX));
        return defl; 
    }
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return _shr_ctoi(s);
    }

    /* try plain "name" */
    if ((s = soc_property_get_str(unit, name)) != NULL) {
        return _shr_ctoi(s);
    }

    return defl;
}

/*
 * Function:
 *      soc_property_suffix_num_get_only_suffix
 * Purpose:
 *      Retrieve a per-enumerated suffix global configuration numeric value.
 * Parameters:
 *      unit - unit number
 *      num - enumerated suffix for which property is applicable
 *      name - base name of numeric property to look up
 *      suffix - suffix of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"0  enumerated suffix name
 *      If none are found then the default value in defl is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */

uint32
soc_property_suffix_num_get_only_suffix(int unit, int num, const char *name,
                            const char *suffix, uint32 defl)
{
    char        *s;
    char        prop[SOC_PROPERTY_NAME_MAX];

    /* try "name_'suffix'0" */
    /* check length of <name>_<suffix><num>\0 */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s_%s%1d", name, suffix, num) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "Unsupported soc_property length for %s_%s%1d. "
                      "Max soc property length:%d\n"), 
           name, suffix, num, SOC_PROPERTY_NAME_MAX));
        return defl; 
    }
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return _shr_ctoi(s);
    }

    /* try "name.port%d.suffix" for explicit match first */
    /* check length of <name>.port<num>.<suffix>\0 */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s.port%d.%s", name, num+1, suffix) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "Unsupported soc_property length for %s.port%d.%s. "
                      "Max soc property length:%d\n"), 
           name, num+1, suffix, SOC_PROPERTY_NAME_MAX));
        return defl; 
    }
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return _shr_ctoi(s);
    }

    /* try "name_'suffix'" */
    /* check length of <name>_<suffix>\0 */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s_%s", name, suffix) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "Unsupported soc_property length for %s_%s. "
                      "Max soc property length:%d\n"), 
           name, suffix, SOC_PROPERTY_NAME_MAX));
        return defl; 
    }
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return _shr_ctoi(s);
    }

    return defl;
}

/*
 * Function:
 *      soc_property_suffix_num_str_get
 * Purpose:
 *      Retrieve a per-enumerated suffix global configuration numeric value.
 * Parameters:
 *      unit - unit number
 *      num - enumerated suffix for which property is applicable
 *      name - base name of numeric property to look up
 *      suffix - suffix of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"0  enumerated suffix name
 *              name            plain name
 *      If none are found then the default value in defl is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */


char*
soc_property_suffix_num_str_get(int unit, int num, const char *name,
                            const char *suffix)
{
    char        *s;
    char        prop[SOC_PROPERTY_NAME_MAX];

    /* try "name_'suffix'0" */
    /* check length of <name>_<suffix>\0 */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s_%s%1d", name, suffix, num) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "Unsupported soc_property length for %s_%s%1d. "
                      "Max soc property length:%d\n"), 
           name, suffix, num, SOC_PROPERTY_NAME_MAX));
        return NULL; 
    }
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return s;
    }


    /* try "name.port%d.suffix" for explicit match first */
    /* check length of <name>.port<num>.<suffix>\0 */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s.port%d.%s", name, num+1, suffix) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "Unsupported soc_property length for %s.port%d.%s. "
                      "Max soc property length:%d\n"), 
           name, num+1, suffix, SOC_PROPERTY_NAME_MAX));
        return NULL; 
    }
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return s;
    }

    /* try "name_'suffix'" */
    /* check length of <name>_<suffix>\0 */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s_%s", name, suffix) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "Unsupported soc_property length for %s_%s. "
                      "Max soc property length:%d\n"), 
           name, suffix, SOC_PROPERTY_NAME_MAX));
        return NULL; 
    }
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return s;
    }

    /* try plain "name" */
    if ((s = soc_property_get_str(unit, name)) != NULL) {
        return s;
    }

    return NULL;
}


/*
 * Function:
 *      soc_property_suffix_num_only_suffix_str_get
 * Purpose:
 *      Retrieve a per-enumerated suffix global configuration numeric value.
 * Parameters:
 *      unit - unit number
 *      num - enumerated suffix for which property is applicable
 *      name - base name of numeric property to look up
 *      suffix - suffix of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"0  enumerated suffix name
 *              name            plain name
 *      If none are found then the default value in defl is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */


char*
soc_property_suffix_num_only_suffix_str_get(int unit, int num, const char *name,
                            const char *suffix)
{
    char        *s;
    char        prop[SOC_PROPERTY_NAME_MAX];

    /* try "name_'suffix'0" */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s_%s%1d", name, suffix, num) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "Unsupported soc_property length for %s_%s%1d. "
                      "Max soc property length:%d\n"), 
           name, suffix, num, SOC_PROPERTY_NAME_MAX));
        return NULL; 
    }
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return s;
    }

    /* try "name.port%d.suffix" for explicit match first */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s.port%d.%s", name, num+1, suffix) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "Unsupported soc_property length for %s.port%d.%s. "
                      "Max soc property length:%d\n"), 
           name, num+1, suffix, SOC_PROPERTY_NAME_MAX));
        return NULL; 
    }
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return s;
    }

    /* try "name_'suffix'" */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s_%s", name, suffix) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "Unsupported soc_property length for %s_%s. "
                      "Max soc property length:%d\n"), 
           name, suffix, SOC_PROPERTY_NAME_MAX));
        return NULL; 

    }
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return s;
    }

    return NULL;
}
/*
 * Function:
 *      soc_property_port_suffix_num_get
 * Purpose:
 *      Retrieve a per-port, per-enumerated suffix global
 *      configuration numeric value.
 * Parameters:
 *      unit - unit number
 *      port - Zero-based port number.
 *      num - enumerated suffix for which property is applicable
 *      name - base name of numeric property to look up
 *      suffix - suffix of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"0_port  enumerated suffix name per port
 *              name_"suffix"0       enumerated suffix name
 *              name_port            plain name per port
 *              name                 plain name
 *      If none are found then the default value in defl is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */

uint32
soc_property_port_suffix_num_get(int unit, soc_port_t port,
                                 int num, const char *name,
                                 const char *suffix, uint32 defl)
{
    char        *s;
    char        prop[SOC_PROPERTY_NAME_MAX];

    /* "name_'suffix'#" */
    /* check length of <name>_<suffix><num>\0 */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s_%s%1d", name, suffix, num) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "Unsupported soc_property length for %s_%s%1d. "
                      "Max soc property length:%d\n"), 
           name, suffix, num, SOC_PROPERTY_NAME_MAX));
        return defl; 
    }

    /* try "name_'suffix'#" per port*/
    if ((s = soc_property_port_get_str(unit, port, prop)) != NULL) {
        return _shr_ctoi(s);
    }

    /* try "name_'suffix'#" */
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return _shr_ctoi(s);
    }

    /* "name_'suffix'" */
    /* check length of <name>_<suffix>\0 */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s_%s", name, suffix) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "Unsupported soc_property length for %s_%s. "
                      "Max soc property length:%d\n"), 
           name, suffix, SOC_PROPERTY_NAME_MAX));
        return defl; 
    }
    /* try "name_'suffix'" per port */
    if ((s = soc_property_port_get_str(unit, port, prop)) != NULL) {
        return _shr_ctoi(s);
    }

    /* try "name_'suffix'"*/
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return _shr_ctoi(s);
    }


    /* try plain "name" per port*/
    if ((s = soc_property_port_get_str(unit, port, name)) != NULL) {
        return _shr_ctoi(s);
    }

    /* try plain "name" */
    if ((s = soc_property_get_str(unit, name)) != NULL) {
        return _shr_ctoi(s);
    }

    return defl;
}

/*
 * Function:
 *      soc_property_port_suffix_num_get_str
 * Purpose:
 *      Retrieve a per-port, per-enumerated suffix global
 *      configuration string value.
 * Parameters:
 *      unit - unit number
 *      port - Zero-based port number.
 *      num - enumerated suffix for which property is applicable
 *      name - base name of numeric property to look up
 *      suffix - suffix of numeric property to look up
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"0_port  enumerated suffix name per port
 *              name_"suffix"0       enumerated suffix name
 *              name_port            plain name per port
 *              name                 plain name
 *      If none are found then the NULL is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */

char*
soc_property_port_suffix_num_get_str(int unit, soc_port_t port,
                                 int num, const char *name,
                                 const char *suffix)
{
    char        *s;
    char        prop[SOC_PROPERTY_NAME_MAX];

    /* "name_'suffix'#" */ 
    /* check length of <name>_<suffix><num>\0 */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s_%s%1d", name, suffix, num) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "Unsupported soc_property length for %s_%s%1d. "
                      "Max soc property length:%d\n"), 
           name, suffix, num, SOC_PROPERTY_NAME_MAX));
        return NULL; 
    }
    /* try "name_'suffix'#" per port*/
    if ((s = soc_property_port_get_str(unit, port, prop)) != NULL) {
        return s;
    }

    /* try "name_'suffix'#" */
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return s;
    }


    /* "name_'suffix'" */
    /* check length of <name>_<suffix>\0 */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s_%s", name, suffix) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "Unsupported soc_property length for %s_%s. "
                      "Max soc property length:%d\n"), 
           name, suffix, SOC_PROPERTY_NAME_MAX));
        return NULL;
    }
    /* try "name_'suffix'" per port */
    if ((s = soc_property_port_get_str(unit, port, prop)) != NULL) {
        return s;
    }
    /* try "name_'suffix'"*/
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return s;
    }

    /* try plain "name" per port*/
    if ((s = soc_property_port_get_str(unit, port, name)) != NULL) {
        return s;
    }

    /* try plain "name" */
    if ((s = soc_property_get_str(unit, name)) != NULL) {
        return s;
    }

    return NULL;
}


/*
 * Function:
 *      soc_property_uc_get_str
 * Purpose:
 *      Retrieve a per-uc global configuration value
 * Parameters:
 *      unit - unit number
 *      uc - Microcontroller number. 0 is PCI Host.
 *      name - base name of numeric property to look up
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"   suffix is pci | uc0 | uc1 ...
 *              name            plain name
 *      If none are found then NULL is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */
char *
soc_property_uc_get_str(int unit, int uc,
                          const char *name)
{
    char        *s;
    char        prop[SOC_PROPERTY_NAME_MAX];

    /* try "name_uc%d" for explicit match first */
    if (0 == uc) {
        /* check length of <name>_pci\0 */
        if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX,  
                         "%s_pci", name) >= SOC_PROPERTY_NAME_MAX) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "Unsupported soc_property length for %s_pci. "
                          "Max soc property length:%d\n"), 
               name, SOC_PROPERTY_NAME_MAX));
            return NULL;

        }
        if ((s = soc_property_get_str(unit, prop)) != NULL) {
            return(s);
        }
    } else {

        /* check length of <name>_uc<uc>\0 */
        if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                         "%s_uc%d", name, (uc - 1)) >= SOC_PROPERTY_NAME_MAX) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "Unsupported soc_property length for %s_uc%d. "
                          "Max soc property length:%d\n"), 
               name, (uc - 1), SOC_PROPERTY_NAME_MAX));
            return NULL;
        }
        if ((s = soc_property_get_str(unit, prop)) != NULL) {
            return(s);
        }
    }    

    /* try plain "name" */
    if ((s = soc_property_get_str(unit, name)) != NULL) {
        return(s);
    }

    return(NULL);
}

/*
 * Function:
 *      soc_property_uc_get
 * Purpose:
 *      Retrieve a per-uc global configuration value
 * Parameters:
 *      unit - unit number
 *      uc - Microcontroller number. 0 is PCI Host.
 *      name - base name of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"   suffix is pci | uc0 | uc1 ...
 *              name            plain name
 *      If none are found then the default value in defl is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */
uint32
soc_property_uc_get(int unit, int uc,
                    const char *name, uint32 defl)
{
    char        *s;

    s = soc_property_uc_get_str(unit, uc, name);
    if (s) {
      return _shr_ctoi(s);
    }

    return defl;
}

/*
 * Function:
 *      soc_property_ci_get_str
 * Purpose:
 *      Retrieve a per-ci global configuration value
 * Parameters:
 *      unit - unit number
 *      ci - Chip Intergface Number.
 *      name - base name of numeric property to look up
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"   suffix is ci0 | ci1 ...
 *              name            plain name
 *      If none are found then NULL is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */
char *
soc_property_ci_get_str(int unit, int ci,
                          const char *name)
{
    char        *s;
    char        prop[SOC_PROPERTY_NAME_MAX];

    /* try "name_ci%d" for explicit match first */
    /* check length of <name>_ci<ci>\0 */
    if (sal_snprintf(prop, SOC_PROPERTY_NAME_MAX, 
                     "%s_ci%d", name, ci) >= SOC_PROPERTY_NAME_MAX) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
          (BSL_META_U(unit,
                      "Unsupported soc_property length for %s_ci%d. "
                      "Max soc property length:%d\n"), 
           name, ci, SOC_PROPERTY_NAME_MAX));
        return NULL;
    }
    if ((s = soc_property_get_str(unit, prop)) != NULL) {
        return(s);
    }
    /* try plain "name" */
    if ((s = soc_property_get_str(unit, name)) != NULL) {
        return(s);
    }

    return(NULL);
}

/*
 * Function:
 *      soc_property_ci_get
 * Purpose:
 *      Retrieve a per-ci global configuration value
 * Parameters:
 *      unit - unit number
 *      ci - Chip Intergface Number.
 *      name - base name of numeric property to look up
 *      defl - default value of numeric property
 * Notes:
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"   suffix is ci0 | ci1 ...
 *              name            plain name
 *      If none are found then the default value in defl is returned.
 *      See soc_property_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */
uint32
soc_property_ci_get(int unit, int ci,
                    const char *name, uint32 defl)
{
    char        *s;

    s = soc_property_ci_get_str(unit, ci, name);
    if (s) {
      return _shr_ctoi(s);
    }

    return defl;
}

/*
 * Function:
 *      soc_property_ci_get_csv
 * Purpose:
 *      Retrieve a per-ci global configuration comma-separated value in an array.
 * Parameters:
 *      unit - unit number
 *      ci - Chip Intergface Number.
 *      name - base name of numeric property to look up
 *      val_array - Integer array to put the values
 *      val_max - Maximum number of elements to put into the array
 * Notes:
 *      Returns the count of values read.
 *      The variable name is lookup up with a number of suffixes
 *      until it is found.
 *              name_"suffix"   suffix is ci0 | ci1 ...
 *              name            plain name
 *      If none are found then the default value in defl is returned.
 *      See soc_property_ci_get_str for additional suffix handling.
 *      The string is parsed as a C-style numeric constant.
 */
int
soc_property_ci_get_csv(int unit, int ci, const char *name,
                          int val_max, int *val_array)
{
    char *str, suffix;
    int count;

    str = soc_property_ci_get_str(unit, ci, name);
    if (str == NULL) {
        return 0;
    }

    count = 0;
    for (count = 0; count < val_max; count++) {
        str = _str_to_val(str, &val_array[count], 0, &suffix);
        if (suffix == ',') {
            str++;
        } else {
            count++;
            break;
        }
    }

    return count;
}

soc_block_name_t        soc_block_port_names[] =
        SOC_BLOCK_PORT_NAMES_INITIALIZER;
soc_block_name_t        soc_block_names[] =
        SOC_BLOCK_NAMES_INITIALIZER;

soc_block_name_t        soc_sbx_block_port_names[] =
        SOC_BLOCK_SBX_PORT_NAMES_INITIALIZER;
soc_block_name_t        soc_sbx_block_names[] =
        SOC_BLOCK_SBX_NAMES_INITIALIZER;

soc_block_name_t        soc_dpp_block_port_names[] =
        SOC_BLOCK_DPP_PORT_NAMES_INITIALIZER;
soc_block_name_t        soc_dpp_block_names[] =
        SOC_BLOCK_DPP_NAMES_INITIALIZER;

char *
soc_block_port_name_lookup_ext(soc_block_t blk, int unit)
{
    int         i;

#if defined(BCM_SIRIUS_SUPPORT)
    if (SOC_IS_SIRIUS(unit)) {
    for (i = 0; soc_sbx_block_port_names[i].blk != SOC_BLK_NONE; i++) {
        if (soc_sbx_block_port_names[i].blk == blk) {
        return soc_sbx_block_port_names[i].name;
        }
    }
    }
    else
#endif
    {
        for (i = 0; soc_block_port_names[i].blk != SOC_BLK_NONE; i++) {
            if (soc_block_port_names[i].blk == blk) {
                if (blk == SOC_BLK_GXPORT) {
                    if (SOC_IS_TD_TT(unit)) {
                        return "xlport";
                    }
                } else if (blk == SOC_BLK_BSAFE) {
                    if (SOC_IS_TD_TT(unit)) {
                        return "otpc";
                    }
                }
                return soc_block_port_names[i].name;
            }
        }
    }
    return "?";
}

char *
soc_block_name_lookup_ext(soc_block_t blk, int unit)
{
    int         i;

#if defined(BCM_SIRIUS_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
    if (SOC_IS_SIRIUS(unit) || SOC_IS_CALADAN3(unit)) {
    for (i = 0; soc_sbx_block_names[i].blk != SOC_BLK_NONE; i++) {
        if (soc_sbx_block_names[i].blk == blk) {
        return soc_sbx_block_names[i].name;
        }
    }
    }
    else
#endif
#if defined(BCM_DFE_SUPPORT) || defined(BCM_PETRA_SUPPORT)
    if (SOC_IS_DPP(unit) || SOC_IS_DFE(unit)) {
        for (i = 0; soc_dpp_block_names[i].blk != SOC_BLK_NONE; i++) {
            if (soc_dpp_block_names[i].blk == blk) {
                return soc_dpp_block_names[i].name;
            }
        }
    }
    else
#endif

    {
        for (i = 0; soc_block_names[i].blk != SOC_BLK_NONE; i++) {
            if (soc_block_names[i].blk == blk) {
                if (blk == SOC_BLK_GXPORT) {
                    if (SOC_IS_TD_TT(unit)) {
                        return "xlport";
                    }
                } else if (blk == SOC_BLK_BSAFE) {
                    if (SOC_IS_TD_TT(unit)) {
                        return "otpc";
                    }
                }
                return soc_block_names[i].name;
            }
        }
    }
    return "?";
}

soc_block_t
soc_block_port_name_match(char *name)
{
    int         i;

    
    for (i = 0; soc_block_port_names[i].blk != SOC_BLK_NONE; i++) {
        if (sal_strcmp(soc_block_port_names[i].name, name) == 0) {
            return soc_block_port_names[i].blk;
        }
    }
    return SOC_BLK_NONE;
}

soc_block_t
soc_block_name_match(char *name)
{
    int         i;

    
    for (i = 0; soc_block_names[i].blk != SOC_BLK_NONE; i++) {
        if (sal_strcmp(soc_block_names[i].name, name) == 0) {
            return soc_block_names[i].blk;
        }
    }
    return SOC_BLK_NONE;
}


