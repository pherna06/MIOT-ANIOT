#include <stdio.h>

// include for logging
#include "esp_log.h"

// include for ESP GPIO
#include "driver/gpio.h"

// input GPIO
#define GPIO_INPUT 5


// include HRT timer
#include "esp_timer.h"

#define PERIOD_MS 1000


// Periodic timer callback
void periodic_timer_callback(void* arg)
{
    // TAG
    static const char* TAG = "TIMER";

    // value
    static int prev_value = -1;

    // read GPIO
    int value = gpio_get_level(GPIO_INPUT);

    if (value != prev_value)
    {
        // log
        ESP_LOGI(TAG, "GPIO %d: %d", GPIO_INPUT, value);

        // update prev_value
        prev_value = value;
    }
}

void app_main(void)
{
    // config GPIO as pulldown input
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << GPIO_INPUT,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Periodic timer
    esp_timer_create_args_t periodic_timer_args = {
        .callback = &periodic_timer_callback,
        .name = "periodic"
    };
    esp_timer_handle_t periodic_timer;
    esp_timer_create(&periodic_timer_args, &periodic_timer);
    esp_timer_start_periodic(periodic_timer, PERIOD_MS * 1000);
}
