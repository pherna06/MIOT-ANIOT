#include <stdio.h>

// Include for ADC functions and Hall sensor
#include <driver/adc.h>

// include for ESP_LOGI and ESP_LOGW
#include <esp_log.h>

// include for FreeRTOS functions
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>





// Period of the main loop in miliseconds
#define PERIOD_MS 1000





// Reading task
struct reading_task_parameters
{
    int period_ms;
    QueueHandle_t hall_queue;
};

void reading_task(void *pvParameter)
{
    static const char *TAG = "reading_task";
    
    struct reading_task_parameters *params =
            (struct reading_task_parameters *)pvParameter;

    ESP_LOGI(TAG, "Starting reading loop with %d ms period...", params->period_ms);
    while (1)
    {
        // Sample Hall sensor
        int hall = hall_sensor_read();

        // Send to queue (unchecked)
        if (params->hall_queue != NULL)
            xQueueSend(params->hall_queue, &hall, 0);

        // Sleep for PERIOD_MS miliseconds
        vTaskDelay(params->period_ms / portTICK_PERIOD_MS);
    }
}





// Main function
void app_main(void)
{
    static const char *TAG = "app_main";
    
    ESP_LOGI(TAG, "Start of app_main");
    ESP_LOGW(TAG, "T\n> This app takes readings from the internal Hall sensor, "
                  "which uses channels 0 and 3 of ADC1 "
                  "(GPIO 36 and 39)"
                  "\n> Do not connect anything to these pings and "
                  "do not change their configuration!!!");

    ESP_LOGI(TAG, "\n> Setting ADC1 precision:"
                  "\n>   - 12 bit");
    adc1_config_width(ADC_WIDTH_BIT_12);

    // Hall readings queue
    QueueHandle_t hall_queue = xQueueCreate(1, sizeof(int));

    // Create task parameters
    struct reading_task_parameters params;
    params.period_ms = PERIOD_MS;
    params.hall_queue = hall_queue;

    // Create task
    xTaskCreate(&reading_task, "reading_task", 2048, &params, 5, NULL);

    // Main loop
    int hall;
    int queue_wait = 10 / portTICK_PERIOD_MS;
    while (1)
    {
        // Check if a new reading is available in the queue
        if (hall_queue != NULL
                && xQueueReceive(hall_queue, &hall, queue_wait) == pdTRUE)
            ESP_LOGI(TAG, "Hall sensor: %d", hall);
    }
}