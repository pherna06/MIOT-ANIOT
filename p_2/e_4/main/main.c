// INCLUDES --------------------------------------------------------------------

/* Tasks */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ADC */
#include "driver/adc.h"

/* Events */
#include <esp_event.h>

/* Logging */
#include <esp_log.h>

static const char *TAG = "app_main";

// -----------------------------------------------------------------------------

// DEFINES --------------------------------------------------------------------

/* Period (ms) of the reading loop */
#define PERIOD_MS 1000

/* Size of the event loop queue */
#define QUEUE_SIZE 5

// -----------------------------------------------------------------------------

// EVENTS ---------------------------------------------------------------------

/* Hall Events */
ESP_EVENT_DEFINE_BASE(HALL_EVENTS);

enum {
    HALL_EVENT_NEW_SAMPLE,
};

/* Hall Events Handler */
static void hall_newsample(
        void *handler_args,
        esp_event_base_t base,
        int32_t id,
        void *event_data)
{
    switch(id) {
        case HALL_EVENT_NEW_SAMPLE:
            ESP_LOGI(TAG, "Hall sensor: %d", *(int *)event_data);
            break;
        default:
            ESP_LOGW(TAG, "Unknown event id: %d", id);
    }
}

// -----------------------------------------------------------------------------

// TASKS ----------------------------------------------------------------------

/* Reading Task Parameters */
struct reading_task_parameters
{
    int period_ms;
    esp_event_loop_handle_t hall_events_loop;
};

/* Reading Task Function */
void reading_task(void *pvParameter)
{
    esp_err_t err;
    
    struct reading_task_parameters *params =
            (struct reading_task_parameters *)pvParameter;

    // > Hall Sensor Reading Loop
    ESP_LOGI(TAG, "Starting reading loop with %d ms period...", params->period_ms);
    while (1) {
        int hall = hall_sensor_read();
        err = esp_event_post_to(
            params->hall_events_loop ,
            HALL_EVENTS              ,
            HALL_EVENT_NEW_SAMPLE    ,
            &hall                    ,
            sizeof(hall)             ,
            0                       );
        if (err == ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "Timeout sending hall reading to event loop");
        } else if (err != ESP_OK) {
            ESP_LOGW(TAG, "Error sending hall reading to event loop: %d", err);
        }
        vTaskDelay(params->period_ms / portTICK_PERIOD_MS);
    }
}

// -----------------------------------------------------------------------------

// APP MAIN -------------------------------------------------------------------

void app_main(void)
{
    esp_err_t err;
    
    // > Log Hall Sensor GPIO use warning
    ESP_LOGW(TAG, "\n> This app takes readings from the internal Hall sensor, "
                  "which uses channels 0 and 3 of ADC1 "
                  "(GPIO 36 and 39)"
                  "\n> Do not connect anything to these pings and "
                  "do not change their configuration!!!");

    // > Enable ADC1 to use Hall Sensor (by configuring width)
    if ( (err = adc1_config_width(ADC_WIDTH_BIT_12)) ) {
        ESP_LOGE(TAG, "Could not set ADC1 width: %s", esp_err_to_name(err));
        return;
    }

    // > Create event loop
    esp_event_loop_handle_t hall_events_loop;
    esp_event_loop_args_t loop_args = {
        .queue_size      = QUEUE_SIZE         ,
        .task_name       = "hall_events_loop" ,
        .task_priority   = 5                  ,
        .task_stack_size = 2048               ,
        .task_core_id    = tskNO_AFFINITY    };
    if ( (err = esp_event_loop_create(&loop_args, &hall_events_loop)) ) {
        ESP_LOGE(TAG, "Could not create event loop: %s", esp_err_to_name(err));
        return;
    }

    // > Register HALL EVENTS handler
    if ( (err = esp_event_handler_instance_register_with(
        hall_events_loop,
        HALL_EVENTS,
        ESP_EVENT_ANY_ID,
        &hall_newsample,
        NULL,
        NULL
    )) ) {
        ESP_LOGE(TAG, "Could not register HALL EVENTS handler: %s",
            esp_err_to_name(err));
        return;
    }

    // > Create task parameters
    struct reading_task_parameters params = {
        .period_ms = PERIOD_MS                ,
        .hall_events_loop = hall_events_loop };

    // > Create taskl
    xTaskCreate(&reading_task, "reading_task", 2048, &params, 5, NULL);

    // > Delay indefinitely (to keep variables in memory)
    while (1) vTaskDelay(portMAX_DELAY);
}

// -----------------------------------------------------------------------------