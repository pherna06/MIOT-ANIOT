#include "distance_sensor.h"

// For ESP errors:
#include "esp_err.h"

// For ESP log:
#include "esp_log.h"

// For RTOS queue:
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct readings {
    QueueHandle_t queue;
    int size;
    int count;
} readings_t;

esp_err_t _configure_readings(
    readings_t *readings,
    int queue_size
) {
    static const char *TAG = "(Distance Sensor Util) Configure Readings";

    // Create queue
    if ( !(readings->queue = xQueueCreate(
        queue_size,
        sizeof(distance_sensor_reading_t)
    )) ) {
        ESP_LOGE(TAG, "Could not create queue");
        return ESP_FAIL;
    }

    // Set size
    readings->size = queue_size;

    // Set count
    readings->count = 0;

    return ESP_OK;
}

void _deconfigure_readings(readings_t *readings) {
    vQueueDelete(readings->queue);
}

// Save reading
void _save_last_reading(
    readings_t *readings,
    distance_sensor_reading_t *reading
) {
    // Update reading count
    reading->num = ++(readings->count);

    // If queue is full, discard oldest reading
    if ( uxQueueSpacesAvailable(readings->queue) == 0 ) {
        distance_sensor_reading_t to_discard;
        xQueueReceive(readings->queue, &to_discard, 0);
    }

    // Save new reading
    xQueueSend(readings->queue, reading, 0);
}

// Get oldest reading
esp_err_t _get_oldest_reading(
    readings_t *readings,
    distance_sensor_reading_t *reading
) {
    if ( xQueueReceive(readings->queue, reading, 0) ) {
        return ESP_OK;
    } else {
        return ESP_FAIL;
    }
}

