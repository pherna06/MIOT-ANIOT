#include "si7021_i2c.h"

// INCLUDES //
#include "string.h"     // for strncpy
#include "esp_err.h"    // for ESP errors
#include "driver/i2c.h" // for ESP I2C driver

// LOGGING //
#include "esp_log.h"
static const char *TAG = "Si7021 I2C";
static const char *CRC_TAG = "Si7021 I2C CRC";


// DEFINITIONS AND MACROS //
#ifndef ESP_RETURN_ON_FALSE
#define ESP_RETURN_ON_FALSE(expr, err, tag, msg) \
    if (!(expr)) { \
        ESP_LOGE(tag, msg); \
        return err; \
    }
#endif

#define SI7021_I2C_NAME_LENGTH 16

#define SI7021_I2C_CREATE_ARGS_ERR_STR       "Pointer to si7021_i2c_create_args_t is NULL."
#define SI7021_I2C_CREATE_HANDLE_ERR_STR     "Pointer to si7021_i2c_handle_t is NULL."
#define SI7021_I2C_CREATE_HANDLE_MEM_ERR_STR "Could not allocate memory for si7021_i2c_handle_t."
#define SI7021_I2C_CREATE_NAME_ERR_STR       "Pointer to create_args->name is NULL."
#define SI7021_I2C_CREATE_I2C_PORT_ERR_STR   "create_args->i2c_port is not I2C_NUM_0 or I2C_NUM_1." 
#define SI7021_I2C_CONFIG_MODE_ERR_STR       "create_args->i2c_driver.config.mode is not I2C_MODE_MASTER."

#define SI7021_I2C_HANDLE_ERR_STR                "si7021_i2c_handle_t is NULL."
#define SI7021_I2C_OUT_PARAM_ERR_STR         "Pointer to out parameter is NULL."
#define SI7021_I2C_CMD_LINK_ERR_STR          "Could not create I2C command link."

// Si7021 I2C Handle //
struct si7021_i2c_handle {
    char name[SI7021_I2C_NAME_LENGTH];
    i2c_port_t i2c_port;
    TickType_t i2c_cmd_timeout;

    struct {
        bool uninstall_at_delete;
    } i2c_driver;
};






// STATIC FUNCTIONS //

// CRC //

#define CRC_POLY 0x31 // x^8 + x^5 + x^4 + 1
#define CRC_INIT 0x00
#define CRC_XOR  0x00

static uint8_t _crc8_general(
    uint8_t  *bytes,
    uint16_t  count,
    uint8_t   init,
    uint8_t   xor
) {
    uint8_t crc = init;
    for (int i = 0; i < count; i++) {
        crc ^= bytes[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc <<= 1;
                crc ^= CRC_POLY;
            } else {
                crc <<= 1;
            }
        }
    crc ^= xor;
    }
    return crc;
}

static uint8_t _crc8(
    uint8_t  *bytes,
    uint16_t  count
) {
    return _crc8_general(bytes, count, CRC_INIT, CRC_XOR);
}


// I2C //

// > I2C Master Write
esp_err_t _si7021_i2c_write(
    const si7021_i2c_handle_t si7021_i2c,
    uint8_t *data,
    size_t data_len
) {
    esp_err_t err;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021_i2c,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_I2C_HANDLE_ERR_STR
    );
    ESP_RETURN_ON_FALSE(
        data,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_I2C_OUT_PARAM_ERR_STR
    );

    // I2C MASTER WRITE //
    TickType_t start_ticks = xTaskGetTickCount();
    while (
        (err = i2c_master_write_to_device(
            si7021_i2c->i2c_port,
            SI7021_I2C_ADDRESS,
            data,
            data_len,
            0
        )) != ESP_OK &&
        xTaskGetTickCount() - start_ticks < si7021_i2c->i2c_cmd_timeout
    );

    // Check Error
    if (err) {
        ESP_LOGE(TAG, "Could not write to Si7021: %s", esp_err_to_name(err));
        return err;
    }

    // Return
    return ESP_OK;
}

