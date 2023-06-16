#ifndef __DISTANCE_SENSOR_UTIL_SAMPLING_TIMER_H__
#define __DISTANCE_SENSOR_UTIL_SAMPLING_TIMER_H__

// For ESP errors
#include "esp_err.h"

// For ESP log
#include "esp_log.h"

// For ESP HRT
#include "esp_timer.h"


// Sampling Timer [for Distance Sensor Handle] //
typedef struct sampling_timer {
    esp_timer_handle_t timer;
    uint32_t period_ms;
} sampling_timer_t;




// FUNCTIONS //

esp_err_t configure_sampling_timer(
    sampling_timer_t *sampling_timer,
    void (*sampling_fn)(void *),
    void *sampling_fn_args,
    uint32_t period_ms
);

void delete_sampling_timer(
    sampling_timer_t *sampling_timer
);

esp_err_t start_sampling_timer(
    sampling_timer_t *sampling_timer
);

esp_err_t stop_sampling_timer(
    sampling_timer_t *sampling_timer
);

#endif // __DISTANCE_SENSOR_UTIL_SAMPLING_TIMER_H__