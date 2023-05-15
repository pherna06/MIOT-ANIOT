#include <stdio.h>

// include logging
#include "esp_log.h"

// include for task delay
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// include for HRT timer
#include "esp_timer.h"

#include "binary_counter_4b.h"

#define BINARY_COUNTER_GPIO_BIT0 25
#define BINARY_COUNTER_GPIO_BIT1 26
#define BINARY_COUNTER_GPIO_BIT2 27
#define BINARY_COUNTER_GPIO_BIT3 14
#define BINARY_COUNTER_INITIAL_VALUE 15

#define COUNTER_PERIOD_MS 1000
#define GPIO_READ_PERIOD_MS 500
#define GPIO_INPUT 4

// counter direction
typedef enum {
    COUNTER_UP,
    COUNTER_DOWN
} counter_direction_t;

static counter_direction_t _counter_direction = COUNTER_UP;

// binary counter periodic timer callback
void binary_counter_timer_callback(void *args)
{
    if (_counter_direction == COUNTER_UP) {
        ESP_ERROR_CHECK(binary_counter_4b_increment(args));
    } else {
        ESP_ERROR_CHECK(binary_counter_4b_decrement(args));
    }
}

// gpio input timer callback
void gpio_input_timer_callback(void *args)
{
    // tag
    static const char *TAG = "GPIO_INPUT_TIMER_CALLBACK";
    
    // GPIO last level
    static int last_level = 0;

    // read GPIO
    int level = gpio_get_level(GPIO_INPUT);

    // if level diff, change counter direction
    if (level != last_level) {
        _counter_direction = !_counter_direction;
        ESP_LOGI(TAG, "counter direction changed to %s",
                     _counter_direction == COUNTER_UP ? "up" : "down");

        // update last level
        last_level = level;
    }
}

void app_main(void)
{
    static const char *TAG = "APP_MAIN";
    
    // create binary counter
    binary_counter_4b_handle_t binary_counter;
    binary_counter_4b_create_args_t binary_counter_args = {
        .bit0_gpio = BINARY_COUNTER_GPIO_BIT0,
        .bit1_gpio = BINARY_COUNTER_GPIO_BIT1,
        .bit2_gpio = BINARY_COUNTER_GPIO_BIT2,
        .bit3_gpio = BINARY_COUNTER_GPIO_BIT3,
        .initial_value = BINARY_COUNTER_INITIAL_VALUE
    };
    ESP_ERROR_CHECK(
        binary_counter_4b_create(&binary_counter_args, &binary_counter)
    );
    ESP_LOGI(TAG, "binary counter created "
                  "with initial value %d", BINARY_COUNTER_INITIAL_VALUE);


    // create binary counter periodic timer
    esp_timer_handle_t binary_counter_timer;
    esp_timer_create_args_t binary_counter_timer_args = {
        .callback = binary_counter_timer_callback,
        .arg = binary_counter,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "binary_counter_timer"
    };
    ESP_ERROR_CHECK(
        esp_timer_create(&binary_counter_timer_args, &binary_counter_timer)
    );
    ESP_LOGI(TAG, "binary counter timer created");

    // create input GPIO read periodic timer
    esp_timer_handle_t gpio_input_timer;
    esp_timer_create_args_t gpio_input_timer_args = {
        .callback = gpio_input_timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "gpio_input_timer"
    };
    ESP_ERROR_CHECK(
        esp_timer_create(&gpio_input_timer_args, &gpio_input_timer)
    );
    ESP_LOGI(TAG, "gpio input timer created");

    // delay 3s until reset
    vTaskDelay(3000 / portTICK_PERIOD_MS);

    // reset binary counter
    ESP_ERROR_CHECK(binary_counter_4b_reset(binary_counter));
    ESP_LOGI(TAG, "binary counter reset");

    // start binary counter timer
    ESP_ERROR_CHECK(
        esp_timer_start_periodic(
            binary_counter_timer,
            COUNTER_PERIOD_MS * 1000
        )
    );
    ESP_LOGI(TAG, "binary counter timer started");

    // start GPIO input timer
    ESP_ERROR_CHECK(
        esp_timer_start_periodic(
            gpio_input_timer,
            GPIO_READ_PERIOD_MS * 1000
        )
    );
    ESP_LOGI(TAG, "gpio input timer started");
}
