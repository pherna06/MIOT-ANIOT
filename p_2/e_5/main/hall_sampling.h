#ifndef _HALL_SAMPLING_H_
#define _HALL_SAMPLING_H_

// INCLUDES --------------------------------------------------------------------

/* Tasks */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* Errors */
#include <esp_err.h>

/* Events */
#include <esp_event.h>

// -----------------------------------------------------------------------------

// EVENTS ----------------------------------------------------------------------

/* Hall sampling events */
ESP_EVENT_DECLARE_BASE(HALL_SAMPLING_EVENTS);

typedef enum {
    HALL_SAMPLING_EVENT_NEW_SAMPLE,
    HALL_SAMPLING_EVENT_FILTER_SAMPLE
} hall_sampling_event_id_t;

// -----------------------------------------------------------------------------

// FUNCTIONS -------------------------------------------------------------------

/* Initialize hall sampling */
esp_err_t hall_sampling_init(void);

/* Deinitialize hall sampling */
esp_err_t hall_sampling_deinit(void);

/* Hall Sampling Start */
esp_err_t hall_sampling_start(
    uint32_t _sample_period_ms    ,
    uint32_t _filter_samples_num );

/* Hall Sampling Stop */
esp_err_t hall_sampling_stop(void);

//// GETTERS -------------------------------------------------------------------

/* Get Event Loop Handle */
const esp_event_loop_handle_t *hall_sampling_get_event_loop_handle(void);

/* Get Sampler Task Handle */
const TaskHandle_t *hall_sampling_get_sampler_task_handle(void);

/* Get Filter Task Handle */
const TaskHandle_t *hall_sampling_get_filter_task_handle(void);

//// ----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

#endif // _HALL_SAMPLING_H_