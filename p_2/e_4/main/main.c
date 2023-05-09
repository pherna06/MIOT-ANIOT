#include <stdio.h>

// Include for ADC functions and Hall sensor
#include <driver/adc.h>

// include for ESP_LOGI and ESP_LOGW
#include <esp_log.h>

// include for events
#include <esp_event.h>

// include for FreeRTOS functions
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>





// Period of the main loop in miliseconds
#define PERIOD_MS 1000

// Event base for Hall sensor events
ESP_EVENT_DECLARE_BASE(HALL_EVENTS);

enum
{
    HALL_NEWSAMPLE
};

ESP_EVENT_DEFINE_BASE(HALL_EVENTS);

// Event loop
static esp_event_loop_handle_t hall_events_loop;





// HALL_NEWSAMPLE event handler
static void hall_newsample(
        void *handler_args,
        esp_event_base_t base,
        int32_t id,
        void *event_data)
{
    static const char *TAG = "hall_newsample";

    ESP_LOGI(TAG, "Hall sensor: %d", *(int *)event_data);
}





// Reading task
struct reading_task_parameters
{
    int period_ms;
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

        // Post HALL_NEWSAMPLE event
        ESP_ERROR_CHECK(
            esp_event_post_to(hall_events_loop, HALL_EVENTS, HALL_NEWSAMPLE,
                &hall, sizeof(hall), portMAX_DELAY)
        );

        // Sleep for PERIOD_MS miliseconds
        vTaskDelay(params->period_ms / portTICK_PERIOD_MS);
    }
}





// Main function
void app_main(void)
{
    static const char *TAG = "app_main";
    
    ESP_LOGI(TAG, "Start of app_main");
    ESP_LOGW(TAG, "\n> This app takes readings from the internal Hall sensor, "
                  "which uses channels 0 and 3 of ADC1 "
                  "(GPIO 36 and 39)"
                  "\n> Do not connect anything to these pings and "
                  "do not change their configuration!!!");

    ESP_LOGI(TAG, "\n> Setting ADC1 precision:"
                  "\n>   - 12 bit");
    adc1_config_width(ADC_WIDTH_BIT_12);

    // Create event loop
    ESP_LOGI(TAG, "Creating event loop...");
    esp_event_loop_args_t loop_args = {
        .queue_size = 5,
        .task_name = "hall_events_loop",
        .task_priority = 5,
        .task_stack_size = 2048,
        .task_core_id = tskNO_AFFINITY
    };

    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &hall_events_loop));

    // Register HALL_NEWSAMPLE event
    ESP_LOGI(TAG, "Registering HALL_NEWSAMPLE event...");
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register_with(
            hall_events_loop, HALL_EVENTS, HALL_NEWSAMPLE,
            &hall_newsample, NULL, NULL)
    );

    // Create task parameters
    struct reading_task_parameters params;
    params.period_ms = PERIOD_MS;

    // Create task
    TaskHandle_t task_handle = NULL;
    xTaskCreate(&reading_task, "reading_task", 2048, &params, 5, &task_handle);

    // Keep `params` in scope until the task is deleted
    while (task_handle != NULL)
        vTaskDelay(1000 / portTICK_PERIOD_MS);
}