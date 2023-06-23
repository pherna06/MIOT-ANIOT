#ifndef __SI7021_I2C_H__
#define __SI7021_I2C_H__

// INCLUDES --------------------------------------------------------------------

#include <stdbool.h>    // bool
#include <stdint.h>     // uint8_t, uint16_t, uint32_t
#include "esp_err.h"    // esp_err_t
#include "driver/i2c.h" // i2c_port_t, i2c_config_t, I2C_MODE_MASTER

// -----------------------------------------------------------------------------

// STRUCTURES ------------------------------------------------------------------

/* Si7021 I2C Handle */
typedef struct si7021_i2c_handle *si7021_i2c_handle_t;

/* Si7021 I2C Create Args */
typedef struct {
    const char *name; // Handle name

    i2c_port_t i2c_port;          // I2C port
    TickType_t i2c_retry_timeout; // I2C retry timeout

    struct {
        bool install;             // Install driver?
        bool set_config;          // Set I2C configuration?
        bool uninstall_at_delete; // Uninstall driver at handle delete?

        i2c_config_t config; // I2C configuration
    } i2c_driver;
} si7021_i2c_create_args_t;

// -----------------------------------------------------------------------------

// DEFINITIONS -----------------------------------------------------------------

#define SI7021_I2C_HANDLE_NAME_LENGTH 16

//// KCONFIG -------------------------------------------------------------------

#define SI7021_I2C_MAX_CLOCK_SPEED                                /* 400000 */ \
        CONFIG_SI7021_I2C_MAX_CLOCK_SPEED

#define SI7021_I2C_ADDRESS                                          /* 0x40 */ \
        CONFIG_SI7021_I2C_ADDRESS

////// Si7021 I2C Command Values -----------------------------------------------

/* Measurements Commands */
#define SI7021_I2C_CMD_MEASURE_RH_HOLD_MASTER                       /* 0xE5 */ \
        CONFIG_SI7021_I2C_CMD_MEASURE_RH_HOLD_MASTER
#define SI7021_I2C_CMD_MEASURE_RH_NOHOLD_MASTER                     /* 0xF5 */ \
        CONFIG_SI7021_I2C_CMD_MEASURE_RH_NOHOLD_MASTER
#define SI7021_I2C_CMD_MEASURE_TEMP_HOLD_MASTER                     /* 0xE3 */ \
        CONFIG_SI7021_I2C_CMD_MEASURE_TEMP_HOLD_MASTER
#define SI7021_I2C_CMD_MEASURE_TEMP_NOHOLD_MASTER                   /* 0xF3 */ \
        CONFIG_SI7021_I2C_CMD_MEASURE_TEMP_NOHOLD_MASTER
#define SI7021_I2C_CMD_READ_TEMP_FROM_PREV_RH_MEASUREMENT           /* 0xE0 */ \
        CONFIG_SI7021_I2C_CMD_READ_TEMP_FROM_PREV_RH_MEASUREMENT

/* Reset and Register Commands*/
#define SI7021_I2C_CMD_RESET                                        /* 0xFE */ \
        CONFIG_SI7021_I2C_CMD_RESET
#define SI7021_I2C_CMD_WRITE_RH_T_USER_REG_1                        /* 0xE6 */ \
        CONFIG_SI7021_I2C_CMD_WRITE_RH_T_USER_REG_1
#define SI7021_I2C_CMD_READ_RH_T_USER_REG_1                         /* 0xE7 */ \
        CONFIG_SI7021_I2C_CMD_READ_RH_T_USER_REG_1
#define SI7021_I2C_CMD_WRITE_HEATER_CONTROL_REG                     /* 0x51 */ \
        CONFIG_SI7021_I2C_CMD_WRITE_HEATER_CONTROL_REG
#define SI7021_I2C_CMD_READ_HEATER_CONTROL_REG                      /* 0x11 */ \
        CONFIG_SI7021_I2C_CMD_READ_HEATER_CONTROL_REG

/* Serial Number and Firmware Commands */
#define SI7021_I2C_CMD_READ_ELECTRONIC_ID_FIRST_BYTES_A             /* 0xFA */ \
        CONFIG_SI7021_I2C_CMD_READ_ELECTRONIC_ID_FIRST_BYTES_A
#define SI7021_I2C_CMD_READ_ELECTRONIC_ID_FIRST_BYTES_B             /* 0x0F */ \
        CONFIG_SI7021_I2C_CMD_READ_ELECTRONIC_ID_FIRST_BYTES_B
#define SI7021_I2C_CMD_READ_ELECTRONIC_ID_LAST_BYTES_A              /* 0xFC */ \
        CONFIG_SI7021_I2C_CMD_READ_ELECTRONIC_ID_LAST_BYTES_A
#define SI7021_I2C_CMD_READ_ELECTRONIC_ID_LAST_BYTES_B              /* 0xC9 */ \
        CONFIG_SI7021_I2C_CMD_READ_ELECTRONIC_ID_LAST_BYTES_B
