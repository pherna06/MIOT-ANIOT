#ifdef CONFIG_DISTANCE_SENSOR_TASK

#include "distance_sensor.h"

// include ESP errors
#include "esp_err.h"

// include ESP events
#include "esp_event.h"

ESP_EVENT_DEFINE_BASE(DISTANCE_SENSOR_EVENTS);

// include ESP logs
#include "esp_log.h"

static const char *DS_TAG = "Distance Sensor Task";

// include ESP GPIO
#include "driver/gpio.h"

// define distance sensor create args
#define DISTANCE_SENSOR_GPIO_INPUT          CONFIG_DISTANCE_SENSOR_GPIO_INPUT
#define DISTANCE_SENSOR_SAMPLING_PERIOD_MS  CONFIG_DISTANCE_SENSOR_SAMPLING_PERIOD_MS
#define DISTANCE_SENSOR_SAMPLES_PER_READING CONFIG_DISTANCE_SENSOR_SAMPLES_PER_READING
#define DISTANCE_SENSOR_QUEUE_SIZE          CONFIG_DISTANCE_SENSOR_QUEUE_SIZE

// Event Handler for Distance Sensor
static void distance_sensor_event_handler(
    void* handler_args,
    esp_event_base_t base,
    int32_t id,
    void* event_data
) {
    static const char *TAG = "DISTANCE_SENSOR_READING_EVENT";
    
    // get handle
    distance_sensor_handle_t handle = *((distance_sensor_handle_t *) event_data);

    // get reading
    distance_sensor_reading_t reading;
    distance_sensor_get_reading(handle, &reading);

    int distance_cm = reading.distance_mm / 10;
    int distance_mm = reading.distance_mm % 10;

    // print reading
    ESP_LOGI(TAG, "\n"
                  " - Name          > %s\n"
                  " - Reading No.   > %lld\n"
                  " - ADC reading   > %d\n"
                  " - Voltage (mV)  > %d\n"
                  " - Distance (cm) > %d.%d\n",
                  distance_sensor_get_name(handle),
                  reading.reading_id,
                  reading.adc_reading,
                  reading.voltage_mv,
                  distance_cm, distance_mm
    );
}

// Distance Sensor Task Function
void distance_sensor_task(void *pvParameter) {
    esp_err_t err;

    ESP_LOGI(DS_TAG, "Distance Sensor Task Running...");

    // Create Default Args
    distance_sensor_create_args_t args = DISTANCE_SENSOR_CREATE_ARGS_DEFAULT();
    args.name                              = "distance_sensor";
    args.adc_input.gpio_num                = DISTANCE_SENSOR_GPIO_INPUT;
    args.sampling_timer.period_ms          = DISTANCE_SENSOR_SAMPLING_PERIOD_MS;
    args.multisampling.samples_per_reading = DISTANCE_SENSOR_SAMPLES_PER_READING;
    args.storage.queue_size                = DISTANCE_SENSOR_QUEUE_SIZE;

    // Create Handle
    distance_sensor_handle_t handle;
    if ( (err = distance_sensor_create(&args, &handle)) ) {
        ESP_LOGE(DS_TAG, "Could not create Distance Sensor Handle: %s", esp_err_to_name(err));
        goto ds_task__error_at_start;
    }

    // Register Event Handler
    if ( (err = esp_event_handler_register(
        DISTANCE_SENSOR_EVENTS,
        DISTANCE_SENSOR_READING_EVENT,
        distance_sensor_event_handler,
        NULL
    )) ) {
        ESP_LOGE(DS_TAG, "Could not register Distance Sensor Event Handler: %s", esp_err_to_name(err));
        goto ds_task__error_after_distance_sensor_create;
    }

    // Start Distance Sensor
    if ( (err = distance_sensor_start(handle)) ) {
        ESP_LOGE(DS_TAG, "Could not start Distance Sensor: %s", esp_err_to_name(err));
        goto ds_task__error_after_register_event_handler;
    }

    // Wait for delete notification
    while (1) {
        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY)) {
            break;
        }
    }

//ds_task__error_after_distance_sensor_start:
    // Stop Distance Sensor
    if ( (err = distance_sensor_stop(handle)) ) {
        ESP_LOGW(DS_TAG, "Could not stop Distance Sensor: %s", esp_err_to_name(err));
    }
ds_task__error_after_register_event_handler:
    // Nothing to do here
ds_task__error_after_distance_sensor_create:
    // Delete Handle
    distance_sensor_delete(handle);
ds_task__error_at_start:
    // Delete Task
    vTaskDelete(NULL);
}

#endif // CONFIG_DISTANCE_SENSOR_TASK