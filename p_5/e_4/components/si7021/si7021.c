#include "si7021.h"

// INCLUDES --------------------------------------------------------------------

#include "string.h"     // for strncpy
#include "esp_err.h"    // for ESP errors
#include "si7021_i2c.h" // for Si7021 I2C API

/* Logging */
#include "esp_log.h"
static const char *TAG = "Si7021";

// -----------------------------------------------------------------------------

// STRUCTURES ------------------------------------------------------------------

/* Si7021 Handle */
struct si7021_handle {
    char name[SI7021_HANDLE_NAME_LENGTH]; // Handle name

    si7021_i2c_handle_t i2c;        // Si7021 I2C handle
    si7021_crc_config_t crc_config; // CRC config
    si7021_read_wait_t  read_wait;  // Read wait timeouts
};

// -----------------------------------------------------------------------------

// MACROS ----------------------------------------------------------------------

//// CREATE & DELETE -----------------------------------------------------------

#define CHECK_ERR_CREATE_ARGS(create_args)                                     \
    if (!create_args) {                                                        \
        ESP_LOGE(TAG, "Pointer to si7021_create_args_t is NULL.");             \
        return ESP_ERR_INVALID_ARG;                                            \
    }

#define CHECK_ERR_OUT_HANDLE(out_handle)                                       \
    if (!out_handle) {                                                         \
        ESP_LOGE(TAG, "Pointer to si7021_handle_t is NULL.");                  \
        return ESP_ERR_INVALID_ARG;                                            \
    }

#define CHECK_ERR_MALLOC(ptr)                                                  \
    if (!ptr) {                                                                \
        ESP_LOGE(TAG, "Could not allocate memory.");                           \
        return ESP_ERR_NO_MEM;                                                 \
    }

#define CHECK_ERR_SI7021_I2C_CREATE(err)                                       \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Could not create Si7021 I2C handle.");                  \
        return err;                                                            \
    }

#define LOG_ERR_SI7021_AT_INIT(err, phase)                                     \
    ESP_LOGE(TAG, "Could not %s Si7021 at init.", phase);                      \
    return err;

#define CHECK_WARN_SI7021_I2C_DELETE(err)                                      \
    if (err) {                                                                 \
        ESP_LOGW(TAG, "Could not delete Si7021 I2C handle: %s",                \
            esp_err_to_name(err));                                             \
    }

//// ---------------------------------------------------------------------------

//// HANDLE & IN/OUT PARAM ERRORS ----------------------------------------------

#define CHECK_ERR_HANDLE(handle)                                               \
    if (!handle) {                                                             \
        ESP_LOGE(TAG, "Si7021 handle is NULL.");                               \
        return ESP_ERR_INVALID_ARG;                                            \
    }

#define CHECK_ERR_IN_PARAM(in_param)                                           \
    if (!in_param) {                                                           \
        ESP_LOGE(TAG, "Pointer to in parameter is NULL.");                     \
        return ESP_ERR_INVALID_ARG;                                            \
    }

#define CHECK_ERR_OUT_PARAM(out_param)                                         \
    if (!out_param) {                                                          \
        ESP_LOGE(TAG, "Pointer to out parameter is NULL.");                    \
        return ESP_ERR_INVALID_ARG;                                            \
    }

//// ---------------------------------------------------------------------------

//// MEASUREMENT ERRORS --------------------------------------------------------

#define CHECK_ERR_SI7021_I2C_MEASURE_RH(err)                                   \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Si7021 I2C measure RH command failed.");                \
        return err;                                                            \
    }

#define CHECK_ERR_SI7021_I2C_MEASURE_TEMP(err)                                 \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Si7021 I2C measure temp command failed.");              \
        return err;                                                            \
    }

#define CHECK_ERR_SI7021_I2C_MEASURE_TEMP_FROM_PREV_RH_MEASUREMENT(err)        \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Si7021 I2C read temp from previous RH measurement "     \
                      "command failed."                                        \
        );                                                                     \
        return err;                                                            \
    }

//// ---------------------------------------------------------------------------

