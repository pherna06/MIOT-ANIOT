#include <stdio.h>


// Sampler Task for Hall Sensor
struct hall_sampler_task_parameters
{
    int period_ms;
    esp_event_loop_handle_t app_events_loop;
};

void hall_sampler_task(void *pvParameters)
{    
    // Configure ADC1 to sample Hall Sensor
    adc1_config_width(ADC_WIDTH_BIT_12);

    // Get task params
    struct hall_sampler_task_parameters *params =
            (struct hall_sampler_task_parameters *)pvParameters;
    
    while (1)
    {
        // Read Hall Sensor
        int hall = hall_sensor_read();

        // Post HALL_NEWSAMPLE event
        ESP_ERROR_CHECK(
            esp_event_post_to(params->app_events_loop, APP_EVENTS,
                HALL_EVENT_NEWSAMPLE, &hall, sizeof(hall), portMAX_DELAY)
        );

        // Sleep for period_ms miliseconds
        vTaskDelay(params->period_ms / portTICK_PERIOD_MS);
    }
}





// Filter Task for Hall Sensor //
#define FILTER_SAMPLES 5

// HALL_EVENT_NEWSAMPLE handler
static void hall_filter_event_handler(
        void *handler_args,
        esp_event_base_t base,
        int32_t id,
        void *event_data)
{
    // Push sample to queue
    fifo_queue_push((struct fifo_queue_t *)handler_args, *(int *)event_data);
}

struct hall_filter_task_parameters
{
    esp_event_loop_handle_t app_events_loop;
    int num_samples;
};

void hall_filter_task(void *pvParameters)
{
    // params
    struct hall_filter_task_parameters *params =
            (struct hall_filter_task_parameters *)pvParameters;
    
    // FIFO queue for samples
    struct fifo_queue_t hall_samples_queue;

    // Register HALL_EVENT_NEWSAMPLE handler
    ESP_ERROR_CHECK(
        esp_event_handler_register_with(
            params->app_events_loop, APP_EVENTS, HALL_EVENT_NEWSAMPLE,
            &hall_filter_event_handler, &hall_samples_queue)
    );

    // Filtering loop
    while (1)
    {
        // Wait for enough samples
        if (hall_samples_queue.size == params->num_samples)
        {
            // Calculate mean
            float mean = fifo_queue_mean(&hall_samples_queue);

            // Post HALL_FILTERSAMPLE event
            ESP_ERROR_CHECK(
                esp_event_post_to(params->app_events_loop, APP_EVENTS,
                    HALL_EVENT_FILTERSAMPLE, &mean, sizeof(mean), portMAX_DELAY)
            );

            // Pop sample
            fifo_queue_pop(&hall_samples_queue);
        }

        // Small sleep
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}