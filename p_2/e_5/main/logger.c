// INCLUDES --------------------------------------------------------------------

/* C-Strings */
#include <string.h>

/* Tasks & Queues */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/* Errors */
#include <esp_err.h>

/* Events */
#include <esp_event.h>

/* Hall Sampling (EVENTS) */
#include <hall_sampling.h>

/* Task Monitor (EVENTS) */
#include <task_monitor.h>

/* Logging */
#include <esp_log.h>

static const char *TAG = "app_main";

// -----------------------------------------------------------------------------

// DEFINES ---------------------------------------------------------------------

/* Event Message */
#define EVENT_MSG_NAME_SIZE 64
#define EVENT_MSG_DATA_SIZE 1028

/* Event Message Queue Size */
#define EVENT_MSG_QUEUE_SIZE 10

// STATIC VARIABLES ------------------------------------------------------------

/* Logger Task Handle */
static TaskHandle_t _logger_task_handle;

// -----------------------------------------------------------------------------

// STRUCTURES -----------------------------------------------------------------

struct event_message {
    char event_name[EVENT_MSG_NAME_SIZE];
    char event_data[EVENT_MSG_DATA_SIZE];
};

// TASKS -----------------------------------------------------------------------

//// EVENT HANDLERS ------------------------------------------------------------

/* Hall Sampling Events Handler */
static void hall_sampling_events_handler(
    void *handler_args    ,
    esp_event_base_t base ,
    int32_t id            ,
    void *event_data      )
{
    BaseType_t rtos;
    
    // > Get event message queue
    QueueHandle_t event_message_queue = *(QueueHandle_t *)handler_args;

    // > Create event message
    struct event_message event_message;

    // > Fill event message
    switch (id) {
        case HALL_SAMPLING_EVENT_NEW_SAMPLE:
            strncpy(
                event_message.event_name         ,
                "HALL_SAMPLING_EVENT_NEW_SAMPLE" ,
                EVENT_MSG_NAME_SIZE             );
            sprintf(
                event_message.event_data ,
                "New sample: %d"         ,
                *(int *)event_data      );
            break;
        case HALL_SAMPLING_EVENT_FILTER_SAMPLE:
            strncpy(
                event_message.event_name            ,
                "HALL_SAMPLING_EVENT_FILTER_SAMPLE" ,
                EVENT_MSG_NAME_SIZE                );
            sprintf(
                event_message.event_data ,
                "Filtered sample: %f"    ,
                *(float *)event_data     );
            break;
        default:
            strncpy(
                event_message.event_name      ,
                "HALL_SAMPLING_EVENT_UNKONWN" ,
                EVENT_MSG_NAME_SIZE          );
            sprintf(
                event_message.event_data ,
                "Unknown event id: %d"   ,
                id                      );
    }

    // > Push event message to queue
    if ( (rtos = xQueueSend(
        event_message_queue ,
        &event_message      ,
        0                   )
    ) != pdTRUE ) {
        ESP_LOGW(TAG, "Error pushing event message to queue");
    }
}

/* Task Monitor Events Handler */
static void task_monitor_events_handler(
    void *handler_args    ,
    esp_event_base_t base ,
    int32_t id            ,
    void *event_data      )
{
    BaseType_t rtos;
    
    // > Get event message queue
    QueueHandle_t event_message_queue = *(QueueHandle_t *)handler_args;

    // > Create event message
    struct event_message event_message;

    // > Fill event message
    switch (id) {
        case TASK_MONITOR_EVENT_NEW_STATUS:
            strncpy(
                event_message.event_name        ,
                "TASK_MONITOR_EVENT_NEW_STATUS" ,
                EVENT_MSG_NAME_SIZE            );
            sprintf(
                event_message.event_data                            ,
                "Task: %s | Priority: %d | Left stack size: %d"     ,
                ((TaskStatus_t *)event_data)->pcTaskName            ,
                ((TaskStatus_t *)event_data)->uxCurrentPriority     ,
                ((TaskStatus_t *)event_data)->usStackHighWaterMark );
            break;
        default:
            strncpy(
                event_message.event_name      ,
                "TASK_MONITOR_EVENT_UNKONWN" ,
                EVENT_MSG_NAME_SIZE          );
            sprintf(
                event_message.event_data ,
                "Unknown event id: %d"   ,
                id                      );
    }

    // > Push event message to queue
    if ( (rtos = xQueueSend(
        event_message_queue ,
        &event_message      ,
        0                   )
    ) != pdTRUE ) {
        ESP_LOGW(TAG, "Error pushing event message to queue");
    }
}

