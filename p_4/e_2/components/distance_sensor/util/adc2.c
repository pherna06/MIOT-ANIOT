#include "util/adc2.h"

// inclde for ESP logs
#include "esp_log.h"

static char const *TAG = "Distance Sensor [ADC2 Utils]";

// Static ADC2 Characteristics //
static esp_adc_cal_characteristics_t _adc2_chars;





// Enum to integer conversion functions //

// From integer to ADC2 Channel enum
adc2_channel_t _adc2_channel_from_int(int i) {
    switch(i) {
        case 0: return ADC2_CHANNEL_0;
        case 1: return ADC2_CHANNEL_1;
        case 2: return ADC2_CHANNEL_2;
        case 3: return ADC2_CHANNEL_3;
        case 4: return ADC2_CHANNEL_4;
        case 5: return ADC2_CHANNEL_5;
        case 6: return ADC2_CHANNEL_6;
        case 7: return ADC2_CHANNEL_7;
        case 8: return ADC2_CHANNEL_8;
        case 9: return ADC2_CHANNEL_9;
        default: return -1;
    }
}

// From ADC2 Channel enum to integer
int _adc2_channel_to_int(adc2_channel_t c) {
    switch (c) {
        case ADC2_CHANNEL_0: return 0;
        case ADC2_CHANNEL_1: return 1;
        case ADC2_CHANNEL_2: return 2;
        case ADC2_CHANNEL_3: return 3;
        case ADC2_CHANNEL_4: return 4;
        case ADC2_CHANNEL_5: return 5;
        case ADC2_CHANNEL_6: return 6;
        case ADC2_CHANNEL_7: return 7;
        case ADC2_CHANNEL_8: return 8;
        case ADC2_CHANNEL_9: return 9;
        default: return -1;
    }
}

// From GPIO Pin to ADC2 Channel enum
adc2_channel_t _adc2_channel_from_gpio(int g) {
    switch (g) {
        case GPIO_NUM_4: return ADC2_GPIO4_CHANNEL;
        case GPIO_NUM_0: return ADC2_GPIO0_CHANNEL;
        case GPIO_NUM_2: return ADC2_GPIO2_CHANNEL;
        case GPIO_NUM_15: return ADC2_GPIO15_CHANNEL;
        case GPIO_NUM_13: return ADC2_GPIO13_CHANNEL;
        case GPIO_NUM_12: return ADC2_GPIO12_CHANNEL;
        case GPIO_NUM_14: return ADC2_GPIO14_CHANNEL;
        case GPIO_NUM_27: return ADC2_GPIO27_CHANNEL;
        case GPIO_NUM_25: return ADC2_GPIO25_CHANNEL;
        case GPIO_NUM_26: return ADC2_GPIO26_CHANNEL;
        default: return -1;
    }
}

// From ADC2 Channel enum to GPIO Pin
int _adc2_channel_to_gpio(adc2_channel_t c) {
    switch (c) {
        case ADC2_GPIO4_CHANNEL: return GPIO_NUM_4;
        case ADC2_GPIO0_CHANNEL: return GPIO_NUM_0;
        case ADC2_GPIO2_CHANNEL: return GPIO_NUM_2;
        case ADC2_GPIO15_CHANNEL: return GPIO_NUM_15;
        case ADC2_GPIO13_CHANNEL: return GPIO_NUM_13;
        case ADC2_GPIO12_CHANNEL: return GPIO_NUM_12;
        case ADC2_GPIO14_CHANNEL: return GPIO_NUM_14;
        case ADC2_GPIO27_CHANNEL: return GPIO_NUM_27;
        case ADC2_GPIO25_CHANNEL: return GPIO_NUM_25;
        case ADC2_GPIO26_CHANNEL: return GPIO_NUM_26;
        default: return -1;
    }
}




// GETTERS //

// Get ADC channel as integer
int get_adc2_channel_num(
    adc2_input_t *adc2_input
) {
    return _adc2_channel_to_int(adc2_input->channel);
}

// Get ADC channel as GPIO number
int get_adc2_gpio_num(
    adc2_input_t *adc2_input
) {
    return _adc2_channel_to_gpio(adc2_input->channel);
}




// FUNCTIONS //

// Configure ADC Input //
esp_err_t configure_adc2_input(
    adc2_input_t *adc2_input,
    int gpio_num,
    int channel_num
) {
    esp_err_t err;

    // Get channel from gpio_num or channel_num
    adc2_channel_t channel;
    if (gpio_num != -1) { // use gpio_num
        if ( (channel = _adc2_channel_from_gpio(gpio_num)) == -1 ) {
            ESP_LOGE(TAG, "Invalid GPIO number: %d", gpio_num);
            return ESP_ERR_INVALID_ARG;
        }
    }
    else { // use channel_num
        if ( (channel = _adc2_channel_from_int(channel_num)) == -1 ) {
            ESP_LOGE(TAG, "Invalid ADC2 channel number: %d", channel_num);
            return ESP_ERR_INVALID_ARG;
        }
    }

    // Update adc2_input channel
    adc2_input->channel = channel;

    // Configure ADC2 if not already configured
    static bool adc2_configured = false;
    if (!adc2_configured) {
        // Get ADC2 characteristics
        if ( (err = esp_adc_cal_characterize(
            ADC_UNIT_2,
            DISTANCE_SENSOR_ADC_ATTEN,
            DISTANCE_SENSOR_ADC_WIDTH,
            DISTANCE_SENSOR_ADC_VREF_MV,
            &_adc2_chars
        )) ) {
            ESP_LOGE(TAG, "Error getting ADC2 characteristics: %s", esp_err_to_name(err));
            return err;
        }

        // Set configured flag
        adc2_configured = true;
    }

    // Configure ADC2 channel attenuation
    if ( DISTANCE_SENSOR_ADC_ATTEN == -1 ) {
        ESP_LOGE(TAG, "Invalid ADC2 attenuation: %d", DISTANCE_SENSOR_ADC_ATTEN);
        return ESP_ERR_INVALID_ARG;
    }
    if ( (err = adc2_config_channel_atten(channel, DISTANCE_SENSOR_ADC_ATTEN)) ) {
        ESP_LOGE(TAG, "Error setting ADC2 attenuation: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

// Delete ADC Input
void delete_adc2_input(
    adc2_input_t *adc2_input
) {
    // Do nothing
}

// Read ADC Input
esp_err_t read_adc2_input(
    adc2_input_t *adc2_input,
    int *value
) {
    esp_err_t err;

    // Read ADC2 channel
    if ( (err = adc2_get_raw(adc2_input->channel, ADC_WIDTH_BIT_12, value)) ) {
        ESP_LOGE(TAG, "Error reading ADC2 channel: %s", esp_err_to_name(err));
        *value = -1;
        return err;
    }

    return ESP_OK;
}

// Convert ADC Input Value to Voltage (mV)
uint32_t adc2_reading_to_voltage_mv(
    int adc_reading
) {
    return esp_adc_cal_raw_to_voltage((uint32_t) adc_reading, &_adc2_chars);
}