#define CHECK_ERR_SI7021_I2C_RESET(err)                                        \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Si7021 I2C command failed.");                           \
        return err;                                                            \
    }

//// USER REGISTER CONTROL ERRORS ----------------------------------------------

#define CHECK_ERR_SI7021_I2C_READ_USER_REG(err)                                \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Si7021 I2C read user register command failed.");        \
        return err;                                                            \
    }

#define CHECK_ERR_SI7021_I2C_WRITE_USER_REG(err)                               \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Si7021 I2C write user register command failed.");       \
        return err;                                                            \
    }

//// ---------------------------------------------------------------------------

//// HEATER REGISTER CONTROL ERRORS --------------------------------------------

#define CHECK_ERR_SI7021_I2C_READ_HEATER_CONTROL_REG(err)                      \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Si7021 I2C read heater control register command"        \
                      " failed.");                                             \
        return err;                                                            \
    }

#define CHECK_ERR_SI7021_I2C_WRITE_HEATER_CONTROL_REG(err)                     \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Si7021 I2C write heater control register command"       \
                      " failed.");                                             \
        return err;                                                            \
    }

//// ---------------------------------------------------------------------------

//// SERIAL NUMBER & FIRMWARE ERRORS -------------------------------------------

#define CHECK_ERR_SI7021_I2C_READ_ELECTRONIC_ID_FIRST_BYTES(err)               \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Si7021 I2C read EID first bytes command failed.");      \
        return err;                                                            \
    }

#define CHECK_ERR_SI7021_I2C_READ_ELECTRONIC_ID_LAST_BYTES(err)                \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Si7021 I2C read EID last bytes command failed.");       \
        return err;                                                            \
    }

#define UINT64_T_FROM_UINT32_T(uint32_1, uint32_0)                             \
    ((uint64_t)uint32_1 << 32) |                                               \
    ((uint64_t)uint32_0 <<  0)

#define ID_FROM_SNB(snb) (si7021_sn_id_t)((snb >> 24) & 0xFF)

#define CHECK_ERR_SI7021_READ_FIRWARE_REVISION(err)                            \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Si7021 I2C read firmware revision command failed.");    \
        return err;                                                            \
    }

//// ---------------------------------------------------------------------------

// -----------------------------------------------------------------------------

// PRIVATE FUNCTIONS -----------------------------------------------------------

/* Create: at init */
esp_err_t _si7021_at_init(
    si7021_handle_t      const  si7021      ,
    si7021_create_args_t const *create_args )
{
    esp_err_t err = ESP_OK;
    
    // > Reset?
    if (
        create_args->at_init.reset &&
        (err = si7021_reset(si7021))
    ) LOG_ERR_SI7021_AT_INIT(err, "reset");

    // > Set heater level?
    if (
        create_args->at_init.set_heater_level &&
        (err = si7021_set_heater_level(
            si7021,
            create_args->at_init.heater_level
        )) != ESP_OK
    ) LOG_ERR_SI7021_AT_INIT(err, "set heater level");

    // > Set user register info?
    if (
        create_args->at_init.set_user_register_info &&
        (err = si7021_set_user_register_info(
            si7021,
            &create_args->at_init.user_register_info
        )) != ESP_OK
    ) LOG_ERR_SI7021_AT_INIT(err, "set user register info");

    // > Dump device info?
    if (
        create_args->at_init.dump.device_info &&
        (err = si7021_dump_device_info(si7021))
    ) LOG_ERR_SI7021_AT_INIT(err, "dump device info");

    // > Dump CRC config?
    if (
        create_args->at_init.dump.crc_config &&
        (err = si7021_dump_crc_config(si7021))
    ) LOG_ERR_SI7021_AT_INIT(err, "dump CRC config");

    // > Dump read wait?
    if (
        create_args->at_init.dump.read_wait &&
        (err = si7021_dump_read_wait(si7021))
    ) LOG_ERR_SI7021_AT_INIT(err, "dump read wait timeouts");
    
    return err;
}

