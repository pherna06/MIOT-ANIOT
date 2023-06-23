#ifndef __SI7021_H__
#define __SI7021_H__1

// INCLUDES //
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c.h"
#include "si7021_i2c.h" // For si7021_i2c_create_args_t


// DEFINITIONS AND MACROS //
#define SI7021_DEFAULT_NAME          /* "si7021" */ CONFIG_SI7021_DEFAULT_NAME

#define SI7021_DEFAULT_CRC_CONFIG_GLOBAL /* true */  CONFIG_SI7021_DEFAULT_CRC_CONFIG_GLOBAL
#define SI7021_DEFAULT_CRC_CONFIG_RH     /* false */ CONFIG_SI7021_DEFAULT_CRC_CONFIG_RH
#define SI7021_DEFAULT_CRC_CONFIG_TEMP   /* false */ CONFIG_SI7021_DEFAULT_CRC_CONFIG_TEMP
#define SI7021_DEFAULT_CRC_CONFIG_SNA    /* false */ CONFIG_SI7021_DEFAULT_CRC_CONFIG_SNA
#define SI7021_DEFAULT_CRC_CONFIG_SNB    /* false */ CONFIG_SI7021_DEFAULT_CRC_CONFIG_SNB

#define SI7021_DEFAULT_READ_WAIT_MS_GLOBAL     /* 0 */  CONFIG_SI7021_DEFAULT_READ_WAIT_MS_GLOBAL
#define SI7021_DEFAULT_READ_WAIT_MS_RH         /* 30 */ CONFIG_SI7021_DEFAULT_READ_WAIT_MS_RH
#define SI7021_DEFAULT_READ_WAIT_MS_TEMP       /* 20 */ CONFIG_SI7021_DEFAULT_READ_WAIT_MS_TEMP
#define SI7021_DEFAULT_READ_WAIT_MS_USER_REG   /* 10 */ CONFIG_SI7021_DEFAULT_READ_WAIT_MS_USER_REG
#define SI7021_DEFAULT_READ_WAIT_MS_HEATER_REG /* 10 */ CONFIG_SI7021_DEFAULT_READ_WAIT_MS_HEATER_REG
#define SI7021_DEFAULT_READ_WAIT_MS_SNA        /* 10 */ CONFIG_SI7021_DEFAULT_READ_WAIT_MS_SNA
#define SI7021_DEFAULT_READ_WAIT_MS_SNB        /* 10 */ CONFIG_SI7021_DEFAULT_READ_WAIT_MS_SNB
#define SI7021_DEFAULT_READ_WAIT_MS_FW         /* 10 */ CONFIG_SI7021_DEFAULT_READ_WAIT_MS_FW

#define SI7021_DEFAULT_AT_INIT_RESET              /* true */  CONFIG_SI7021_DEFAULT_AT_INIT_RESET

#define SI7021_DEFAULT_AT_INIT_SET_HEATER_LEVEL   /* false */ CONFIG_SI7021_DEFAULT_AT_INIT_SET_HEATER_LEVEL
#define SI7021_DEFAULT_AT_INIT_HEATER_LEVEL       /* 0x00 */  CONFIG_SI7021_DEFAULT_AT_INIT_HEATER_LEVEL

#define SI7021_DEFAULT_AT_INIT_SET_USER_REGISTER_INFO   /* false */                         CONFIG_SI7021_DEFAULT_AT_INIT_SET_USER_REGISTER_INFO
#define SI7021_DEFAULT_AT_INIT_RESOLUTION               /* SI7021_RESOLUTION_RH12_TEMP14 */ CONFIG_SI7021_DEFAULT_AT_INIT_RESOLUTION
#define SI7021_DEFAULT_AT_INIT_HEATER_STATE             /* SI7021_HEATER_STATE_DISABLE */   CONFIG_SI7021_DEFAULT_AT_INIT_HEATER_STATE

#define SI7021_DEFAULT_AT_INIT_DUMP_DEVICE_INFO   /* true */  CONFIG_SI7021_DEFAULT_AT_INIT_DUMP_DEVICE_INFO
#define SI7021_DEFAULT_AT_INIT_DUMP_CRC_CONFIG    /* false */ CONFIG_SI7021_DEFAULT_AT_INIT_DUMP_CRC_CONFIG
#define SI7021_DEFAULT_AT_INIT_DUMP_READ_WAIT     /* false */ CONFIG_SI7021_DEFAULT_AT_INIT_DUMP_READ_WAIT

