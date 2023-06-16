#ifndef __DISTANCE_SENSOR_UTIL_MULTISAMPLING_H__
#define __DISTANCE_SENSOR_UTIL_MULTISAMPLING_H__

// For ESP errors
#include "esp_err.h"

// For bool
#include <stdbool.h>

// Multisampling [for Distance Sensor Handle] //
typedef struct multisampling {
    bool enabled;
    int samples_per_reading;
} multisampling_t;




// FUNCTIONS //

esp_err_t configure_multisampling(
    multisampling_t *multisampling,
    int samples_per_reading
);

void delete_multisampling(
    multisampling_t *multisampling
);

esp_err_t do_multisampling(
    multisampling_t *multisampling,
    esp_err_t (*read_fn)(void *, int *),
    void *read_fn_arg,
    int *result
);

#endif // __DISTANCE_SENSOR_UTIL_MULTISAMPLING_H__