/* Si7021 I2C Measure RH */
inline esp_err_t _si7021_i2c_measure_rh(
    si7021_i2c_handle_t  i2c           ,
    uint16_t            *out_rh_code   ,
    bool                 crc_config    ,
    TickType_t           read_wait     )
{
#ifdef CONFIG_SI7021_USE_MEASURE_RH_HOLD_MASTER
    return si7021_i2c_measure_rh_hold_master(
        i2c,
        out_rh_code,
        crc_config,
        read_wait
    );
#else
    return si7021_i2c_measure_rh_nohold_master(
        i2c,
        out_rh_code,
        crc_config,
        read_wait
    );
#endif
}

/* Si7021 I2C Measure Temp */
inline esp_err_t _si7021_i2c_measure_temp(
    si7021_i2c_handle_t  i2c           ,
    uint16_t            *out_temp_code ,
    bool                 crc_config    ,
    TickType_t           read_wait     )
{
#ifdef SI7021_USE_MEASURE_TEMP_HOLD_MASTER
    return si7021_i2c_measure_temp_hold_master(
        i2c,
        out_temp_code,
        crc_config,
        read_wait
    );
#else
    return si7021_i2c_measure_temp_nohold_master(
        i2c,
        out_temp_code,
        crc_config,
        read_wait
    );
#endif
}

// -----------------------------------------------------------------------------

// PUBLIC FUNCTIONS ------------------------------------------------------------

//// Conversion ----------------------------------------------------------------

/* Get RH percentage from 16-bit RH code */
float si7021_convert_rh(
    uint16_t rh_code )
{
    float rh = (125.0 * (float) rh_code) / 65536.0 - 6.0;
    return rh < 0.0   ? 0.0 
         : rh > 100.0 ? 100.0
         : rh;
}

/* Get temperature in Celsius from 16-bit temperature code*/
float si7021_convert_temp(
    uint16_t temp_code )
{ return (175.72 * (float) temp_code) / 65536.0 - 46.85; }

/* Get current (mA) from heater level code */
float si7021_calc_heater_level(
    uint8_t heater_level )
{
    const static float value_0000 = 3.09;
    const static float value_0001 = 9.18;
    const static float value_0010 = 15.24;
    const static float value_0100 = 27.39;
    const static float value_1000 = 51.69;
    const static float value_1111 = 94.20;
    
    const static float step_1111_to_1000 = (value_1111 - value_1000) /
                                           (0b1111 - 0b1000)         ;
    const static float step_1000_to_0100 = (value_1000 - value_0100) /
                                           (0b1000 - 0b0100)         ;
    
    // > Apply mask
    heater_level &= SI7021_HEATER_REGISTER_MASK_HEATER_LEVEL;

    switch (heater_level) {
        case SI7021_HEATER_LEVEL_3_09_MA:  return value_0000;
        case SI7021_HEATER_LEVEL_9_18_MA:  return value_0001;
        case SI7021_HEATER_LEVEL_15_24_MA: return value_0010;
        case SI7021_HEATER_LEVEL_27_39_MA: return value_0100;
        case SI7021_HEATER_LEVEL_51_69_MA: return value_1000;
        case SI7021_HEATER_LEVEL_94_20_MA: return value_1111;
        default: {
            /* Between 1111 and 1000 */
            if (heater_level & 0b1000) {
                return value_1000 + step_1111_to_1000 * (heater_level - 0b1000);
            }
            /* Between 1000 and 0100 */
            else if (heater_level & 0b0100) {
                return value_0100 + step_1000_to_0100 * (heater_level - 0b0100);
            }
            /* Between 0100 and 0000: {0000, 0001, 0010, 0011}
             * then it is 0011 (mean of 0100 and 0010)
             */
            else {
                return (value_0100 + value_0010) / 2;
            }
        }
    }
}

//// ---------------------------------------------------------------------------

//// Create & Delete -----------------------------------------------------------

