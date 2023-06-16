#ifndef __BINARY_COUNTER_3B_H__
#define __BINARY_COUNTER_3B_H__

// include ESP errors
#include "esp_err.h"

// include binary_counter_3b_create_args_t
#include "binary_counter_3b_create_args.h"

// HANDLE //
typedef struct binary_counter_3b *binary_counter_3b_handle_t;



// FUNCTIONS //

esp_err_t binary_counter_3b_create(
    binary_counter_3b_create_args_t *args,
    binary_counter_3b_handle_t *out_handle
);

void binary_counter_3b_delete(
    binary_counter_3b_handle_t handle
);

esp_err_t binary_counter_3b_increment(
    binary_counter_3b_handle_t handle
);

esp_err_t binary_counter_3b_decrement(
    binary_counter_3b_handle_t handle
);

esp_err_t binary_counter_3b_reset(
    binary_counter_3b_handle_t handle
);

esp_err_t binary_counter_3b_get_value(
    binary_counter_3b_handle_t handle,
    uint8_t *out_value
);


#endif // __BINARY_COUNTER_3B_H__