#ifndef __HALL_SENSOR_H__
#define __HALL_SENSOR_H__

// ESP-IDF component for Hall Sensor
#include "esp_err.h" // esp_err_t


// FUNCTIONS //

esp_err_t hall_sensor_init(
    void
);

int hall_sensor_get_reading(
    void
);

void hall_sensor_deinit(
    void
);

#endif // __HALL_SENSOR_H__
