#ifndef __DISTANCE_SENSOR_CREATE_ARGS_H__
#define __DISTANCE_SENSOR_CREATE_ARGS_H__

// DEFAULT ARGS [MENUCONFIG] //
#define DISTANCE_SENSOR_DEFAULT_NAME         CONFIG_DISTANCE_SENSOR_DEFAULT_NAME
#define DISTANCE_SENSOR_DEFAULT_ADC1_CHANNEL CONFIG_DISTANCE_SENSOR_DEFAULT_ADC1_CHANNEL
#define DISTANCE_SENSOR_DEFAULT_ADC2_CHANNEL CONFIG_DISTANCE_SENSOR_DEFAULT_ADC2_CHANNEL
#define DISTANCE_SENSOR_DEFAULT_PERIOD_MS    CONFIG_DISTANCE_SENSOR_DEFAULT_PERIOD_MS
#define DISTANCE_SENSOR_DEFAULT_SAMPLES      CONFIG_DISTANCE_SENSOR_DEFAULT_SAMPLES
#define DISTANCE_SENSOR_DEFAULT_QUEUE_SIZE   CONFIG_DISTANCE_SENSOR_DEFAULT_QUEUE_SIZE

#ifdef CONFIG_DISTANCE_SENSOR_ADC_UNIT_1
  #define DISTANCE_SENSOR_DEFAULT_ADC_CHANNEL DISTANCE_SENSOR_DEFAULT_ADC1_CHANNEL
#elif CONFIG_DISTANCE_SENSOR_ADC_UNIT_2
  #define DISTANCE_SENSOR_DEFAULT_ADC_CHANNEL DISTANCE_SENSOR_DEFAULT_ADC2_CHANNEL
#endif

// CREATE ARGS //
typedef struct distance_sensor_create_args {
    const char *name;

    struct {
        int channel_num;
        int gpio_num;
    } adc_input;

    struct {
        int period_ms;
    } sampling_timer;

    struct {
        int samples_per_reading;
    } multisampling;

    struct {
        int queue_size;
    } storage;
} distance_sensor_create_args_t;

// DEFAULT MACRO //
#define DISTANCE_SENSOR_CREATE_ARGS_DEFAULT() { \
    .name = DISTANCE_SENSOR_DEFAULT_NAME, \
    .adc_input = { \
        .channel_num = DISTANCE_SENSOR_DEFAULT_ADC_CHANNEL, \
        .gpio_num = -1, \
    }, \
    .sampling_timer = { \
        .period_ms = DISTANCE_SENSOR_DEFAULT_PERIOD_MS, \
    }, \
    .multisampling = { \
        .samples_per_reading = DISTANCE_SENSOR_DEFAULT_SAMPLES, \
    }, \
    .storage = { \
        .queue_size = DISTANCE_SENSOR_DEFAULT_QUEUE_SIZE, \
    }, \
}

#endif // __DISTANCE_SENSOR_CREATE_ARGS_H__