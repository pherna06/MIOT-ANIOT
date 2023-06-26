// INCLUDES --------------------------------------------------------------------

/* Tasks */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Errors */
#include "esp_err.h"

/* Hall Sampling */
#include "hall_sampling.h"

/* Task Monitor */
#include "task_monitor.h"

/* Logger */
#include "logger.h"

// -----------------------------------------------------------------------------

// DEFINES ---------------------------------------------------------------------

#define HALL_SAMPLING_SAMPLE_PERIOD_MS 1000
#define HALL_SAMPLING_FILTER_SAMPLES_NUM 5

#define TASK_MONITOR_PERIOD_MS 60000

#define APP_DURATION_MS 150000

// -----------------------------------------------------------------------------

// APP MAIN --------------------------------------------------------------------

void app_main(void)
{
    // > Initialize Hall Sampling
    ESP_ERROR_CHECK(hall_sampling_init());

    // > Initialize Task Monitor
    ESP_ERROR_CHECK(task_monitor_init());

    // > Add Tasks to Task Monitor
    ESP_ERROR_CHECK(task_monitor_add_task(
        hall_sampling_get_sampler_task_handle()));
    ESP_ERROR_CHECK(task_monitor_add_task(
        hall_sampling_get_filter_task_handle()));
    ESP_ERROR_CHECK(task_monitor_add_task(
        logger_get_logger_task_handle()));
    ESP_ERROR_CHECK(task_monitor_add_task(
        task_monitor_get_monitor_task_handle()));
    

    // > Start Logger
    ESP_ERROR_CHECK(logger_start());


    // > Start Hall Sampling
    ESP_ERROR_CHECK(hall_sampling_start(
        HALL_SAMPLING_SAMPLE_PERIOD_MS   ,
        HALL_SAMPLING_FILTER_SAMPLES_NUM )
    );

    // > Start Task Monitor
    ESP_ERROR_CHECK(task_monitor_start(TASK_MONITOR_PERIOD_MS));

    // > Delay for APP_DURATION_MS
    vTaskDelay(APP_DURATION_MS / portTICK_PERIOD_MS);

    // > Stop Task Monitor
    ESP_ERROR_CHECK(task_monitor_stop());

    // > Stop Hall Sampling
    ESP_ERROR_CHECK(hall_sampling_stop());

    // > Stop Logger
    ESP_ERROR_CHECK(logger_stop());

    // > Deinitialize Task Monitor
    ESP_ERROR_CHECK(task_monitor_deinit());

    // > Deinitialize Hall Sampling
    ESP_ERROR_CHECK(hall_sampling_deinit());
}

// -----------------------------------------------------------------------------