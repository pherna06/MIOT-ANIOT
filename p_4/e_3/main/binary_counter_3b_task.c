#ifdef CONFIG_BINARY_COUNTER_3B_TASK

#include "binary_counter_3b.h"

// include ESP errors
#include "esp_err.h"

// include ESP HRT
#include "esp_timer.h"

// include ESP logs
#include "esp_log.h"

static const char *BC_TASK = "Binary Counter 3B Task";

// include for ESP GPIO
#include "driver/gpio.h"

// define GPIO output pins for Binary Counter 3B bits
#define BINARY_COUNTER_3B_GPIO_BIT0 CONFIG_BINARY_COUNTER_3B_GPIO_BIT0
#define BINARY_COUNTER_3B_GPIO_BIT1 CONFIG_BINARY_COUNTER_3B_GPIO_BIT1
#define BINARY_COUNTER_3B_GPIO_BIT2 CONFIG_BINARY_COUNTER_3B_GPIO_BIT2

// define Bynari Counter 3B initial value
#define BINARY_COUNTER_3B_INITIAL_VALUE CONFIG_BINARY_COUNTER_3B_INITIAL_VALUE

// define GPIO input pin for Binary Counter 3B reset
#define BINARY_COUNTER_3B_GPIO_RESET CONFIG_BINARY_COUNTER_3B_GPIO_RESET

// include ESP events
#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(BINARY_COUNTER_3B_EVENTS);

enum {
    BINARY_COUNTER_3B_NEW_VALUE_EVENT,
    BINARY_COUNTER_3B_RESET_EVENT
};

ESP_EVENT_DEFINE_BASE(BINARY_COUNTER_3B_EVENTS);

// Event Handler for Binary Counter 3B
static void _binary_counter_3b_event_handler(
    void* handler_args,
    esp_event_base_t base,
    int32_t id,
    void* event_data
) {
    static const char *NEW_VALUE_TAG = "BINARY_COUNTER_3B_NEW_VALUE_EVENT";
    static const char *RESET_TAG     = "BINARY_COUNTER_3B_RESET_EVENT";

    switch(id) {
        case BINARY_COUNTER_3B_NEW_VALUE_EVENT: {
            // print value
            ESP_LOGI(NEW_VALUE_TAG, "\n"
                                    " - Value > %d\n",
                                    *((uint8_t *) event_data)
            );

            break;
        }
        case BINARY_COUNTER_3B_RESET_EVENT: {
            // print reset
            ESP_LOGI(RESET_TAG, "\n");
            break;
        }
    }
}




// Timer for Binary Counter 3B
static esp_timer_handle_t _binary_counter_3b_timer = NULL;
static uint32_t _period_ms = 1000;

// Timer Callback Function
static void _binary_counter_3b_timer_callback(void *args) {
    binary_counter_3b_handle_t handle = (binary_counter_3b_handle_t) args;

    // Increment Counter
    binary_counter_3b_increment(handle);

    // Get Counter Value
    uint8_t value;
    binary_counter_3b_get_value(handle, &value);

    // Post event
    esp_event_post(
        BINARY_COUNTER_3B_EVENTS,
        BINARY_COUNTER_3B_NEW_VALUE_EVENT,
        &value,
        sizeof(value),
        0
    );
}

// GPIO Input ISR Handler
static void _gpio_input_isr_handler(void *args) {
    binary_counter_3b_handle_t handle = (binary_counter_3b_handle_t) args;

    // Reset counter
    binary_counter_3b_reset(handle);

    // Post event
    esp_event_post(
        BINARY_COUNTER_3B_EVENTS,
        BINARY_COUNTER_3B_RESET_EVENT,
        NULL,
        0,
        0
    );
}

