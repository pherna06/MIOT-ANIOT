// INCLUDES -------------------------------------------------------------------

/* Logging */
#include <esp_log.h>

static const char *TAG = "app_main";

// -----------------------------------------------------------------------------

// DEFINES --------------------------------------------------------------------

/* Period (ms) of the reading loop */
#define PERIOD_MS 2000

// -----------------------------------------------------------------------------

// APP MAIN -------------------------------------------------------------------

void app_main(void)
{
    esp_err_t err;
    
    // > Log Hall Sensor GPIO use warning
    ESP_LOGW(TAG, "\n> This app takes readings from the internal Hall sensor, "
                  "which uses channels 0 and 3 of ADC1 "
                  "(GPIO 36 and 39)"
                  "\n> Do not connect anything to these pings and "
                  "do not change their configuration!!!");

    // > Enable ADC1 to use Hall Sensor (by configuring width)
    if ( (err = adc1_config_width(ADC_WIDTH_BIT_12)) ) {
        ESP_LOGE(TAG, "Could not set ADC1 width: %s", esp_err_to_name(err));
        return;
    }

    // > Hall Sensor Reading Loop
    ESP_LOGI(TAG, "Starting reading loop with %d ms period...", PERIOD_MS);
    while (1) {
        int hall = hall_sensor_read();
        ESP_LOGI(TAG, "Hall sensor: %d", hall);
        vTaskDelay(PERIOD_MS / portTICK_PERIOD_MS);
    }
}

// -----------------------------------------------------------------------------