// > I2C Master Read
esp_err_t _si7021_i2c_read(
    const si7021_i2c_handle_t si7021_i2c,
    uint8_t *data,
    size_t data_len
) {
    esp_err_t err;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021_i2c,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_I2C_HANDLE_ERR_STR
    );
    ESP_RETURN_ON_FALSE(
        data,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_I2C_OUT_PARAM_ERR_STR
    );

    // I2C MASTER READ //
    TickType_t start_ticks = xTaskGetTickCount();
    while (
        (err = i2c_master_read_from_device(
            si7021_i2c->i2c_port,
            SI7021_I2C_ADDRESS,
            data,
            data_len,
            0
        )) != ESP_OK &&
        xTaskGetTickCount() - start_ticks < si7021_i2c->i2c_cmd_timeout
    );

    // Check Error
    if (err) {
        ESP_LOGE(TAG, "Could not read from Si7021: %s", esp_err_to_name(err));
        return err;
    }

    // Return
    return ESP_OK;
}

// > I2C Master Write then Read
esp_err_t _si7021_do_write_then_read(
    const si7021_i2c_handle_t si7021_i2c,
    uint8_t *write_data,
    size_t write_data_len,
    uint8_t *read_data,
    size_t read_data_len,
    TickType_t wait_before_read
) {
    esp_err_t err;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021_i2c,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_I2C_HANDLE_ERR_STR
    );
    ESP_RETURN_ON_FALSE(
        write_data,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_I2C_OUT_PARAM_ERR_STR
    );
    ESP_RETURN_ON_FALSE(
        read_data,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_I2C_OUT_PARAM_ERR_STR
    );

    // Write
    if ( (err = _si7021_i2c_write(
        si7021_i2c,
        write_data,
        write_data_len
    )) ) {
        ESP_LOGE(TAG, "Could not write to Si7021: %s", esp_err_to_name(err));
        return err;
    }

    // Wait
    if (wait_before_read) vTaskDelay(wait_before_read);

    // Read
    if ( (err = _si7021_i2c_read(
        si7021_i2c,
        read_data,
        read_data_len
    )) ) {
        ESP_LOGE(TAG, "Could not read from Si7021: %s", esp_err_to_name(err));
    }

    // Return
    return ESP_OK;
}

esp_err_t _si7021_i2c_do_measure_command(
    const si7021_i2c_handle_t si7021_i2c,
    uint8_t command,
    uint16_t *out_data,
    bool crc_check,
    TickType_t wait_before_read
) {
    esp_err_t err;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021_i2c,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_I2C_HANDLE_ERR_STR
    );
    ESP_RETURN_ON_FALSE(
        out_data,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_I2C_OUT_PARAM_ERR_STR
    );

    // > Write command then read data
    uint8_t data[3];
    if ( (err = _si7021_do_write_then_read(
        si7021_i2c,
        (uint8_t[]) {command},
        1,
        data,
        crc_check ? 3 : 2,
        wait_before_read
    )) ) {
        ESP_LOGE(TAG, "Could not write then read from Si7021: %s", esp_err_to_name(err));
        return err;
    }

    // > Check CRC
    if (crc_check) {
        uint8_t crc = _crc8(&data[0], 2);
        if (crc != data[2]) {
            ESP_LOGE(CRC_TAG, "CRC check failed:\n"
                          " - From     > 0x%02x 0x%02x\n"
                          " - Calc     > 0x%02x\n"
                          " - Expected > 0x%02x",
                            data[0], data[1], crc, data[2]
            );
            return ESP_FAIL;
        }
        // Only for this exercise
        ESP_LOGI(CRC_TAG, "CRC check passed.\n"
                     " - From     > 0x%02x 0x%02x\n"
                     " - Calc     > 0x%02x\n"
                     " - Expected > 0x%02x",
                       data[0], data[1], crc, data[2]
        );
    }

    // > Parse Data
    *out_data = ((uint16_t) data[0] << 8) |
                ((uint16_t) data[1] << 0) ;

    // Return
    return ESP_OK;
}

