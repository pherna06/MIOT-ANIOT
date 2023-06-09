#ifndef __DISTANCE_SENSOR_UTIL_ADC_H__
#define __DISTANCE_SENSOR_UTIL_ADC_H__

// Include corresponding ADC unit header //
#ifdef CONFIG_DISTANCE_SENSOR_ADC_UNIT_1
    #include "util/adc1.h"
    typedef adc1_input_t adc_input_t;

    int (*get_channel_num)(
        adc_input_t *adc_input
    ) = get_adc1_channel_num;

    int (*get_channel_gpio_num)(
        adc_input_t *adc_input
    ) = get_adc1_channel_gpio_num;

    esp_err_t (*configure_adc_input)(
        adc_input_t *adc_input,
        int gpio_num,
        int channel_num
    ) = configure_adc1_input;

    void (*delete_adc_input)(
        adc_input_t *adc_input
    ) = delete_adc1_input;

    esp_err_t (*read_adc_input)(
        adc_input_t *adc_input,
        int *value
    ) = read_adc1_input;

    uint32_t (*adc_reading_to_voltage_mv)(
        int adc_reading
    ) = adc1_reading_to_voltage_mv;

#elif CONFIG_DISTANCE_SENSOR_ADC_UNIT_2
    #include "util/adc2.h"
    typedef adc2_input_t adc_input_t;

    int (*get_channel_num)(
      adc_input_t *adc_input
    ) = get_adc2_channel_num;

    int (*get_channel_gpio_num)(
        adc_input_t *adc_input
    ) = get_adc2_channel_gpio_num;

    esp_err_t (*configure_adc_input)(
        adc_input_t *adc_input,
        int gpio_num,
        int channel_num
    ) = configure_adc2_input;

    void (*delete_adc_input)(
        adc_input_t *adc_input
    ) = delete_adc2_input;

    esp_err_t (*read_adc_input)(
        adc_input_t *adc_input,
        int *value
    ) = read_adc2_input;

    uint32_t (*adc_reading_to_voltage_mv)(
        int adc_reading
    ) = adc2_reading_to_voltage_mv;

#else
  // Nothing
#endif

#endif // __DISTANCE_SENSOR_UTIL_ADC_H__