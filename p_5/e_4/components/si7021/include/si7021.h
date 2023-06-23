#ifndef __SI7021_H__
#define __SI7021_H__

// INCLUDES --------------------------------------------------------------------

#include <stdbool.h>    // bool
#include <stdint.h>     // uint8_t, uint16_t, uint64_t
#include "esp_err.h"    // esp_err_t
#include "si7021_i2c.h" // si7021_i2c_create_args_t, si7021_i2c_handle_t

// -----------------------------------------------------------------------------

// ENUMS -----------------------------------------------------------------------

/* Si7021 Resolution */
typedef enum {
    SI7021_RESOLUTION_RH12_TEMP14 = 0b00000000,
    SI7021_RESOLUTION_RH08_TEMP12 = 0b00000001,
    SI7021_RESOLUTION_RH10_TEMP13 = 0b10000000,
    SI7021_RESOLUTION_RH11_TEMP11 = 0b10000001,
} si7021_resolution_t;

/* Si7021 Heater State */
typedef enum {
    SI7021_HEATER_STATE_ENABLE  = 0b00000100,
    SI7021_HEATER_STATE_DISABLE = 0b00000000,
} si7021_heater_state_t;

/* Si7021 VDD Status */
typedef enum {
    SI7021_VDD_STATUS_OK  = 0b00000000,
    SI7021_VDD_STATUS_LOW = 0b01000000,
} si7021_vdd_status_t;

/* Si7021 Heater Level */
typedef enum {
    SI7021_HEATER_LEVEL_3_09_MA  = 0x00,
    SI7021_HEATER_LEVEL_9_18_MA  = 0x01,
    SI7021_HEATER_LEVEL_15_24_MA = 0x02,
    SI7021_HEATER_LEVEL_27_39_MA = 0x04,
    SI7021_HEATER_LEVEL_51_69_MA = 0x08,
    SI7021_HEATER_LEVEL_94_20_MA = 0x0F,
} si7021_heater_level_t;

/* Si7021 Serial Number ID */
typedef enum {
    SI7021_SN_ID_DEV_00 = 0x00,
    SI7021_SN_ID_SI7013 = 0x0D,
    SI7021_SN_ID_SI7020 = 0x14,
    SI7021_SN_ID_SI7021 = 0x15,
    SI7021_SN_ID_DEV_FF = 0xFF,
} si7021_sn_id_t;

/* Si7021 Firmware Version */
typedef enum {
    SI7021_FW_VERSION_1_0 = 0xFF,
    SI7021_FW_VERSION_2_0 = 0x20,
} si7021_fw_version_t;

// -----------------------------------------------------------------------------

// STRUCTURES ------------------------------------------------------------------

/* Si7021 Handle */
typedef struct si7021_handle *si7021_handle_t;

/* Si7021 CRC Config */
typedef struct {
    bool global; // For all commands with CRC support

    /* If false, global is used */
    bool rh;     // For RH measurements
    bool temp;   // For temperature measurements
    bool sna;    // For first bytes of serial number [63:32]
    bool snb;    // For last bytes of serial number [31:0]
} si7021_crc_config_t;

/* Si7021 Read Wait */
typedef struct {
    TickType_t global;     // For all commands reading data from the sensor

    /* If 0, global is used */
    TickType_t rh;         // For RH measurements
    TickType_t temp;       // For temperature measurements
    TickType_t user_reg;   // For reading the user register
    TickType_t heater_reg; // For reading the heater register
    TickType_t sna;        // For reading the serial number [63:32]
    TickType_t snb;        // For reading the serial number [31:0]
    TickType_t fw;         // For reading the firmware version
} si7021_read_wait_t;

/* Si7021 User Register Info */
typedef struct {
    si7021_resolution_t   resolution;
    si7021_heater_state_t heater_state;
    si7021_vdd_status_t   vdd_status;
} si7021_user_register_info_t;

/* Si7021 Create Args */
typedef struct {
    const char               *name;       // Handle name
    si7021_i2c_create_args_t  i2c;        // Si7021 I2C create args
    si7021_crc_config_t       crc_config; // CRC config
    si7021_read_wait_t        read_wait;  // Read wait timeouts

    struct {
        bool reset; // Issue a reset command at init
        
        // Set the heater level at init
        bool    set_heater_level;
        uint8_t heater_level;

        // Set the user register info at init
        bool                        set_user_register_info;
        si7021_user_register_info_t user_register_info;

        struct {
            bool device_info; // Dump device info at init
            bool crc_config;  // Dump CRC config at init
            bool read_wait;   // Dump read wait timeouts at init
        } dump;
    } at_init;
} si7021_create_args_t;

