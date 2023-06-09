#include "util/adc1.h"

// Static ADC1 Characteristics //
static esp_adc_cal_characteristics_t _adc1_chars;





// Enum to integer conversion functions //

// From integer to ADC1 Channel enum
adc1_channel_t _adc1_channel_from_int(int i) {
    switch(i) {
        case 0: return ADC1_CHANNEL_0;
        case 1: return ADC1_CHANNEL_1;
        case 2: return ADC1_CHANNEL_2;
        case 3: return ADC1_CHANNEL_3;
        case 4: return ADC1_CHANNEL_4;
        case 5: return ADC1_CHANNEL_5;
        case 6: return ADC1_CHANNEL_6;
        case 7: return ADC1_CHANNEL_7;
        default: return -1;
    }
}

// From ADC1 Channel enum to integer
int _adc1_channel_to_int(adc1_channel_t c) {
    switch (c) {
        case ADC1_CHANNEL_0: return 0;
        case ADC1_CHANNEL_1: return 1;
        case ADC1_CHANNEL_2: return 2;
        case ADC1_CHANNEL_3: return 3;
        case ADC1_CHANNEL_4: return 4;
        case ADC1_CHANNEL_5: return 5;
        case ADC1_CHANNEL_6: return 6;
        case ADC1_CHANNEL_7: return 7;
        default: return -1;
    }
}

// From GPIO Pin to ADC1 Channel enum
adc1_channel_t _adc1_channel_from_gpio(int g) {
    switch (g) {
        case GPIO_NUM_36: return ADC1_GPIO36_CHANNEL;
        case GPIO_NUM_37: return ADC1_GPIO37_CHANNEL;
        case GPIO_NUM_38: return ADC1_GPIO38_CHANNEL;
        case GPIO_NUM_39: return ADC1_GPIO39_CHANNEL;
        case GPIO_NUM_32: return ADC1_GPIO32_CHANNEL;
        case GPIO_NUM_33: return ADC1_GPIO33_CHANNEL;
        case GPIO_NUM_34: return ADC1_GPIO34_CHANNEL;
        case GPIO_NUM_35: return ADC1_GPIO35_CHANNEL;
        default: return -1;
    }
}

// From ADC1 Channel enum to GPIO Pin
int _adc1_channel_to_gpio(adc1_channel_t c) {
    switch (c) {
        case ADC1_GPIO36_CHANNEL: return GPIO_NUM_36;
        case ADC1_GPIO37_CHANNEL: return GPIO_NUM_37;
        case ADC1_GPIO38_CHANNEL: return GPIO_NUM_38;
        case ADC1_GPIO39_CHANNEL: return GPIO_NUM_39;
        case ADC1_GPIO32_CHANNEL: return GPIO_NUM_32;
        case ADC1_GPIO33_CHANNEL: return GPIO_NUM_33;
        case ADC1_GPIO34_CHANNEL: return GPIO_NUM_34;
        case ADC1_GPIO35_CHANNEL: return GPIO_NUM_35;
        default: return -1;
    }
}





// GETTERS //

// Get ADC channel as integer
int get_adc1_channel_num(
    adc1_input_t *adc1_input
) {
    return _adc1_channel_to_int(adc1_input->channel);
}

// Get ADC channel as GPIO number
int get_adc1_channel_gpio_num(
    adc1_input_t *adc1_input
) {
    return _adc1_channel_to_gpio(adc1_input->channel);
}




// FUNCTIONS //

// Configure ADC Input
esp_err_t configure_adc1_input(
    adc1_input_t *adc1_input,
    int gpio_num,
    int channel_num
) {
    static const char *TAG = "(Distance Sensor Utils) Configure ADC Input";
    esp_err_t err;

    // Get channel from gpio_num or channel_num
    adc1_channel_t channel = -1;
    if (gpio_num != -1) { // use gpio_num
        if ( (channel = _adc1_channel_from_gpio(gpio_num)) == -1 ) {
            ESP_LOGE(TAG, "Invalid GPIO number: %d", gpio_num);
            return ESP_ERR_INVALID_ARG;
        }
    }
    else { // use channel_num
        if ( (channel = _adc1_channel_from_int(channel_num)) == -1 ) {
            ESP_LOGE(TAG, "Invalid ADC1 channel number: %d", channel_num);
            return ESP_ERR_INVALID_ARG;
        }
    }

    // Update adc1_input channel
    adc1_input->channel = channel;

    // Configure ADC1 if not already configured
    static bool adc1_configured = false;
    if (!adc1_configured) {
        // Set width
        if ( DISTANCE_SENSOR_ADC_WIDTH == -1 ) {
            ESP_LOGE(TAG, "Invalid ADC1 width: %d", DISTANCE_SENSOR_ADC_WIDTH);
            return ESP_ERR_INVALID_ARG;
        }
        if ( (err = adc1_config_width(DISTANCE_SENSOR_ADC_WIDTH)) ) {
            ESP_LOGE(TAG, "Error setting ADC1 width: %s", esp_err_to_name(err));
            return err;
        }

        // Get ADC1 characteristics
        if ( (err = esp_adc_cal_characterize(
            ADC_UNIT_1,
            DISTANCE_SENSOR_ADC_ATTEN,
            DISTANCE_SENSOR_ADC_WIDTH,
            DISTANCE_SENSOR_ADC_VREF_MV,
            &_adc1_chars
        )) ) {
            ESP_LOGE(TAG, "Error getting ADC1 characteristics: %s", esp_err_to_name(err));
            return err;
        }

        // Set configured flag
        adc1_configured = true;
    }

    // Configure ADC1 channel attenuation
    if ( DISTANCE_SENSOR_ADC_ATTEN == -1 ) {
        ESP_LOGE(TAG, "Invalid ADC1 attenuation: %d", DISTANCE_SENSOR_ADC_ATTEN);
        return ESP_ERR_INVALID_ARG;
    }
    if ( (err = adc1_config_channel_atten(channel, DISTANCE_SENSOR_ADC_ATTEN)) ) {
        ESP_LOGE(TAG, "Error setting ADC1 channel attenuation: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

// Delete ADC Input
void delete_adc1_input(
    adc1_input_t *adc1_input
) {
    // Do nothing
}

// Read ADC Input
esp_err_t read_adc1_input(
    adc1_input_t *adc1_input,
    int *value
) {
    static const char *TAG = "(Distance Sensor Utils) Read ADC Input";

    // Read ADC1 channel
    if ( (*value = adc1_get_raw(adc1_input->channel)) == -1 ) {
        ESP_LOGE(TAG, "Error reading ADC1");
        return ESP_FAIL;
    }

    return ESP_OK;
}

// Convert ADC Input Value to Voltage (mV)
uint32_t adc1_reading_to_voltage_mv(
    int adc_reading
) {
    return esp_adc_cal_raw_to_voltage((uint32_t) adc_reading, &_adc1_chars);
}