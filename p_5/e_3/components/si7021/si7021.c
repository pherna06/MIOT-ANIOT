#include "si7021.h"

// INCLUDES //
#include "string.h" // for strncpy
#include "esp_err.h" // for ESP errors
#include "si7021_i2c.h" // for Si7021 I2C API

// LOGGING //
#include "esp_log.h"
static const char *TAG = "Si7021";




// DEFINITIONS AND MACROS //
#define SI7021_USER_REGISTER_DEFAULT           0b00111010
#define SI7021_USER_REGISTER_MASK_RESOLUTION   0b10000001
#define SI7021_USER_REGISTER_MASK_VDD_STATUS   0b01000000
#define SI7021_USER_REGISTER_MASK_HEATER_STATE 0b00000100

#define SI7021_HEATER_REGISTER_DEFAULT           0b00000000
#define SI7021_HEATER_REGISTER_MASK_HEATER_LEVEL 0b00001111

#ifndef ESP_RETURN_ON_FALSE
#define ESP_RETURN_ON_FALSE(expr, err, tag, msg) \
    if (!(expr)) { \
        ESP_LOGE(tag, msg); \
        return err; \
    }
#endif

#define SI7021_TODO_CRC_CHECK false
#define SI7021_TODO_TICKS_TO_READ 1000 / portTICK_PERIOD_MS

#define SI7021_NAME_LENGTH 16   

#define SI7021_CREATE_HANDLE_ERR_STR     "Pointer to si7021_handle_t is NULL."
#define SI7021_CREATE_ARGS_ERR_STR       "Pointer to si7021_create_args_t is NULL."
#define SI7021_CREATE_HANDLE_MEM_ERR_STR "Could not allocate memory for si7021_handle_t."

#define SI7021_OUT_PARAM_ERR_STR "Pointer to out parameter is NULL."




// Si7021 Handle //
struct si7021_handle {
    // Name
    char name[SI7021_NAME_LENGTH];

    // I2C
    si7021_i2c_handle_t i2c;

    // CRC Config
    si7021_crc_config_t crc_config;

    // Read Wait
    si7021_read_wait_t read_wait;
};




// STATIC FUNCTIONS //
esp_err_t _si7021_create_at_init(
    const si7021_handle_t si7021,
    const si7021_create_args_t *create_args
) {
    esp_err_t err;
    
    // RESET //
    if (create_args->at_init.reset) {
        ESP_LOGI(TAG, "Resetting Si7021 at init.");
        if ( (err = si7021_reset(si7021)) != ESP_OK ) {
            ESP_LOGE(TAG, "Could not reset Si7021 at init: %s", esp_err_to_name(err));
            return err;
        }
    }

    // HEATER LEVEL //
    if (create_args->at_init.set_heater_level) {
        ESP_LOGI(TAG, "Setting Si7021 heater level at init.");
        if ( (err = si7021_set_heater_level(
            si7021,
            create_args->at_init.heater_level
        )) != ESP_OK ) {
            ESP_LOGW(TAG, "Could not set Si7021 heater level at init: %s", esp_err_to_name(err));
            return err;
        }
    }

    // USER REGISTER INFO //
    if (create_args->at_init.set_user_register_info) {
        ESP_LOGI(TAG, "Setting Si7021 user register info at init.");
        if ( (err = si7021_set_user_register_info(
            si7021,
            &create_args->at_init.user_register_info
        )) != ESP_OK ) {
            ESP_LOGW(TAG, "Could not set Si7021 user register info at init: %s", esp_err_to_name(err));
            return err;
        }
    }

    // DUMP INFO //
    if (create_args->at_init.dump.device_info) {
        if ( (err = si7021_dump_device_info(si7021)) != ESP_OK ) {
            ESP_LOGW(TAG, "Could not dump Si7021 info at init: %s", esp_err_to_name(err));
            return err;
        }
    }

    // DUMP CRC CONFIG //
    if (create_args->at_init.dump.crc_config) {
        if ( (err = si7021_dump_crc_config(si7021)) != ESP_OK ) {
            ESP_LOGW(TAG, "Could not dump Si7021 CRC config at init: %s", esp_err_to_name(err));
            return err;
        }
    }

    // DUMP READ WAIT //
    if (create_args->at_init.dump.read_wait) {
        if ( (err = si7021_dump_read_wait(si7021)) != ESP_OK ) {
            ESP_LOGW(TAG, "Could not dump Si7021 read wait at init: %s", esp_err_to_name(err));
            return err;
        }
    }

    // Return
    return ESP_OK;
}