// -----------------------------------------------------------------------------

// DEFINITIONS -----------------------------------------------------------------

#define SI7021_HANDLE_NAME_LENGTH 16

/* User Register */
#define SI7021_USER_REGISTER_DEFAULT           0b00111010
#define SI7021_USER_REGISTER_MASK_RESOLUTION   0b10000001
#define SI7021_USER_REGISTER_MASK_VDD_STATUS   0b01000000
#define SI7021_USER_REGISTER_MASK_HEATER_STATE 0b00000100

/* Heater Register */
#define SI7021_HEATER_REGISTER_DEFAULT           0b00000000
#define SI7021_HEATER_REGISTER_MASK_HEATER_LEVEL 0b00001111

//// KCONFIG -------------------------------------------------------------------

////// Si7021 Default Create Args ----------------------------------------------

/* Handle name */
#define SI7021_DEFAULT_NAME                                     /* "si7021" */ \
        CONFIG_SI7021_DEFAULT_NAME

/* CRC Configuration */
#define SI7021_DEFAULT_CRC_CONFIG_GLOBAL                        /* (true) 1 */ \
        (bool)CONFIG_SI7021_DEFAULT_CRC_CONFIG_GLOBAL
#define SI7021_DEFAULT_CRC_CONFIG_RH                           /* (false) 0 */ \
        (bool)CONFIG_SI7021_DEFAULT_CRC_CONFIG_RH
#define SI7021_DEFAULT_CRC_CONFIG_TEMP                         /* (false) 0 */ \
        (bool)CONFIG_SI7021_DEFAULT_CRC_CONFIG_TEMP
#define SI7021_DEFAULT_CRC_CONFIG_SNA                          /* (false) 0 */ \
        (bool)CONFIG_SI7021_DEFAULT_CRC_CONFIG_SNA
#define SI7021_DEFAULT_CRC_CONFIG_SNB                          /* (false) 0 */ \
        (bool)CONFIG_SI7021_DEFAULT_CRC_CONFIG_SNB

/* Read Wait Timeouts (ms) */
#define SI7021_DEFAULT_READ_WAIT_MS_GLOBAL                             /* 0 */ \
        CONFIG_SI7021_DEFAULT_READ_WAIT_MS_GLOBAL
#define SI7021_DEFAULT_READ_WAIT_MS_RH                                /* 30 */ \
        CONFIG_SI7021_DEFAULT_READ_WAIT_MS_RH
#define SI7021_DEFAULT_READ_WAIT_MS_TEMP                              /* 20 */ \
        CONFIG_SI7021_DEFAULT_READ_WAIT_MS_TEMP
#define SI7021_DEFAULT_READ_WAIT_MS_USER_REG                          /* 10 */ \
        CONFIG_SI7021_DEFAULT_READ_WAIT_MS_USER_REG
#define SI7021_DEFAULT_READ_WAIT_MS_HEATER_REG                        /* 10 */ \
        CONFIG_SI7021_DEFAULT_READ_WAIT_MS_HEATER_REG
#define SI7021_DEFAULT_READ_WAIT_MS_SNA                               /* 10 */ \
        CONFIG_SI7021_DEFAULT_READ_WAIT_MS_SNA
#define SI7021_DEFAULT_READ_WAIT_MS_SNB                               /* 10 */ \
        CONFIG_SI7021_DEFAULT_READ_WAIT_MS_SNB
#define SI7021_DEFAULT_READ_WAIT_MS_FW                                /* 10 */ \
        CONFIG_SI7021_DEFAULT_READ_WAIT_MS_FW

/* At Init Options */
#define SI7021_DEFAULT_AT_INIT_RESET                            /* (true) 1 */ \
        (bool)CONFIG_SI7021_DEFAULT_AT_INIT_RESET

#define SI7021_DEFAULT_AT_INIT_SET_HEATER_LEVEL                /* (false) 0 */ \
        (bool)CONFIG_SI7021_DEFAULT_AT_INIT_SET_HEATER_LEVEL
#define SI7021_DEFAULT_AT_INIT_HEATER_LEVEL                         /* 0x00 */ \
        CONFIG_SI7021_DEFAULT_AT_INIT_HEATER_LEVEL

