#include <stdio.h>

// include esp_event for event loop
#include <esp_event.h>

// include esp_log for logging
#include <esp_log.h>

// include ADC1 and Hall Sensor
#include <driver/adc.h>

// include task.h for tasks
#include <freertos/task.h>

// include queue.h for queues
#include <freertos/queue.h>

#include "app_events.h"
ESP_EVENT_DEFINE_BASE(APP_EVENTS);

#include "fifo_queue.c"
#include "hall_sensor.c"
#include "logger.c"
#include "monitor.c"

void app_main(void)
{
    // TAG
    static const char *TAG = "app_main";
    
    // Create APP_EVENTS event loop
    esp_event_loop_handle_t app_events_loop;
    ESP_LOGI(TAG, "Creating APP_EVENTS event loop");
    esp_event_loop_args_t app_events_args = {
        .queue_size = 10,
        .task_name = "app_events_loop",
        .task_priority = 5,
        .task_stack_size = 8192,
        .task_core_id = tskNO_AFFINITY
    };
    ESP_ERROR_CHECK(esp_event_loop_create(&app_events_args, &app_events_loop));
    
    // Start Logger Task
    ESP_LOGI(TAG, "Starting logger task");
    TaskHandle_t logger_task_handle;
    struct logger_task_parameters logger_task_params = {
        .app_events_loop = app_events_loop
    };
    xTaskCreate(logger_task, "logger_task", 16 * 1024,
            &logger_task_params, 1, &logger_task_handle);

    // Start Sampler Task for Hall Sensor
    ESP_LOGI(TAG, "Starting hall sampler task");
    TaskHandle_t hall_sampler_task_handle;
    struct hall_sampler_task_parameters hall_sampler_task_params = {
        .period_ms = 1000,
        .app_events_loop = app_events_loop
    };
    xTaskCreate(hall_sampler_task, "hall_sampler_task", 2048,
            &hall_sampler_task_params, 1, &hall_sampler_task_handle);

    // Start Filter Task for Hall Sensor
    ESP_LOGI(TAG, "Starting hall filter task");
    TaskHandle_t hall_filter_task_handle;
    struct hall_filter_task_parameters hall_filter_task_params = {
        .num_samples = 5,
        .app_events_loop = app_events_loop
    };
    xTaskCreate(hall_filter_task, "hall_filter_task", 2048,
            &hall_filter_task_params, 1, &hall_filter_task_handle);

    // Start Monitor Task
    ESP_LOGI(TAG, "Starting monitor task");
    TaskHandle_t monitor_task_handle;
    TaskHandle_t handles[] = {
        logger_task_handle,
        hall_sampler_task_handle,
        hall_filter_task_handle
    };
    struct monitor_task_parameters monitor_task_params = {
        .handles = handles,
        .num_handles = sizeof(handles) / sizeof(handles[0]),
        .period_ms = 60000,
        .app_events_loop = app_events_loop
    };
    xTaskCreate(monitor_task, "monitor_task", 2048,
            &monitor_task_params, 1, &monitor_task_handle);

    // Wait while monitor task is running (to keep handles in context)
    while (monitor_task_handle != NULL)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}