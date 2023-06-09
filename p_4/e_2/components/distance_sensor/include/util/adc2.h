#ifndef __DISTANCE_SENSOR_UTIL_ADC2_H__
#define __DISTANCE_SENSOR_UTIL_ADC2_H__

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
typedef struct adc2_input {
    adc2_channel_t channel;
} adc2_input_t;


// GETTERS //
int get_channel_num(
    adc2_input_t *adc2_input
);

int get_channel_gpio_num(
    adc2_input_t *adc2_input
);

// FUNCTIONS //
esp_err_t configure_adc2_input(
    adc2_input_t *adc2_input,
    int gpio_num,
    int channel_num
);

void delete_adc2_input(
    adc2_input_t *adc2_input
);

esp_err_t read_adc2_input(
    adc2_input_t *adc2_input,
    int *value
);

uint32_t adc_reading_to_voltage_mv(
    int adc_reading
);

#endif // __DISTANCE_SENSOR_UTIL_ADC2_H__