#define SI7021_DEFAULT_AT_INIT_SET_USER_REGISTER_INFO          /* (false) 0 */ \
        (bool)CONFIG_SI7021_DEFAULT_AT_INIT_SET_USER_REGISTER_INFO
#define SI7021_DEFAULT_AT_INIT_RESOLUTION                                      \
                                    /* (SI7021_RESOLUTION_RH12_TEMP14) 0x00 */ \
        (si7021_resolution_t)CONFIG_SI7021_DEFAULT_AT_INIT_RESOLUTION
#define SI7021_DEFAULT_AT_INIT_HEATER_STATE                                    \
                                      /* (SI7021_HEATER_STATE_DISABLE) 0x00 */ \
        (si7021_heater_state_t)CONFIG_SI7021_DEFAULT_AT_INIT_HEATER_STATE

/* At Init Dumps */
#define SI7021_DEFAULT_AT_INIT_DUMP_DEVICE_INFO                 /* (true) 1 */ \
        (bool)CONFIG_SI7021_DEFAULT_AT_INIT_DUMP_DEVICE_INFO
#define SI7021_DEFAULT_AT_INIT_DUMP_CRC_CONFIG                 /* (false) 0 */ \
        (bool)CONFIG_SI7021_DEFAULT_AT_INIT_DUMP_CRC_CONFIG
#define SI7021_DEFAULT_AT_INIT_DUMP_READ_WAIT                  /* (false) 0 */ \
        (bool)CONFIG_SI7021_DEFAULT_AT_INIT_DUMP_READ_WAIT

////// -------------------------------------------------------------------------

//// ---------------------------------------------------------------------------

// -----------------------------------------------------------------------------

// MACROS ----------------------------------------------------------------------

//// For Si7021 Create Args ----------------------------------------------------

/* Si7021 Default Create Args */
#define SI7021_DEFAULT_CREATE_ARGS() {                                         \
    .name = SI7021_DEFAULT_NAME,                                               \
    .i2c  = SI7021_I2C_DEFAULT_CREATE_ARGS(),                                  \
    .crc_config = {                                                            \
        .global = SI7021_DEFAULT_CRC_CONFIG_GLOBAL,                            \
        .rh     = SI7021_DEFAULT_CRC_CONFIG_RH,                                \
        .temp   = SI7021_DEFAULT_CRC_CONFIG_TEMP,                              \
        .sna    = SI7021_DEFAULT_CRC_CONFIG_SNA,                               \
        .snb    = SI7021_DEFAULT_CRC_CONFIG_SNB                                \
    },                                                                         \
    .read_wait = {                                                             \
        .global     = SI7021_DEFAULT_READ_WAIT_MS_GLOBAL / portTICK_PERIOD_MS, \
        .rh         = SI7021_DEFAULT_READ_WAIT_MS_RH / portTICK_PERIOD_MS,     \
        .temp       = SI7021_DEFAULT_READ_WAIT_MS_TEMP / portTICK_PERIOD_MS,   \
        .user_reg   = SI7021_DEFAULT_READ_WAIT_MS_USER_REG                     \
            / portTICK_PERIOD_MS,                                              \
        .heater_reg = SI7021_DEFAULT_READ_WAIT_MS_HEATER_REG                   \
            / portTICK_PERIOD_MS,                                              \
        .sna        = SI7021_DEFAULT_READ_WAIT_MS_SNA / portTICK_PERIOD_MS,    \
        .snb        = SI7021_DEFAULT_READ_WAIT_MS_SNB / portTICK_PERIOD_MS,    \
        .fw         = SI7021_DEFAULT_READ_WAIT_MS_FW / portTICK_PERIOD_MS      \
    },                                                                         \
    .at_init = {                                                               \
        .reset = SI7021_DEFAULT_AT_INIT_RESET,                                 \
                                                                               \
        .set_heater_level = SI7021_DEFAULT_AT_INIT_SET_HEATER_LEVEL,           \
        .heater_level     = SI7021_DEFAULT_AT_INIT_HEATER_LEVEL,               \
                                                                               \
        .set_user_register_info =                                              \
            SI7021_DEFAULT_AT_INIT_SET_USER_REGISTER_INFO,                     \
        .user_register_info = {                                                \
            .resolution   = SI7021_DEFAULT_AT_INIT_RESOLUTION,                 \
            .heater_state = SI7021_DEFAULT_AT_INIT_HEATER_STATE,               \
            .vdd_status   = SI7021_VDD_STATUS_OK                               \
        },                                                                     \
                                                                               \
        .dump = {                                                              \
            .device_info = SI7021_DEFAULT_AT_INIT_DUMP_DEVICE_INFO,            \
            .crc_config  = SI7021_DEFAULT_AT_INIT_DUMP_CRC_CONFIG,             \
            .read_wait   = SI7021_DEFAULT_AT_INIT_DUMP_READ_WAIT               \
        }                                                                      \
    }                                                                          \
}