// PUBLIC FUNCTIONS //

// Handle Functions //

// > Create Si7021 Handle
esp_err_t si7021_create(
    const si7021_create_args_t *create_args,
    si7021_handle_t *out_handle
) {
    esp_err_t err;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        out_handle,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_CREATE_HANDLE_ERR_STR
    );
    ESP_RETURN_ON_FALSE(
        create_args,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_CREATE_ARGS_ERR_STR
    );

    // ALLOCATE HANDLE //
    *out_handle = malloc(sizeof(struct si7021_handle));
    ESP_RETURN_ON_FALSE(
        *out_handle,
        ESP_ERR_NO_MEM,
        TAG, SI7021_CREATE_HANDLE_MEM_ERR_STR
    );

    // INITIALIZE HANDLE //
    // > Name
    strncpy((*out_handle)->name, create_args->name, SI7021_NAME_LENGTH);

    // > I2C
    if ( (err = si7021_i2c_create(
        (const si7021_i2c_create_args_t*) &create_args->i2c,
        &((*out_handle)->i2c)
    )) ) {
        ESP_LOGE(TAG, "Could not create Si7021 I2C: %s", esp_err_to_name(err));
        free(*out_handle);
        return err;
    }

    // > CRC Config
    (*out_handle)->crc_config = create_args->crc_config;

    // > Read Wait
    (*out_handle)->read_wait = create_args->read_wait;

    // AT INIT //
    if ( (err = _si7021_create_at_init(
        *out_handle,
        create_args
    )) ) {
        free(*out_handle);
        return err;
    }

    // Return
    return ESP_OK;
}

// > Delete Si7021 Handle
esp_err_t si7021_delete(
    si7021_handle_t si7021
) {
    esp_err_t err = ESP_OK;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_CREATE_HANDLE_ERR_STR
    );

    // DELETE I2C //
    if ( (err = si7021_i2c_delete(si7021->i2c)) != ESP_OK ) {
        ESP_LOGW(TAG, "Could not delete Si7021 I2C: %s", esp_err_to_name(err));
    }

    // FREE HANDLE //
    free(si7021);

    // Return
    return err;
}











// Conversion Functions //

// > Get RH percentage from 16-bit RH code
float si7021_convert_rh(
    uint16_t rh_code
) {
    float rh = (125.0 * (float) rh_code) / 65536.0 - 6.0;
    return rh < 0.0   ? 0.0 
         : rh > 100.0 ? 100.0
         : rh;
}

// > Get temperature in Celsius from 16-bit temperature code
float si7021_convert_temp(
    uint16_t temp_code
) {
    return (175.72 * (float) temp_code) / 65536.0 - 46.85;
}







// CRC Configuration Functions //

// > Get CRC Config
esp_err_t si7021_get_crc_config(
    const si7021_handle_t si7021,
    si7021_crc_config_t *out_crc_config
) {
    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_CREATE_HANDLE_ERR_STR
    );

    // GET CRC CONFIG //
    *out_crc_config = si7021->crc_config;

    // Return
    return ESP_OK;
}

// > Set CRC Config
esp_err_t si7021_set_crc_config(
    const si7021_handle_t si7021,
    const si7021_crc_config_t *crc_config
) {
    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_CREATE_HANDLE_ERR_STR
    );

    // SET CRC CONFIG //
    si7021->crc_config = *crc_config;

    // Return
    return ESP_OK;
}


// Read Wait Functions //

