#include <stdio.h>

#include "distance_sensor.h"

#include "esp_log.h"

#include "soc/adc_channel.h"


ESP_EVENT_DEFINE_BASE(DISTANCE_SENSOR_EVENT);

// event handler
static void distance_sensor_event_handler(
    void* handler_args,
    esp_event_base_t base,
    int32_t id,
    void* event_data
) {
    static const char *TAG = "DISTANCE_SENSOR_EVENT_READING";
    
    // get handle
    distance_sensor_handle_t handle = *((distance_sensor_handle_t *) event_data);

    // get reading
    distance_sensor_reading_t reading;
    distance_sensor_get_reading(handle, &reading);

    // print reading
    ESP_LOGI(TAG, "\n"
                  " - Name          > %s\n"
                  " - Reading No.   > %lld\n"
                  " - Timestamp     > %lld\n"
                  " - ADC reading   > %d\n"
                  " - Voltage (mV)  > %d\n"
                  " - Distance (mm) > %d",
                  distance_sensor_get_name(handle),
                  reading.num,
                  reading.timestamp,
                  reading.adc_reading,
                  reading.voltage_mv,
                  reading.distance_mm
    );
}

void app_main(void)
{
    static const char *TAG = "APP_MAIN";
    
    // Create args
    distance_sensor_create_args_t create_args = DISTANCE_SENSOR_CREATE_ARGS_DEFAULT();
    create_args.name = "distance_sensor";
    create_args.adc_input.channel_num = 0;
    create_args.sampling.period_ms = 1000;
    create_args.sampling.samples = 32;
    create_args.readings.queue_size = 10;

    // log GPIO num
    ESP_LOGI(TAG, "GPIO: %d", ADC1_CHANNEL_0_GPIO_NUM);
    
    // Create handle
    distance_sensor_handle_t handle;
    distance_sensor_create(&create_args, &handle);

    // Register DISTANCE_SENSOR_EVENT_READING event handler
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(
        DISTANCE_SENSOR_EVENT,
        DISTANCE_SENSOR_EVENT_READING,
        distance_sensor_event_handler,
        NULL
    ));

    // Start
    distance_sensor_start(handle);

    // Wait
    vTaskDelay(10000 / portTICK_PERIOD_MS);

    // Stop
    distance_sensor_stop(handle);

    // Start
    distance_sensor_start(handle);

    // Wait
    vTaskDelay(20000 / portTICK_PERIOD_MS);

    // Stop
    distance_sensor_stop(handle);

    // Delete
    distance_sensor_delete(handle);

    // Unregister DISTANCE_SENSOR_EVENT_READING event handler
    esp_event_handler_unregister(
        DISTANCE_SENSOR_EVENT,
        DISTANCE_SENSOR_EVENT_READING,
        distance_sensor_event_handler
    );

    return;
}
