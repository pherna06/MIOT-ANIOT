#include "si7021_i2c.h"

// INCLUDES --------------------------------------------------------------------

#include "string.h"     // for strncpy
#include "esp_err.h"    // for ESP errors
#include "driver/i2c.h" // for ESP I2C driver

/* Logging */
#include "esp_log.h"
static const char *TAG = "Si7021 I2C";

// -----------------------------------------------------------------------------

// STRUCTURES ------------------------------------------------------------------

/* Si7021 I2C Handle */
struct si7021_i2c_handle {
    char name[SI7021_I2C_HANDLE_NAME_LENGTH]; // Handle name

    i2c_port_t i2c_port;                // I2C port
    TickType_t i2c_retry_timeout;       // I2C retry timeout
    bool       i2c_uninstall_at_delete; // Uninstall I2C driver at delete
};

// -----------------------------------------------------------------------------

// MACROS ----------------------------------------------------------------------

//// CREATE & DELETE -----------------------------------------------------------

#define CHECK_ERR_CREATE_ARGS(create_args)                                     \
    if (!create_args) {                                                        \
        ESP_LOGE(TAG, "Pointer to si7021_i2c_create_args_t is NULL.");         \
        return ESP_ERR_INVALID_ARG;                                            \
    }

#define CHECK_ERR_OUT_HANDLE(out_handle)                                       \
    if (!out_handle) {                                                         \
        ESP_LOGE(TAG, "Pointer to si7021_i2c_handle_t is NULL.");              \
        return ESP_ERR_INVALID_ARG;                                            \
    }

#define CHECK_ERR_I2C_MODE_IS_MASTER(mode)                                     \
    if (mode != I2C_MODE_MASTER) {                                             \
        ESP_LOGE(TAG, "I2C driver mode is not I2C_MODE_MASTER.");              \
        return ESP_ERR_INVALID_ARG;                                            \
    }

#define LOG_I2C_DRIVER_CONFIG(config)                                          \
    ESP_LOGI(TAG, "I2C driver configuration:\n"                                \
                  "  > Mode: %s\n"                                             \
                  "  > SDA GPIO: %d (pull-up: %s)\n"                           \
                  "  > SCL GPIO: %d (pull-up: %s)\n"                           \
                  "  > Clock Speed: %d Hz",                                    \
                    config.mode == I2C_MODE_MASTER ? "Master" : "Slave",       \
                    config.sda_io_num,                                         \
                    config.sda_pullup_en ? "yes" : "no",                       \
                    config.scl_io_num,                                         \
                    config.scl_pullup_en ? "yes" : "no",                       \
                    (int) config.master.clk_speed                              \
    );                                                                         \
    if (config.master.clk_speed > SI7021_I2C_MAX_CLOCK_SPEED) {                \
        ESP_LOGW(TAG, "I2C clock speed is greater than maximum recommended "   \
                      "value of %d Hz.", SI7021_I2C_MAX_CLOCK_SPEED);          \
    }

#define CHECK_ERR_I2C_PARAM_CONFIG(err)                                        \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Could not set I2C driver configuration: %s",            \
            esp_err_to_name(err)                                               \
        );                                                                     \
        return err;                                                            \
    }

#define CHECK_ERR_I2C_DRIVER_INSTALL(err)                                      \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Could not install I2C driver: %s",                      \
            esp_err_to_name(err)                                               \
        );                                                                     \
        return err;                                                            \
    }

#define CHECK_ERR_MALLOC(ptr)                                                  \
    if (!ptr) {                                                                \
        ESP_LOGE(TAG, "Could not allocate memory.");                           \
        return ESP_ERR_NO_MEM;                                                 \
    }

#define CHECK_WARN_I2C_CLOCK_SPEED(i2c_clock_speed)                            \
    if (i2c_clock_speed > SI7021_I2C_MAX_CLOCK_SPEED) {                        \
        ESP_LOGW(TAG, "I2C clock speed is greater than maximum recommended "   \
                      "value of %d Hz.", SI7021_I2C_MAX_CLOCK_SPEED);          \
    }