// > Get Read Wait
esp_err_t si7021_get_read_wait(
    const si7021_handle_t si7021,
    si7021_read_wait_t *out_read_wait
) {
    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_CREATE_HANDLE_ERR_STR
    );
    ESP_RETURN_ON_FALSE(
        out_read_wait,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_OUT_PARAM_ERR_STR
    );

    // GET READ WAIT //
    *out_read_wait = si7021->read_wait;

    // Return
    return ESP_OK;
}

// > Set Read Wait
esp_err_t si7021_set_read_wait(
    const si7021_handle_t si7021,
    const si7021_read_wait_t *read_wait
) {
    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_CREATE_HANDLE_ERR_STR
    );
    ESP_RETURN_ON_FALSE(
        read_wait,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_OUT_PARAM_ERR_STR
    );

    // SET READ WAIT //
    si7021->read_wait = *read_wait;

    // Return
    return ESP_OK;
}








// Measurement Functions //

// > Measure Relative Humidity and Temperature
esp_err_t si7021_measure_rh_and_temp(
    const si7021_handle_t si7021,
    float *out_rh_percent,
    float *out_temp_celsius
) {
    esp_err_t err;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_CREATE_HANDLE_ERR_STR
    );
    ESP_RETURN_ON_FALSE(
        out_rh_percent,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_OUT_PARAM_ERR_STR
    );

    // MEASURE RELATIVE HUMIDITY (NO HOLD)//
    uint16_t rh = 0;
    if ( (err = si7021_i2c_measure_rh_hold_master(
        si7021->i2c,
        &rh,
        si7021->crc_config.global || si7021->crc_config.rh,
        si7021->read_wait.global ? si7021->read_wait.global : si7021->read_wait.rh
    )) ) {
        ESP_LOGE(TAG, "Could not measure Si7021 relative humidity: %s", esp_err_to_name(err));
        return err;
    }

    // MEASURE TEMPERATURE FROM PREV RH MEASUREMENT //
    uint16_t temp = 0;
    if ( (err = si7021_i2c_read_temp_from_prev_rh_measurement(
        si7021->i2c,
        &temp,
        si7021->read_wait.global ? si7021->read_wait.global : si7021->read_wait.temp
    )) ) {
        ESP_LOGE(TAG, "Could not read Si7021 temperature from previous relative humidity measurement: %s", esp_err_to_name(err));
        return err;
    }

    // CONVERT //
    *out_rh_percent = si7021_convert_rh(rh);
    *out_temp_celsius = si7021_convert_temp(temp);

    // Return
    return ESP_OK;
}

// Derived from si7021_measure_rh_and_temp -------------------------------------
esp_err_t si7021_measure_rh(
    const si7021_handle_t si7021,
    float *out_rh_percent
) {
    float temp_celsius;
    return si7021_measure_rh_and_temp(
        si7021,
        out_rh_percent,
        &temp_celsius
    );
}
// -----------------------------------------------------------------------------

// > Measure Temperature
esp_err_t si7021_measure_temp(
    const si7021_handle_t si7021,
    float *out_temp_celsius
) {
    esp_err_t err;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_CREATE_HANDLE_ERR_STR
    );
    ESP_RETURN_ON_FALSE(
        out_temp_celsius,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_OUT_PARAM_ERR_STR
    );

    // MEASURE TEMPERATURE (NO HOLD)//
    uint16_t temp = 0;
    if ( (err = si7021_i2c_measure_temp_hold_master(
        si7021->i2c,
        &temp,
        si7021->crc_config.global || si7021->crc_config.temp,
        si7021->read_wait.global ? si7021->read_wait.global : si7021->read_wait.temp
    )) ) {
        ESP_LOGE(TAG, "Could not measure Si7021 temperature: %s", esp_err_to_name(err));
        return err;
    }

    // CONVERT TO CELSIUS //
    *out_temp_celsius = si7021_convert_temp(temp);

    // Return
    return ESP_OK;
}










// Control Functions //

// > Reset
esp_err_t si7021_reset(
    const si7021_handle_t si7021
) {
    esp_err_t err;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_CREATE_HANDLE_ERR_STR
    );

    // RESET //
    if ( (err = si7021_i2c_reset(si7021->i2c)) != ESP_OK ) {
        ESP_LOGE(TAG, "Could not reset Si7021: %s", esp_err_to_name(err));
        return err;
    }

    // Return
    return ESP_OK;
}

