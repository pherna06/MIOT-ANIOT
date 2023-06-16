#ifndef __HALL_SENSOR_H__
#define __HALL_SENSOR_H__

// ESP-IDF component for Hall Sensor

// include for ESP error codes
#include "esp_err.h"

// For Hall Sensor Events
#include "hall_sensor_events.h"

// For ESP errors:
#include "esp_err.h"


// FUNCTIONS //

esp_err_t hall_sensor_init(
    void
);

esp_err_t hall_sensor_start(
    uint32_t period_ms
);

esp_err_t hall_sensor_stop(
    void
);

void hall_sensor_deinit(
    void
);

#endif // __HALL_SENSOR_H__