// > Create Si7021 Handle
esp_err_t si7021_create(
    si7021_create_args_t const *create_args ,
    si7021_handle_t            *out_handle  )
{
    CHECK_ERR_CREATE_ARGS(create_args);
    CHECK_ERR_OUT_HANDLE(out_handle);
    esp_err_t err = ESP_OK;

    // > Allocate Memory for Handle
    *out_handle = malloc(sizeof(struct si7021_handle));
    CHECK_ERR_MALLOC(*out_handle);

    // > Create Si7021 I2C handle
    si7021_i2c_handle_t i2c = NULL;
    err = si7021_i2c_create(&create_args->i2c, &i2c);
    CHECK_ERR_SI7021_I2C_CREATE(err);

    // > Set Handle
    /* Name */
    strncpy((*out_handle)->name, create_args->name, SI7021_HANDLE_NAME_LENGTH);

    /* Si7021 I2C handle, CRC config & Read Wait Timeouts */
    (*out_handle)->i2c = i2c;
    (*out_handle)->crc_config = create_args->crc_config;
    (*out_handle)->read_wait = create_args->read_wait;

    // > Do at init
    if ( (err = _si7021_at_init(*out_handle, create_args)) ) free(*out_handle);
    
    return err;
}

// > Delete Si7021 Handle
esp_err_t si7021_delete(
    si7021_handle_t si7021 )
{
    CHECK_ERR_HANDLE(si7021);
    esp_err_t err = ESP_OK;

    // > Delete Si7021 I2C handle
    err = si7021_i2c_delete(si7021->i2c);
    CHECK_WARN_SI7021_I2C_DELETE(err);

    // FREE HANDLE //
    free(si7021);

    // Return
    return err;
}

//// ---------------------------------------------------------------------------

//// CRC Config ----------------------------------------------------------------

/* Get CRC Config */
esp_err_t si7021_get_crc_config(
    si7021_handle_t     const  si7021         ,
    si7021_crc_config_t       *out_crc_config )
{
    CHECK_ERR_HANDLE(si7021);
    CHECK_ERR_OUT_PARAM(out_crc_config);
    *out_crc_config = si7021->crc_config;
    return ESP_OK;
}

// > Set CRC Config
esp_err_t si7021_set_crc_config(
    si7021_handle_t     const  si7021     ,
    si7021_crc_config_t const *crc_config )
{
    CHECK_ERR_HANDLE(si7021);
    CHECK_ERR_IN_PARAM(crc_config);
    si7021->crc_config = *crc_config;
    return ESP_OK;
}

//// ---------------------------------------------------------------------------

//// Read Wait Timeouts --------------------------------------------------------

/* Get Read Wait Timeouts */
esp_err_t si7021_get_read_wait(
    si7021_handle_t    const  si7021        ,
    si7021_read_wait_t       *out_read_wait )
{
    CHECK_ERR_HANDLE(si7021);
    CHECK_ERR_OUT_PARAM(out_read_wait);
    *out_read_wait = si7021->read_wait;
    return ESP_OK;
}

/* Set Read Wait Timeouts */
esp_err_t si7021_set_read_wait(
    si7021_handle_t    const  si7021    ,
    si7021_read_wait_t const *read_wait )
{
    CHECK_ERR_HANDLE(si7021);
    CHECK_ERR_IN_PARAM(read_wait);
    si7021->read_wait = *read_wait;
    return ESP_OK;
}

//// ---------------------------------------------------------------------------

//// Measurement ---------------------------------------------------------------

/* Measure Relative Humidity */
esp_err_t si7021_measure_rh(
    si7021_handle_t const  si7021         ,
    float                 *out_rh_percent )
{
    CHECK_ERR_HANDLE(si7021);
    CHECK_ERR_OUT_PARAM(out_rh_percent);
    esp_err_t err = ESP_OK;

    // > Measure Relative Humidity
    uint16_t   rh_code   = 0;
    bool       crc_check = si7021->crc_config.global || si7021->crc_config.rh;
    TickType_t read_wait = si7021->read_wait.global
        ? si7021->read_wait.global
        : si7021->read_wait.rh;
    err = _si7021_i2c_measure_rh(
        si7021->i2c ,
        &rh_code    ,
        crc_check   ,
        read_wait  );
    CHECK_ERR_SI7021_I2C_MEASURE_RH(err);

    // > Convert
    *out_rh_percent = si7021_convert_rh(rh_code);

    return err;
}

