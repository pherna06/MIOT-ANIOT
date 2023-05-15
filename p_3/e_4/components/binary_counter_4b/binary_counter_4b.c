#include <stdio.h>
#include "binary_counter_4b.h"

// include logging
#include "esp_log.h"

// define logging tag
static const char* TAG = "binary_counter_4b";

struct binary_counter_4b {
    gpio_num_t bit0_gpio;
    gpio_num_t bit1_gpio;
    gpio_num_t bit2_gpio;
    gpio_num_t bit3_gpio;
    uint8_t    value;
};

// private function to set gpio to counter value
static esp_err_t binary_counter_4b_set_gpio(
        binary_counter_4b_handle_t binary_counter
) {
    // set the GPIOs
    if (gpio_set_level(binary_counter->bit0_gpio, binary_counter->value & 0x01) != ESP_OK) {
        return ESP_FAIL;
    }
    if (gpio_set_level(binary_counter->bit1_gpio, binary_counter->value & 0x02) != ESP_OK) {
        return ESP_FAIL;
    }
    if (gpio_set_level(binary_counter->bit2_gpio, binary_counter->value & 0x04) != ESP_OK) {
        return ESP_FAIL;
    }
    if (gpio_set_level(binary_counter->bit3_gpio, binary_counter->value & 0x08) != ESP_OK) {
        return ESP_FAIL;
    }

    // return
    return ESP_OK;
}

// create a new binary_counter
esp_err_t binary_counter_4b_create(
        binary_counter_4b_create_args_t *args,
        binary_counter_4b_handle_t *handle
) {
    // allocate memory for the binary_counter
    binary_counter_4b_handle_t binary_counter = 
        (binary_counter_4b_handle_t) malloc(sizeof(struct binary_counter_4b));
    if (binary_counter == NULL) {
        ESP_LOGE(TAG, "could not allocate memory for binary_counter");
        return ESP_ERR_NO_MEM;
    }

    // initialize the binary_counter
    binary_counter->bit0_gpio = args->bit0_gpio;
    binary_counter->bit1_gpio = args->bit1_gpio;
    binary_counter->bit2_gpio = args->bit2_gpio;
    binary_counter->bit3_gpio = args->bit3_gpio;
    binary_counter->value = args->initial_value % 0x10; // 0x10 = 16

    // initialize the GPIOs
    uint64_t pin_bit_mask = 0;
    pin_bit_mask |= 1ULL << args->bit0_gpio;
    pin_bit_mask |= 1ULL << args->bit1_gpio;
    pin_bit_mask |= 1ULL << args->bit2_gpio;
    pin_bit_mask |= 1ULL << args->bit3_gpio;
    gpio_config_t io_conf = {
        .pin_bit_mask = pin_bit_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    if (gpio_config(&io_conf) != ESP_OK) {
        ESP_LOGE(TAG, "could not configure GPIOs");
        return ESP_FAIL;
    }

    // set the GPIOs to the initial value
    if (binary_counter_4b_set_gpio(binary_counter) != ESP_OK) {
        ESP_LOGE(TAG, "could not set GPIOs");
        return ESP_FAIL;
    }

    // return the binary_counter
    *handle = binary_counter;
    return ESP_OK;
}

// delete the binary_counter
esp_err_t binary_counter_4b_delete(
        binary_counter_4b_handle_t *handle )
{
    // reset GPIOs
    if (gpio_reset_pin((*handle)->bit0_gpio) != ESP_OK) {
        ESP_LOGW(TAG, "could not reset GPIO");
    }
    if (gpio_reset_pin((*handle)->bit1_gpio) != ESP_OK) {
        ESP_LOGW(TAG, "could not reset GPIO");
    }
    if (gpio_reset_pin((*handle)->bit2_gpio) != ESP_OK) {
        ESP_LOGW(TAG, "could not reset GPIO");
    }
    if (gpio_reset_pin((*handle)->bit3_gpio) != ESP_OK) {
        ESP_LOGW(TAG, "could not reset GPIO");
    }
    
    // free the memory
    free(*handle);

    // set the handle to NULL
    *handle = NULL;

    // return
    return ESP_OK;
}

// add one to the counter
esp_err_t binary_counter_4b_increment(
        binary_counter_4b_handle_t binary_counter
) {
    // increment the value
    binary_counter->value = (binary_counter->value + 1) % 0x10; // 0x10 = 16

    // set the GPIOs
    if (binary_counter_4b_set_gpio(binary_counter) != ESP_OK) {
        ESP_LOGE(TAG, "could not set GPIOs");
        return ESP_FAIL;
    }

    // return
    return ESP_OK;
}

// subtract one from the counter
esp_err_t binary_counter_4b_decrement(
        binary_counter_4b_handle_t binary_counter
) {
    // decrement the value
    binary_counter->value = (binary_counter->value - 1) % 0x10; // 0x10 = 16

    // set the GPIOs
    if (binary_counter_4b_set_gpio(binary_counter) != ESP_OK) {
        ESP_LOGE(TAG, "could not set GPIOs");
        return ESP_FAIL;
    }

    // return
    return ESP_OK;
}

// reset counter to zero
esp_err_t binary_counter_4b_reset(
        binary_counter_4b_handle_t binary_counter
) {
    // reset the value
    binary_counter->value = 0;

    // set the GPIOs
    if (binary_counter_4b_set_gpio(binary_counter) != ESP_OK) {
        ESP_LOGE(TAG, "could not set GPIOs");
        return ESP_FAIL;
    }

    // return
    return ESP_OK;
}
