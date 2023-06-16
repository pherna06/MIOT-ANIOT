#ifndef __HALL_SENSOR_EVENTS_H__
#define __HALL_SENSOR_EVENTS_H__

// For ESP events
#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(HALL_SENSOR_EVENTS);
enum {
    HALL_SENSOR_READING_EVENT,
};

void post_hall_sensor_reading_event(
    void *event_data,
    size_t event_data_size
);

#endif // __HALL_SENSOR_EVENTS_H__