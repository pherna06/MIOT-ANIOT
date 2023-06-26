// INCLUDES --------------------------------------------------------------------

/* Header */
#include "task_monitor.h"

/* Tasks */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


/* Errors */
#include <esp_err.h>

/* Events */
#include <esp_event.h>

/* Logging */
#include <esp_log.h>

static const char *TAG = "task_monitor";

// -----------------------------------------------------------------------------

// DEFINES ---------------------------------------------------------------------

/* Events Loop */
#define EVENT_LOOP_QUEUE_SIZE 10
#define EVENT_LOOP_TASK_STACK_SIZE 4096

// -----------------------------------------------------------------------------

// MACROS ----------------------------------------------------------------------

/* Check if initialized */
#define CHECK_INITIALIZED() \
    if (!_initialized) { \
        ESP_LOGE(TAG, "Not initialized"); \
        return ESP_ERR_INVALID_STATE; \
    }

// -----------------------------------------------------------------------------

// EVENTS ----------------------------------------------------------------------

/* Define event base */
ESP_EVENT_DEFINE_BASE(TASK_MONITOR_EVENTS);

// -----------------------------------------------------------------------------

// STRUCTURES ------------------------------------------------------------------

/* Linked List for TaskHandle_t */
struct task_handle_node {
    const TaskHandle_t *handle;
    struct task_handle_node *next;
};

// -----------------------------------------------------------------------------

// STATIC VARIABLES ------------------------------------------------------------

/* Initialized Flag */
static bool _initialized = false;

/* Task Handles List */
static struct task_handle_node *_task_handles_list = NULL;

/* Monitor Task Handle */
uint32_t _monitor_task_period_ms;;
static TaskHandle_t _monitor_task_handle = NULL;

/* Event Loop Queue */
static esp_event_loop_handle_t _events_loop;

// -----------------------------------------------------------------------------

// STATIC FUNCTIONS ------------------------------------------------------------

/* Monitor Task */
static void monitor_task(
    void *pvParameter )
{
    esp_err_t err;
    
    while (1) {
        // > Traverse task handles list
        struct task_handle_node *node = _task_handles_list;
        while (node != NULL) {
            // >> Get task handle
            TaskHandle_t handle = *(node->handle);

            // >> Task status
            TaskStatus_t status;err = esp_event_loop_create_default();
            vTaskGetInfo(handle, &status, pdTRUE, eInvalid);

            // >> Post TASK_MONITOR_EVENT_NEW_STATUS
            err = esp_event_post_to(
                _events_loop                  ,
                TASK_MONITOR_EVENTS           ,
                TASK_MONITOR_EVENT_NEW_STATUS ,
                &status                       ,
                sizeof(status)                ,
                0                            );
            if (err == ESP_ERR_TIMEOUT) {
                ESP_LOGE(TAG, "Event Queue Full");
            }
            else if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error posting event: %s", esp_err_to_name(err));
            }

            // >> Next node
            node = node->next;
        }
        vTaskDelay(_monitor_task_period_ms / portTICK_PERIOD_MS);
    }
}

// -----------------------------------------------------------------------------

// PUBLIC FUNCTIONS ------------------------------------------------------------

/* Initialize Task Monitor */
esp_err_t task_monitor_init(
    void )
{
    esp_err_t err;

    // > Check if already initialized
    if (_initialized) {
        ESP_LOGE(TAG, "Already initialized");
        return ESP_FAIL;
    }

    // > Create event loop
    esp_event_loop_args_t loop_args = {
        .queue_size      = EVENT_LOOP_QUEUE_SIZE      ,
        .task_name       = "hall_events_loop"         ,
        .task_priority   = 5                          ,
        .task_stack_size = EVENT_LOOP_TASK_STACK_SIZE ,
        .task_core_id    = tskNO_AFFINITY            };
    if ( (err = esp_event_loop_create(&loop_args, &_events_loop)) ) {
        ESP_LOGE(TAG, "Error creating event loop: %s", esp_err_to_name(err));
        return err;
    }

    // > Initialized flag
    _initialized = true;

    return ESP_OK;
}

/* Deinitialize Task Monitor */
esp_err_t task_monitor_deinit(
    void )
{
    CHECK_INITIALIZED();
    esp_err_t err = ESP_OK;

    // > Delete event loop
    if ( (err = esp_event_loop_delete(_events_loop)) ) {
        ESP_LOGW(TAG, "Could not delete event loop: %s", esp_err_to_name(err));
    }

    // > Free task handles list
    struct task_handle_node *node = _task_handles_list;
    while (node != NULL) {
        struct task_handle_node *next_node = node->next;
        free(node);
        node = next_node;
    }

    // > Initialized Flag
    _initialized = false;

    return err;
}

/* Task Monitor Start */
esp_err_t task_monitor_start(
    uint32_t period_ms )
{
    CHECK_INITIALIZED();
    BaseType_t rtos;

    // > Monitor task
    if (_monitor_task_handle != NULL) {
        ESP_LOGE(TAG, "Monitor task already created");
        return ESP_FAIL;
    }
    else {
        _monitor_task_period_ms = period_ms;

        // > Create Monitor Task
        ESP_LOGI(TAG, "Creating monitor task");
        if ( (rtos = xTaskCreate(
            monitor_task                  ,
            "monitor_task"                ,
            2048                          ,
            NULL                          ,
            5                             ,
            &_monitor_task_handle         )
        ) != pdPASS ) {
            ESP_LOGE(TAG, "Error creating monitor task: %s",
                esp_err_to_name(rtos));
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}

/* Task Monitor Stop */
esp_err_t task_monitor_stop(
    void )
{
    CHECK_INITIALIZED();

    // > Delete Monitor Task
    if (_monitor_task_handle == NULL) {
        ESP_LOGW(TAG, "Monitor task not running");
    }
    else {
        ESP_LOGI(TAG, "Deleting monitor task");
        vTaskDelete(_monitor_task_handle);
    }

    return ESP_OK;
}

/* Add task handle to task monitor */
esp_err_t task_monitor_add_task(
    const TaskHandle_t *task_handle )
{
    CHECK_INITIALIZED();

    // > Create new node
    struct task_handle_node *node = malloc(sizeof(struct task_handle_node));
    if (node == NULL) {
        ESP_LOGE(TAG, "Error allocating memory for new node");
        return ESP_ERR_NO_MEM;
    }
    node->handle = task_handle;
    node->next = NULL;

    // > Add node to list
    if (_task_handles_list == NULL) {
        _task_handles_list = node;
    }
    else {
        struct task_handle_node *last_node = _task_handles_list;
        while (last_node->next != NULL) {
            last_node = last_node->next;
        }
        last_node->next = node;
    }

    return ESP_OK;
}

//// GETTERS -------------------------------------------------------------------

/* Get Event Loop Handle */
const esp_event_loop_handle_t *task_monitor_get_event_loop_handle(
    void )
{ return &_events_loop; }

/* Get Monitor Task Handle */
const TaskHandle_t *task_monitor_get_monitor_task_handle(
    void )
{ return &_monitor_task_handle; }

//// ----------------------------------------------------------------------------

// -----------------------------------------------------------------------------