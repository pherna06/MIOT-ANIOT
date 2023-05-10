#include <stdio.h>
#include <string.h>

// Event Log Structure
struct EventLog_t
{
    char event_name[64];
    char event_data[1028];
};

// Logger Task Event handler
void logger_task_event_handler(
        void *handler_args,
        esp_event_base_t base,
        int32_t id,
        void *event_data)
{
    // Get event log queue
    QueueHandle_t event_log_queue = *(QueueHandle_t *)handler_args;

    // Create event log
    struct EventLog_t event_log;
    
    // If HALL_EVENT_NEWSAMPLE
    if (id == HALL_EVENT_NEWSAMPLE)
    {
        // Copy event name
        strcpy(event_log.event_name, "HALL_EVENT_NEWSAMPLE");

        // Make event_data message
        sprintf(event_log.event_data, "New sample: %d", *(int *)event_data);
    }
    
    // If HALL_EVENT_FILTERSAMPLE
    else if (id == HALL_EVENT_FILTERSAMPLE)
    {
        // Copy event name
        strcpy(event_log.event_name, "HALL_EVENT_FILTERSAMPLE");

        // Make event_data message
        sprintf(event_log.event_data, "Filtered sample: %f", *(float *)event_data);
    }

    // If APP_EVENT_MONITOR
    else if (id == APP_EVENT_MONITOR)
    {
        // Copy event name
        strcpy(event_log.event_name, "APP_EVENT_MONITOR");

        // Make event_data message
        // - task name
        // - task priority
        // - task left stack size
        TaskStatus_t *task_status = (TaskStatus_t *)event_data;
        sprintf(event_log.event_data,
            "Task: %s | Priority: %d | Left stack size: %d",
            task_status->pcTaskName,
            task_status->uxCurrentPriority,
            task_status->usStackHighWaterMark
        );
    }

    // Unknown event
    else
    {
        // Copy event name
        strcpy(event_log.event_name, "UNKNOWN_EVENT");

        // Make event_data message
        strcpy(event_log.event_data, "");
    }

    // Push to event queue
    xQueueSend(event_log_queue, &event_log, 0);
}

struct logger_task_parameters
{
    esp_event_loop_handle_t app_events_loop;
};

void logger_task(void *pvParameters)
{
    // TAG
    static const char *TAG = "LOGGER_TASK";

    // params
    struct logger_task_parameters *params =
            (struct logger_task_parameters *)pvParameters;
    
    // Create Event Logs queue
    QueueHandle_t event_logs_queue;
    event_logs_queue = xQueueCreate(10, sizeof(struct EventLog_t));
    
    // APP_EVENTS handler for logger task
    ESP_ERROR_CHECK(
        esp_event_handler_register_with(
            params->app_events_loop, APP_EVENTS, ESP_EVENT_ANY_ID,
            &logger_task_event_handler, &event_logs_queue)
    );

    struct EventLog_t event_log;
    int queue_wait = 10 / portTICK_PERIOD_MS;
    while (1)
    {
        // Check if event in queue
        if (xQueueReceive(event_logs_queue, &event_log, queue_wait) == pdTRUE)
        {
            // Log event
            ESP_LOGI(TAG, "..."
                          "\n  > Event: %s"
                          "\n  > Data: %s",
                          event_log.event_name, event_log.event_data);
        }
    }
}