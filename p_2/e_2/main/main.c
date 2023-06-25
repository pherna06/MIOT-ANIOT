// INCLUDES --------------------------------------------------------------------

/* Tasks */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ADC */
#include "driver/adc.h"

/* Logging */
#include <esp_log.h>

static const char *TAG = "app_main";

// -----------------------------------------------------------------------------

// DEFINES --------------------------------------------------------------------

/* Period (ms) of the reading loop */
#define PERIOD_MS 1000

// -----------------------------------------------------------------------------

// TASKS ----------------------------------------------------------------------

/* Reading Task Parameters */
struct reading_task_parameters
{
    int period_ms;
    int hall;
    bool read;
};

/* Reading Task Function */
void reading_task(void *pvParameter)
{
    struct reading_task_parameters *params =
            (struct reading_task_parameters *)pvParameter;

    // > Hall Sensor Reading Loop
    ESP_LOGI(TAG, "Starting reading loop with %d ms period...",
        params->period_ms);
    while (1) {
        if (params->read) ESP_LOGW(TAG, "Overwriting unread reading!!!");
        params->hall = hall_sensor_read();
        params->read = true;
        vTaskDelay(params->period_ms / portTICK_PERIOD_MS);
    }
}

// -----------------------------------------------------------------------------



// APP MAIN -------------------------------------------------------------------

void app_main(void)
{    
    esp_err_t err;
    
    // > Log Hall Sensor GPIO use warning
    ESP_LOGW(TAG, "T\n> This app takes readings from the internal Hall sensor, "
                  "which uses channels 0 and 3 of ADC1 "
                  "(GPIO 36 and 39)"
                  "\n> Do not connect anything to these pings and "
                  "do not change their configuration!!!");

    
    // > Enable ADC1 to use Hall Sensor (by configuring width)
    if ( (err = adc1_config_width(ADC_WIDTH_BIT_12)) ) {
        ESP_LOGE(TAG, "Could not set ADC1 width: %s", esp_err_to_name(err));
        return;
    }

    // > Create task parameters
    struct reading_task_parameters params = {
        .period_ms = PERIOD_MS ,
        .hall      = 0         ,
        .read      = false    };

    // > Create task
    xTaskCreate(&reading_task, "reading_task", 2048, &params, 5, NULL);

    // > Log when reading available loop
    while (1) {
        if (params.read) {
            ESP_LOGI(TAG, "Hall sensor: %d", params.hall);
            params.read = false;
        }
        vTaskDelay(1);
    }
}

// -----------------------------------------------------------------------------