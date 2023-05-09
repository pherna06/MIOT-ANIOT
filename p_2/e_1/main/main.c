#include <stdio.h>

// Include for ADC functions and Hall sensor
#include <driver/adc.h>

// include for ESP_LOGI and ESP_LOGW
#include <esp_log.h>

// include for vTaskDelay
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


// Period of the main loop in miliseconds
#define PERIOD_MS 1000

const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Start of app_main");
    ESP_LOGW(TAG, "T\n> This app takes readings from the internal Hall sensor, "
                  "which uses channels 0 and 3 of ADC1 "
                  "(GPIO 36 and 39)"
                  "\n> Do not connect anything to these pings and "
                  "do not change their configuration!!!");

    ESP_LOGI(TAG, "\n> Setting ADC1 precision:"
                  "\n>   - 12 bit");
    adc1_config_width(ADC_WIDTH_BIT_12);

    ESP_LOGI(TAG, "Starting reading loop with %d ms period...", PERIOD_MS);
    while (1)
    {
        // Sample Hall sensor
        int hall = hall_sensor_read();
        ESP_LOGI(TAG, "Hall sensor: %d", hall);

        // Sleep for PERIOD_MS miliseconds
        vTaskDelay(PERIOD_MS / portTICK_PERIOD_MS);
    }
}