// > Get User Register Info
esp_err_t si7021_get_user_register_info(
    const si7021_handle_t si7021,
    uint8_t *out_user_register,
    si7021_user_register_info_t *out_user_register_info
) {
    esp_err_t err;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_CREATE_HANDLE_ERR_STR
    );
    ESP_RETURN_ON_FALSE(
        out_user_register_info,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_OUT_PARAM_ERR_STR
    );

    // GET USER REGISTER //
    uint8_t user_register = 0;
    if ( (err = si7021_i2c_read_user_reg_1(
        si7021->i2c,
        &user_register,
        si7021->read_wait.global ? si7021->read_wait.global : si7021->read_wait.user_reg
    )) ) {
        ESP_LOGE(TAG, "Could not read Si7021 user register: %s", esp_err_to_name(err));
        return err;
    }

    // USER REGISTER INFO //
    if (out_user_register) *out_user_register = user_register;
    out_user_register_info->resolution = (si7021_resolution_t)(user_register & SI7021_USER_REGISTER_MASK_RESOLUTION);
    out_user_register_info->heater_state = (si7021_heater_state_t)(user_register & SI7021_USER_REGISTER_MASK_HEATER_STATE);
    out_user_register_info->vdd_status = (si7021_vdd_status_t)(user_register & SI7021_USER_REGISTER_MASK_VDD_STATUS);

    // Return
    return ESP_OK;
}

// Derived from si7021_get_user_register_info ----------------------------------
esp_err_t si7021_get_resolution(
    const si7021_handle_t si7021,
    si7021_resolution_t *out_resolution
) {
    esp_err_t err;
    si7021_user_register_info_t user_register_info;
    if ( (err = si7021_get_user_register_info(
        si7021,
        NULL,
        out_resolution ? &user_register_info : NULL
    )) ) return err;
    *out_resolution = user_register_info.resolution;
    return ESP_OK;
}

esp_err_t si7021_get_heater_state(
    const si7021_handle_t si7021,
    si7021_heater_state_t *out_heater_state
) {
    esp_err_t err;
    si7021_user_register_info_t user_register_info;
    if ( (err = si7021_get_user_register_info(
        si7021,
        NULL,
        out_heater_state ? &user_register_info : NULL
    )) ) return err;
    *out_heater_state = user_register_info.heater_state;
    return ESP_OK;
}

esp_err_t si7021_get_vdd_status(
    const si7021_handle_t si7021,
    si7021_vdd_status_t *out_vdd_status
) {
    esp_err_t err;
    si7021_user_register_info_t user_register_info;
    if ( (err = si7021_get_user_register_info(
        si7021,
        NULL,
        out_vdd_status ? &user_register_info : NULL
    )) ) return err;
    *out_vdd_status = user_register_info.vdd_status;
    return ESP_OK;
}
// -----------------------------------------------------------------------------



// > Get Heater Level
esp_err_t si7021_get_heater_level(
    const si7021_handle_t si7021,
    si7021_heater_level_t *out_heater_level
) {
    esp_err_t err;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_CREATE_HANDLE_ERR_STR
    );
    ESP_RETURN_ON_FALSE(
        out_heater_level,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_OUT_PARAM_ERR_STR
    );

    // GET HEATER REGISTER //
    uint8_t heater_register = 0;
    if ( (err = si7021_i2c_read_heater_control_reg(
        si7021->i2c,
        &heater_register,
        si7021->read_wait.global ? si7021->read_wait.global : si7021->read_wait.heater_reg
    )) ) {
        ESP_LOGE(TAG, "Could not read Si7021 heater register: %s", esp_err_to_name(err));
        return err;
    }

    // HEATER LEVEL //
    *out_heater_level = (si7021_heater_level_t)(heater_register & SI7021_HEATER_REGISTER_MASK_HEATER_LEVEL);

    // Return
    return ESP_OK;
}