//// ---------------------------------------------------------------------------

/* Logger Task */
void logger_task(
    void *pvParameters )
{
    BaseType_t rtos;
    esp_err_t err;

    // > Create event message queue
    QueueHandle_t event_message_queue;
    event_message_queue = xQueueCreate(
        EVENT_MSG_QUEUE_SIZE          ,
        sizeof(struct event_message) );
    if (event_message_queue == NULL) {
        ESP_LOGE(TAG, "Error creating event message queue");
        vTaskDelete(NULL);
    }

    // > Register HALL_SAMPLING_EVENTS handler
    const esp_event_loop_handle_t *hall_sampling_events_loop = 
        hall_sampling_get_event_loop_handle();
    if ( (err = esp_event_handler_instance_register_with(
        *hall_sampling_events_loop    ,
        HALL_SAMPLING_EVENTS          ,
        ESP_EVENT_ANY_ID              ,
        &hall_sampling_events_handler ,
        &event_message_queue          ,
        NULL                          )
    ) ) {
        ESP_LOGE(TAG, "Error registering HALL_SAMPLING_EVENTS handler");
        vQueueDelete(event_message_queue);
        vTaskDelete(NULL);
    }

    // > Register TASK_MONITOR_EVENTS handler
    const esp_event_loop_handle_t *monitor_events_loop =
        task_monitor_get_event_loop_handle();
    if ( (err = esp_event_handler_instance_register_with(
        *monitor_events_loop         ,
        TASK_MONITOR_EVENTS          ,
        ESP_EVENT_ANY_ID             ,
        &task_monitor_events_handler ,
        &event_message_queue         ,
        NULL                         )
    ) ) {
        ESP_LOGE(TAG, "Error registering TASK_MONITOR_EVENTS handler");
        vQueueDelete(event_message_queue);
        vTaskDelete(NULL);
    }

    // > Logging Loop
    struct event_message event_message;
    while (1) {
        // > Wait for new event message
        if ( (rtos = xQueueReceive(
            event_message_queue ,
            &event_message      ,
            portMAX_DELAY       )
        ) != pdTRUE ) {
            ESP_LOGE(TAG, "Error receiving event message from queue");
            vTaskDelete(NULL);
        }

        // > Log event message
        ESP_LOGI(TAG, "..."
                      "\n  > Event: %s"
                      "\n  > Data: %s"          ,
                      event_message.event_name  ,
                      event_message.event_data );
    }
}

// -----------------------------------------------------------------------------

// PUBLIC FUNCTIONS ------------------------------------------------------------

/* Start Logger Task */
esp_err_t logger_start(
    void )
{
    BaseType_t rtos;

    // > Check if logger task is already running
    if (_logger_task_handle != NULL) {
        ESP_LOGW(TAG, "Logger task is already running");
        return ESP_FAIL;
    }

    // > Create logger task
    if ( (rtos = xTaskCreate(
        logger_task            ,
        "logger_task"          ,
        8192                   ,
        NULL                   ,
        1                      ,
        &_logger_task_handle   )
    ) != pdPASS ) {
        ESP_LOGE(TAG, "Error creating logger task");
        return ESP_FAIL;
    }

    // > Return
    return ESP_OK;
}

/* Stop Logger Task */
esp_err_t logger_stop(
    void )
{
    // > Check if logger task is running
    if (_logger_task_handle == NULL) {
        ESP_LOGW(TAG, "Logger task is not running");
        return ESP_FAIL;
    }

    // > Delete logger task
    vTaskDelete(_logger_task_handle);

    // > Return
    return ESP_OK;
}

//// GETTERS -------------------------------------------------------------------

/* Get Logger Task Handle */
const TaskHandle_t *logger_get_logger_task_handle(
    void )
{ return &_logger_task_handle; }

//// ---------------------------------------------------------------------------

// -----------------------------------------------------------------------------