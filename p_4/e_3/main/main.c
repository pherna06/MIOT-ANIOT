#include <stdio.h>

// include ESP logs
#include "esp_log.h"

// inlude ESP events
#include "esp_event.h"

// include FreeRTOS tasks
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "distance_sensor_task.c"
#include "hall_sensor_task.c"
#include "binary_counter_3b_task.c"

#ifdef CONFIG_APP_TIMEOUT
#define APP_TIMEOUT_MS CONFIG_APP_TIMEOUT_MS
#else
#define APP_TIMEOUT_MS 0
#endif

void app_main(void)
{
    // Remove GPIO logs
    esp_log_level_set("gpio", ESP_LOG_WARN);
    
    // Default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Task Handles
    TaskHandle_t distance_sensor_task_handle = NULL;
    TaskHandle_t hall_sensor_task_handle = NULL;
    TaskHandle_t binary_counter_3b_task_handle = NULL;
    
    // Start Distance Sensor Task
    #ifdef CONFIG_DISTANCE_SENSOR_TASK
    xTaskCreate(
        distance_sensor_task,
        "distance_sensor_task",
        2048,
        NULL,
        10,
        &distance_sensor_task_handle
    );
    #endif

    // Start Hall Sensor Task
    #ifdef CONFIG_HALL_SENSOR_TASK
    xTaskCreate(
        hall_sensor_task,
        "hall_sensor_task",
        2048,
        NULL,
        10,
        &hall_sensor_task_handle
    );
    #endif

    // Start Binary Counter 3B Task
    #ifdef CONFIG_BINARY_COUNTER_3B_TASK
    xTaskCreate(
        binary_counter_3b_task,
        "binary_counter_3b_task",
        2048,
        NULL,
        10,
        &binary_counter_3b_task_handle
    );
    #endif

    // Timeout
    if (APP_TIMEOUT_MS) {
        vTaskDelay(APP_TIMEOUT_MS / portTICK_PERIOD_MS);

        // Send notification to tasks
        xTaskNotifyGive(distance_sensor_task_handle);
        xTaskNotifyGive(hall_sensor_task_handle);
        xTaskNotifyGive(binary_counter_3b_task_handle);
    }
}