/* Measure Temperature */
esp_err_t si7021_measure_temp(
    si7021_handle_t const  si7021           ,
    float                 *out_temp_celsius )
{
    CHECK_ERR_HANDLE(si7021);
    CHECK_ERR_OUT_PARAM(out_temp_celsius);
    esp_err_t err = ESP_OK;

    // > Measure Temperature
    uint16_t   temp_code = 0;
    bool       crc_check = si7021->crc_config.global || si7021->crc_config.temp;
    TickType_t read_wait = si7021->read_wait.global
        ? si7021->read_wait.global
        : si7021->read_wait.temp;
    err = _si7021_i2c_measure_temp(
        si7021->i2c ,
        &temp_code  ,
        crc_check   ,
        read_wait  );
    CHECK_ERR_SI7021_I2C_MEASURE_TEMP(err);

    // > Convert
    *out_temp_celsius = si7021_convert_temp(temp_code);

    return err;
}

/* Measure RH and Temp */
esp_err_t si7021_measure_rh_and_temp(
    si7021_handle_t const  si7021           ,
    float                 *out_rh_percent   ,
    float                 *out_temp_celsius )
{
    CHECK_ERR_OUT_PARAM(out_temp_celsius);
    esp_err_t err = ESP_OK;

    // > Call si7021_measure_rh
    err = si7021_measure_rh(si7021, out_rh_percent);
    if (err) return err;

    // > Measure Temperature from previous RH measurement
    uint16_t   temp_code = 0;
    TickType_t read_wait = si7021->read_wait.global
        ? si7021->read_wait.global
        : si7021->read_wait.temp;
    err = si7021_i2c_read_temp_from_prev_rh_measurement(
        si7021->i2c ,
        &temp_code  ,
        read_wait  );
    CHECK_ERR_SI7021_I2C_MEASURE_TEMP_FROM_PREV_RH_MEASUREMENT(err);

    // > Convert
    *out_temp_celsius = si7021_convert_temp(temp_code);

    return err;
}

//// ---------------------------------------------------------------------------

/* Reset */
esp_err_t si7021_reset(
    si7021_handle_t const si7021 )
{
    CHECK_ERR_HANDLE(si7021);
    esp_err_t err = ESP_OK;
    err = si7021_i2c_reset(si7021->i2c);
    CHECK_ERR_SI7021_I2C_RESET(err);
    return err;
}

//// User Register Control -----------------------------------------------------

/* Get User Register Info */
esp_err_t si7021_get_user_register_info(
    si7021_handle_t             const  si7021                 ,
    uint8_t                           *out_user_register      ,
    si7021_user_register_info_t       *out_user_register_info )
{
    CHECK_ERR_HANDLE(si7021);
    CHECK_ERR_OUT_PARAM(out_user_register_info);
    esp_err_t err = ESP_OK;

    // > Read User Register
    uint8_t    user_register = 0;
    TickType_t read_wait     = si7021->read_wait.global
        ? si7021->read_wait.global
        : si7021->read_wait.user_reg;
    err = si7021_i2c_read_user_reg_1(si7021->i2c, &user_register, read_wait);
    CHECK_ERR_SI7021_I2C_READ_USER_REG(err);

    // > User Register Info
    if (out_user_register) *out_user_register = user_register;
    out_user_register_info->resolution   = (si7021_resolution_t)(
        user_register & SI7021_USER_REGISTER_MASK_RESOLUTION);
    out_user_register_info->heater_state = (si7021_heater_state_t)(
        user_register & SI7021_USER_REGISTER_MASK_HEATER_STATE);
    out_user_register_info->vdd_status   = (si7021_vdd_status_t)(
        user_register & SI7021_USER_REGISTER_MASK_VDD_STATUS);

    return err;
}


