#ifndef __DISTANCE_SENSOR_H__
#define __DISTANCE_SENSOR_H__

// ESP-IDF component for Sharp GP2Y0A21YK0F IR distance sensor

// include for ESP error codes
#include "esp_err.h"

// For Distance Sensor Events
#include "distance_sensor_events.h"

// For Distance Sensor Create Args
#include "distance_sensor_create_args.h"

// For Distance Sensor Reading
#include "distance_sensor_reading.h"


// HANDLE //
typedef struct distance_sensor_handle *distance_sensor_handle_t;





// FUNCTIONS //

esp_err_t distance_sensor_create(
    distance_sensor_create_args_t *args,
    distance_sensor_handle_t *handle
);

void distance_sensor_delete(
    distance_sensor_handle_t handle
);

esp_err_t distance_sensor_start(
    distance_sensor_handle_t handle
);

esp_err_t distance_sensor_stop(
    distance_sensor_handle_t handle
);

esp_err_t distance_sensor_get_reading(
    distance_sensor_handle_t handle,
    distance_sensor_reading_t *reading
);

const char *distance_sensor_get_name(
    distance_sensor_handle_t handle
);

#endif // __DISTANCE_SENSOR_H__