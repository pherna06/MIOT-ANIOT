// INCLUDES --------------------------------------------------------------------

/* Header */
#include "hall_sampling.h"

/* Tasks & Queues */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/* ADC */
#include "driver/adc.h"

/* Errors */
#include <esp_err.h>

/* Events */
#include <esp_event.h>

/* Logging */
#include <esp_log.h>

static const char *TAG = "hall_sampling";

// -----------------------------------------------------------------------------

// DEFINES ---------------------------------------------------------------------

/* Size of event loop queue */
#define EVENT_LOOP_QUEUE_SIZE 5
#define EVENT_LOOP_TASK_STACK_SIZE 4096

/* Size of filter task queue */
#define FILTER_TASK_QUEUE_SIZE 5

/* Delay to wait for Filter Task to subscribe */
#define DELAY_BETWEEN_TASKS_MS 1000

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
ESP_EVENT_DEFINE_BASE(HALL_SAMPLING_EVENTS);

// -----------------------------------------------------------------------------

// STATIC VARIABLES ------------------------------------------------------------

/* Initialized Flag */
static bool _initialized = false;

/* Event Loop Handle */
static esp_event_loop_handle_t _events_loop = NULL;

/* Sampler Task */
static uint32_t _sampler_period_ms;
static TaskHandle_t _sampler_task_handle = NULL;

/* Filter Task Handle */
static uint32_t _filter_samples_num;
static TaskHandle_t _filter_task_handle = NULL;

// -----------------------------------------------------------------------------

// STATIC FUNCTIONS ------------------------------------------------------------

//// TASKS FUNCTIONS -----------------------------------------------------------

/* Sampler Task Function */
static void sampler_task(
    void *arg )
{
    esp_err_t err;

    // > Hall sensor reading loop
    while (1) {
        int hall = hall_sensor_read();
        err = esp_event_post_to(
            _events_loop                   ,
            HALL_SAMPLING_EVENTS           ,
            HALL_SAMPLING_EVENT_NEW_SAMPLE ,
            &hall                          ,
            sizeof(hall)                   ,
            0                             );
        if (err == ESP_ERR_TIMEOUT) {
            ESP_LOGE(TAG, "Sampler Task: Event loop queue full");
        }
        else if (err != ESP_OK) {
            ESP_LOGE(TAG, "Sampler Task: Error posting event: %s",
                esp_err_to_name(err));
        }
        vTaskDelay(_sampler_period_ms / portTICK_PERIOD_MS);
    }
}

/* Filter Task Event Handler */
static void filter_task_event_handler(
    void *handler_args    ,
    esp_event_base_t base ,
    int32_t id            ,
    void *event_data      )
{
    BaseType_t rtos;
    QueueHandle_t new_sample_queue = (QueueHandle_t)handler_args;

    switch (id) {
        case HALL_SAMPLING_EVENT_NEW_SAMPLE:
            if ( (rtos = xQueueSend(
                new_sample_queue ,
                event_data       ,
                0                )
            ) != pdTRUE) {
                ESP_LOGE(TAG, "Error sending new sample to filter task queue");
            }
            break;
        default:
            ESP_LOGW(TAG, "Unhandled event id: %d", id);
    }
}

/* Filter Task Function */
static void filter_task(
    void *arg )
{
    esp_err_t err;
    BaseType_t rtos;
    int hall_samples[_filter_samples_num];

    int hall_samples_idx  = 0;
    int hall_samples_sum  = 0;
    float hall_samples_mean = 0;

    // > Create queue to receive new samples from event handler
    QueueHandle_t new_sample_queue = xQueueCreate(
        FILTER_TASK_QUEUE_SIZE ,
        sizeof(int)           );
    if (new_sample_queue == NULL) {
        ESP_LOGE(TAG, "Filter Task: Error creating queue");
        ESP_LOGW(TAG, "Filter Task: Deleting task");
        vTaskDelete(NULL);
    }

    // > Register event handler
    ESP_LOGI(TAG, "Filter Task: Registering event handler");
    if ( (err = esp_event_handler_instance_register_with(
            _events_loop                   ,
            HALL_SAMPLING_EVENTS           ,
            HALL_SAMPLING_EVENT_NEW_SAMPLE ,
            &filter_task_event_handler     ,
            new_sample_queue               ,
            NULL                           )
    ) ) {
        ESP_LOGE(TAG, "Filter Task: Error registering event handler: %s",
            esp_err_to_name(err));
        vTaskDelete(NULL);
    }

    // > Fill hall_samples array up to _filter_samples_num - 1
    while (hall_samples_idx < _filter_samples_num - 1) {
        if ( (rtos = xQueueReceive(
            new_sample_queue                ,
            &hall_samples[hall_samples_idx] ,
            portMAX_DELAY                   )
        ) != pdTRUE) {
            ESP_LOGE(TAG, "Filter Task: Error receiving from new sample queue");
            vTaskDelete(NULL);
        }
        hall_samples_sum += hall_samples[hall_samples_idx];
        hall_samples_idx++;
    }

    // > Filtering loop
    while (1) {
        // > Wait for new sample
        if ( (rtos = xQueueReceive(
            new_sample_queue                 ,
            &hall_samples[hall_samples_idx]  ,
            portMAX_DELAY                    )
        ) != pdTRUE) {
            ESP_LOGE(TAG, "Filter Task: Error receiving from new sample queue");
            vTaskDelete(NULL);
        }

        // > Add new sample to sum
        hall_samples_sum += hall_samples[hall_samples_idx];

        // > Calculate new mean (float)
        hall_samples_mean = (float)hall_samples_sum / _filter_samples_num;

        // > Post FILTER SAMPLE event
        err = esp_event_post_to(
            _events_loop                      ,
            HALL_SAMPLING_EVENTS              ,
            HALL_SAMPLING_EVENT_FILTER_SAMPLE ,
            &hall_samples_mean                ,
            sizeof(hall_samples_mean)         ,
            0                                );
        if (err == ESP_ERR_TIMEOUT) {
            ESP_LOGE(TAG, "Filter Task: Event loop queue full");
        }
        else if (err != ESP_OK) {
            ESP_LOGE(TAG, "Filter Task: Error posting event: %s",
                esp_err_to_name(err));
        }

        // > Increment index
        hall_samples_idx = (hall_samples_idx + 1) % _filter_samples_num;

        // > Substract oldest sample from sum
        hall_samples_sum -= hall_samples[hall_samples_idx];
    }
}