/* Set User Register Info */
esp_err_t si7021_set_user_register_info(
    si7021_handle_t             const  si7021             ,
    si7021_user_register_info_t const *user_register_info )
{
    CHECK_ERR_HANDLE(si7021);
    CHECK_ERR_IN_PARAM(user_register_info);
    esp_err_t err = ESP_OK;

    // > Write User Register
    uint8_t user_register  = SI7021_USER_REGISTER_DEFAULT;
            user_register |=
                (uint8_t)(user_register_info->resolution) &
                SI7021_USER_REGISTER_MASK_RESOLUTION      ;
            user_register |=
                (uint8_t)(user_register_info->heater_state) &
                SI7021_USER_REGISTER_MASK_HEATER_STATE      ;
    /* vdd_status is read only */
    err = si7021_i2c_write_user_reg_1(si7021->i2c, user_register);
    CHECK_ERR_SI7021_I2C_WRITE_USER_REG(err);

    return err;
}
//// ---------------------------------------------------------------------------

//// User Register Control (Derived) -------------------------------------------

/* Get Resolution */
esp_err_t si7021_get_resolution(
    si7021_handle_t     const  si7021         ,
    si7021_resolution_t       *out_resolution )
{
    CHECK_ERR_OUT_PARAM(out_resolution);
    esp_err_t err = ESP_OK;
    si7021_user_register_info_t user_register_info;
    err = si7021_get_user_register_info(si7021, NULL, &user_register_info);
    if (err) return err;
    *out_resolution = user_register_info.resolution;
    return err;
}

/* Get Heater State */
esp_err_t si7021_get_heater_state(
    si7021_handle_t       const  si7021           ,
    si7021_heater_state_t       *out_heater_state )
{
    CHECK_ERR_OUT_PARAM(out_heater_state);
    esp_err_t err = ESP_OK;
    si7021_user_register_info_t user_register_info;
    err = si7021_get_user_register_info(si7021, NULL, &user_register_info);
    if (err) return err;
    *out_heater_state = user_register_info.heater_state;
    return err;
}

/* Get VDD Status */
esp_err_t si7021_get_vdd_status(
    si7021_handle_t     const  si7021         ,
    si7021_vdd_status_t       *out_vdd_status )
{
    CHECK_ERR_OUT_PARAM(out_vdd_status);
    esp_err_t err = ESP_OK;
    si7021_user_register_info_t user_register_info;
    err = si7021_get_user_register_info(si7021, NULL, &user_register_info);
    if (err) return err;
    *out_vdd_status = user_register_info.vdd_status;
    return err;
}

/* Set Resolution */
esp_err_t si7021_set_resolution(
    si7021_handle_t     const si7021     ,
    si7021_resolution_t       resolution )
{
    esp_err_t err = ESP_OK;

    // > Get User Register Info
    si7021_user_register_info_t user_register_info;
    err = si7021_get_user_register_info(si7021, NULL, &user_register_info);
    if (err) return err;

    // > Set Resolution
    user_register_info.resolution = resolution;

    // > Set User Register Info
    err = si7021_set_user_register_info(si7021, &user_register_info);
    return err;
}

/* Set Heater State */
esp_err_t si7021_set_heater_state(
    si7021_handle_t       const si7021       ,
    si7021_heater_state_t       heater_state )
{
    esp_err_t err = ESP_OK;

    // > Get User Register Info
    si7021_user_register_info_t user_register_info;
    err = si7021_get_user_register_info(si7021, NULL, &user_register_info);
    if (err) return err;

    // > Set Heater State
    user_register_info.heater_state = heater_state;

    // > Set User Register Info
    err = si7021_set_user_register_info(si7021, &user_register_info);
    return err;
}

//// ---------------------------------------------------------------------------

//// Heater Register Control ---------------------------------------------------

/* Get Heater Level */
esp_err_t si7021_get_heater_level(
    si7021_handle_t       const  si7021           ,
    si7021_heater_level_t       *out_heater_level )
{
    CHECK_ERR_HANDLE(si7021);
    CHECK_ERR_OUT_PARAM(out_heater_level);
    esp_err_t err = ESP_OK;

    // > Read Heater Register
    uint8_t    heater_register = 0;
    TickType_t read_wait       = si7021->read_wait.global
        ? si7021->read_wait.global
        : si7021->read_wait.heater_reg;
    err = si7021_i2c_read_heater_control_reg(
        si7021->i2c      ,
        &heater_register ,
        read_wait       );
    CHECK_ERR_SI7021_I2C_READ_HEATER_CONTROL_REG(err);

    // > Heater Level
    *out_heater_level = (si7021_heater_level_t)(
        heater_register & SI7021_HEATER_REGISTER_MASK_HEATER_LEVEL);

    return err;
}