// > Calculate Heater Level
float si7021_calc_heater_level(
    uint8_t heater_level
) {
    const static float value_0000 = 3.09;
    const static float value_0001 = 9.18;
    const static float value_0010 = 15.24;
    const static float value_0100 = 27.39;
    const static float value_1000 = 51.69;
    const static float value_1111 = 94.20;
    
    const static float step_1111_to_1000 = (value_1111 - value_1000) / (0b1111 - 0b1000);
    const static float step_1000_to_0100 = (value_1000 - value_0100) / (0b1000 - 0b0100);
    
    // Apply mask
    heater_level &= SI7021_HEATER_REGISTER_MASK_HEATER_LEVEL;

    switch (heater_level) {
        case SI7021_HEATER_LEVEL_3_09_MA:  return value_0000;
        case SI7021_HEATER_LEVEL_9_18_MA:  return value_0001;
        case SI7021_HEATER_LEVEL_15_24_MA: return value_0010;
        case SI7021_HEATER_LEVEL_27_39_MA: return value_0100;
        case SI7021_HEATER_LEVEL_51_69_MA: return value_1000;
        case SI7021_HEATER_LEVEL_94_20_MA: return value_1111;
        default: {
            // Between 1111 and 1000
            if (heater_level & 0b1000) {
                return value_1000 + step_1111_to_1000 * (heater_level - 0b1000);
            }
            // Between 1000 and 0100
            else if (heater_level & 0b0100) {
                return value_0100 + step_1000_to_0100 * (heater_level - 0b0100);
            }
            // Between 0100 and 0000: {0000, 0001, 0010, 0011}
            // then it is 0011 (mean of 0100 and 0010)
            else {
                return (value_0100 + value_0010) / 2;
            }
        }
    }
}

// > Set User Register Info
esp_err_t si7021_set_user_register_info(
    const si7021_handle_t si7021,
    const si7021_user_register_info_t *user_register_info
) {
    esp_err_t err;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_CREATE_HANDLE_ERR_STR
    );
    ESP_RETURN_ON_FALSE(
        user_register_info,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_OUT_PARAM_ERR_STR
    );

    // The whole user register is configured, so we do not need to read it first
    // and use the default reset value SI7021_USER_REGISTER_DEFAULT

    // SET USER REGISTER //
    uint8_t user_register = SI7021_USER_REGISTER_DEFAULT;
    user_register |= (uint8_t)(user_register_info->resolution);
    user_register |= (uint8_t)(user_register_info->heater_state);
    user_register |= (uint8_t)(user_register_info->vdd_status); // read-only > no effect

    if ( (err = si7021_i2c_write_user_reg_1(
        si7021->i2c,
        user_register
    )) ) {
        ESP_LOGE(TAG, "Could not write Si7021 user register: %s", esp_err_to_name(err));
        return err;
    }

    // Return
    return ESP_OK;
}

// Derived from si7021_set_user_register_info ----------------------------------
esp_err_t si7021_set_resolution(
    const si7021_handle_t si7021,
    const si7021_resolution_t resolution
) {
    esp_err_t err;
    si7021_user_register_info_t user_register_info;
    if ( (err = si7021_get_user_register_info(
        si7021,
        NULL,
        &user_register_info
    )) ) return err;
    user_register_info.resolution = resolution;
    return si7021_set_user_register_info(
        si7021,
        &user_register_info
    );
}

esp_err_t si7021_set_heater_state(
    const si7021_handle_t si7021,
    const si7021_heater_state_t heater_state
) {
    esp_err_t err;
    si7021_user_register_info_t user_register_info;
    if ( (err = si7021_get_user_register_info(
        si7021,
        NULL,
        &user_register_info
    )) ) return err;
    user_register_info.heater_state = heater_state;
    return si7021_set_user_register_info(
        si7021,
        &user_register_info
    );
}
// -----------------------------------------------------------------------------


