#include "util/sampling_timer.h"



// FUNCTIONS //

esp_err_t configure_sampling_timer(
    sampling_timer_t *sampling_timer,
    void (*sampling_fn)(void *),
    void *sampling_fn_arg,
    uint32_t period_ms
) {
    static const char *TAG = "(Distance Sensor Util) Configure Sampling Timer";
    esp_err_t err;

    // Create timer
    esp_timer_create_args_t timer_args = {
        .callback = sampling_fn,
        .arg = sampling_fn_arg,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "sampling_timer",
    };
    if ( (err = esp_timer_create(&timer_args, &(sampling_timer->timer))) ) {
        ESP_LOGE(TAG, "Could not create timer");
        return err;
    }

    // Set period
    sampling_timer->period_ms = period_ms;

    return ESP_OK;
}

void delete_sampling_timer(
    sampling_timer_t *sampling_timer
) {
    static const char *TAG = "(Distance Sensor Util) Delete Sampling Timer";
    
    esp_err_t err;
    if ( (err = esp_timer_delete(sampling_timer->timer)) ) {
        ESP_LOGW(TAG, "Could not delete timer: %s", esp_err_to_name(err));
    }
}

esp_err_t start_sampling_timer(
    sampling_timer_t *sampling_timer
) {
    static const char *TAG = "(Distance Sensor Util) Start Sampling Timer";
    esp_err_t err;
    
    if ( (err = esp_timer_start_periodic(
        sampling_timer->timer,
        sampling_timer->period_ms * 1000
    )) ) {
        ESP_LOGE(TAG, "Could not start timer: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t stop_sampling_timer(
    sampling_timer_t *sampling_timer
) {
    static const char *TAG = "(Distance Sensor Util) Stop Sampling Timer";
    esp_err_t err;

    if ( !(esp_timer_is_active(sampling_timer->timer)) ) {
        ESP_LOGD(TAG, "Timer is already stopped");
        return ESP_OK;
    }

    if ( (err = esp_timer_stop(sampling_timer->timer)) ) {
        ESP_LOGE(TAG, "Could not stop timer: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}