#include "distance_sensor_events.h"

void post_distance_sensor_reading_event(
    void *event_data,
    size_t event_data_size
) {
    esp_event_post(
        DISTANCE_SENSOR_EVENTS,
        DISTANCE_SENSOR_READING_EVENT,
        event_data,
        event_data_size,
        0
    );
}