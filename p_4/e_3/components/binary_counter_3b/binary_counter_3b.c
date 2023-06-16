#include <stdio.h>
#include "binary_counter_3b.h"

#include "string.h"

// include ESP logs
#include "esp_log.h"

static char const *TAG = "Binary Counter 3B";


// Binary Counter 3B [Handle] //
#define BINARY_COUNTER_3B_NAME_BUFSIZE 32

struct binary_counter_3b {
    // Name
    char name[BINARY_COUNTER_3B_NAME_BUFSIZE];

    // GPIOs
    gpio_num_t bit0_gpio;
    gpio_num_t bit1_gpio;
    gpio_num_t bit2_gpio;

    // Current value
    uint8_t value;
};



// SET GPIO OUTPUT TO CURRENT VALUE //
static esp_err_t _set_gpio(
    binary_counter_3b_handle_t handle
) {
    esp_err_t err;
    
    // set the GPIOs
    if ( (err = gpio_set_level(handle->bit0_gpio, handle->value & 0x01)) ) {
        ESP_LOGE(TAG, "gpio_set_level failed for bit0_gpio");
        return err;
    }
    if ( (err = gpio_set_level(handle->bit1_gpio, handle->value & 0x02)) ) {
        ESP_LOGE(TAG, "gpio_set_level failed for bit1_gpio");
        return err;
    }
    if ( (err = gpio_set_level(handle->bit2_gpio, handle->value & 0x04)) ) {
        ESP_LOGE(TAG, "gpio_set_level failed for bit2_gpio");
        return err;
    }

    return ESP_OK;
}




// FUNCTIONS //

// Create Binary Counter 3B
esp_err_t binary_counter_3b_create(
    binary_counter_3b_create_args_t *args,
    binary_counter_3b_handle_t *out_handle
) {
    esp_err_t err;

    // Check handle is not created
    if ( *out_handle != NULL ) {
        ESP_LOGE(TAG, "Handle must be NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // Create handle
    *out_handle = malloc(sizeof(struct binary_counter_3b));
    if ( *out_handle == NULL ) {
        ESP_LOGE(TAG, "Could not allocate memory for handle");
        return ESP_ERR_NO_MEM;
    }

    // Name
    strncpy((*out_handle)->name, args->name, BINARY_COUNTER_3B_NAME_BUFSIZE);

    // Configure GPIOs
    uint64_t pin_bit_mask = 0;
    pin_bit_mask |= (1ULL << args->bit0_gpio);
    pin_bit_mask |= (1ULL << args->bit1_gpio);
    pin_bit_mask |= (1ULL << args->bit2_gpio);
    gpio_config_t io_conf = {
        .pin_bit_mask = pin_bit_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    if ( (err = gpio_config(&io_conf)) ) {
        ESP_LOGE(TAG, "gpio_config failed");
        goto bc3b__error_after_handle_allocation;
    }

    // GPIOs
    (*out_handle)->bit0_gpio = args->bit0_gpio;
    (*out_handle)->bit1_gpio = args->bit1_gpio;
    (*out_handle)->bit2_gpio = args->bit2_gpio;

    // Current value
    (*out_handle)->value = args->initial_value % 8;

    // Set GPIOs to current value
    if ( (err = _set_gpio(*out_handle)) ) {
        ESP_LOGE(TAG, "Could not set GPIOs to initial value");
        goto bc3b__error_after_gpio_configuration;
    }

    return ESP_OK;

bc3b__error_after_gpio_configuration:
    gpio_reset_pin((*out_handle)->bit2_gpio);
    gpio_reset_pin((*out_handle)->bit1_gpio);
    gpio_reset_pin((*out_handle)->bit0_gpio);
bc3b__error_after_handle_allocation:
    free(*out_handle);

    return err;
}


// Delete Binary Counter 3B
void binary_counter_3b_delete(
    binary_counter_3b_handle_t handle
) {
    if ( handle == NULL ) {
        ESP_LOGW(TAG, "Handle is NULL");
        return;
    }
    
    // Reset GPIOs
    gpio_reset_pin(handle->bit2_gpio);
    gpio_reset_pin(handle->bit1_gpio);
    gpio_reset_pin(handle->bit0_gpio);

    // Free handle
    free(handle);
}


// Increment Binary Counter 3B
esp_err_t binary_counter_3b_increment(
    binary_counter_3b_handle_t handle
) {
    esp_err_t err;

    if ( handle == NULL ) {
        ESP_LOGE(TAG, "Handle is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // Increment value
    handle->value++;
    handle->value %= 8;

    // Set GPIOs to current value
    if ( (err = _set_gpio(handle)) ) {
        ESP_LOGE(TAG, "Could not increment GPIOs to current value");
        
        // Revert value
        handle->value--;
        handle->value %= 8;
        
        return err;
    }

    return ESP_OK;
}


// Decrement Binary Counter 3B
esp_err_t binary_counter_3b_decrement(
    binary_counter_3b_handle_t handle
) {
    esp_err_t err;

    if ( handle == NULL ) {
        ESP_LOGE(TAG, "Handle is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // Decrement value
    handle->value--;
    handle->value %= 8;

    // Set GPIOs to current value
    if ( (err = _set_gpio(handle)) ) {
        ESP_LOGE(TAG, "Could not decrement GPIOs to current value");

        // Revert value
        handle->value++;
        handle->value %= 8;

        return err;
    }

    return ESP_OK;
}


// Get Binary Counter 3B Reset
esp_err_t binary_counter_3b_reset(
    binary_counter_3b_handle_t handle
) {
    esp_err_t err;

    if ( handle == NULL ) {
        ESP_LOGE(TAG, "Handle is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // Reset value
    handle->value = 0;

    // Set GPIOs to current value
    if ( (err = _set_gpio(handle)) ) {
        ESP_LOGE(TAG, "Could not reset GPIOs to current value");
        return err;
    }

    return ESP_OK;
}

// Get Binary Counter 3B Value
esp_err_t binary_counter_3b_get_value(
    binary_counter_3b_handle_t handle,
    uint8_t *out_value
) {
    if ( handle == NULL ) {
        ESP_LOGE(TAG, "Handle is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    *out_value = handle->value;

    return ESP_OK;
}