#define CHECK_WARN_I2C_DRIVER_DELETE(err)                                      \
    if (err) {                                                                 \
        ESP_LOGW(TAG, "Could not uninstall I2C driver: %s",                    \
            esp_err_to_name(err)                                               \
        );                                                                     \
    }

//// ---------------------------------------------------------------------------

//// HANDLE & OUT PARAM ERRORS -------------------------------------------------

#define CHECK_ERR_HANDLE(handle)                                               \
    if (!handle) {                                                             \
        ESP_LOGE(TAG, "Si7021 I2C handle is NULL.");                           \
        return ESP_ERR_INVALID_ARG;                                            \
    }

#define CHECK_ERR_OUT_PARAM(out_param)                                         \
    if (!out_param) {                                                          \
        ESP_LOGE(TAG, "Pointer to out parameter is NULL.");                    \
        return ESP_ERR_INVALID_ARG;                                            \
    }

//// ---------------------------------------------------------------------------

//// I2C ERRORS ----------------------------------------------------------------

#define CHECK_ERR_SI7021_I2C_WRITE(err)                                        \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Could not write to Si7021: %s", esp_err_to_name(err));  \
        return err;                                                            \
    }

#define CHECK_ERR_SI7021_I2C_READ(err)                                         \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Could not read from Si7021: %s", esp_err_to_name(err)); \
        return err;                                                            \
    }

#define CHECK_ERR_SI7021_I2C_DO_WRITE_THEN_READ(err)                           \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Could not write then read from Si7021: %s",             \
            esp_err_to_name(err));                                             \
        return err;                                                            \
    }

//// ---------------------------------------------------------------------------

//// CRC ERRORS ----------------------------------------------------------------

#define CHECK_ERR_COMPARE_MEASURE_CRC(byte_0, byte_1, calc, recv)              \
    if (calc != recv) {                                                        \
        ESP_LOGE(TAG, "CRC check failed:\n"                                    \
                          " - From > 0x%02x 0x%02x\n"                          \
                          " - Calc > 0x%02x\n"                                 \
                          " - Recv > 0x%02x",                                  \
                          byte_0, byte_1, calc, recv                           \
        );                                                                     \
        return ESP_FAIL;                                                       \
    }

#define CHECK_ERR_COMPARE_FIRST_EID_CRC(init, byte, calc, recv)                \
    if (calc != recv) {                                                        \
        ESP_LOGE(TAG, "CRC check failed:\n"                                \
                          " - Init > 0x%02x\n"                                 \
                          " - From > 0x%02x\n"                                 \
                          " - Calc > 0x%02x\n"                                 \
                          " - Recv > 0x%02x",                                  \
                          init, byte, calc, recv                               \
        );                                                                     \
        return ESP_FAIL;                                                       \
    }

#define CHECK_ERR_COMPARE_LAST_EID_CRC(init, byte_0, byte_1, calc, recv)       \
    if (calc != recv) {                                                        \
        ESP_LOGE(TAG, "CRC check failed:\n"                                \
                          " - Init > 0x%02x\n"                                 \
                          " - From > 0x%02x 0x%02x\n"                          \
                          " - Calc > 0x%02x\n"                                 \
                          " - Recv > 0x%02x",                                  \
                          init, byte_0, byte_1, calc, recv                     \
        );                                                                     \
        return ESP_FAIL;                                                       \
    }

//// ---------------------------------------------------------------------------

//// CONVERSIONS FROM UINT8_T --------------------------------------------------

#define UINT16_T_FROM_UINT8_T(uint8_1, uint8_0)                                \
    ((uint16_t) uint8_1 << 8) |                                                \
    ((uint16_t) uint8_0 << 0)

#define UINT32_T_FROM_UINT8_T(uint8_3, uint8_2, uint8_1, uint8_0)              \
    ((uint32_t) uint8_3 << 24) |                                               \
    ((uint32_t) uint8_2 << 16) |                                               \
    ((uint32_t) uint8_1 <<  8) |                                               \
    ((uint32_t) uint8_0 <<  0)

