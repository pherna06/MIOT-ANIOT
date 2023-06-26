#ifndef __LOGGER_H__
#define __LOGGER_H__

// INCLUDES --------------------------------------------------------------------

/* Tasks */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* Errors */
#include <esp_err.h>

// -----------------------------------------------------------------------------

// FUNCTIONS -------------------------------------------------------------------

esp_err_t logger_start(void);

esp_err_t logger_stop(void);

//// GETTERS -------------------------------------------------------------------

/* Get Logger Task Handle */
const TaskHandle_t *logger_get_logger_task_handle(void);

//// ----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

#endif // __LOGGER_H__