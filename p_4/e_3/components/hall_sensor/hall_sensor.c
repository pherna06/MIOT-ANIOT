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
esp_timer_handle_t _sampling_timer = NULL;





// SAMPLING FUNCTION //
static void _sampling_fn(void *arg) {
    // Sample Hall Sensor
    int hall_sensor_reading = hall_sensor_read();

    // Post HALL_SENSOR_READING_EVENT
    post_hall_sensor_reading_event(
        &hall_sensor_reading,
        sizeof(int)
    );
}




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

    // Create sampling timer
    const esp_timer_create_args_t sampling_timer_args = {
        .callback = _sampling_fn,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "hall_sensor_sampling_timer"
    };
    if ( (err = esp_timer_create(&sampling_timer_args, &_sampling_timer)) ) {
        ESP_LOGE(TAG, "Sampling timer creation failed");
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


// Start Hall Sensor
esp_err_t hall_sensor_start(
    uint32_t period_ms
) {
    esp_err_t err;

    // Check Hall Sensor is initialized
    if ( !_initialized ) {
        ESP_LOGE(TAG, "Hall Sensor is not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Start sampling timer
    if ( (err = esp_timer_start_periodic(_sampling_timer, period_ms * 1000)) ) {
        ESP_LOGE(TAG, "Sampling timer start failed");
        return err;
    }

    return ESP_OK;
}

// Stop Hall Sensor
esp_err_t hall_sensor_stop(
    void
) {
    esp_err_t err;

    // Check Hall Sensor is initialized
    if ( !_initialized ) {
        ESP_LOGE(TAG, "Hall Sensor is not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Stop sampling timer
    if ( (err = esp_timer_stop(_sampling_timer)) ) {
        ESP_LOGE(TAG, "Sampling timer stop failed");
        return err;
    }

    return ESP_OK;
}

// Deinitialize Hall Sensor
void hall_sensor_deinit(
    void
) {
    // Delete sampling timer
    esp_timer_delete(_sampling_timer);

    // Set initialized to false
    _initialized = false;

    ESP_LOGI(TAG, "Deinitialized");
}



