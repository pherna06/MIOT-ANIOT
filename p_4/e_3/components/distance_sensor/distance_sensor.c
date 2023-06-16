#include "distance_sensor.h"

#include "util/adc.h"
#include "util/sampling_timer.h"
#include "util/multisampling.h"
#include "util/storage.h"

#include "string.h"

// inclde for ESP logs
#include "esp_log.h"

static char const *TAG = "Distance Sensor";

// Voltage to Distance Logic
// Points (mV, 1/mm) //
// Point 0
#define POINT0_X 300
#define POINT0_Y 1 / (400 + 4.2)

// Point 1
#define POINT1_X 2000
#define POINT1_Y 1 / (60 + 4.2)

// Point 2
#define POINT2_X 3000
#define POINT2_Y 1 / (35 + 4.2)

// Slopes
static float _SLOPE_01 = (POINT1_Y - POINT0_Y) / (POINT1_X - POINT0_X);
static float _SLOPE_12 = (POINT2_Y - POINT1_Y) / (POINT2_X - POINT1_X);

// Distance Sensor [Handle] //
#define DISTANCE_SENSOR_NAME_BUFSIZE 32

struct distance_sensor_handle {
    char name[DISTANCE_SENSOR_NAME_BUFSIZE];

    // ADC Input
    adc_input_t adc_input;

    // Sampling Timer
    sampling_timer_t sampling_timer;

    // Multisampling
    multisampling_t multisampling;

    // Readings
    storage_t storage;
};

// VOLTAGE TO DISTANCE FUNCTION [LINEAR APPROXIMATION] //
static int _voltage_mv_to_distance_mm(int voltage_mv) {
    // Out of range
    if ( voltage_mv < POINT0_X ) {
        return -1;
    }

    float y = (voltage_mv < POINT1_X) ?
        (voltage_mv - POINT0_X) * _SLOPE_01 + POINT0_Y :
        (voltage_mv - POINT1_X) * _SLOPE_12 + POINT1_Y;

    return (int) ((1 / y) - 4.2);
}

// SAMPLING FUNCTION [FOR SAMPLING TIMER] //
static void _sampling_fn(void *arg) {
    esp_err_t err;

    distance_sensor_handle_t handle = (distance_sensor_handle_t) arg;

    // Multisample ADC readings
    int adc_reading = -1;
    if ( (err = do_multisampling(
        &(handle->multisampling),
        read_adc_input,
        &(handle->adc_input),
        &adc_reading
    )) ) {
        ESP_LOGE(TAG, "Multisampling failed");
        return;
    }

    // To voltage
    int voltage_mv = adc_reading_to_voltage_mv(adc_reading);

    // To distance
    int distance_mm = _voltage_mv_to_distance_mm(voltage_mv);

    // Create reading
    distance_sensor_reading_t reading;
    make_distance_sensor_reading(
        &reading,
        adc_reading,
        voltage_mv,
        distance_mm
    );

    // Save reading
    if ( (err = push_reading(&(handle->storage), &reading)) ) {
        ESP_LOGE(TAG, "Could not save reading");
        return;
    }

    // Post DISTANCE_SENSOR_READING_EVENT
    post_distance_sensor_reading_event(
        &handle,
        sizeof(distance_sensor_handle_t)
    );
}





// FUNCTIONS //

