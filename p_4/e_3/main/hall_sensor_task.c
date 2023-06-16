#ifdef CONFIG_HALL_SENSOR_TASK

#include "hall_sensor.h"

// include ESP errors
#include "esp_err.h"

// include ESP events
#include "esp_event.h"

ESP_EVENT_DEFINE_BASE(HALL_SENSOR_EVENTS);

// include ESP logs
#include "esp_log.h"

static const char *HS_TASK = "Hall Sensor Task";

// define Hall Sensor period
#define HALL_SENSOR_PERIOD_MS CONFIG_HALL_SENSOR_PERIOD_MS

// Event Handler for Hall Sensor
static void hall_sensor_event_handler(
    void* handler_args,
    esp_event_base_t base,
    int32_t id,
    void* event_data
) {
    static const char *TAG = "HALL_SENSOR_READING_EVENT";
    
    // get handle
    int hall_reading = *((int *) event_data);

    // print reading
    ESP_LOGI(TAG, "\n"
                  " - Hall Sensor Value > %d\n",
                  hall_reading
    );
}

// Hall Sensor Task Function
void hall_sensor_task(void *pvParameter) {
    esp_err_t err;

    ESP_LOGI(HS_TASK, "Hall Sensor Task Running...");

    // Hall Sensor Init
    if ( (err = hall_sensor_init())) {
        ESP_LOGW(HS_TASK, "Hall Sensor Init Failed: %s", esp_err_to_name(err));
        goto hs_task__error_at_start;
    }

    // Register Event Handler
    if ( (err = esp_event_handler_register(
        HALL_SENSOR_EVENTS,
        HALL_SENSOR_READING_EVENT,
        &hall_sensor_event_handler,
        NULL
    ))) {
        ESP_LOGW(HS_TASK, "Hall Sensor Event Handler Register Failed: %s", esp_err_to_name(err));
        goto hs_task__error_after_hall_sensor_init;
    }

    // Hall Sensor Start
    if ( (err = hall_sensor_start(HALL_SENSOR_PERIOD_MS))) {
        ESP_LOGW(HS_TASK, "Hall Sensor Start Failed: %s", esp_err_to_name(err));
        goto hs_task__error_after_register_event_handler;
    }
    
    // Wait for delete notification
    while (1) {
        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY)) {
            break;
        }
    }

//hs_task__error_after_hall_sensor_start:
    // Stop Hall Sensor
    if ( (err = hall_sensor_stop())) {
        ESP_LOGW(HS_TASK, "Hall Sensor Stop Failed: %s", esp_err_to_name(err));
    }
hs_task__error_after_register_event_handler:
    // Nothing to do here
hs_task__error_after_hall_sensor_init:
    // Deinit Hall Sensor
    hall_sensor_deinit();
hs_task__error_at_start:
    vTaskDelete(NULL);
}

#endif // CONFIG_HALL_SENSOR_TASK