esp_err_t _si7021_i2c_do_read_reg_command(
    const si7021_i2c_handle_t si7021_i2c,
    uint8_t command,
    uint8_t *out_reg_value,
    TickType_t wait_before_read
) {
    return _si7021_do_write_then_read(
        si7021_i2c,
        (uint8_t[]) {command},
        1,
        out_reg_value,
        1,
        wait_before_read
    );
}






// PUBLIC FUNCTIONS //

// Handle Functions //

// > Create Si7021 I2C Handle
esp_err_t si7021_i2c_create(
    const si7021_i2c_create_args_t *create_args,
    si7021_i2c_handle_t *out_handle
) {
    esp_err_t err;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        out_handle,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_I2C_CREATE_HANDLE_ERR_STR
    );
    ESP_RETURN_ON_FALSE(
        create_args,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_I2C_CREATE_ARGS_ERR_STR
    );
    ESP_RETURN_ON_FALSE(
        create_args->i2c_port == I2C_NUM_0 || create_args->i2c_port == I2C_NUM_1,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_I2C_CREATE_I2C_PORT_ERR_STR
    )

    // I2C DRIVER //
    if (create_args->i2c_driver.install) {
        ESP_LOGI(TAG, "Installing I2C driver...");

        // Set I2C Driver Configuration
        if (create_args->i2c_driver.set_config) {
            ESP_LOGI(TAG, "Setting I2C driver configuration...\n"
                          "  > Mode: %s\n"
                          "  > SDA GPIO: %d\n"
                          "  > SCL GPIO: %d\n"
                          "  > Clock Speed: %d Hz",
                          create_args->i2c_driver.config.mode == I2C_MODE_MASTER ? "Master" : "Slave",
                          create_args->i2c_driver.config.sda_io_num,
                          create_args->i2c_driver.config.scl_io_num,
                          (int) create_args->i2c_driver.config.master.clk_speed
            );

            ESP_RETURN_ON_FALSE(
                create_args->i2c_driver.config.mode == I2C_MODE_MASTER,
                ESP_ERR_INVALID_ARG,
                TAG, SI7021_I2C_CONFIG_MODE_ERR_STR
            );

            if (create_args->i2c_driver.config.master.clk_speed > SI7021_I2C_MAX_CLOCK_SPEED) {
                ESP_LOGW(TAG, "I2C clock speed is greater than maximum recommended value of %d Hz.", SI7021_I2C_MAX_CLOCK_SPEED);
            }

            if ((err = i2c_param_config(
                create_args->i2c_port,
                &create_args->i2c_driver.config
            )) ) {
                ESP_LOGE(TAG, "Could not set I2C driver configuration: %s", esp_err_to_name(err));
                return err;
            }
        }

        // Install I2C Driver
        if ((err = i2c_driver_install(
            create_args->i2c_port,
            create_args->i2c_driver.config.mode,
            0, 0, 0
        )) ) {
            ESP_LOGE(TAG, "Could not install I2C driver: %s", esp_err_to_name(err));
            return err;
        }
    }

    // ALLOCATE HANDLE //
    *out_handle = malloc(sizeof(struct si7021_i2c_handle));
    ESP_RETURN_ON_FALSE(
        *out_handle,
        ESP_ERR_NO_MEM,
        TAG, SI7021_I2C_CREATE_HANDLE_MEM_ERR_STR
    );

    // INITIALIZE HANDLE //
    // > Name
    strncpy((*out_handle)->name, create_args->name, SI7021_I2C_NAME_LENGTH);

    // > I2C Port
    (*out_handle)->i2c_port = create_args->i2c_port;

    // > I2C Command Timeout
    (*out_handle)->i2c_cmd_timeout = create_args->i2c_cmd_timeout;

    // > I2C Driver
    (*out_handle)->i2c_driver.uninstall_at_delete =
        create_args->i2c_driver.install &&
        create_args->i2c_driver.uninstall_at_delete;
    
    // Return
    return ESP_OK;
}