//// ---------------------------------------------------------------------------

// -----------------------------------------------------------------------------

// PRIVATE FUNCTIONS -----------------------------------------------------------

//// CRC -----------------------------------------------------------------------

#define CRC_POLY 0x31 // x^8 + x^5 + x^4 + 1
#define CRC_INIT 0x00
#define CRC_XOR  0x00

static uint8_t _crc8_general(
    uint8_t  *bytes ,
    uint16_t  count ,
    uint8_t   init  ,
    uint8_t   xor   )
{
    uint8_t crc = init;

    for (int i = 0; i < count; i++) {
        crc ^= bytes[i];
        for (int j = 0; j < 8; j++)
            if (crc & 0x80) crc = (crc << 1) ^ CRC_POLY;
            else            crc = (crc << 1);
        crc ^= xor;
    }

    return crc;
}

static inline uint8_t _crc8(
    uint8_t  *bytes ,
    uint16_t  count )
{ return _crc8_general(bytes, count, CRC_INIT, CRC_XOR); }

//// ---------------------------------------------------------------------------


//// Si7021 I2C ----------------------------------------------------------------

/* I2C Write */
esp_err_t _si7021_i2c_write(
    i2c_port_t  i2c_port          ,
    TickType_t  i2c_retry_timeout ,
    uint8_t    *data              ,
    size_t      data_len          )
{
    esp_err_t err = ESP_OK;

    // > Try to write until timeout
    TickType_t start_ticks = xTaskGetTickCount();
    while (
        (err = i2c_master_write_to_device(
            i2c_port,
            SI7021_I2C_ADDRESS,
            data,
            data_len,
            0
        )) != ESP_OK &&
        xTaskGetTickCount() - start_ticks < i2c_retry_timeout
    );

    return err;
}

// > I2C Read
esp_err_t _si7021_i2c_read(
    i2c_port_t  i2c_port          ,
    TickType_t  i2c_retry_timeout ,
    uint8_t    *data              ,
    size_t      data_len          )
{
    esp_err_t err = ESP_OK;

    // > Try to read until timeout
    TickType_t start_ticks = xTaskGetTickCount();
    while (
        (err = i2c_master_read_from_device(
            i2c_port,
            SI7021_I2C_ADDRESS,
            data,
            data_len,
            0
        )) != ESP_OK &&
        xTaskGetTickCount() - start_ticks < i2c_retry_timeout
    );

    return err;
}

// > Do Write then Read
esp_err_t _si7021_do_write_then_read(
    i2c_port_t  i2c_port          ,
    TickType_t  i2c_retry_timeout ,
    uint8_t    *write_data        ,
    size_t      write_data_len    ,
    uint8_t    *read_data         ,
    size_t      read_data_len     ,
    TickType_t  wait_before_read  )
{
    esp_err_t err = ESP_OK;

    // > I2C Write
    err = _si7021_i2c_write(
        i2c_port           ,
        i2c_retry_timeout  ,
        write_data         ,
        write_data_len    );
    CHECK_ERR_SI7021_I2C_WRITE(err);

    // > Wait before read
    if (wait_before_read) vTaskDelay(wait_before_read);

    // > I2C Read
    err = _si7021_i2c_read(
        i2c_port           ,
        i2c_retry_timeout  ,
        read_data          ,
        read_data_len     );
    CHECK_ERR_SI7021_I2C_READ(err);

    return err;
}

//// ---------------------------------------------------------------------------

//// Si7021 I2C Commands Util --------------------------------------------------

esp_err_t _si7021_i2c_do_measure_command(
    i2c_port_t  i2c_port          ,
    TickType_t  i2c_retry_timeout ,
    uint8_t     command           ,
    uint16_t   *out_data          ,
    bool        crc_check         ,
    TickType_t  wait_before_read  )
{
    esp_err_t err = ESP_OK;

    // > Write <command> then read <data>
    uint8_t data[3] = {0};
    err = _si7021_do_write_then_read(
        i2c_port          ,
        i2c_retry_timeout ,
        &command          ,
        1                 ,
        data              ,
        crc_check ? 3 : 2 ,
        wait_before_read  );
    CHECK_ERR_SI7021_I2C_DO_WRITE_THEN_READ(err);

    // > Check CRC
    if (crc_check) {
        uint8_t crc = _crc8(&data[0], 2);
        CHECK_ERR_COMPARE_MEASURE_CRC(data[0], data[1], crc, data[2]);
    }

    // > Parse Data
    *out_data = UINT16_T_FROM_UINT8_T(data[0], data[1]);

    return err;
}

