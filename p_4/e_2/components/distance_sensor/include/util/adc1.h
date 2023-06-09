#ifndef __DISTANCE_SENSOR_UTIL_ADC1_H__
#define __DISTANCE_SENSOR_UTIL_ADC1_H__

// For ESP errors
#include "esp_err.h"

// For ESP log
#include "esp_log.h"

// For ESP ADC
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "soc/adc_channel.h"

// For ADC MENUCONFIG
#include "adc_config.h"

// ADC Input [for Distance Sensor Handle] //
typedef struct adc1_input {
    adc1_channel_t channel;
} adc1_input_t;


// GETTERS //
int get_adc1_channel_num(
    adc1_input_t *adc1_input
);

int get_adc1_channel_gpio_num(
    adc1_input_t *adc1_input
);

// FUNCTIONS //
esp_err_t configure_adc1_input(
    adc1_input_t *adc1_input,
    int gpio_num,
    int channel_num
);

void delete_adc1_input(
    adc1_input_t *adc1_input
);

esp_err_t read_adc1_input(
    adc1_input_t *adc1_input,
    int *value
);

uint32_t adc1_reading_to_voltage_mv(
    int adc_reading
);

#endif // __DISTANCE_SENSOR_UTIL_ADC1_H__