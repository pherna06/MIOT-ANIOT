#include "util/multisampling.h"

// FUNCTIONS //

// Configure Multisampling
esp_err_t configure_multisampling(
    multisampling_t *multisampling,
    int samples_per_reading
) {
    static const char *TAG = "(Distance Sensor Utils) Configure Multisampling";

    // If samples per reading < 1, multisampling is disabled
    if (samples_per_reading < 1) {
        multisampling->enabled = false;
        ESP_LOGI(TAG, "Multisampling disabled");
        return ESP_OK;
    }

    // Enable multisampling
    multisampling->enabled = true;
    multisampling->samples_per_reading = samples_per_reading;
    ESP_LOGI(TAG, "Multisampling enabled with %d samples per reading", samples_per_reading);

    return ESP_OK;
}

// Delete Multisampling
void delete_multisampling(
    multisampling_t *multisampling
) {
    // Do nothing
}

// Do Multisampling
esp_err_t do_multisampling(
    multisampling_t *multisampling,
    esp_err_t (*read_fn)(void *, int *),
    void *read_fn_arg,
    int *result
) {
    static const char *TAG = "(Distance Sensor Utils) Do Multisampling";
    esp_err_t err;

    // If multisampling is disabled, just read once
    if (!multisampling->enabled) {
        if ( (err = read_fn(read_fn_arg, result)) ) {
            ESP_LOGE(TAG, "Reading failed");
            return err;
        }
        return ESP_OK;
    }

    // Read samples and get mean value
    int sum = 0;
    for (int i = 0; i < multisampling->samples_per_reading; i++) {
        int sample;
        read_fn(read_fn_arg, &sample);
        sum += sample;
    }
    *result = sum / multisampling->samples_per_reading;

    return ESP_OK;
}