#define SI7021_I2C_CMD_READ_FIRMWARE_REVISION_A                     /* 0x84 */ \
        CONFIG_SI7021_I2C_CMD_READ_FIRMWARE_REVISION_A
#define SI7021_I2C_CMD_READ_FIRMWARE_REVISION_B                     /* 0xB8 */ \
        CONFIG_SI7021_I2C_CMD_READ_FIRMWARE_REVISION_B

////// -------------------------------------------------------------------------

////// Si7021 I2C Default Create Args ------------------------------------------

/* Name, I2C Port & I2C Retry Timeout */
#define SI7021_I2C_DEFAULT_NAME                             /* "si7021_i2c" */ \
        CONFIG_SI7021_I2C_DEFAULT_NAME
#define SI7021_I2C_DEFAULT_I2C_PORT                        /* (I2C_NUM_0) 0 */ \
        (i2c_port_t)CONFIG_SI7021_I2C_DEFAULT_I2C_PORT

/* On commands like measuring without holding master, where the sensor NACKs
 * read trials until the value is ready, the I2C driver can get stuck while
 * processing a reading command waiting for the ACK. In that case, the minimum
 * waiting time is I2C_CMD_ALIVE_INTERVAL_TICK (see source 'i2c.c'), which
 * amounts to 1000 ms.
 * 
 * Because of that, default command timeout is set to 2000 ms, so that the
 * command can be processed again in case that it fails like described. In the
 * no hold master measure case, the next command processing should success.
 * 
 * Nevertheless, to avoid this stuck state, the user can set a time to wait
 * until the reading command is processed first. This time is a parameter of
 * those functions where data is read from the Si7021 sensor.
 */
#define SI7021_I2C_DEFAULT_I2C_DRIVER_RETRY_TIMEOUT_MS              /* 2000 */ \
        CONFIG_SI7021_I2C_DEFAULT_I2C_DRIVER_RETRY_TIMEOUT_MS

/* I2C Driver Flags: install, set config & uninstall at delete */
#define SI7021_I2C_DEFAULT_I2C_DRIVER_INSTALL                   /* (true) 1 */ \
        (bool)CONFIG_SI7021_I2C_DEFAULT_I2C_DRIVER_INSTALL
#define SI7021_I2C_DEFAULT_I2C_DRIVER_SET_CONFIG                /* (true) 1 */ \
        (bool)CONFIG_SI7021_I2C_DEFAULT_I2C_DRIVER_SET_CONFIG
#define SI7021_I2C_DEFAULT_I2C_DRIVER_UNINSTALL_AT_DELETE       /* (true) 1 */ \
        (bool)CONFIG_SI7021_I2C_DEFAULT_I2C_DRIVER_UNINSTALL_AT_DELETE

/* I2C Driver Config */
#define SI7021_I2C_DEFAULT_I2C_DRIVER_CONFIG_MODE                I2C_MODE_MASTER
#define SI7021_I2C_DEFAULT_I2C_DRIVER_CONFIG_SCL_IO                   /* 19 */ \
        CONFIG_SI7021_I2C_DEFAULT_I2C_DRIVER_CONFIG_SCL_IO
#define SI7021_I2C_DEFAULT_I2C_DRIVER_CONFIG_SDA_IO                   /* 18 */ \
        CONFIG_SI7021_I2C_DEFAULT_I2C_DRIVER_CONFIG_SDA_IO
#define SI7021_I2C_DEFAULT_I2C_DRIVER_CONFIG_SCL_PULLUP_EN      /* (true) 1 */ \
        (bool)CONFIG_SI7021_I2C_DEFAULT_I2C_DRIVER_CONFIG_SCL_PULLUP_EN
#define SI7021_I2C_DEFAULT_I2C_DRIVER_CONFIG_SDA_PULLUP_EN      /* (true) 1 */ \
        (bool)CONFIG_SI7021_I2C_DEFAULT_I2C_DRIVER_CONFIG_SDA_PULLUP_EN
#define SI7021_I2C_DEFAULT_I2C_DRIVER_CONFIG_MASTER_CLK_SPEED     /* 200000 */ \
        CONFIG_SI7021_I2C_DEFAULT_I2C_DRIVER_CONFIG_MASTER_CLK_SPEED

////// -------------------------------------------------------------------------

//// ---------------------------------------------------------------------------

// -----------------------------------------------------------------------------

// MACROS ----------------------------------------------------------------------