/* Set Heater Level */
esp_err_t si7021_set_heater_level(
    si7021_handle_t const si7021       ,
    uint8_t               heater_level )
{
    CHECK_ERR_HANDLE(si7021);
    esp_err_t err = ESP_OK;

    // > Set Heater Register
    uint8_t heater_register  = SI7021_HEATER_REGISTER_DEFAULT;
            heater_register |=
                heater_level &
                SI7021_HEATER_REGISTER_MASK_HEATER_LEVEL;
    err = si7021_i2c_write_heater_control_reg(si7021->i2c, heater_register );
    CHECK_ERR_SI7021_I2C_WRITE_HEATER_CONTROL_REG(err);

    return err;
}

//// ---------------------------------------------------------------------------

//// Serial Number & Firmware --------------------------------------------------

/* Get Serial Number Info */
esp_err_t si7021_get_serial_number_info(
    si7021_handle_t const  si7021            ,
    uint64_t              *out_serial_number ,
    si7021_sn_id_t        *out_sn_id         )
{
    CHECK_ERR_HANDLE(si7021);
    CHECK_ERR_OUT_PARAM(out_serial_number);
    esp_err_t err = ESP_OK;

    uint32_t sna = 0;
    uint32_t snb = 0;
    bool crc_check;
    TickType_t read_wait;

    // > Read Electronic ID First Bytes
    crc_check = si7021->crc_config.global || si7021->crc_config.sna;
    read_wait = si7021->read_wait.global
        ? si7021->read_wait.global
        : si7021->read_wait.sna;
    err = si7021_i2c_read_electronic_id_first_bytes(
        si7021->i2c ,
        &sna        ,
        crc_check   ,
        read_wait  );
    CHECK_ERR_SI7021_I2C_READ_ELECTRONIC_ID_FIRST_BYTES(err);

    // > Read Electronic ID Last Bytes
    crc_check = si7021->crc_config.global || si7021->crc_config.snb;
    read_wait = si7021->read_wait.global
        ? si7021->read_wait.global
        : si7021->read_wait.snb;
    err = si7021_i2c_read_electronic_id_last_bytes(
        si7021->i2c ,
        &snb        ,
        crc_check   ,
        read_wait  );
    CHECK_ERR_SI7021_I2C_READ_ELECTRONIC_ID_LAST_BYTES(err);

    // > Serial Number
    *out_serial_number = UINT64_T_FROM_UINT32_T(sna, snb);

    // > Serial Number ID
    if (out_sn_id) *out_sn_id = ID_FROM_SNB(snb);

    return err;
}

/* Get Firmware Revision */
esp_err_t si7021_get_firmware_revision_info(
    si7021_handle_t     const  si7021            ,
    si7021_fw_version_t       *out_fw_version   )
{
    CHECK_ERR_HANDLE(si7021);
    CHECK_ERR_OUT_PARAM(out_fw_version);
    esp_err_t err = ESP_OK;

    // > Read Firmware Revision
    uint8_t    fw_version = 0;
    TickType_t read_wait  = si7021->read_wait.global
        ? si7021->read_wait.global
        : si7021->read_wait.fw;
    err = si7021_i2c_read_firmware_revision(
        si7021->i2c    ,
        &fw_version    ,
        read_wait     );
    CHECK_ERR_SI7021_READ_FIRWARE_REVISION(err);

    // > Firmware Revision
    *out_fw_version = (si7021_fw_version_t)fw_version;

    return err;
}

//// ---------------------------------------------------------------------------

//// Dump ----------------------------------------------------------------------