// Si7021 Default Create Args Macro //
#define SI7021_DEFAULT_CREATE_ARGS() { \
    .name = SI7021_DEFAULT_NAME, \
    .i2c  = SI7021_I2C_DEFAULT_CREATE_ARGS(), \
    .crc_config = { \
        .global = SI7021_DEFAULT_CRC_CONFIG_GLOBAL, \
        .rh     = SI7021_DEFAULT_CRC_CONFIG_RH, \
        .temp   = SI7021_DEFAULT_CRC_CONFIG_TEMP, \
        .sna    = SI7021_DEFAULT_CRC_CONFIG_SNA, \
        .snb    = SI7021_DEFAULT_CRC_CONFIG_SNB \
    }, \
    .read_wait = { \
        .global     = SI7021_DEFAULT_READ_WAIT_MS_GLOBAL / portTICK_PERIOD_MS, \
        .rh         = SI7021_DEFAULT_READ_WAIT_MS_RH / portTICK_PERIOD_MS, \
        .temp       = SI7021_DEFAULT_READ_WAIT_MS_TEMP / portTICK_PERIOD_MS, \
        .user_reg   = SI7021_DEFAULT_READ_WAIT_MS_USER_REG / portTICK_PERIOD_MS, \
        .heater_reg = SI7021_DEFAULT_READ_WAIT_MS_HEATER_REG / portTICK_PERIOD_MS, \
        .sna        = SI7021_DEFAULT_READ_WAIT_MS_SNA / portTICK_PERIOD_MS, \
        .snb        = SI7021_DEFAULT_READ_WAIT_MS_SNB / portTICK_PERIOD_MS, \
        .fw         = SI7021_DEFAULT_READ_WAIT_MS_FW / portTICK_PERIOD_MS \
    }, \
    .at_init = { \
        .reset = SI7021_DEFAULT_AT_INIT_RESET, \
        .set_heater_level = SI7021_DEFAULT_AT_INIT_SET_HEATER_LEVEL, \
        .heater_level = SI7021_DEFAULT_AT_INIT_HEATER_LEVEL, \
        .set_user_register_info = SI7021_DEFAULT_AT_INIT_SET_USER_REGISTER_INFO, \
        .user_register_info = { \
            .resolution   = SI7021_DEFAULT_AT_INIT_RESOLUTION, \
            .heater_state = SI7021_DEFAULT_AT_INIT_HEATER_STATE, \
            .vdd_status   = SI7021_VDD_STATUS_OK \
        }, \
        .dump = { \
            .device_info = SI7021_DEFAULT_AT_INIT_DUMP_DEVICE_INFO, \
            .crc_config  = SI7021_DEFAULT_AT_INIT_DUMP_CRC_CONFIG, \
            .read_wait   = SI7021_DEFAULT_AT_INIT_DUMP_READ_WAIT \
        } \
    } \
}

// Si7021 CRC Config: Set all to //
#define SI7021_CRC_CONFIG_ALL(value) (si7021_crc_config_t) { \
    value /* global */, \
    value /* rh */, \
    value /* temp */, \
    value /* sna */, \
    value /* snb */ \
}


// Si7021 Read Wait: Set all to //
#define SI7021_READ_WAIT_ALL(value) (si7021_read_wait_t) { \
    value /* global */, \
    value /* rh */, \
    value /* temp */, \
    value /* user_reg */, \
    value /* heater_reg */, \
    value /* sna */, \
    value /* snb */, \
    value /* fw */ \
}


// Si7021 Resolution to String Macro //
#define SI7021_RESOLUTION_TO_STRING(resolution) ( \
    (resolution == SI7021_RESOLUTION_RH12_TEMP14) ? "RH: 12 bit | Temp: 14 bit" : \
    (resolution == SI7021_RESOLUTION_RH08_TEMP12) ? "RH:  8 bit | Temp: 12 bit" : \
    (resolution == SI7021_RESOLUTION_RH10_TEMP13) ? "RH: 10 bit | Temp: 13 bit" : \
    (resolution == SI7021_RESOLUTION_RH11_TEMP11) ? "RH: 11 bit | Temp: 11 bit" : \
    "Unknown" \
)

// Si7021 Heater to String Macro //
#define SI7021_HEATER_STATE_TO_STRING(heater) ( \
    (heater == SI7021_HEATER_STATE_ENABLE)  ? "Enabled" : \
    (heater == SI7021_HEATER_STATE_DISABLE) ? "Disabled" : \
    "Unknown" \
)

// Si7021 VDD Status to String Macro //
#define SI7021_VDD_STATUS_TO_STRING(vdd_status) ( \
    (vdd_status == SI7021_VDD_STATUS_OK)  ? "OK" : \
    (vdd_status == SI7021_VDD_STATUS_LOW) ? "Low" : \
    "Unknown" \
)