esp_err_t _si7021_i2c_do_read_reg_command(
    i2c_port_t  i2c_port          ,
    TickType_t  i2c_retry_timeout ,
    uint8_t     command           ,
    uint8_t    *out_reg_value     ,
    TickType_t  wait_before_read  )
{
    esp_err_t err = ESP_OK;

    // > Write <command> then read <reg_value>
    uint8_t reg_value;
    err = _si7021_do_write_then_read(
        i2c_port          ,
        i2c_retry_timeout ,
        &command          ,
        1                 ,
        &reg_value        ,
        1                 ,
        wait_before_read  );
    CHECK_ERR_SI7021_I2C_DO_WRITE_THEN_READ(err);

    // > Parse Data
    *out_reg_value = reg_value;

    return err;
}

//// ---------------------------------------------------------------------------

// -----------------------------------------------------------------------------

// PUBLIC FUNCTIONS ------------------------------------------------------------

//// Create & Delete -----------------------------------------------------------

// > Create Si7021 I2C Handle
esp_err_t si7021_i2c_create(
    si7021_i2c_create_args_t const *create_args ,
    si7021_i2c_handle_t            *out_handle  )
{
    CHECK_ERR_CREATE_ARGS(create_args);
    CHECK_ERR_OUT_HANDLE(out_handle);
    esp_err_t err = ESP_OK;

    // > I2C Driver Install?
    if (create_args->i2c_driver.install) {

        // > I2C Set Config?
        if (create_args->i2c_driver.set_config) {
            CHECK_ERR_I2C_MODE_IS_MASTER(create_args->i2c_driver.config.mode);
            LOG_I2C_DRIVER_CONFIG(create_args->i2c_driver.config);

            err = i2c_param_config(
                create_args->i2c_port            ,
                &create_args->i2c_driver.config );
            CHECK_ERR_I2C_PARAM_CONFIG(err);
        }

        // > I2C Driver Install
        ESP_LOGI(TAG, "Installing I2C driver...");
        err = i2c_driver_install(
            create_args->i2c_port                ,
            create_args->i2c_driver.config.mode  ,
            0, 0, 0                             );
        CHECK_ERR_I2C_DRIVER_INSTALL(err);
    }

    // > Allocate Memory for Handle
    *out_handle = malloc(sizeof(struct si7021_i2c_handle));
    CHECK_ERR_MALLOC(*out_handle);

    // > Set Handle
    /* Name */
    strncpy(
        (*out_handle)->name            ,
        create_args->name              ,
        SI7021_I2C_HANDLE_NAME_LENGTH );

    /* Port, Retry Timeout & Uninstall Flag */
    (*out_handle)->i2c_port = create_args->i2c_port;
    (*out_handle)->i2c_retry_timeout = create_args->i2c_retry_timeout;
    (*out_handle)->i2c_uninstall_at_delete =
        create_args->i2c_driver.install &&
        create_args->i2c_driver.uninstall_at_delete;
    
    return err;
}

// > Delete Si7021 I2C Handle
esp_err_t si7021_i2c_delete(
    si7021_i2c_handle_t si7021_i2c )
{
    CHECK_ERR_HANDLE(si7021_i2c);
    esp_err_t err = ESP_OK;

    // > Uninstall I2C Driver?
    if (si7021_i2c->i2c_uninstall_at_delete) {
        ESP_LOGI(TAG, "Uninstalling I2C driver...");
        err = i2c_driver_delete(si7021_i2c->i2c_port);
        CHECK_WARN_I2C_DRIVER_DELETE(err);
    }

    // > Free Memory
    free(si7021_i2c);

    return err;
}