esp_err_t si7021_set_heater_level(
    const si7021_handle_t si7021,
    uint8_t heater_level
) {
    esp_err_t err;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_CREATE_HANDLE_ERR_STR
    );

    // SET HEATER REGISTER //
    uint8_t heater_register = SI7021_HEATER_REGISTER_DEFAULT;
    heater_register |= (heater_level & SI7021_HEATER_REGISTER_MASK_HEATER_LEVEL);

    if ( (err = si7021_i2c_write_heater_control_reg(
        si7021->i2c,
        heater_register
    )) ) {
        ESP_LOGE(TAG, "Could not write Si7021 heater register: %s", esp_err_to_name(err));
        return err;
    }

    // Return
    return ESP_OK;
}











// Information Functions //

// > Get Serial Number Info
esp_err_t si7021_get_serial_number_info(
    const si7021_handle_t si7021,
    uint64_t *out_serial_number,
    si7021_sn_id_t *out_sn_id
) {
    esp_err_t err;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_CREATE_HANDLE_ERR_STR
    );
    ESP_RETURN_ON_FALSE(
        out_serial_number,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_OUT_PARAM_ERR_STR
    );

    // GET SERIAL NUMBER //
    uint32_t sna = 0;
    uint32_t snb = 0;
    if ( (err = si7021_i2c_read_electronic_id_first_bytes(
        si7021->i2c,
        &sna,
        si7021->crc_config.global || si7021->crc_config.sna,
        si7021->read_wait.global ? si7021->read_wait.global : si7021->read_wait.sna
    )) ) {
        ESP_LOGE(TAG, "Could not read Si7021 electronic ID first bytes: %s", esp_err_to_name(err));
        return err;
    }
    if ( (err = si7021_i2c_read_electronic_id_last_bytes(
        si7021->i2c,
        &snb,
        si7021->crc_config.global || si7021->crc_config.snb,
        si7021->read_wait.global ? si7021->read_wait.global : si7021->read_wait.snb
    )) ) {
        ESP_LOGE(TAG, "Could not read Si7021 electronic ID last bytes: %s", esp_err_to_name(err));
        return err;
    }

    // SERIAL NUMBER //
    *out_serial_number = ((uint64_t)sna << 32) |
                         ((uint64_t)snb <<  0) ;

    // SERIAL NUMBER ID (on SNB3) //
    if (out_sn_id) *out_sn_id = (si7021_sn_id_t)((snb >> 24) & 0xFF);

    // Return
    return ESP_OK;
}

// > Get Firmware Revision
esp_err_t si7021_get_firmware_revision_info(
    const si7021_handle_t si7021,
    si7021_fw_version_t *out_fw_version
) {
    esp_err_t err;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_CREATE_HANDLE_ERR_STR
    );
    ESP_RETURN_ON_FALSE(
        out_fw_version,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_OUT_PARAM_ERR_STR
    );

    // GET FIRMWARE REVISION //
    uint8_t fw_version = 0;
    if ( (err = si7021_i2c_read_firmware_revision(
        si7021->i2c,
        &fw_version,
        si7021->read_wait.global ? si7021->read_wait.global : si7021->read_wait.fw
    )) ) {
        ESP_LOGE(TAG, "Could not read Si7021 firmware revision: %s", esp_err_to_name(err));
        return err;
    }

    // FIRMWARE REVISION //
    *out_fw_version = (si7021_fw_version_t)(fw_version);

    // Return
    return ESP_OK;
}











// Dump Functions //

