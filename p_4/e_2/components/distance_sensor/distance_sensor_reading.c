#include "distance_sensor_reading.h"

void make_distance_sensor_reading(
    distance_sensor_reading_t *reading,
    int adc_reading,
    int voltage_mv,
    int distance_mm
) {
    static uint64_t reading_id_counter = 0;

    reading->reading_id = ++reading_id_counter;
    reading->adc_reading = adc_reading;
    reading->voltage_mv = voltage_mv;
    reading->distance_mm = distance_mm;
}