//// ---------------------------------------------------------------------------

//// Measure -------------------------------------------------------------------

/* Measure Relative Humidity (Hold Master) */
esp_err_t si7021_i2c_measure_rh_hold_master(
    si7021_i2c_handle_t const  si7021_i2c       ,
    uint16_t                  *out_rh           ,
    bool                       crc_check        ,
    TickType_t                 wait_before_read )
{
    CHECK_ERR_HANDLE(si7021_i2c);
    CHECK_ERR_OUT_PARAM(out_rh);
    return _si7021_i2c_do_measure_command(
        si7021_i2c->i2c_port                  ,
        si7021_i2c->i2c_retry_timeout         ,
        SI7021_I2C_CMD_MEASURE_RH_HOLD_MASTER ,
        out_rh                                ,
        crc_check                             ,
        wait_before_read                     );
}

/* Measure Relative Humidity (No Hold Master) */
esp_err_t si7021_i2c_measure_rh_nohold_master(
    si7021_i2c_handle_t const  si7021_i2c       ,
    uint16_t                  *out_rh           ,
    bool                       crc_check        ,
    TickType_t                 wait_before_read )
{
    CHECK_ERR_HANDLE(si7021_i2c);
    CHECK_ERR_OUT_PARAM(out_rh);
    return _si7021_i2c_do_measure_command(
        si7021_i2c->i2c_port                    ,
        si7021_i2c->i2c_retry_timeout           ,
        SI7021_I2C_CMD_MEASURE_RH_NOHOLD_MASTER ,
        out_rh                                  ,
        crc_check                               ,
        wait_before_read                       );
}

/* Measure Temperature (Hold Master) */
esp_err_t si7021_i2c_measure_temp_hold_master(
    si7021_i2c_handle_t const  si7021_i2c       ,
    uint16_t                  *out_temp         ,
    bool                       crc_check        ,
    TickType_t                 wait_before_read )
{
    CHECK_ERR_HANDLE(si7021_i2c);
    CHECK_ERR_OUT_PARAM(out_temp);
    return _si7021_i2c_do_measure_command(
        si7021_i2c->i2c_port                    ,
        si7021_i2c->i2c_retry_timeout           ,
        SI7021_I2C_CMD_MEASURE_TEMP_HOLD_MASTER ,
        out_temp                                ,
        crc_check                               ,
        wait_before_read                       );
}

/* Measure Temperature (No Hold Master) */
esp_err_t si7021_i2c_measure_temp_nohold_master(
    si7021_i2c_handle_t const  si7021_i2c       ,
    uint16_t                  *out_temp         ,
    bool                       crc_check        ,
    TickType_t                 wait_before_read )
{
    CHECK_ERR_HANDLE(si7021_i2c);
    CHECK_ERR_OUT_PARAM(out_temp);
    return _si7021_i2c_do_measure_command(
        si7021_i2c->i2c_port                      ,
        si7021_i2c->i2c_retry_timeout             ,
        SI7021_I2C_CMD_MEASURE_TEMP_NOHOLD_MASTER ,
        out_temp                                  ,
        crc_check                                 ,
        wait_before_read                         );
}

/* Read Temperature Value From Previous RH Measurement */
esp_err_t si7021_i2c_read_temp_from_prev_rh_measurement(
    si7021_i2c_handle_t const  si7021_i2c       ,
    uint16_t                  *out_temp         ,
    TickType_t                 wait_before_read )
{
    CHECK_ERR_HANDLE(si7021_i2c);
    CHECK_ERR_OUT_PARAM(out_temp);
    return _si7021_i2c_do_measure_command(
        si7021_i2c->i2c_port                              ,
        si7021_i2c->i2c_retry_timeout                     ,
        SI7021_I2C_CMD_READ_TEMP_FROM_PREV_RH_MEASUREMENT ,
        out_temp                                          ,
        false                                             ,
        wait_before_read                                 );
}

//// ---------------------------------------------------------------------------