/* Dump Device Info */
esp_err_t si7021_dump_device_info(
    si7021_handle_t const si7021 )
{
    esp_err_t err = ESP_OK;

    // > Get User Register Info
    uint8_t user_reg = 0;
    si7021_user_register_info_t user_reg_info;
    err = si7021_get_user_register_info(si7021, &user_reg, &user_reg_info);
    if (err) return err;

    // > Get Heater Level
    si7021_heater_level_t heater_level;
    err = si7021_get_heater_level(si7021, &heater_level);
    if (err) return err;

    // > Get Serial Number Info
    uint64_t serial_number;
    si7021_sn_id_t sn_id;
    err = si7021_get_serial_number_info(si7021, &serial_number, &sn_id);
    if (err) return err;

    // > Get Firmware Revision
    si7021_fw_version_t fw_version;
    err = si7021_get_firmware_revision_info(si7021, &fw_version);
    if (err) return err;

    // > Log
    ESP_LOGI(TAG, "Si7021 Info:\n"
                  " - Name: %s\n"
                  " - User Register 0x%02X:\n"
                  "   - Resolution: %s\n"
                  "   - Heater State: %s\n"
                  "   - VDD Status: %s\n"
                  " - Heater Level: 0x%02X (Current: %s)\n"
                  " - Serial Number: 0x%016llX (ID: %s)\n"
                  " - Firmware Revision: 0x%02X (Version: %s)\n"            ,
                  si7021->name                                              ,
                  user_reg                                                  ,
                  SI7021_RESOLUTION_TO_STRING(user_reg_info.resolution)     ,
                  SI7021_HEATER_STATE_TO_STRING(user_reg_info.heater_state) ,
                  SI7021_VDD_STATUS_TO_STRING(user_reg_info.vdd_status)     ,
                  heater_level, SI7021_HEATER_LEVEL_TO_STRING(heater_level) ,
                  serial_number, SI7021_SN_ID_TO_STRING(sn_id)              ,
                  fw_version, SI7021_FW_VERSION_TO_STRING(fw_version)      );

    return err;
}

/* Dump CRC Config */
esp_err_t si7021_dump_crc_config(
    si7021_handle_t const si7021 )
{
    CHECK_ERR_HANDLE(si7021);
    ESP_LOGI(TAG, "Si7021 CRC Config:\n"
                  " - Global: %s\n"
                  "\n"
                  " - RH:     %s\n"
                  " - Temp:   %s\n"
                  " - SNA:    %s\n"
                  " - SNB:    %s\n"                                   ,
                  si7021->crc_config.global ? "Enabled" : "Disabled"  ,
                  si7021->crc_config.rh     ? "Enabled" : "Disabled"  ,
                  si7021->crc_config.temp   ? "Enabled" : "Disabled"  ,
                  si7021->crc_config.sna    ? "Enabled" : "Disabled"  ,
                  si7021->crc_config.snb    ? "Enabled" : "Disabled" );
    return ESP_OK;
}

/* Dump Read Wait Timeouts */
esp_err_t si7021_dump_read_wait(
    si7021_handle_t const si7021 )
{
    CHECK_ERR_HANDLE(si7021);
    ESP_LOGI(TAG, "Si7021 Read Wait:\n"
                  " - Global:            %d ms\n"
                  "\n"
                  " - RH:                %d ms\n"
                  " - Temp:              %d ms\n"
                  " - User Register:     %d ms\n"
                  " - Heater Register:   %d ms\n"
                  " - SNA:               %d ms\n"
                  " - SNB:               %d ms\n"
                  " - Firmware Revision: %d ms\n"                            ,
                  (int) (si7021->read_wait.global     * portTICK_PERIOD_MS)  ,
                  (int) (si7021->read_wait.rh         * portTICK_PERIOD_MS)  ,
                  (int) (si7021->read_wait.temp       * portTICK_PERIOD_MS)  ,
                  (int) (si7021->read_wait.user_reg   * portTICK_PERIOD_MS)  ,
                  (int) (si7021->read_wait.heater_reg * portTICK_PERIOD_MS)  ,
                  (int) (si7021->read_wait.sna        * portTICK_PERIOD_MS)  ,
                  (int) (si7021->read_wait.snb        * portTICK_PERIOD_MS)  ,
                  (int) (si7021->read_wait.fw         * portTICK_PERIOD_MS) );

    // Return
    return ESP_OK;
}

//// ---------------------------------------------------------------------------

// -----------------------------------------------------------------------------