// > Delete Si7021 I2C Handle
esp_err_t si7021_i2c_delete(
    si7021_i2c_handle_t si7021_i2c
) {
    esp_err_t err;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021_i2c,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_I2C_HANDLE_ERR_STR
    );

    // I2C DRIVER //
    if (si7021_i2c->i2c_driver.uninstall_at_delete) {
        ESP_LOGI(TAG, "Uninstalling I2C driver...");
        // Uninstall I2C Driver
        if ((err = i2c_driver_delete(si7021_i2c->i2c_port))) {
            ESP_LOGW(TAG, "Could not uninstall I2C driver: %s", esp_err_to_name(err));
        }
    }

    // FREE HANDLE //
    free(si7021_i2c);

    // Return
    return ESP_OK;
}


// Measurement Commands //

// > Measure Relative Humidity (Hold Master)
esp_err_t si7021_i2c_measure_rh_hold_master(
    const si7021_i2c_handle_t si7021_i2c,
    uint16_t *out_rh,
    bool crc_check,
    TickType_t wait_before_read
) {
    return _si7021_i2c_do_measure_command(
        si7021_i2c,
        SI7021_I2C_CMD_MEASURE_RH_HOLD_MASTER,
        out_rh,
        crc_check,
        wait_before_read
    );
}

// > Measure Relative Humidity (No Hold Master)
esp_err_t si7021_i2c_measure_rh_nohold_master(
    const si7021_i2c_handle_t si7021_i2c,
    uint16_t *out_rh,
    bool crc_check,
    TickType_t wait_before_read
) {
    return _si7021_i2c_do_measure_command(
        si7021_i2c,
        SI7021_I2C_CMD_MEASURE_RH_NOHOLD_MASTER,
        out_rh,
        crc_check,
        wait_before_read
    );
}

// > Measure Temperature (Hold Master)
esp_err_t si7021_i2c_measure_temp_hold_master(
    const si7021_i2c_handle_t si7021_i2c,
    uint16_t *out_temp,
    bool crc_check,
    TickType_t wait_before_read
) {
    return _si7021_i2c_do_measure_command(
        si7021_i2c,
        SI7021_I2C_CMD_MEASURE_TEMP_HOLD_MASTER,
        out_temp,
        crc_check,
        wait_before_read
    );
}

// > Measure Temperature (No Hold Master)
esp_err_t si7021_i2c_measure_temp_nohold_master(
    const si7021_i2c_handle_t si7021_i2c,
    uint16_t *out_temp,
    bool crc_check,
    TickType_t wait_before_read
) {
    return _si7021_i2c_do_measure_command(
        si7021_i2c,
        SI7021_I2C_CMD_MEASURE_TEMP_NOHOLD_MASTER,
        out_temp,
        crc_check,
        wait_before_read
    );
}

// > Read Temperature Value from Previous RH Measurement
esp_err_t si7021_i2c_read_temp_from_prev_rh_measurement(
    const si7021_i2c_handle_t si7021_i2c,
    uint16_t *out_temp,
    TickType_t wait_before_read
) {
    return _si7021_i2c_do_measure_command(
        si7021_i2c,
        SI7021_I2C_CMD_READ_TEMP_FROM_PREV_RH_MEASUREMENT,
        out_temp,
        false,
        wait_before_read
    );
}

// Control Commands //

// > Reset
esp_err_t si7021_i2c_reset(
    const si7021_i2c_handle_t si7021_i2c
) {
    return _si7021_i2c_write(
        si7021_i2c,
        (uint8_t[]) {SI7021_I2C_CMD_RESET},
        1
    );
}

