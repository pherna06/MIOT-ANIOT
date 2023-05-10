#include <stdio.h>

// Monitor task
struct monitor_task_parameters
{
    TaskHandle_t *handles;
    int num_handles;

    int period_ms;

    esp_event_loop_handle_t app_events_loop;
};

void monitor_task(void *pvParameter)
{
    // Get monitor task parameters
    struct monitor_task_parameters *params =
        (struct monitor_task_parameters *)pvParameter;
    
    // Infinite loop
    while (1)
    {
        // Log task statuses
        for (int i = 0; i < params->num_handles; i++)
        {
            // Get task handle
            TaskHandle_t handle = params->handles[i];

            // Task status
            TaskStatus_t status;
            vTaskGetInfo(handle, &status, pdTRUE, eInvalid);

            // Post APP_EVENT_MONITOR event
            ESP_ERROR_CHECK(
                esp_event_post_to(params->app_events_loop, APP_EVENTS,
                    APP_EVENT_MONITOR, &status, sizeof(status), portMAX_DELAY)
            );
        }

        // Delay for period_ms miliseconds
        vTaskDelay(params->period_ms / portTICK_PERIOD_MS);
    }
}