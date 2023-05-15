#ifndef __BINARY_COUNTER_4B_H__
#define __BINARY_COUNTER_4B_H__

// include ESP errors
#include "esp_err.h"

// include GPIO logic
#include "driver/gpio.h"

typedef struct binary_counter_4b *binary_counter_4b_handle_t;

typedef struct binary_counter_4b_create_args {
    gpio_num_t bit0_gpio;
    gpio_num_t bit1_gpio;
    gpio_num_t bit2_gpio;
    gpio_num_t bit3_gpio;
    uint8_t    initial_value;
} binary_counter_4b_create_args_t;

esp_err_t binary_counter_4b_create(
        binary_counter_4b_create_args_t *args,
        binary_counter_4b_handle_t *handle
);

esp_err_t binary_counter_4b_delete(
        binary_counter_4b_handle_t *handle
);

esp_err_t binary_counter_4b_increment(
        binary_counter_4b_handle_t binary_counter
);

esp_err_t binary_counter_4b_decrement(
        binary_counter_4b_handle_t binary_counter
);

esp_err_t binary_counter_4b_reset(
        binary_counter_4b_handle_t binary_counter
);

#endif // __BINARY_COUNTER_4B_H__