// Create Distance Sensor Handle
esp_err_t distance_sensor_create(
    distance_sensor_create_args_t *args,
    distance_sensor_handle_t *handle
) {
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

    // Configure ADC Input
    if ( (err = configure_adc_input(
        &((*handle)->adc_input),
        args->adc_input.gpio_num,
        args->adc_input.channel_num
    )) ) {
        ESP_LOGE(TAG, "ADC input configuration failed");
        goto ds_create__error_after_handle_allocation;
    }

    // Configure Sampling Timer
    if ( ( err = configure_sampling_timer(
        &((*handle)->sampling_timer),
        _sampling_fn,
        *handle,
        args->sampling_timer.period_ms
    ))) {
        ESP_LOGE(TAG, "Sampling timer configuration failed");
        goto ds_create__error_after_adc_input_configuration;
    }

    // Configure Multisampling
    if ( (err = configure_multisampling(
        &((*handle)->multisampling),
        args->multisampling.samples_per_reading
    )) ) {
        ESP_LOGE(TAG, "Multisampling configuration failed");
        goto ds_create__error_after_sampling_timer_configuration;
    }

    // Configure Storage
    if ( (err = configure_storage(
        &((*handle)->storage),
        args->storage.queue_size
    )) ) {
        ESP_LOGE(TAG, "Storage configuration failed");
        goto ds_create__error_after_multisampling_configuration;
    }

    // Log handle properties
    ESP_LOGI(TAG, "Created handle with properties:\n"
                  " - Name                   > %s\n"
                  " - ADC Channel            > %d\n"
                  " - Corresponding GPIO Pin > %d\n"
                  " - Sampling Period (ms)   > %d\n"
                  " - Multisampling          > %s\n"
                  " - Samples Per Reading    > %d",
                  (*handle)->name,
                  get_channel_num(&((*handle)->adc_input)),
                  get_channel_gpio_num(&((*handle)->adc_input)),
                  (*handle)->sampling_timer.period_ms,
                  (*handle)->multisampling.enabled ? "Enabled" : "Disabled",
                  (*handle)->multisampling.samples_per_reading
    );

    return ESP_OK;

ds_create__error_after_multisampling_configuration:
    delete_multisampling(&((*handle)->multisampling));
ds_create__error_after_sampling_timer_configuration:
    delete_sampling_timer(&((*handle)->sampling_timer));
ds_create__error_after_adc_input_configuration:
    delete_adc_input(&((*handle)->adc_input));
ds_create__error_after_handle_allocation:
    free(*handle);

    return err;
}

// Delete Distance Sensor Handle
void distance_sensor_delete(distance_sensor_handle_t handle) {

    // Stop sampling
    stop_sampling_timer(&(handle->sampling_timer));

    // Delete storage
    delete_storage(&(handle->storage));

    // Delete multisampling
    delete_multisampling(&(handle->multisampling));

    // Delete sampling timer
    delete_sampling_timer(&(handle->sampling_timer));

    // Delete ADC input
    delete_adc_input(&(handle->adc_input));

    // Free handle
    free(handle);

    ESP_LOGI(TAG, "Deleted handle");
}

// Start Distance Sensor Sampling
esp_err_t distance_sensor_start(distance_sensor_handle_t handle) {
    esp_err_t err;

    if ( handle == NULL ) {
        ESP_LOGE(TAG, "Handle must not be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // Start sampling timer
    if ( (err = start_sampling_timer(&(handle->sampling_timer))) ) {
        ESP_LOGE(TAG, "Error starting sampling timer");
        return err;
    }

    ESP_LOGI(TAG, "Started sampling");
    return ESP_OK;
}

// Stop Distance Sensor Sampling
esp_err_t distance_sensor_stop(distance_sensor_handle_t handle) {
    esp_err_t err;

    if ( handle == NULL ) {
        ESP_LOGE(TAG, "Handle must not be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // Stop sampling timer
    if ( (err = stop_sampling_timer(&(handle->sampling_timer))) ) {
        ESP_LOGE(TAG, "Error stopping sampling timer");
        return err;
    }

    ESP_LOGI(TAG, "Stopped sampling");
    return ESP_OK;
}

// GET LAST READING FUNCTION
esp_err_t distance_sensor_get_reading(
    distance_sensor_handle_t handle,
    distance_sensor_reading_t *reading
) {
    esp_err_t err;

    if ( handle == NULL ) {
        ESP_LOGE(TAG, "Handle must not be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // Retrieve last reading
    if ( (err = pop_reading(&(handle->storage), reading)) ) {
        ESP_LOGE(TAG, "Could not retrieve last reading");
        return err;
    }

    return ESP_OK;
}

// GET NAME FUNCTION
const char *distance_sensor_get_name(distance_sensor_handle_t handle) {
    return handle->name;
}
