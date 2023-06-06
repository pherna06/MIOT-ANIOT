#include "distance_sensor.h"
#include "./util/adc.c"
#include "./util/sampling.c"
#include "./util/readings.c"

#include "string.h"

#define DISTANCE_SENSOR_NAME_BUFSIZE 32

struct distance_sensor_handle {
    char name[DISTANCE_SENSOR_NAME_BUFSIZE];

    // ADC Input
    adc_input_t adc_input;

    // Sampling
    sampling_t sampling;

    // Readings
    readings_t readings;
};

// VOLTAGE TO DISTANCE FUNCTION
uint8_t _voltage_mv_to_distance_mm(int voltage_mv) {
    // P1
    // - x (mV) = 300
    // - y (1/mm) = 1/(400 + 4.2) = 0.002474

    // P2
    // - x (mV) = 3000
    // - y (1/mm) = 1/(35 + 4.2)  = 0.025510

    // line equation given two points
    static const float m = (0.025510 - 0.002474) / (3000 - 300);
    float y = m * ( (float) voltage_mv - 300 ) - 0.002474;

    // from 1/(L + 4.2) = y
    return (uint8_t) ( (1 / y) - 4.2 );
}

// SAMPLING FUNCTION
void _sampling_fn(void *arg) {
    static const char *TAG = "(Distance Sensor) Sampling Function";
    esp_err_t err;

    distance_sensor_handle_t handle = (distance_sensor_handle_t) arg;

    // Reading
    distance_sensor_reading_t reading;

    // Timestamp
    reading.timestamp = _get_timestamp();

    // Multisample ADC readings
    reading.adc_reading = _adc_multisample(
        &(handle->sampling),
        _adc_read,
        &(handle->adc_input)
    );

    // To voltage
    reading.voltage_mv = _adc_reading_to_voltage_mv(reading.adc_reading);

    // To distance
    reading.distance_mm = _voltage_mv_to_distance_mm(reading.voltage_mv);

    // Save reading
    _save_last_reading(&(handle->readings), &reading);

    // post DISTANCE_SENSOR_EVENT_READING
    if ( (err = esp_event_post(
        DISTANCE_SENSOR_EVENT,
        DISTANCE_SENSOR_EVENT_READING,
        &handle,
        sizeof(distance_sensor_reading_t),
        0
    )) ) {
        ESP_LOGE(TAG, "Could not post DISTANCE_SENSOR_EVENT_READING");
    }
}

// CREATE FUNCTION
esp_err_t distance_sensor_create(
    distance_sensor_create_args_t *args,
    distance_sensor_handle_t *handle
) {
    static const char *TAG = "(Distance Sensor) Create";
    esp_err_t err;

    // Check handle is not created
    if ( *handle != NULL ) {
        ESP_LOGE(TAG, "Handle must be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // Create handle
    *handle = malloc(sizeof(struct distance_sensor_handle));
    if ( *handle == NULL ) {
        ESP_LOGE(TAG, "Could not allocate memory for handle");
        return ESP_ERR_NO_MEM;
    }

    // Name
    strncpy((*handle)->name, args->name, DISTANCE_SENSOR_NAME_BUFSIZE);

    // ADC Input
    if ( (err = _configure_adc_input(
        &((*handle)->adc_input),
        args->adc_input.gpio_num,
        args->adc_input.channel_num
    )) ) {
        ESP_LOGE(TAG, "ADC configuration failed");
        goto ds_create_error__after_handle_allocation;
    }

    // Sampling
    if ( (err = _configure_sampling(
        &((*handle)->sampling),
        _sampling_fn,
        *handle,
        args->sampling.period_ms,
        args->sampling.samples
    )) ) {
        ESP_LOGE(TAG, "Sampling configuration failed");
        goto ds_create_error__after_adc_input_configuration;
    }

    // Readings
    if ( (err = _configure_readings(
        &((*handle)->readings),
        args->readings.queue_size
    ))) {
        ESP_LOGE(TAG, "Readings configuration failed");
        goto ds_create_error__after_sampling_configuration;
    }

    return ESP_OK;

ds_create_error__after_sampling_configuration:
    _deconfigure_sampling(&((*handle)->sampling));
ds_create_error__after_adc_input_configuration:
    _deconfigure_adc_input(&((*handle)->adc_input));
ds_create_error__after_handle_allocation:
    free(*handle);

    return err;
}

// DESTROY FUNCTION
void distance_sensor_delete(distance_sensor_handle_t handle) {
    _deconfigure_readings(&(handle->readings));
    _deconfigure_sampling(&(handle->sampling));
    _deconfigure_adc_input(&(handle->adc_input));
    free(handle);
}

// START FUNCTION
esp_err_t distance_sensor_start(distance_sensor_handle_t handle) {
    static const char *TAG = "(Distance Sensor) Start";
    esp_err_t err;

    if ( handle == NULL ) {
        ESP_LOGE(TAG, "Handle must not be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // Start sampling
    if ( (err = _start_sampling(&(handle->sampling))) ) {
        ESP_LOGE(TAG, "Error starting sampling");
        return err;
    }

    return ESP_OK;
}

// STOP FUNCTION
esp_err_t distance_sensor_stop(distance_sensor_handle_t handle) {
    static const char *TAG = "(Distance Sensor) Stop";
    esp_err_t err;

    if ( handle == NULL ) {
        ESP_LOGE(TAG, "Handle must not be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // Stop sampling
    if ( (err = _stop_sampling(&(handle->sampling))) ) {
        ESP_LOGE(TAG, "Error stopping sampling");
        return err;
    }

    return ESP_OK;
}

// GET LAST READING FUNCTION
esp_err_t distance_sensor_get_reading(
    distance_sensor_handle_t handle,
    distance_sensor_reading_t *reading
) {
    static const char *TAG = "(Distance Sensor) Get Last Reading";
    esp_err_t err;

    if ( handle == NULL ) {
        ESP_LOGE(TAG, "Handle must not be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // Get oldest reading
    if ( (err = _get_oldest_reading(
        &(handle->readings),
        reading
    )) ) {
        ESP_LOGE(TAG, "Error getting oldest reading");
        return err;
    }

    return ESP_OK;
}

// GET NAME FUNCTION
const char *distance_sensor_get_name(distance_sensor_handle_t handle) {
    return handle->name;
}
