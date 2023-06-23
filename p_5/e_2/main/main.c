#include <stdio.h>

#include "si7021.h"

// include for periodic timer
#include "esp_timer.h"

// include for RTOS locks
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"


#include "esp_err.h"
#include "esp_log.h"
static const char *TAG = "main";

// DEFINITIONS //
#define APP_DURATION_MS  /* 60000 */ CONFIG_APP_DURATION_MS

#define APP_SI7021_PERIOD_MS  /* 1000 */ CONFIG_APP_SI7021_PERIOD_MS

// I2C LOCK //
SemaphoreHandle_t i2c_lock = NULL;

// Periodic timer callback
void si7021_periodic_timer_callback(void *arg)
{
    si7021_handle_t si7021 = (si7021_handle_t)arg;

    // Measure Rh and Temp (with lock)
    float rh, temp;
    if (xSemaphoreTake(i2c_lock, portMAX_DELAY) == pdTRUE) {
        si7021_measure_rh_and_temp(si7021, &rh, &temp);
        xSemaphoreGive(i2c_lock);

        // Log values
        ESP_LOGI(TAG, "RH: %.2f%% | Temp: %.2fC", rh, temp);
    }
}



void app_main(void)
{
    esp_err_t err;

    // I2C LOCK //
    i2c_lock = xSemaphoreCreateMutex();
    if (i2c_lock == NULL) {
        ESP_LOGE(TAG, "Could not create I2C lock");
        return;
    }

    // SI7021 //
    si7021_create_args_t si7021_create_args = SI7021_DEFAULT_CREATE_ARGS();
    si7021_create_args.crc_config = SI7021_CRC_CONFIG_ALL(false); // CRC not implemented (avoid LOGW)

    si7021_handle_t si7021;
    if ( (err = si7021_create(&si7021_create_args, &si7021)) != ESP_OK) {
        ESP_LOGE(TAG, "Could not create SI7021: %d", err);
        return;
    }

    // Periodic timer //
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &si7021_periodic_timer_callback,
        .arg = si7021,
        .name = "si7021_periodic_timer"
    };
    esp_timer_handle_t periodic_timer;
    if ( (err = esp_timer_create(&periodic_timer_args, &periodic_timer)) != ESP_OK) {
        ESP_LOGE(TAG, "Could not create periodic timer: %d", err);
        return;
    }

    // ---

    // Start periodic timer //
    if ( (err = esp_timer_start_periodic(periodic_timer, APP_SI7021_PERIOD_MS * 1000)) != ESP_OK) {
        ESP_LOGE(TAG, "Could not start periodic timer: %d", err);
        return;
    }

    if (!APP_DURATION_MS) {
        return;
    }

    // Wait for APP_DURATION_MS //
    vTaskDelay(APP_DURATION_MS / portTICK_PERIOD_MS);

    // ---

    // Stop periodic timer //
    if ( (err = esp_timer_stop(periodic_timer)) != ESP_OK) {
        ESP_LOGE(TAG, "Could not stop periodic timer: %d", err);
        return;
    }

    // Delete periodic timer //
    if ( (err = esp_timer_delete(periodic_timer)) != ESP_OK) {
        ESP_LOGE(TAG, "Could not delete periodic timer: %d", err);
        return;
    }

    // Delete SI7021 //
    if ( (err = si7021_delete(si7021)) != ESP_OK) {
        ESP_LOGE(TAG, "Could not delete SI7021: %d", err);
        return;
    }

    // Delete I2C lock //
    vSemaphoreDelete(i2c_lock);

    ESP_LOGI(TAG, "Done");

    return;
}
