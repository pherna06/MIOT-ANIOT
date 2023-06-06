#include "distance_sensor.h"

// For ESP errors:
#include "esp_err.h"

// For ESP log:
#include "esp_log.h"

// For ESP HRT:
#include "esp_timer.h"

typedef struct sampling {
    esp_timer_handle_t timer;
    uint32_t period_ms;
    uint32_t samples_per_reading;
} sampling_t;

esp_err_t _configure_sampling(
    sampling_t *sampling,
    esp_timer_cb_t sampling_fn,
    void *sampling_fn_arg,
    uint32_t period_ms,
    uint32_t samples_per_reading
) {
    static const char *TAG = "(Distance Sensor Util) Configure Sampling";
    esp_err_t err;

    // Create timer
    esp_timer_create_args_t timer_args = {
        .callback = sampling_fn,
        .arg = sampling_fn_arg,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "sampling_timer",
    };
    if ( (err = esp_timer_create(&timer_args, &(sampling->timer))) ) {
        ESP_LOGE(TAG, "Could not create timer");
        return err;
    }

    // Set period and number of samples per reading
    sampling->period_ms = period_ms;
    sampling->samples_per_reading = samples_per_reading;

    return ESP_OK;
}

void _deconfigure_sampling(sampling_t *sampling) {
    esp_timer_delete(sampling->timer);
}

esp_err_t _start_sampling(sampling_t *sampling) {
    return esp_timer_start_periodic(
        sampling->timer,
        sampling->period_ms * 1000
    );
}

esp_err_t _stop_sampling(sampling_t *sampling) {
    return esp_timer_stop(sampling->timer);
}

int _adc_multisample(
    sampling_t *sampling,
    int (*adc_read_fn)(void *),
    void *adc_read_fn_arg
) {
    int adc_reading_sum = 0;
    for ( int i = 0; i < sampling->samples_per_reading; i++ ) {
        adc_reading_sum += adc_read_fn(adc_read_fn_arg);
    }
    return adc_reading_sum / sampling->samples_per_reading;
}

inline uint64_t _get_timestamp() {
    return esp_timer_get_time();
}