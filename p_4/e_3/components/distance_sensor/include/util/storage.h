#ifndef __DISTANCE_SENSOR_UTIL_STORAGE_H__
#define __DISTANCE_SENSOR_UTIL_STORAGE_H__

// For ESP errors
#include "esp_err.h"

// For FreeRTOS queue
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// For distance_sensor_reading_t
#include "distance_sensor_reading.h"


// Storage [for Distance Sensor Handle] //
typedef struct storage {
    QueueHandle_t queue;
    int queue_size;
} storage_t;




// FUNCTIONS //
esp_err_t configure_storage(
    storage_t *storage,
    int queue_size
);

void delete_storage(
    storage_t *storage
);

esp_err_t push_reading(
    storage_t *storage,
    distance_sensor_reading_t *reading
);

esp_err_t pop_reading(
    storage_t *storage,
    distance_sensor_reading_t *reading
);

#endif // __DISTANCE_SENSOR_UTIL_STORAGE_H__