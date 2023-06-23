#include <stdio.h>
#include "hall_sensor.h"

// For ESP Logs
#include "esp_log.h"

static const char *TAG = "Hall Sensor";

// For Hall Sensor readings
#include "driver/adc.h"

// For ESP HRT
#include "esp_timer.h"

// Hall Sensor Variables //
static bool _initialized = false;


// FUNCTIONS //

// Initialize Hall Sensor
esp_err_t hall_sensor_init(
    void
) {
    esp_err_t err;

    // Configure ADC1 to max width
    if ( (err = adc1_config_width(ADC_WIDTH_BIT_12)) ) {
        ESP_LOGE(TAG, "ADC width configuration failed");
        return err;
    }

    // Set initialized to true
    _initialized = true;

    ESP_LOGI(TAG, "Initialized");
    ESP_LOGW(TAG, "\n> This app takes readings from the internal Hall sensor, "
                  "which uses channels 0 and 3 of ADC1 "
                  "(GPIO 36 and 39)"
                  "\n> Do not connect anything to these pings and "
                  "do not change their configuration!!!");

    return ESP_OK;
}

// Read Hall Sensor
inline int hall_sensor_get_reading(
    void
) {
    return hall_sensor_read();
}

// Deinitialize Hall Sensor
void hall_sensor_deinit(
    void
) {
    // Set initialized to false
    _initialized = false;

    ESP_LOGI(TAG, "Deinitialized");
}



