
// Tasks
// - Sample Hall Sensor each second
// - Sample temperature (Si7021) each 2 seconds
// - Log Hall and Temperature readings to UART each 5 seconds
// - Set GPIO outs for LEDS array
// - At startup, first temperature reading is set as baseline.
//   At this temperature, 1 LED is lit. Each 1 degree above baseline,
//   1 more LED is lit. Each 1 degree below baseline, 1 less LED is lit.
// - Events are post for each temperature change (above or below baseline)
// - When Hall sensor varies more than 20% (???) from baseline, all LEDs are
//   set to blink.

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "hall_sensor.h"
#include "app_leds.h"
#include "si7021.h"

#include "esp_timer.h"

#include "esp_event.h"
#include "esp_log.h"

static const char *TAG = "App Main";

#define APP_TIMER_READ_HALL_EVERY /* 1 */ CONFIG_APP_TIMER_READ_HALL_EVERY
#define APP_TIMER_READ_TEMP_EVERY /* 2 */ CONFIG_APP_TIMER_READ_TEMP_EVERY
#define APP_TIMER_LOG_READINGS_EVERY /* 5 */ CONFIG_APP_TIMER_LOG_READINGS_EVERY

#define APP_TIMER_PERIOD_MS /* 1000 */ CONFIG_APP_TIMER_PERIOD_MS

#define APP_HALL_THRESH /* 20 */ CONFIG_APP_HALL_THRESH

#define APP_HALL_BASE_TIMEOUT_MS /* 5000 */ CONFIG_APP_HALL_BASE_TIMEOUT_MS
#define APP_TEMP_BASE_TIMEOUT_MS /* 5000 */ CONFIG_APP_TEMP_BASE_TIMEOUT_MS

ESP_EVENT_DEFINE_BASE(APP_TIMER_EVENTS);

enum {
    APP_TIMER_EVENT_HALL_READING,
    APP_TIMER_EVENT_TEMP_READING,
    APP_TIMER_EVENT_LOG_READINGS,
} app_timer_event_t;

ESP_EVENT_DEFINE_BASE(APP_LEDS_EVENTS);

enum {
    APP_LEDS_EVENT_BLINK_START,
    APP_LEDS_EVENT_BLINK_STOP,
    APP_LEDS_EVENT_SET_METER,
} app_leds_event_t;

// Timer State
typedef struct {
    esp_event_loop_handle_t app_timer_event_loop;

    si7021_handle_t si7021;

    uint8_t read_hall;
    uint8_t read_temp;
    uint8_t log_readings;
} app_timer_data_t;

// Timer Callback
void app_timer_callback(
    void *arg
) {
    // Get event loop handle
    app_timer_data_t *timer = (app_timer_data_t *) arg;

    // Hall
    if (timer->read_hall == 0) {
        int hall_reading = hall_sensor_get_reading();
        esp_event_post_to(
            timer->app_timer_event_loop  ,
            APP_TIMER_EVENTS             ,
            APP_TIMER_EVENT_HALL_READING ,
            &hall_reading                ,
            sizeof(hall_reading)         ,
            0                           );
    }
    timer->read_hall = (timer->read_hall + 1) % APP_TIMER_READ_HALL_EVERY;

    // Temp
    if (timer->read_temp == 0) {
        float temp_reading = 0;
        si7021_measure_temp(timer->si7021, &temp_reading);
        esp_event_post_to(
            timer->app_timer_event_loop  ,
            APP_TIMER_EVENTS             ,
            APP_TIMER_EVENT_TEMP_READING ,
            &temp_reading                ,
            sizeof(temp_reading)         ,
            0                           );
    }
    timer->read_temp = (timer->read_temp + 1) % APP_TIMER_READ_TEMP_EVERY;

    // Log
    if (timer->log_readings == 0) {
        esp_event_post_to(
            timer->app_timer_event_loop  ,
            APP_TIMER_EVENTS             ,
            APP_TIMER_EVENT_LOG_READINGS ,
            NULL                         ,
            0                            ,
            0                           );
    }
    timer->log_readings = (timer->log_readings + 1) % APP_TIMER_LOG_READINGS_EVERY;
}