// Si7021 Heater Level to String Macro //
#define SI7021_HEATER_LEVEL_TO_STRING(heater_level) ( \
    (heater_level == SI7021_HEATER_LEVEL_3_09_MA)  ? "3.09 mA"  : \
    (heater_level == SI7021_HEATER_LEVEL_9_18_MA)  ? "9.18 mA"  : \
    (heater_level == SI7021_HEATER_LEVEL_15_24_MA) ? "15.24 mA" : \
    (heater_level == SI7021_HEATER_LEVEL_27_39_MA) ? "27.39 mA" : \
    (heater_level == SI7021_HEATER_LEVEL_51_69_MA) ? "51.69 mA" : \
    (heater_level == SI7021_HEATER_LEVEL_94_20_MA) ? "94.20 mA" : \
    "Not defined (use si7021_calc_heater_level for an approximation)" \
)

// Si7021 SN ID to String Macro //
#define SI7021_SN_ID_TO_STRING(sn_id) ( \
    (sn_id == SI7021_SN_ID_DEV_00) || \
    (sn_id == SI7021_SN_ID_DEV_FF) ? "Engineering Sample" : \
    (sn_id == SI7021_SN_ID_SI7013) ? "Si7013" : \
    (sn_id == SI7021_SN_ID_SI7020) ? "Si7020" : \
    (sn_id == SI7021_SN_ID_SI7021) ? "Si7021" : \
    "Unknown" \
)

// Si7021 FW Version to String Macro //
#define SI7021_FW_VERSION_TO_STRING(fw_version) ( \
    (fw_version == SI7021_FW_VERSION_1_0) ? "1.0" : \
    (fw_version == SI7021_FW_VERSION_2_0) ? "2.0" : \
    "Unknown" \
)




// TYPEDEFS //

// Si7021 Handle //
typedef struct si7021_handle *si7021_handle_t;

// Si7021 CRC Config //
typedef struct {
    bool global; // For all commands with CRC support

    /* If false, global is used */
    bool rh;     // For RH measurements
    bool temp;   // For temperature measurements (not supported in si7021_measure_rh_and_temp)
    bool sna;    // For first bytes of serial number [63:32]
    bool snb;    // For last bytes of serial number [31:0]
} si7021_crc_config_t;

// Si7021 Read Wait //
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

// Si7021 User Register Info //

// > Si7021 Resolution
typedef enum {
    SI7021_RESOLUTION_RH12_TEMP14 = 0b00000000,
    SI7021_RESOLUTION_RH08_TEMP12 = 0b00000001,
    SI7021_RESOLUTION_RH10_TEMP13 = 0b10000000,
    SI7021_RESOLUTION_RH11_TEMP11 = 0b10000001,
} si7021_resolution_t;

typedef enum {
    SI7021_HEATER_STATE_ENABLE  = 0b00000100,
    SI7021_HEATER_STATE_DISABLE = 0b00000000,
} si7021_heater_state_t;

typedef enum {
    SI7021_VDD_STATUS_OK  = 0b00000000,
    SI7021_VDD_STATUS_LOW = 0b01000000,
} si7021_vdd_status_t;

typedef struct {
    si7021_resolution_t   resolution;
    si7021_heater_state_t heater_state;
    si7021_vdd_status_t   vdd_status;
} si7021_user_register_info_t;

// Si7021 Heater Level //
typedef enum {
    SI7021_HEATER_LEVEL_3_09_MA  = 0x00,
    SI7021_HEATER_LEVEL_9_18_MA  = 0x01,
    SI7021_HEATER_LEVEL_15_24_MA = 0x02,
    SI7021_HEATER_LEVEL_27_39_MA = 0x04,
    SI7021_HEATER_LEVEL_51_69_MA = 0x08,
    SI7021_HEATER_LEVEL_94_20_MA = 0x0F,
} si7021_heater_level_t;

// Si7021 Serial Number ID //
typedef enum {
    SI7021_SN_ID_DEV_00 = 0x00,
    SI7021_SN_ID_SI7013 = 0x0D,
    SI7021_SN_ID_SI7020 = 0x14,
    SI7021_SN_ID_SI7021 = 0x15,
    SI7021_SN_ID_DEV_FF = 0xFF,
} si7021_sn_id_t;

// Si7021 Firmware Version //
typedef enum {
    SI7021_FW_VERSION_1_0 = 0xFF,
    SI7021_FW_VERSION_2_0 = 0x20,
} si7021_fw_version_t;

