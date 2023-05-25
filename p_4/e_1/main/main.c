#include <stdio.h>

// include for ADC readings
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "soc/adc_channel.h"

// include to delay tasks
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// include to log
#include "esp_log.h"

// reading period
#define PERIOD_MS CONFIG_APP_PERIOD_MS

// samples per reading
#define SAMPLES_PER_READING CONFIG_APP_SAMPLES_PER_READING

void app_main(void)
{
    // tag
    static const char *TAG = "APP_MAIN";
    
    // ADC configuration
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);

    // ADC calibration
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_characterize(
        ADC_UNIT_1,
        ADC_ATTEN_DB_11,
        ADC_WIDTH_BIT_12,
        1100,
        &adc_chars
    );

    // log GPIO num
    ESP_LOGI(TAG, "GPIO: %d", ADC1_CHANNEL_0_GPIO_NUM);

    // log samples per reading
    ESP_LOGI(TAG, "Samples per reading: %d", SAMPLES_PER_READING);

    // ADC reading
    while (1) {
        // take samples and do mean
        uint64_t adc_reading_sum = 0;
        for (int i = 0; i < SAMPLES_PER_READING; i++) {
            adc_reading_sum += (uint64_t) adc1_get_raw(ADC1_CHANNEL_0);
        }
        uint32_t adc_reading = adc_reading_sum / SAMPLES_PER_READING;

        // to voltage
        uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, &adc_chars);
        ESP_LOGI(TAG, "Raw: %d\tVoltage: %dmV", adc_reading, voltage);

        vTaskDelay(PERIOD_MS / portTICK_PERIOD_MS);
    }
}