// Binary Counter 3B Task Function
void binary_counter_3b_task(void *pvParameter) {
    esp_err_t err;

    ESP_LOGI(BC_TASK, "Binary Counter 3B Task Running...");

    // Create Args
    binary_counter_3b_create_args_t args = BINARY_COUNTER_3B_CREATE_ARGS_DEFAULT();
    args.name          = "binary_counter_3b";
    args.bit0_gpio     = BINARY_COUNTER_3B_GPIO_BIT0;
    args.bit1_gpio     = BINARY_COUNTER_3B_GPIO_BIT1;
    args.bit2_gpio     = BINARY_COUNTER_3B_GPIO_BIT2;
    args.initial_value = BINARY_COUNTER_3B_INITIAL_VALUE;

    // Create Binary Counter 3B
    binary_counter_3b_handle_t handle;
    if ( (err = binary_counter_3b_create(&args, &handle)) ) {
        ESP_LOGE(BC_TASK, "Binary Counter 3B Create Failed: %s", esp_err_to_name(err));
        goto bc3b_task__error_at_start;
    }

    // Register Event Handler
    if ( (err = esp_event_handler_register(
        BINARY_COUNTER_3B_EVENTS,
        ESP_EVENT_ANY_ID,
        &_binary_counter_3b_event_handler,
        handle
    ))) {
        ESP_LOGE(BC_TASK, "Binary Counter 3B Event Handler Register Failed: %s", esp_err_to_name(err));
        goto bc3b_task__error_after_counter_create;
    }

    // Create Timer Args
    esp_timer_create_args_t timer_args = {
        .callback = _binary_counter_3b_timer_callback,
        .arg = handle,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "binary_counter_3b_timer"
    };

    // Create Timer
    if ( (err = esp_timer_create(&timer_args, &_binary_counter_3b_timer)) ) {
        ESP_LOGE(BC_TASK, "Binary Counter 3B Timer Create Failed: %s", esp_err_to_name(err));
        goto bc3b_task__error_after_register_event_handler;
    }

    // Install GPIO interrupt service
    if ( (err = gpio_install_isr_service(0)) ) {
        ESP_LOGE(BC_TASK, "GPIO ISR Service Install Failed: %s", esp_err_to_name(err));
        goto bc3b_task__error_after_timer_create;
    }

    // Configure GPIO input interrupt type (when button pressed -> posedge)
    if ( (err = gpio_set_intr_type(BINARY_COUNTER_3B_GPIO_RESET, GPIO_INTR_POSEDGE)) ) {
        ESP_LOGE(BC_TASK, "GPIO Set Interrupt Type Failed: %s", esp_err_to_name(err));
        goto bc3b_task__error_after_isr_service_install;
    }

    // Start Timer
    if ( (err = esp_timer_start_periodic(
        _binary_counter_3b_timer,
        _period_ms * 1000
    )) ) {
        ESP_LOGE(BC_TASK, "Binary Counter 3B Timer Start Failed: %s", esp_err_to_name(err));
        goto bc3b_task__error_after_isr_service_install;
    }

    // Add GPIO input ISR handler
    if ( (err = gpio_isr_handler_add(
        BINARY_COUNTER_3B_GPIO_RESET,
        _gpio_input_isr_handler,
        handle
    )) ) {
        ESP_LOGE(BC_TASK, "GPIO ISR Handler Add Failed: %s", esp_err_to_name(err));
        goto bc3b_task__error_after_timer_start_periodic;
    }

    // Wait for delete notification
    while (1) {
        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY)) {
            break;
        }
    }

//bc3b_task__error_after_isr_handler_add:
    // Remove GPIO input ISR handler
    if ( (err = gpio_isr_handler_remove(BINARY_COUNTER_3B_GPIO_RESET)) ) {
        ESP_LOGW(BC_TASK, "GPIO ISR Handler Remove Failed: %s", esp_err_to_name(err));
    }
bc3b_task__error_after_timer_start_periodic:
    // Stop Timer
    if ( (err = esp_timer_stop(_binary_counter_3b_timer)) ) {
        ESP_LOGW(BC_TASK, "Binary Counter 3B Timer Stop Failed: %s", esp_err_to_name(err));
    }
bc3b_task__error_after_isr_service_install:
    // Uninstall GPIO interrupt service
    gpio_uninstall_isr_service();
bc3b_task__error_after_timer_create:
    // Delete Timer
    if ( (err = esp_timer_delete(_binary_counter_3b_timer)) ) {
        ESP_LOGW(BC_TASK, "Binary Counter 3B Timer Delete Failed: %s", esp_err_to_name(err));
    }
bc3b_task__error_after_register_event_handler:
    // Unregister Event Handler
    if ( (err = esp_event_handler_unregister(
        BINARY_COUNTER_3B_EVENTS,
        ESP_EVENT_ANY_ID,
        &_binary_counter_3b_event_handler
    ))) {
        ESP_LOGW(BC_TASK, "Binary Counter 3B Event Handler Unregister Failed: %s", esp_err_to_name(err));
    }
bc3b_task__error_after_counter_create:
    // Delete Binary Counter 3B
    binary_counter_3b_delete(handle);
bc3b_task__error_at_start:
    vTaskDelete(NULL);
}

#endif // CONFIG_BINARY_COUNTER_3B_TASK