// > Dump Device Info
esp_err_t si7021_dump_device_info(
    const si7021_handle_t si7021
) {
    esp_err_t err;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_CREATE_HANDLE_ERR_STR
    );

    // USER REGISTER INFO //
    uint8_t user_register = 0;
    si7021_user_register_info_t user_register_info;
    if ( (err = si7021_get_user_register_info(
        si7021,
        &user_register,
        &user_register_info
    )) ) {
        ESP_LOGE(TAG, "Could not get Si7021 user register info: %s", esp_err_to_name(err));
        return err;
    }

    // HEATER LEVEL //
    si7021_heater_level_t heater_level;
    if ( (err = si7021_get_heater_level(
        si7021,
        &heater_level
    )) ) {
        ESP_LOGE(TAG, "Could not get Si7021 heater level: %s", esp_err_to_name(err));
        return err;
    }

    // SERIAL NUMBER //
    uint64_t serial_number;
    si7021_sn_id_t sn_id;
    if ( (err = si7021_get_serial_number_info(
        si7021,
        &serial_number,
        &sn_id
    )) ) {
        ESP_LOGE(TAG, "Could not get Si7021 serial number info: %s", esp_err_to_name(err));
        return err;
    }

    // FIRMWARE REVISION //
    si7021_fw_version_t fw_version;
    if ( (err = si7021_get_firmware_revision_info(
        si7021,
        &fw_version
    )) ) {
        ESP_LOGE(TAG, "Could not get Si7021 firmware revision info: %s", esp_err_to_name(err));
        return err;
    }

    // LOG //
    ESP_LOGI(TAG, "Si7021 Info:\n"
                  " - Name: %s\n"
                  " - User Register 0x%02X:\n"
                  "   - Resolution: %s\n"
                  "   - Heater State: %s\n"
                  "   - VDD Status: %s\n"
                  " - Heater Level: 0x%02X (Current: %s)\n"
                  " - Serial Number: 0x%016llX (ID: %s)\n"
                  " - Firmware Revision: 0x%02X (Version: %s)\n",
                  si7021->name,
                  user_register,
                  SI7021_RESOLUTION_TO_STRING(user_register_info.resolution),
                  SI7021_HEATER_STATE_TO_STRING(user_register_info.heater_state),
                  SI7021_VDD_STATUS_TO_STRING(user_register_info.vdd_status),
                  heater_level, SI7021_HEATER_LEVEL_TO_STRING(heater_level),
                  serial_number, SI7021_SN_ID_TO_STRING(sn_id),
                  fw_version, SI7021_FW_VERSION_TO_STRING(fw_version)
    );

    // Return
    return ESP_OK;
}

// > Dump CRC Config
esp_err_t si7021_dump_crc_config(
    const si7021_handle_t si7021
) {
    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_CREATE_HANDLE_ERR_STR
    );

    // LOG //
    ESP_LOGI(TAG, "Si7021 CRC Config:\n"
                  " - Global: %s\n"
                  "\n"
                  " - RH:     %s\n"
                  " - Temp:   %s\n"
                  " - SNA:    %s\n"
                  " - SNB:    %s\n",
                  si7021->crc_config.global ? "Enabled" : "Disabled",
                  si7021->crc_config.rh ? "Enabled" : "Disabled",
                  si7021->crc_config.temp ? "Enabled" : "Disabled",
                  si7021->crc_config.sna ? "Enabled" : "Disabled",
                  si7021->crc_config.snb ? "Enabled" : "Disabled"
    );

    // Return
    return ESP_OK;
}

// > Dump Read Wait
esp_err_t si7021_dump_read_wait(
    const si7021_handle_t si7021
) {
    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_CREATE_HANDLE_ERR_STR
    );

    // LOG //
    ESP_LOGI(TAG, "Si7021 Read Wait:\n"
                  " - Global:            %d ms\n"
                  "\n"
                  " - RH:                %d ms\n"
                  " - Temp:              %d ms\n"
                  " - User Register:     %d ms\n"
                  " - Heater Register:   %d ms\n"
                  " - SNA:               %d ms\n"
                  " - SNB:               %d ms\n"
                  " - Firmware Revision: %d ms\n",
                  (int) (si7021->read_wait.global * portTICK_PERIOD_MS),
                  (int) (si7021->read_wait.rh * portTICK_PERIOD_MS),
                  (int) (si7021->read_wait.temp * portTICK_PERIOD_MS),
                  (int) (si7021->read_wait.user_reg * portTICK_PERIOD_MS),
                  (int) (si7021->read_wait.heater_reg * portTICK_PERIOD_MS),
                  (int) (si7021->read_wait.sna * portTICK_PERIOD_MS),
                  (int) (si7021->read_wait.snb * portTICK_PERIOD_MS),
                  (int) (si7021->read_wait.fw * portTICK_PERIOD_MS)
    );

    // Return
    return ESP_OK;
}