// > Write RH/T User Register 1
esp_err_t si7021_i2c_write_user_reg_1(
    const si7021_i2c_handle_t si7021_i2c,
    uint8_t reg_value
) {
    return _si7021_i2c_write(
        si7021_i2c,
        (uint8_t[]) {SI7021_I2C_CMD_WRITE_RH_T_USER_REG_1, reg_value},
        2
    );
}

// > Read RH/T User Register 1
esp_err_t si7021_i2c_read_user_reg_1(
    const si7021_i2c_handle_t si7021_i2c,
    uint8_t *out_reg_value,
    TickType_t wait_before_read
) {
    return _si7021_i2c_do_read_reg_command(
        si7021_i2c,
        SI7021_I2C_CMD_READ_RH_T_USER_REG_1,
        out_reg_value,
        wait_before_read
    );
}

// > Write Heater Control Register
esp_err_t si7021_i2c_write_heater_control_reg(
    const si7021_i2c_handle_t si7021_i2c,
    uint8_t reg_value
) {
    return _si7021_i2c_write(
        si7021_i2c,
        (uint8_t[]) {SI7021_I2C_CMD_WRITE_HEATER_CONTROL_REG, reg_value},
        2
    );
}

// > Read Heater Control Register
esp_err_t si7021_i2c_read_heater_control_reg(
    const si7021_i2c_handle_t si7021_i2c,
    uint8_t *out_reg_value,
    TickType_t wait_before_read
) {
    return _si7021_i2c_do_read_reg_command(
        si7021_i2c,
        SI7021_I2C_CMD_READ_HEATER_CONTROL_REG,
        out_reg_value,
        wait_before_read
    );
}

// Information Commands //

// > Read Electronic ID First Bytes
esp_err_t si7021_i2c_read_electronic_id_first_bytes(
    const si7021_i2c_handle_t si7021_i2c,
    uint32_t *out_eid_first_bytes,
    bool crc_check,
    TickType_t wait_before_read
) {
    esp_err_t err;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021_i2c,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_I2C_HANDLE_ERR_STR
    );
    ESP_RETURN_ON_FALSE(
        out_eid_first_bytes,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_I2C_OUT_PARAM_ERR_STR
    );

    // > Write commands then read data
    uint8_t data[8];
    if ( (err = _si7021_do_write_then_read(
        si7021_i2c,
        (uint8_t[]) {
            SI7021_I2C_CMD_READ_ELECTRONIC_ID_FIRST_BYTES_A,
            SI7021_I2C_CMD_READ_ELECTRONIC_ID_FIRST_BYTES_B
        },
        2,
        data,
        8,
        wait_before_read
    )) ) {
        ESP_LOGE(TAG, "Could not write then read from Si7021: %s", esp_err_to_name(err));
        return err;
    }

    // > Check CRC
    if (crc_check) {
        uint8_t init = CRC_INIT;
        for (int i = 0; i < 8; i += 2) {
            uint8_t crc = _crc8_general(&data[i], 1, init, CRC_XOR);
            if (crc != data[i + 1]) {
                ESP_LOGE(CRC_TAG, "CRC check failed (%d/4):\n"
                                    " - Init     > 0x%02x\n"
                                    " - From     > 0x%02x\n"
                                    " - Calc     > 0x%02x\n"
                                    " - Expected > 0x%02x",
                                    i / 2 + 1, init, data[i], crc, data[i + 1]
                );
                return ESP_FAIL;
            }
            // Only for this exercise
            ESP_LOGI(CRC_TAG, "CRC check passed (%d/4).\n"
                        " - Init     > 0x%02x\n"
                        " - From     > 0x%02x\n"
                        " - Calc     > 0x%02x\n"
                        " - Expected > 0x%02x",
                        i / 2 + 1, init, data[i], crc, data[i + 1]
            );
            init = crc;
        }
    }

    // > Parse Data
    *out_eid_first_bytes = ((uint32_t) data[0] << 24) |
                           ((uint32_t) data[2] << 16) |
                           ((uint32_t) data[4] << 8)  |
                           ((uint32_t) data[6] << 0)  ;

    // Return
    return ESP_OK;
}

