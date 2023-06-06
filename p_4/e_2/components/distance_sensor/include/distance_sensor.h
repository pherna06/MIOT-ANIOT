#ifndef __DISTANCE_SENSOR_H__
#define __DISTANCE_SENSOR_H__

// ESP-IDF component for Sharp GP2Y0A21YK0F IR distance sensor

// include for ESP events
#include "esp_event.h"

// include for ESP error codes
#include "esp_err.h"

// EVENTS
ESP_EVENT_DECLARE_BASE(DISTANCE_SENSOR_EVENT);
enum {
    DISTANCE_SENSOR_EVENT_READING,
};

// HANDLE
typedef struct distance_sensor_handle *distance_sensor_handle_t;

// CREATE ARGS
typedef struct distance_sensor_create_args {
    const char *name;

    struct {
        int channel_num;
        int gpio_num;
    } adc_input;

    struct {
        int period_ms;
        int samples;
    } sampling;

    struct {
        int queue_size;
    } readings;
} distance_sensor_create_args_t;

#define DISTANCE_SENSOR_DEFAULT_NAME CONFIG_DISTANCE_SENSOR_DEFAULT_NAME
#define DISTANCE_SENSOR_DEFAULT_ADC1_CHANNEL CONFIG_DISTANCE_SENSOR_DEFAULT_ADC1_CHANNEL
#define DISTANCE_SENSOR_DEFAULT_ADC2_CHANNEL CONFIG_DISTANCE_SENSOR_DEFAULT_ADC2_CHANNEL
#define DISTANCE_SENSOR_DEFAULT_PERIOD_MS CONFIG_DISTANCE_SENSOR_DEFAULT_PERIOD_MS
#define DISTANCE_SENSOR_DEFAULT_SAMPLES CONFIG_DISTANCE_SENSOR_DEFAULT_SAMPLES
#define DISTANCE_SENSOR_DEFAULT_QUEUE_SIZE CONFIG_DISTANCE_SENSOR_DEFAULT_QUEUE_SIZE

#ifdef CONFIG_DISTANCE_SENSOR_ADC_UNIT_1
#define DISTANCE_SENSOR_DEFAULT_ADC_CHANNEL DISTANCE_SENSOR_DEFAULT_ADC1_CHANNEL
#elif CONFIG_DISTANCE_SENSOR_ADC_UNIT_2
#define DISTANCE_SENSOR_DEFAULT_ADC_CHANNEL DISTANCE_SENSOR_DEFAULT_ADC2_CHANNEL
#endif

#define DISTANCE_SENSOR_CREATE_ARGS_DEFAULT() { \
    .name = DISTANCE_SENSOR_DEFAULT_NAME, \
    .adc_input = { \
        .channel_num = DISTANCE_SENSOR_DEFAULT_ADC_CHANNEL, \
        .gpio_num = -1, \
    }, \
    .sampling = { \
        .period_ms = DISTANCE_SENSOR_DEFAULT_PERIOD_MS, \
        .samples = DISTANCE_SENSOR_DEFAULT_SAMPLES, \
    }, \
    .readings = { \
        .queue_size = DISTANCE_SENSOR_DEFAULT_QUEUE_SIZE, \
    }, \
}

// READING
typedef struct distance_sensor_reading {
    uint64_t num;
    uint64_t timestamp;
    int      adc_reading;
    int      voltage_mv;
    uint8_t  distance_mm;
} distance_sensor_reading_t;


// FUNCTIONS
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