// Si7021 Create Args //
typedef struct {
    // Name
    const char *name;

    // I2C
    si7021_i2c_create_args_t i2c;

    // CRC Config
    si7021_crc_config_t crc_config;

    // Read Wait
    si7021_read_wait_t read_wait;

    // At init
    struct {
        bool reset;

        bool set_heater_level;
        uint8_t heater_level;

        bool set_user_register_info;
        si7021_user_register_info_t user_register_info;

        struct {
            bool device_info;
            bool crc_config;
            bool read_wait;
        } dump;
    } at_init;
} si7021_create_args_t;




// FUNCTION DECLARATIONS //

// Handle Functions //

esp_err_t si7021_create(
    const si7021_create_args_t *create_args,
    si7021_handle_t *out_handle
);

esp_err_t si7021_delete(
    si7021_handle_t si7021
);






// Conversion Functions //

float si7021_convert_rh(
    uint16_t rh_code
);

float si7021_convert_temp(
    uint16_t temp_code
);





// CRC Configuration Functions //

esp_err_t si7021_get_crc_config(
    const si7021_handle_t si7021,
    si7021_crc_config_t *out_crc_config
);

esp_err_t si7021_set_crc_config(
    const si7021_handle_t si7021,
    const si7021_crc_config_t *crc_config
);

// Read Wait Functions //
esp_err_t si7021_get_read_wait(
    const si7021_handle_t si7021,
    si7021_read_wait_t *out_read_wait
);

esp_err_t si7021_set_read_wait(
    const si7021_handle_t si7021,
    const si7021_read_wait_t *read_wait
);





// Measurement Functions //

esp_err_t si7021_measure_rh_and_temp(
    const si7021_handle_t si7021,
    float *out_rh_percent,
    float *out_temp_celsius
);

// This functions uses si7021_measure_rh_and_temp ------------------------------
// * issuing a measure RH command to the sensor obtains a temperature reading
//   to use internally; this reading can be obtained without issuing a separate
//   measure temperature command.
esp_err_t si7021_measure_rh(
    const si7021_handle_t si7021,
    float *out_rh_percent
);
// -----------------------------------------------------------------------------

esp_err_t si7021_measure_temp(
    const si7021_handle_t si7021,
    float *out_temp_celsius
);






// Control Functions //

esp_err_t si7021_reset(
    const si7021_handle_t si7021
);

esp_err_t si7021_get_user_register_info(
    const si7021_handle_t si7021,
    uint8_t *out_user_register,
    si7021_user_register_info_t *out_user_register_info
);

// These functions use si7021_get_user_register_info ---------------------------
esp_err_t si7021_get_resolution(
    const si7021_handle_t si7021,
    si7021_resolution_t *out_resolution
);

esp_err_t si7021_get_heater_state(
    const si7021_handle_t si7021,
    si7021_heater_state_t *out_heater_state
);

esp_err_t si7021_get_vdd_status(
    const si7021_handle_t si7021,
    si7021_vdd_status_t *out_vdd_status
);
// -----------------------------------------------------------------------------

esp_err_t si7021_get_heater_level(
    const si7021_handle_t si7021,
    si7021_heater_level_t *out_heater_level
);

float si7021_calc_heater_level(
    uint8_t heater_level
);

esp_err_t si7021_set_user_register_info(
    const si7021_handle_t si7021,
    const si7021_user_register_info_t *user_register_info
);

// These functions use si7021_set_user_register_info ---------------------------
// * set_user_register_info does not need to read the user register info, as
//   it sets both the resolution and heater state at the same time, from the
//   user register default value (see datasheet).
// * VDD Status bit is read only, so it is ignored.
esp_err_t si7021_set_resolution(
    const si7021_handle_t si7021,
    const si7021_resolution_t resolution
);

esp_err_t si7021_set_heater_state(
    const si7021_handle_t si7021,
    const si7021_heater_state_t heater_state
);
// -----------------------------------------------------------------------------

esp_err_t si7021_set_heater_level(
    const si7021_handle_t si7021,
    uint8_t heater_level
);






// Information Functions //

esp_err_t si7021_get_serial_number_info(
    const si7021_handle_t si7021,
    uint64_t *out_serial_number,
    si7021_sn_id_t *out_sn_id
);

esp_err_t si7021_get_firmware_revision_info(
    const si7021_handle_t si7021,
    si7021_fw_version_t *out_fw_version
);






// Dump Functions //

esp_err_t si7021_dump_device_info(
    const si7021_handle_t si7021
);

esp_err_t si7021_dump_crc_config(
    const si7021_handle_t si7021
);

esp_err_t si7021_dump_read_wait(
    const si7021_handle_t si7021
);



#endif // __SI7021_H__