// App Timer FSM
typedef struct {
    esp_event_loop_handle_t app_leds_event_loop;

    // Hall
    int hall_base;

    int     hall_reading_mean;
    uint8_t hall_reading_count;

    // Temp
    float temp_base;

    float   temp_reading_mean;
    uint8_t temp_reading_count;

    // Blink flag
    bool blink;

    // Temp State
    uint8_t temp_state;
} app_timer_fsm_t;

// App Timer Event Handler
void app_timer_event_handler(
    void *handler_arg   ,
    esp_event_base_t base,
    int32_t id         ,
    void *event_data   )
{
    // Get timer handle
    app_timer_fsm_t *arg = (app_timer_fsm_t *) handler_arg;

    switch(id) {
        case APP_TIMER_EVENT_HALL_READING: ;
            // Get hall reading
            int hall_reading = *((int *) event_data);

            // Update hall reading mean
            arg->hall_reading_mean = (
                (arg->hall_reading_mean * arg->hall_reading_count) +
                hall_reading
            ) / (arg->hall_reading_count + 1);

            // Update hall reading count
            arg->hall_reading_count++;

            // If not blinking, check if hall reading is out of threshold
            if (!arg->blink) {
                if (
                    (hall_reading > (arg->hall_base + APP_HALL_THRESH)) ||
                    (hall_reading < (arg->hall_base - APP_HALL_THRESH))
                ) {
                    // Post event to start blinking
                    esp_event_post_to(
                        arg->app_leds_event_loop ,
                        APP_LEDS_EVENTS            ,
                        APP_LEDS_EVENT_BLINK_START ,
                        NULL                       ,
                        0                          ,
                        0                         );

                    arg->blink = true;
                }
            }

            // If blinking, check if hall reading is within threshold
            if (arg->blink) {
                if (
                    (hall_reading < (arg->hall_base + APP_HALL_THRESH)) &&
                    (hall_reading > (arg->hall_base - APP_HALL_THRESH))
                ) {
                    // Post event to stop blinking
                    esp_event_post_to(
                        arg->app_leds_event_loop ,
                        APP_LEDS_EVENTS            ,
                        APP_LEDS_EVENT_BLINK_STOP  ,
                        NULL                       ,
                        0                          ,
                        0                         );
                    
                    // Post event to set meter (value is temp state)
                    esp_event_post_to(
                        arg->app_leds_event_loop ,
                        APP_LEDS_EVENTS            ,
                        APP_LEDS_EVENT_SET_METER   ,
                        &(arg->temp_state)         ,
                        sizeof(arg->temp_state)    ,
                        0                         );

                    arg->blink = false;
                }
            }
            break;

        case APP_TIMER_EVENT_TEMP_READING: ;
            // Get temp reading
            float temp_reading = *((float *) event_data);

            // Update temp reading mean
            arg->temp_reading_mean = (
                (arg->temp_reading_mean * arg->temp_reading_count) +
                temp_reading
            ) / (arg->temp_reading_count + 1);

            // Update temp reading count
            arg->temp_reading_count++;

            // Set new temp state
            int diff = (int) temp_reading - (int) arg->temp_base;
            arg->temp_state = diff < 0 ? 0 : (uint8_t) diff;

            // If not blinking, post event to set meter (value is temp state)
            if (!arg->blink) {
                esp_event_post_to(
                    arg->app_leds_event_loop ,
                    APP_LEDS_EVENTS            ,
                    APP_LEDS_EVENT_SET_METER   ,
                    &(arg->temp_state)         ,
                    sizeof(arg->temp_state)    ,
                    0                         );
            }
            break;

        case APP_TIMER_EVENT_LOG_READINGS:
            // Log hall reading mean
            if (arg->hall_reading_count > 0) {
                ESP_LOGI(TAG, "Hall Reading Mean: %d", arg->hall_reading_mean);
            }
            else {
                ESP_LOGI(TAG, "Hall Reading Mean: N/A");
            }

            // Reset hall reading mean and count
            arg->hall_reading_mean = 0;
            arg->hall_reading_count = 0;

            // Log temp reading mean
            if (arg->temp_reading_count > 0) {
                ESP_LOGI(TAG, "Temp Reading Mean: %f", arg->temp_reading_mean);
            }
            else {
                ESP_LOGI(TAG, "Temp Reading Mean: N/A");
            }

            // Reset temp reading mean and count
            arg->temp_reading_mean = 0;
            arg->temp_reading_count = 0;
            break;
        default:
            ESP_LOGW(TAG, "Unhandled event ID: %d", id);
            break;
    }
}

