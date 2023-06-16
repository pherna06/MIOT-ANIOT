#ifndef __DISTANCE_SENSOR_READING_H__
#define __DISTANCE_SENSOR_READING_H__

#include <stdint.h>

typedef struct distance_sensor_reading {
    uint64_t reading_id;
    int      adc_reading;
    int      voltage_mv;
    int      distance_mm;
} distance_sensor_reading_t;

void make_distance_sensor_reading(
    distance_sensor_reading_t *reading,
    int adc_reading,
    int voltage_mv,
    int distance_mm
);

#endif // __DISTANCE_SENSOR_READING_H__