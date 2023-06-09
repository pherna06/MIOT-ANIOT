#include "util/storage.h"

// inclde for ESP logs
#include "esp_log.h"

static char const *TAG = "Distance Sensor [Storage Utils]";

// Storage [for Distance Sensor Handle] //
#define READING_SIZE sizeof(distance_sensor_reading_t)

// FUNCTIONS //

// Configure Storage
esp_err_t configure_storage(
    storage_t *storage,
    int queue_size
) {
    // If queue size < 2, queue size is 1
    if (queue_size < 2) {
        queue_size = 1;
    }

    // Create queue
    if ( (storage->queue = xQueueCreate(queue_size, READING_SIZE)) == NULL ) {
        ESP_LOGE(TAG, "Could not create queue");
        return ESP_FAIL;
    }
    storage->queue_size = queue_size;

    return ESP_OK;
}

// Delete Storage
void delete_storage(
    storage_t *storage
) {
    vQueueDelete(storage->queue);
}

// Push Reading
esp_err_t push_reading(
    storage_t *storage,
    distance_sensor_reading_t *reading
) {
    esp_err_t err;

    // If queue is full, pop oldest reading
    if ( uxQueueSpacesAvailable(storage->queue) == 0 ) {
        distance_sensor_reading_t oldest_reading;
        if ( (err = pop_reading(storage, &oldest_reading)) ) {
            ESP_LOGE(TAG, "Could not leave space for new reading");
            return err;
        }
    }

    // Push reading to queue
    if ( xQueueSend(storage->queue, reading, 0) != pdTRUE ) {
        ESP_LOGE(TAG, "Could not push reading to queue");
        return ESP_FAIL;
    }

    return ESP_OK;
}

// Pop Reading
esp_err_t pop_reading(
    storage_t *storage,
    distance_sensor_reading_t *reading
) {
    // Pop reading from queue
    if ( xQueueReceive(storage->queue, reading, 0) != pdTRUE ) {
        ESP_LOGE(TAG, "Could not pop reading from queue");
        return ESP_FAIL;
    }

    return ESP_OK;
}