//// Reset & Registers ---------------------------------------------------------

/* Reset */
esp_err_t si7021_i2c_reset(
    si7021_i2c_handle_t const si7021_i2c )
{
    CHECK_ERR_HANDLE(si7021_i2c);
    esp_err_t err = ESP_OK;

    // > Write Reset Command
    err = _si7021_i2c_write(
        si7021_i2c->i2c_port               ,
        si7021_i2c->i2c_retry_timeout      ,
        (uint8_t[]) {SI7021_I2C_CMD_RESET} ,
        1                                 );
    CHECK_ERR_SI7021_I2C_WRITE(err);

    return err;
}

/* Write RH/T User Register 1 */
esp_err_t si7021_i2c_write_user_reg_1(
    si7021_i2c_handle_t const si7021_i2c ,
    uint8_t                   reg_value  )
{
    CHECK_ERR_HANDLE(si7021_i2c);
    esp_err_t err = ESP_OK;

    // > Write RH/T User Register 1
    err = _si7021_i2c_write(
        si7021_i2c->i2c_port                                          ,
        si7021_i2c->i2c_retry_timeout                                 ,
        (uint8_t[]) {SI7021_I2C_CMD_WRITE_RH_T_USER_REG_1, reg_value} ,
        2                                                            );
    CHECK_ERR_SI7021_I2C_WRITE(err);

    return err;
}

/* Read RH/T User Register 1 */
esp_err_t si7021_i2c_read_user_reg_1(
    si7021_i2c_handle_t const  si7021_i2c       ,
    uint8_t                   *out_reg_value    ,
    TickType_t                 wait_before_read )
{
    CHECK_ERR_HANDLE(si7021_i2c);
    CHECK_ERR_OUT_PARAM(out_reg_value);
    return _si7021_i2c_do_read_reg_command(
        si7021_i2c->i2c_port                ,
        si7021_i2c->i2c_retry_timeout       ,
        SI7021_I2C_CMD_READ_RH_T_USER_REG_1 ,
        out_reg_value                       ,
        wait_before_read                   );
}

/* Write Heater Control Register */
esp_err_t si7021_i2c_write_heater_control_reg(
    si7021_i2c_handle_t const si7021_i2c ,
    uint8_t                   reg_value  )
{
    CHECK_ERR_HANDLE(si7021_i2c);
    esp_err_t err = ESP_OK;

    // > Write Heater Control Register
    err = _si7021_i2c_write(
        si7021_i2c->i2c_port                                             ,
        si7021_i2c->i2c_retry_timeout                                    ,
        (uint8_t[]) {SI7021_I2C_CMD_WRITE_HEATER_CONTROL_REG, reg_value} ,
        2                                                               );
    CHECK_ERR_SI7021_I2C_WRITE(err);

    return err;
}

/* Read Heater Control Register */
esp_err_t si7021_i2c_read_heater_control_reg(
    si7021_i2c_handle_t const  si7021_i2c       ,
    uint8_t                   *out_reg_value    ,
    TickType_t                 wait_before_read )
{
    CHECK_ERR_HANDLE(si7021_i2c);
    CHECK_ERR_OUT_PARAM(out_reg_value);
    return _si7021_i2c_do_read_reg_command(
        si7021_i2c->i2c_port                   ,
        si7021_i2c->i2c_retry_timeout          ,
        SI7021_I2C_CMD_READ_HEATER_CONTROL_REG ,
        out_reg_value                          ,
        wait_before_read                      );
}

//// ---------------------------------------------------------------------------

//// Serial Number & Firmware --------------------------------------------------