/* Si7021 I2C Default Create Args */
#define SI7021_I2C_DEFAULT_CREATE_ARGS() {                                     \
    .name              = SI7021_I2C_DEFAULT_NAME,                              \
    .i2c_port          = SI7021_I2C_DEFAULT_I2C_PORT,                          \
    .i2c_retry_timeout = SI7021_I2C_DEFAULT_I2C_DRIVER_RETRY_TIMEOUT_MS        \
                             / portTICK_PERIOD_MS,                             \
    .i2c_driver = {                                                            \
        .install             = SI7021_I2C_DEFAULT_I2C_DRIVER_INSTALL,          \
        .set_config          = SI7021_I2C_DEFAULT_I2C_DRIVER_SET_CONFIG,       \
        .uninstall_at_delete =                                                 \
            SI7021_I2C_DEFAULT_I2C_DRIVER_UNINSTALL_AT_DELETE,                 \
        .config = {                                                            \
            .mode          = SI7021_I2C_DEFAULT_I2C_DRIVER_CONFIG_MODE,        \
            .scl_io_num    = SI7021_I2C_DEFAULT_I2C_DRIVER_CONFIG_SCL_IO,      \
            .sda_io_num    = SI7021_I2C_DEFAULT_I2C_DRIVER_CONFIG_SDA_IO,      \
            .scl_pullup_en =                                                   \
                SI7021_I2C_DEFAULT_I2C_DRIVER_CONFIG_SCL_PULLUP_EN,            \
            .sda_pullup_en =                                                   \
                SI7021_I2C_DEFAULT_I2C_DRIVER_CONFIG_SDA_PULLUP_EN,            \
            .master        = {                                                 \
                .clk_speed =                                                   \
                    SI7021_I2C_DEFAULT_I2C_DRIVER_CONFIG_MASTER_CLK_SPEED      \
            }                                                                  \
        }                                                                      \
    }                                                                          \
}

// -----------------------------------------------------------------------------

// FUNCTIONS -------------------------------------------------------------------

/* Create & Delete */
esp_err_t si7021_i2c_create(
    si7021_i2c_create_args_t const *create_args  ,
    si7021_i2c_handle_t            *out_handle  );

esp_err_t si7021_i2c_delete(
    si7021_i2c_handle_t si7021_i2c );

/* Measure */
esp_err_t si7021_i2c_measure_rh_hold_master(
    si7021_i2c_handle_t const  si7021_i2c        ,
    uint16_t                  *out_rh            ,
    bool                       crc_check         ,
    TickType_t                 wait_before_read );

esp_err_t si7021_i2c_measure_rh_nohold_master(
    si7021_i2c_handle_t const  si7021_i2c        ,
    uint16_t                  *out_rh            ,
    bool                       crc_check         ,
    TickType_t                 wait_before_read );

esp_err_t si7021_i2c_measure_temp_hold_master(
    si7021_i2c_handle_t const  si7021_i2c        ,
    uint16_t                  *out_temp          ,
    bool                       crc_check         ,
    TickType_t                 wait_before_read );

esp_err_t si7021_i2c_measure_temp_nohold_master(
    si7021_i2c_handle_t const  si7021_i2c        ,
    uint16_t                  *out_temp          ,
    bool                       crc_check         ,
    TickType_t                 wait_before_read );

esp_err_t si7021_i2c_read_temp_from_prev_rh_measurement(
    si7021_i2c_handle_t const  si7021_i2c        ,
    uint16_t                  *out_temp          ,
    TickType_t                 wait_before_read );

/* Reset & Registers */
esp_err_t si7021_i2c_reset(
    si7021_i2c_handle_t const si7021_i2c );

esp_err_t si7021_i2c_write_user_reg_1(
    si7021_i2c_handle_t const si7021_i2c ,
    uint8_t                   reg_value );

esp_err_t si7021_i2c_read_user_reg_1(
    si7021_i2c_handle_t const  si7021_i2c        ,
    uint8_t                   *out_reg_value     ,
    TickType_t                 wait_before_read );

esp_err_t si7021_i2c_write_heater_control_reg(
    si7021_i2c_handle_t const si7021_i2c ,
    uint8_t                   reg_value );

esp_err_t si7021_i2c_read_heater_control_reg(
    si7021_i2c_handle_t const  si7021_i2c        ,
    uint8_t                   *out_reg_value     ,
    TickType_t                 wait_before_read );

/* Serial Number and Firmware */
esp_err_t si7021_i2c_read_electronic_id_first_bytes(
    si7021_i2c_handle_t const  si7021_i2c           ,
    uint32_t                  *out_eid_first_bytes  ,
    bool                       crc_check            ,
    TickType_t                 wait_before_read    );

esp_err_t si7021_i2c_read_electronic_id_last_bytes(
    si7021_i2c_handle_t const  si7021_i2c          ,
    uint32_t                  *out_eid_last_bytes  ,
    bool                       crc_check           ,
    TickType_t                 wait_before_read   );

esp_err_t si7021_i2c_read_firmware_revision(
    si7021_i2c_handle_t const  si7021_i2c             ,
    uint8_t                   *out_firmware_revision  ,
    TickType_t                 wait_before_read      );

// -----------------------------------------------------------------------------

#endif // __SI7021_I2C_H__