// -----------------------------------------------------------------------------

// PUBLIC FUNCTIONS ------------------------------------------------------------

/* Initialize Hall sampling */
esp_err_t hall_sampling_init(
    void )
{
    esp_err_t err;

    // > Check if already initialized
    if (_initialized) {
        ESP_LOGE(TAG, "Already initialized");
        return ESP_OK;
    }

    // > Log Hall Sensor GPIO use warning
    ESP_LOGW(TAG, "\n> This app takes readings from the internal Hall sensor, "
                  "which uses channels 0 and 3 of ADC1 "
                  "(GPIO 36 and 39)"
                  "\n> Do not connect anything to these pings and "
                  "do not change their configuration!!!");

    // > Enable ADC1 to use Hall Sensor (by configuring width)
    if ( (err = adc1_config_width(ADC_WIDTH_BIT_12)) ) {
        ESP_LOGE(TAG, "Could not set ADC1 width: %s", esp_err_to_name(err));
        return err;
    }

    // > Create event loop
    esp_event_loop_args_t loop_args = {
        .queue_size      = EVENT_LOOP_QUEUE_SIZE      ,
        .task_name       = "hall_events_loop"         ,
        .task_priority   = 5                          ,
        .task_stack_size = EVENT_LOOP_TASK_STACK_SIZE ,
        .task_core_id    = tskNO_AFFINITY            };
    if ( (err = esp_event_loop_create(&loop_args, &_events_loop)) ) {
        ESP_LOGE(TAG, "Could not create event loop: %s", esp_err_to_name(err));
        return err;
    }

    // > Initialized Flag
    _initialized = true;

    return ESP_OK;
}

/* Deinitialize Hall sampling */
esp_err_t hall_sampling_deinit(
    void )
{
    CHECK_INITIALIZED();
    esp_err_t err = ESP_OK;

    // > Delete event loop
    if ( (err = esp_event_loop_delete(_events_loop)) ) {
        ESP_LOGW(TAG, "Could not delete event loop: %s", esp_err_to_name(err));
    }

    // > Initialized Flag
    _initialized = false;

    return err;
}

/* Start Hall sampling */
esp_err_t hall_sampling_start(
    uint32_t sample_period_ms   ,
    uint32_t filter_samples_num )
{
    CHECK_INITIALIZED();
    BaseType_t rtos;
    
    // > Filter Task
    if (_filter_task_handle != NULL) {
        ESP_LOGW(TAG, "Filter Task already running");
    }
    else {
        _filter_samples_num = filter_samples_num;

        // > Create Filter Task
        ESP_LOGI(TAG, "Creating Filter Task");
        if ( (rtos = xTaskCreate(
            filter_task          ,
            "filter_task"        ,
            2048                 ,
            NULL                 ,
            1                    ,
            &_filter_task_handle )
        ) != pdTRUE) {
            ESP_LOGE(TAG, "Could not create Filter Task");
            return ESP_FAIL;
        }
    }

    vTaskDelay(DELAY_BETWEEN_TASKS_MS / portTICK_PERIOD_MS);

    // > Sampler Task
    if (_sampler_task_handle != NULL) {
        ESP_LOGW(TAG, "Sampler Task already running");
    }
    else {
        _sampler_period_ms = sample_period_ms;
        
        // > Create Sampler Task
        ESP_LOGI(TAG, "Creating Sampler Task");
        if ( (rtos = xTaskCreate(
            sampler_task          ,
            "sampler_task"        ,
            2048                  ,
            NULL                  ,
            1                     ,
            &_sampler_task_handle  )
        ) != pdTRUE) {
            ESP_LOGE(TAG, "Could not create Sampler Task");
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}

/* Hall sampling stop */
esp_err_t hall_sampling_stop(
    void )
{
    CHECK_INITIALIZED();

    // > Delete Sampler Task
    if (_sampler_task_handle == NULL) {
        ESP_LOGW(TAG, "Sampler Task not running");
    }
    else {
        ESP_LOGI(TAG, "Deleting Sampler Task");
        vTaskDelete(_sampler_task_handle);
    }

    // > Delete Filter Task
    if (_filter_task_handle == NULL) {
        ESP_LOGW(TAG, "Filter Task not running");
    }
    else {
        ESP_LOGI(TAG, "Deleting Filter Task");
        vTaskDelete(_filter_task_handle);
    }

    return ESP_OK;
}

//// GETTERS -------------------------------------------------------------------

/* Get Event Loop Handle */
const esp_event_loop_handle_t *hall_sampling_get_event_loop_handle(
    void )
{ return &_events_loop; }

/* Get Sampler Task Handle */
const TaskHandle_t *hall_sampling_get_sampler_task_handle(
    void )
{ return &_sampler_task_handle; }

/* Get Filter Task Handle */
const TaskHandle_t *hall_sampling_get_filter_task_handle(
    void )
{ return &_filter_task_handle; }

//// ---------------------------------------------------------------------------

// -----------------------------------------------------------------------------