// > Read Electronic ID Last Bytes
esp_err_t si7021_i2c_read_electronic_id_last_bytes(
    const si7021_i2c_handle_t si7021_i2c,
    uint32_t *out_eid_last_bytes,
    bool crc_check,
    TickType_t wait_before_read
) {
    esp_err_t err;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021_i2c,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_I2C_HANDLE_ERR_STR
    );
    ESP_RETURN_ON_FALSE(
        out_eid_last_bytes,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_I2C_OUT_PARAM_ERR_STR
    );

    // > Write commands then read data
    uint8_t data[6];
    if ( (err = _si7021_do_write_then_read(
        si7021_i2c,
        (uint8_t[]) {
            SI7021_I2C_CMD_READ_ELECTRONIC_ID_LAST_BYTES_A,
            SI7021_I2C_CMD_READ_ELECTRONIC_ID_LAST_BYTES_B
        },
        2,
        data,
        6,
        wait_before_read
    )) ) {
        ESP_LOGE(TAG, "Could not write then read from Si7021: %s", esp_err_to_name(err));
        return err;
    }

    // > Check CRC
    if (crc_check) {
        uint8_t init = CRC_INIT;
        for (int i = 0; i < 6; i += 3) {
            uint8_t crc = _crc8_general(&data[i], 2, init, CRC_XOR);
            if (crc != data[i + 2]) {
                ESP_LOGE(CRC_TAG, "CRC check failed (%d/4):\n"
                                    " - Init     > 0x%02x\n"
                                    " - From     > 0x%02x 0x%02x\n"
                                    " - Calc     > 0x%02x\n"
                                    " - Expected > 0x%02x",
                                    i / 3 + 1, init, data[i], data[i + 1], crc, data[i + 2]
                );
                return ESP_FAIL;
            }
            // Only for this exercise
            ESP_LOGI(CRC_TAG, "CRC check passed (%d/4).\n"
                        " - Init     > 0x%02x\n"
                        " - From     > 0x%02x 0x%02x\n"
                        " - Calc     > 0x%02x\n"
                        " - Expected > 0x%02x",
                        i / 3 + 1, init, data[i], data[i + 1], crc, data[i + 2]
            );
            init = crc;
        }
    }

    // > Parse Data
    *out_eid_last_bytes = ((uint32_t) data[0] << 24) |
                          ((uint32_t) data[1] << 16) |
                          ((uint32_t) data[3] << 8)  |
                          ((uint32_t) data[4] << 0)  ;

    // Return
    return ESP_OK;
}

// > Read Firmware Revision
esp_err_t si7021_i2c_read_firmware_revision(
    const si7021_i2c_handle_t si7021_i2c,
    uint8_t *out_firmware_revision,
    TickType_t wait_before_read
) {
    esp_err_t err;

    // CHECKS //
    ESP_RETURN_ON_FALSE(
        si7021_i2c,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_I2C_HANDLE_ERR_STR
    );
    ESP_RETURN_ON_FALSE(
        out_firmware_revision,
        ESP_ERR_INVALID_ARG,
        TAG, SI7021_I2C_OUT_PARAM_ERR_STR
    );

    // > Write commands then read data
    if ( (err = _si7021_do_write_then_read(
        si7021_i2c,
        (uint8_t[]) {SI7021_I2C_CMD_READ_FIRMWARE_REVISION_A, SI7021_I2C_CMD_READ_FIRMWARE_REVISION_B},
        2,
        out_firmware_revision,
        1,
        wait_before_read
    )) ) {
        ESP_LOGE(TAG, "Could not write then read from Si7021: %s", esp_err_to_name(err));
        return err;
    }

    // Return
    return ESP_OK;
}
