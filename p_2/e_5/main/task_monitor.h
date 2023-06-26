#ifndef __TASK_MONITOR_H__
#define __TASK_MONITOR_H__

// INCLUDES --------------------------------------------------------------------

/* Errors */
#include <esp_err.h>

/* Events */
#include <esp_event.h>

// -----------------------------------------------------------------------------

// EVENTS ----------------------------------------------------------------------

/* Task Monitor events */
ESP_EVENT_DECLARE_BASE(TASK_MONITOR_EVENTS);

typedef enum {
    TASK_MONITOR_EVENT_NEW_STATUS
} task_monitor_event_t;

// -----------------------------------------------------------------------------

// FUNCTIONS -------------------------------------------------------------------

/* Initialize task monitor */
esp_err_t task_monitor_init(void);

/* Deinitialize task monitor */
esp_err_t task_monitor_deinit(void);

/* Start task monitor */
esp_err_t task_monitor_start(
    uint32_t period_ms );

/* Stop task monitor */
esp_err_t task_monitor_stop(void);

/* Add task handle to task monitor */
esp_err_t task_monitor_add_task(
    const TaskHandle_t *task_handle );

//// GETTERS -------------------------------------------------------------------

/* Get Monitor Task Handle */
const TaskHandle_t *task_monitor_get_monitor_task_handle(void);

/* Get Event Loop Handle */
const esp_event_loop_handle_t *task_monitor_get_event_loop_handle(void);

//// ---------------------------------------------------------------------------

// -----------------------------------------------------------------------------

#endif // __TASK_MONITOR_H__