#ifndef __DISTANCE_SENSOR_EVENTS_H__
#define __DISTANCE_SENSOR_EVENTS_H__

// For ESP events
#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(DISTANCE_SENSOR_EVENTS);
enum {
    DISTANCE_SENSOR_READING_EVENT,
};

void post_distance_sensor_reading_event(
    void *event_data,
    size_t event_data_size
);

#endif // __DISTANCE_SENSOR_EVENTS_H__