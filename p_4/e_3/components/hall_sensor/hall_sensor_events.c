#include "hall_sensor_events.h"

void post_hall_sensor_reading_event(
    void *event_data,
    size_t event_data_size
) {
    esp_event_post(
        HALL_SENSOR_EVENTS,
        HALL_SENSOR_READING_EVENT,
        event_data,
        event_data_size,
        0
    );
}