/* Read Electronic ID First Bytes */
esp_err_t si7021_i2c_read_electronic_id_first_bytes(
    si7021_i2c_handle_t const  si7021_i2c          ,
    uint32_t                  *out_eid_first_bytes ,
    bool                       crc_check           ,
    TickType_t                 wait_before_read    )
{
    CHECK_ERR_HANDLE(si7021_i2c);
    CHECK_ERR_OUT_PARAM(out_eid_first_bytes);
    esp_err_t err = ESP_OK;

    // > Write commands then read data
    uint8_t data[8] = {0};
    err = _si7021_do_write_then_read(
        si7021_i2c->i2c_port                                 ,
        si7021_i2c->i2c_retry_timeout                        ,
        (uint8_t[]) {
            SI7021_I2C_CMD_READ_ELECTRONIC_ID_FIRST_BYTES_A  ,
            SI7021_I2C_CMD_READ_ELECTRONIC_ID_FIRST_BYTES_B },
        2                                                    ,
        data                                                 ,
        8                                                    ,
        wait_before_read                                    );
    CHECK_ERR_SI7021_I2C_DO_WRITE_THEN_READ(err);

    // > Check CRC
    if (crc_check) {
        uint8_t init = CRC_INIT;
        for (int i = 0; i < 8; i += 2) {
            uint8_t crc = _crc8_general(&data[i], 1, init, CRC_XOR);
            CHECK_ERR_COMPARE_FIRST_EID_CRC(init, data[i], crc, data[i + 1]);
            init = crc;
        }
    }

    // > Parse Data
    *out_eid_first_bytes = UINT32_T_FROM_UINT8_T(
        data[0], data[1], data[2], data[3]);

    return err;
}

/* Read Electronic ID Last Bytes */
esp_err_t si7021_i2c_read_electronic_id_last_bytes(
    si7021_i2c_handle_t const  si7021_i2c         ,
    uint32_t                  *out_eid_last_bytes ,
    bool                       crc_check          ,
    TickType_t                 wait_before_read   )
{
    CHECK_ERR_HANDLE(si7021_i2c);
    CHECK_ERR_OUT_PARAM(out_eid_last_bytes);
    esp_err_t err = ESP_OK;

    // > Write commands then read data
    uint8_t data[6];
    err = _si7021_do_write_then_read(
        si7021_i2c->i2c_port                                ,
        si7021_i2c->i2c_retry_timeout                       ,
        (uint8_t[]) {
            SI7021_I2C_CMD_READ_ELECTRONIC_ID_LAST_BYTES_A  ,
            SI7021_I2C_CMD_READ_ELECTRONIC_ID_LAST_BYTES_B },
        2                                                   ,
        data                                                ,
        6                                                   ,
        wait_before_read                                   );
    CHECK_ERR_SI7021_I2C_DO_WRITE_THEN_READ(err);

    // > Check CRC
    if (crc_check) {
        uint8_t init = CRC_INIT;
        for (int i = 0; i < 6; i += 3) {
            uint8_t crc = _crc8_general(&data[i], 2, init, CRC_XOR);
            CHECK_ERR_COMPARE_LAST_EID_CRC(
                init, data[i], data[i + 1], crc, data[i + 2]);
            init = crc;
        }
    }

    // > Parse Data
    *out_eid_last_bytes = UINT32_T_FROM_UINT8_T(
        data[0], data[1], data[3], data[4]);

    return err;
}

/* Read Firmware Revision */
esp_err_t si7021_i2c_read_firmware_revision(
    si7021_i2c_handle_t const  si7021_i2c            ,
    uint8_t                   *out_firmware_revision ,
    TickType_t                 wait_before_read      )
{
    CHECK_ERR_HANDLE(si7021_i2c);
    CHECK_ERR_OUT_PARAM(out_firmware_revision);
    esp_err_t err = ESP_OK;

    // > Write commands then read data
    err = _si7021_do_write_then_read(
        si7021_i2c->i2c_port                         ,
        si7021_i2c->i2c_retry_timeout                ,
        (uint8_t[]) {
            SI7021_I2C_CMD_READ_FIRMWARE_REVISION_A  ,
            SI7021_I2C_CMD_READ_FIRMWARE_REVISION_B },
        2                                            ,
        out_firmware_revision                        ,
        1                                            ,
        wait_before_read                            );
    CHECK_ERR_SI7021_I2C_DO_WRITE_THEN_READ(err);
    
    return err;
}

//// ---------------------------------------------------------------------------

// -----------------------------------------------------------------------------