#include <stdio.h>

// include for ESP GPIO
#include "driver/gpio.h"

// Set output GPIO
#define GPIO_OUTPUT 18

// include for ESP HRT timer
#include "esp_timer.h"

// Timer period in ms
#define PERIOD_MS 1000


// Periodic timer callback
static void periodic_timer_callback(void* arg)
{
    // Swap GPIO level
    static int gpio = 0;
    gpio_set_level(GPIO_OUTPUT, gpio);
    gpio = !gpio;
}

void app_main(void)
{
    // GPIO config
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_OUTPUT),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));


    // Timer config
    esp_timer_create_args_t timer_config = {
        .callback = &periodic_timer_callback,
        .arg = NULL,
        .name = "periodic"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&timer_config, &periodic_timer));

    // Start timer periodic
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, PERIOD_MS * 1000));
}