/* Set All CRC flags to <true|false> */
#define SI7021_CREATE_ARGS_CRC_CONFIG_SET_ALL(config, value) {                 \
    config.global = value;                                                     \
    config.rh     = value;                                                     \
    config.temp   = value;                                                     \
    config.sna    = value;                                                     \
    config.snb    = value;                                                     \
}

/* Set All Read Wait Timeouts to <value> */
#define SI7021_CREATE_ARGS_READ_WAIT_SET_ALL(config, value) {                  \
    config.global     = value;                                                 \
    config.rh         = value;                                                 \
    config.temp       = value;                                                 \
    config.user_reg   = value;                                                 \
    config.heater_reg = value;                                                 \
    config.sna        = value;                                                 \
    config.snb        = value;                                                 \
    config.fw         = value;                                                 \
}

//// ---------------------------------------------------------------------------

//// Enum to String Macros -----------------------------------------------------

/* Si7021 Resolution to String */
#define SI7021_RESOLUTION_TO_STRING(resolution) (                              \
    (resolution == SI7021_RESOLUTION_RH12_TEMP14) ?                            \
        "RH: 12 bit | Temp: 14 bit" :                                          \
    (resolution == SI7021_RESOLUTION_RH08_TEMP12) ?                            \
        "RH:  8 bit | Temp: 12 bit" :                                          \
    (resolution == SI7021_RESOLUTION_RH10_TEMP13) ?                            \
        "RH: 10 bit | Temp: 13 bit" :                                          \
    (resolution == SI7021_RESOLUTION_RH11_TEMP11) ?                            \
        "RH: 11 bit | Temp: 11 bit" :                                          \
    "Unknown"                                                                  \
)

/* Si7021 Heater State to String */
#define SI7021_HEATER_STATE_TO_STRING(heater) (                                \
    (heater == SI7021_HEATER_STATE_ENABLE)  ? "Enabled" :                      \
    (heater == SI7021_HEATER_STATE_DISABLE) ? "Disabled" :                     \
    "Unknown"                                                                  \
)

/* Si7021 VDD Status to String */
#define SI7021_VDD_STATUS_TO_STRING(vdd_status) (                              \
    (vdd_status == SI7021_VDD_STATUS_OK)  ? "OK" :                             \
    (vdd_status == SI7021_VDD_STATUS_LOW) ? "Low" :                            \
    "Unknown"                                                                  \
)

/* Si7021 Heater Level to String */
#define SI7021_HEATER_LEVEL_TO_STRING(heater_level) (                          \
    (heater_level == SI7021_HEATER_LEVEL_3_09_MA)  ? "3.09 mA"  :              \
    (heater_level == SI7021_HEATER_LEVEL_9_18_MA)  ? "9.18 mA"  :              \
    (heater_level == SI7021_HEATER_LEVEL_15_24_MA) ? "15.24 mA" :              \
    (heater_level == SI7021_HEATER_LEVEL_27_39_MA) ? "27.39 mA" :              \
    (heater_level == SI7021_HEATER_LEVEL_51_69_MA) ? "51.69 mA" :              \
    (heater_level == SI7021_HEATER_LEVEL_94_20_MA) ? "94.20 mA" :              \
    "Not defined (use si7021_calc_heater_level() for an approximation)"        \
)

/* Si7021 SN ID to String */
#define SI7021_SN_ID_TO_STRING(sn_id) (                                        \
    (sn_id == SI7021_SN_ID_DEV_00) ||                                          \
    (sn_id == SI7021_SN_ID_DEV_FF) ? "Engineering Sample" :                    \
    (sn_id == SI7021_SN_ID_SI7013) ? "Si7013" :                                \
    (sn_id == SI7021_SN_ID_SI7020) ? "Si7020" :                                \
    (sn_id == SI7021_SN_ID_SI7021) ? "Si7021" :                                \
    "Unknown"                                                                  \
)

/* Si7021 FW Version to String */
#define SI7021_FW_VERSION_TO_STRING(fw_version) (                              \
    (fw_version == SI7021_FW_VERSION_1_0) ? "1.0" :                            \
    (fw_version == SI7021_FW_VERSION_2_0) ? "2.0" :                            \
    "Unknown"                                                                  \
)

