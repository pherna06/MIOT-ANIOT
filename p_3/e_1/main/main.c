#include <stdio.h>

// include for ESP logging
#include "esp_log.h"

// include for ESP events
#include "esp_event.h"
ESP_EVENT_DECLARE_BASE(APP_EVENTS);

enum
{
    HALL_EVENT_NEWSAMPLE
};

ESP_EVENT_DEFINE_BASE(APP_EVENTS);

// APP_EVENTS event loop
esp_event_loop_handle_t event_loop;

// include for ESP HRT timer
#include "esp_timer.h"

// include for Hall sensor reading
#include "driver/adc.h"

#define PERIOD_MS 2000

// HALL_EVENT_NEWSAMPLE event handler
static void hall_event_newsample_handler(
        void* arg,
        esp_event_base_t event_base,
        int32_t event_id, 
        void* event_data)
{
    // TAG
    static const char* TAG = "HALL_EVENT_NEWSAMPLE";
    
    // read Hall sensor
    int hall = *(int*) event_data;

    // print Hall sensor value
    ESP_LOGI(TAG, "Hall sensor: %d", hall);
}

// periodic timer callback
static void periodic_timer_callback(void* arg)
{
    // event loop
    esp_event_loop_handle_t* event_loop = (esp_event_loop_handle_t*) arg;
    
    // read Hall sensor
    int hall = hall_sensor_read();

    // post HALL_EVENT_NEWSAMPLE
    ESP_ERROR_CHECK(
        esp_event_post_to(*event_loop, APP_EVENTS, HALL_EVENT_NEWSAMPLE,
            &hall, sizeof(hall), portMAX_DELAY)
    );
}

void app_main(void)
{    
    // Initialize event loop
    esp_event_loop_args_t event_loop_args = {
        .queue_size = 5,
        .task_name = "event_loop_task",
        .task_priority = 1,
        .task_stack_size = 2048,
        .task_core_id = tskNO_AFFINITY
    };

    ESP_ERROR_CHECK(esp_event_loop_create(&event_loop_args, &event_loop));

    // Register HALL_EVENT_NEWSAMPLE event handler
    ESP_ERROR_CHECK(
        esp_event_handler_register_with(
            event_loop, APP_EVENTS, HALL_EVENT_NEWSAMPLE,
            &hall_event_newsample_handler, NULL
        )
    );


    // Timer args
    esp_timer_create_args_t periodic_timer_args = {
        .callback = &periodic_timer_callback,
        .arg = &event_loop,
        .name = "periodic"
    };

    // Create timer
    esp_timer_handle_t periodic_timer;
    esp_timer_create(&periodic_timer_args, &periodic_timer);

    // Start timer
    esp_timer_start_periodic(periodic_timer, PERIOD_MS * 1000);
}