// App LEDs Event Handler
void app_leds_event_handler(
    void *handler_arg   ,
    esp_event_base_t base,
    int32_t id         ,
    void *event_data   )
{
    switch(id) {
        case APP_LEDS_EVENT_BLINK_START:
            app_leds_start_blink();
            break;
        case APP_LEDS_EVENT_BLINK_STOP:
            app_leds_stop_blink();
            break;
        case APP_LEDS_EVENT_SET_METER:
            app_leds_set_meter(*((uint8_t *) event_data));
            break;
        default:
            ESP_LOGW(TAG, "Unhandled event ID: %d", id);
            break;
    }
}


void app_main(void)
{
    // Initialize Hall Sensor
    if (hall_sensor_init() != ESP_OK) {
        ESP_LOGE(TAG, "Hall Sensor initialization failed");
        return;
    }

    // Initialize App LEDs
    if (app_leds_init() != ESP_OK) {
        ESP_LOGE(TAG, "App LEDs initialization failed");
        return;
    }

    // Create Si7021
    si7021_create_args_t si7021_create_args = SI7021_DEFAULT_CREATE_ARGS();

    si7021_handle_t si7021;
    if (si7021_create(&si7021_create_args, &si7021) != ESP_OK) {
        ESP_LOGE(TAG, "Si7021 creation failed");
        return;
    }

    // Reset Si7021
    if (si7021_reset(si7021) != ESP_OK) {
        ESP_LOGE(TAG, "Si7021 reset failed");
        return;
    }

    // Create App Timer Event Loop
    esp_event_loop_args_t app_timer_event_loop_args = {
        .queue_size = 10,
        .task_name = "App Timer Event Loop Task",
        .task_priority = 1,
        .task_stack_size = 2048,
        .task_core_id = tskNO_AFFINITY,
    };

    esp_event_loop_handle_t app_timer_event_loop;
    if (esp_event_loop_create(&app_timer_event_loop_args, &app_timer_event_loop) != ESP_OK) {
        ESP_LOGE(TAG, "App Timer Event Loop creation failed");
        return;
    }

    // Make App Timer Data
    app_timer_data_t app_timer_data = {
        .app_timer_event_loop = app_timer_event_loop,
        .si7021 = si7021,
        .read_hall = 0,
        .read_temp = 0,
        .log_readings = 0,
    };

    // Create "App Timer"
    esp_timer_create_args_t app_timer_args = {
        .callback = app_timer_callback,
        .arg = &app_timer_data,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "App Timer",
    };

    esp_timer_handle_t app_timer;
    if (esp_timer_create(&app_timer_args, &app_timer) != ESP_OK) {
        ESP_LOGE(TAG, "App Timer creation failed");
        return;
    }

    // Create App LEDs Event Loop
    esp_event_loop_args_t app_leds_event_loop_args = {
        .queue_size = 10,
        .task_name = "App LEDs Event Loop Task",
        .task_priority = 1,
        .task_stack_size = 2048,
        .task_core_id = tskNO_AFFINITY,
    };

    esp_event_loop_handle_t app_leds_event_loop;
    if (esp_event_loop_create(&app_leds_event_loop_args, &app_leds_event_loop) != ESP_OK) {
        ESP_LOGE(TAG, "App LEDs Event Loop creation failed");
        return;
    }

    // Take Hall Base
    TickType_t hall_base_timeout = APP_HALL_BASE_TIMEOUT_MS / portTICK_PERIOD_MS;
    TickType_t ticks_start = xTaskGetTickCount();
    int hall_base = hall_sensor_get_reading();
    int hall_count = 1;
    while (xTaskGetTickCount() - ticks_start < hall_base_timeout) {
        // Do Mean
        hall_base = (
            (hall_base * hall_count) +
            hall_sensor_get_reading()
        ) / (hall_count + 1);
        hall_count++;
    }
    ESP_LOGI(TAG, "Hall Base: %d (%d samples)", hall_base, hall_count);

    // Take Temp Base
    TickType_t temp_base_timeout = APP_TEMP_BASE_TIMEOUT_MS / portTICK_PERIOD_MS;
    ticks_start = xTaskGetTickCount();
    float temp_base = 0;
    int temp_count = 0;
    while (xTaskGetTickCount() - ticks_start < temp_base_timeout) {
        float temp_reading = 0;
        si7021_measure_temp(si7021, &temp_reading);
        // Do Mean
        temp_base = (
            (temp_base * temp_count) +
            temp_reading
        ) / (temp_count + 1);
        temp_count++;
    }
    ESP_LOGI(TAG, "Current Temp: %f (%d samples)", temp_base, temp_count);
    temp_base = (float)((uint8_t) temp_base - 1);
    ESP_LOGI(TAG, "Temp Base: %f", temp_base);

    // Make App Timer FSM
    app_timer_fsm_t app_timer_fsm = {
        .app_leds_event_loop = app_leds_event_loop,
        .hall_base = hall_base,
        .hall_reading_mean = 0,
        .hall_reading_count = 0,
        .temp_base = temp_base,
        .temp_reading_mean = 0,
        .temp_reading_count = 0,
        .blink = false,
        .temp_state = 1, // current temp is 1 LED
    };

    // Register App Timer Event Handler
    if (esp_event_handler_register_with(
        app_timer_event_loop,
        APP_TIMER_EVENTS,
        ESP_EVENT_ANY_ID,
        app_timer_event_handler,
        &app_timer_fsm
    ) != ESP_OK) {
        ESP_LOGE(TAG, "App Timer Event Handler registration failed");
        return;
    }



    // Register App LEDs Event Handler
    if (esp_event_handler_register_with(
        app_leds_event_loop,
        APP_LEDS_EVENTS,
        ESP_EVENT_ANY_ID,
        app_leds_event_handler,
        NULL
    ) != ESP_OK) {
        ESP_LOGE(TAG, "App LEDs Event Handler registration failed");
        return;
    }

    // Post event with current temp state
    esp_event_post_to(
        app_leds_event_loop              ,
        APP_LEDS_EVENTS                  ,
        APP_LEDS_EVENT_SET_METER         ,
        &(app_timer_fsm.temp_state)      ,
        sizeof(app_timer_fsm.temp_state) ,
        0                               );

    // Start App Timer
    if (esp_timer_start_periodic(app_timer, APP_TIMER_PERIOD_MS * 1000) != ESP_OK) {
        ESP_LOGE(TAG, "App Timer start failed");
        return;
    }

    // Wait 5 seconds
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    esp_err_t err;
    // Set Heater Level to 1st level
    if ( (err = si7021_set_heater_level(si7021, SI7021_HEATER_LEVEL_3_09_MA)) ) {
        ESP_LOGE(TAG, "Si7021 set heater level failed");
        return;
    }

    // Set Heater ON
    if ( (err = si7021_set_heater_state(si7021, SI7021_HEATER_STATE_ENABLE)) ) {
        ESP_LOGE(TAG, "Si7021 set heater state failed");
        return;
    }
    ESP_LOGI(TAG, "Heater ON");

    // Wait 20 seconds
    vTaskDelay(20000 / portTICK_PERIOD_MS);

    // Set Heater Level to max level
    if ( (err = si7021_set_heater_level(si7021, SI7021_HEATER_LEVEL_94_20_MA)) ) {
        ESP_LOGE(TAG, "Si7021 set heater level failed");
        return;
    }
    ESP_LOGI(TAG, "Heater Level: %d", SI7021_HEATER_LEVEL_94_20_MA);

    // Wait 20 seconds
    vTaskDelay(20000 / portTICK_PERIOD_MS);

    // Set Heater Level to 1st level
    if ( (err = si7021_set_heater_level(si7021, SI7021_HEATER_LEVEL_3_09_MA)) ) {
        ESP_LOGE(TAG, "Si7021 set heater level failed");
        return;
    }
    ESP_LOGI(TAG, "Heater Level: %d", SI7021_HEATER_LEVEL_3_09_MA);



    // Delay forever to conserve memory
    while (1) {
        vTaskDelay(portMAX_DELAY);
    }
}