//// ---------------------------------------------------------------------------

// -----------------------------------------------------------------------------

// FUNCTIONS -------------------------------------------------------------------

/* Conversion */
float si7021_convert_rh(
    uint16_t rh_code );

float si7021_convert_temp(
    uint16_t temp_code );

float si7021_calc_heater_level(
    uint8_t heater_level );

/* Create & Delete */
esp_err_t si7021_create(
    si7021_create_args_t const *create_args ,
    si7021_handle_t            *out_handle );

esp_err_t si7021_delete(
    si7021_handle_t si7021 );

/* CRC Config */
esp_err_t si7021_get_crc_config(
    si7021_handle_t     const si7021           ,
    si7021_crc_config_t       *out_crc_config );

esp_err_t si7021_set_crc_config(
    si7021_handle_t     const  si7021      ,
    si7021_crc_config_t const *crc_config );

/* Read Wait Timeouts */
esp_err_t si7021_get_read_wait(
    si7021_handle_t    const  si7021         ,
    si7021_read_wait_t       *out_read_wait );

esp_err_t si7021_set_read_wait(
    si7021_handle_t    const  si7021     ,
    si7021_read_wait_t const *read_wait );

/* Measurement */
esp_err_t si7021_measure_rh(
    si7021_handle_t const  si7021          ,
    float                 *out_rh_percent );

esp_err_t si7021_measure_temp(
    si7021_handle_t const  si7021            ,
    float                 *out_temp_celsius );

/* Internally, this function only needs to call si7021_measure_rh, as issuing
 * a 'Measure RH' command automatically takes a temperature measurement too,
 * whose value can be retrieved with a specific command without issuing a
 * 'Measure Temp' command again.
 */
esp_err_t si7021_measure_rh_and_temp(
    si7021_handle_t const  si7021            ,
    float                 *out_rh_percent    ,
    float                 *out_temp_celsius );

/* Reset */
esp_err_t si7021_reset(
    si7021_handle_t const si7021 );

/* User Register Control */
esp_err_t si7021_get_user_register_info(
    si7021_handle_t             const  si7021                  ,
    uint8_t                           *out_user_register       ,
    si7021_user_register_info_t       *out_user_register_info );

esp_err_t si7021_set_user_register_info(
    si7021_handle_t             const  si7021              ,
    si7021_user_register_info_t const *user_register_info );

/* User Register Control (derived)*/
/* Internally, these functions use si7021_get_user_register_info and
 * si7021_get_user_register_info, as these two functions read and write over
 * the same Si7021 register. As such, if the user wishes to check or modify both
 * resolution and heater state, it is more efficient to use the previous two
 * functions.
 */
esp_err_t si7021_get_resolution(
    si7021_handle_t     const  si7021          ,
    si7021_resolution_t       *out_resolution );

esp_err_t si7021_get_heater_state(
    si7021_handle_t       const  si7021            ,
    si7021_heater_state_t       *out_heater_state );

esp_err_t si7021_get_vdd_status(
    si7021_handle_t     const  si7021          ,
    si7021_vdd_status_t       *out_vdd_status );

esp_err_t si7021_set_resolution(
    si7021_handle_t     const si7021      ,
    si7021_resolution_t       resolution );

esp_err_t si7021_set_heater_state(
    si7021_handle_t       const si7021        ,
    si7021_heater_state_t       heater_state );

/* Heater Register Control */
esp_err_t si7021_get_heater_level(
    si7021_handle_t       const  si7021            ,
    si7021_heater_level_t       *out_heater_level );

esp_err_t si7021_set_heater_level(
    si7021_handle_t const si7021        ,
    uint8_t               heater_level );

/* Serial Number & Firmware */
esp_err_t si7021_get_serial_number_info(
    si7021_handle_t const  si7021            ,
    uint64_t              *out_serial_number ,
    si7021_sn_id_t        *out_sn_id        );

esp_err_t si7021_get_firmware_revision_info(
    si7021_handle_t     const  si7021            ,
    si7021_fw_version_t       *out_fw_version   );

/* Dump */

esp_err_t si7021_dump_device_info(
    si7021_handle_t const si7021 );

esp_err_t si7021_dump_crc_config(
    si7021_handle_t const si7021 );

esp_err_t si7021_dump_read_wait(
    si7021_handle_t const si7021 );

// -----------------------------------------------------------------------------

#endif // __SI7021_H__