#include "distance_sensor.h"

// For ESP errors:
#include "esp_err.h"

// For ESP log:
#include "esp_log.h"

// For ESP ADC:
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "soc/adc_channel.h"

// ADC1 Translations
// Channel enum from integer
static adc1_channel_t _channel_num_to_adc1_channel(int channel_num) {
    switch (channel_num) {
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

// Integer from channel enum
static int _adc1_channel_to_channel_num(adc1_channel_t channel) {
    switch (channel) {
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

// Channel enum from GPIO integer
static adc1_channel_t _gpio_num_to_adc1_channel(gpio_num_t gpio_num) {
    switch (gpio_num) {
        case GPIO_NUM_32: return ADC1_GPIO32_CHANNEL;
        case GPIO_NUM_33: return ADC1_GPIO33_CHANNEL;
        case GPIO_NUM_34: return ADC1_GPIO34_CHANNEL;
        case GPIO_NUM_35: return ADC1_GPIO35_CHANNEL;
        case GPIO_NUM_36: return ADC1_GPIO36_CHANNEL;
        case GPIO_NUM_37: return ADC1_GPIO37_CHANNEL;
        case GPIO_NUM_38: return ADC1_GPIO38_CHANNEL;
        case GPIO_NUM_39: return ADC1_GPIO39_CHANNEL;
        default: return -1;
    }
}

// GPIO integer from channel enum
static gpio_num_t _adc1_channel_to_gpio_num(adc1_channel_t channel) {
    switch (channel) {
        case ADC1_CHANNEL_0: return ADC1_CHANNEL_0_GPIO_NUM;
        case ADC1_CHANNEL_1: return ADC1_CHANNEL_1_GPIO_NUM;
        case ADC1_CHANNEL_2: return ADC1_CHANNEL_2_GPIO_NUM;
        case ADC1_CHANNEL_3: return ADC1_CHANNEL_3_GPIO_NUM;
        case ADC1_CHANNEL_4: return ADC1_CHANNEL_4_GPIO_NUM;
        case ADC1_CHANNEL_5: return ADC1_CHANNEL_5_GPIO_NUM;
        case ADC1_CHANNEL_6: return ADC1_CHANNEL_6_GPIO_NUM;
        case ADC1_CHANNEL_7: return ADC1_CHANNEL_7_GPIO_NUM;
        default: return -1;
    }
}

// ADC2 Translations
// Channel enum from integer
static adc2_channel_t _channel_num_to_adc2_channel(int channel_num) {
    switch (channel_num) {
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

// Integer from channel enum
static int _adc2_channel_to_channel_num(adc2_channel_t channel) {
    switch (channel) {
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

// Channel enum from GPIO integer
static adc2_channel_t _gpio_num_to_adc2_channel(gpio_num_t gpio_num) {
    switch (gpio_num) {
        case GPIO_NUM_0:  return ADC2_GPIO0_CHANNEL;
        case GPIO_NUM_2:  return ADC2_GPIO2_CHANNEL;
        case GPIO_NUM_4:  return ADC2_GPIO4_CHANNEL;
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

// GPIO integer from channel name
static gpio_num_t _adc2_channel_to_gpio_num(adc2_channel_t channel) {
    switch (channel) {
        case ADC2_CHANNEL_0:  return ADC2_CHANNEL_0_GPIO_NUM;
        case ADC2_CHANNEL_1:  return ADC2_CHANNEL_1_GPIO_NUM;
        case ADC2_CHANNEL_2:  return ADC2_CHANNEL_2_GPIO_NUM;
        case ADC2_CHANNEL_3:  return ADC2_CHANNEL_3_GPIO_NUM;
        case ADC2_CHANNEL_4:  return ADC2_CHANNEL_4_GPIO_NUM;
        case ADC2_CHANNEL_5:  return ADC2_CHANNEL_5_GPIO_NUM;
        case ADC2_CHANNEL_6:  return ADC2_CHANNEL_6_GPIO_NUM;
        case ADC2_CHANNEL_7:  return ADC2_CHANNEL_7_GPIO_NUM;
        case ADC2_CHANNEL_8:  return ADC2_CHANNEL_8_GPIO_NUM;
        case ADC2_CHANNEL_9:  return ADC2_CHANNEL_9_GPIO_NUM;
        default: return -1;
    }
}


// Menuconfig > ADC WIDTH
#ifdef CONFIG_DISTANCE_SENSOR_ADC_WIDTH_BIT_9
  #define DISTANCE_SENSOR_ADC_WIDTH ADC_WIDTH_BIT_9
#elif CONFIG_DISTANCE_SENSOR_ADC_WIDTH_BIT_10
  #define DISTANCE_SENSOR_ADC_WIDTH ADC_WIDTH_BIT_10
#elif CONFIG_DISTANCE_SENSOR_ADC_WIDTH_BIT_11
  #define DISTANCE_SENSOR_ADC_WIDTH ADC_WIDTH_BIT_11
#elif CONFIG_DISTANCE_SENSOR_ADC_WIDTH_BIT_12
  #define DISTANCE_SENSOR_ADC_WIDTH ADC_WIDTH_BIT_12
#else
  #define DISTANCE_SENSOR_ADC_WIDTH -1
#endif

// Menuconfig > ADC ATTEN
#ifdef CONFIG_DISTANCE_SENSOR_ADC_ATTEN_DB_0
  #define DISTANCE_SENSOR_ADC_ATTEN ADC_ATTEN_DB_0
#elif CONFIG_DISTANCE_SENSOR_ADC_ATTEN_DB_2_5
  #define DISTANCE_SENSOR_ADC_ATTEN ADC_ATTEN_DB_2_5
#elif CONFIG_DISTANCE_SENSOR_ADC_ATTEN_DB_6
  #define DISTANCE_SENSOR_ADC_ATTEN ADC_ATTEN_DB_6
#elif CONFIG_DISTANCE_SENSOR_ADC_ATTEN_DB_11
  #define DISTANCE_SENSOR_ADC_ATTEN ADC_ATTEN_DB_11
#else
  #define DISTANCE_SENSOR_ADC_ATTEN -1
#endif

// Menuconfig > ADC VREF
#define DISTANCE_SENSOR_ADC_VREF_MV CONFIG_DISTANCE_SENSOR_ADC_VREF_MV

// Characterization
static esp_adc_cal_characteristics_t _adc_chars;

// Distance Sensor Handle > ADC Input
typedef struct adc_input adc_input_t;

// Configure ADC
#ifdef CONFIG_DISTANCE_SENSOR_ADC_UNIT_1
struct adc_input {
    adc1_channel_t channel;
};

int _channel_num_from_adc_input(adc_input_t *adc_input) {
    return _adc1_channel_to_channel_num(adc_input->channel);
}

int _gpio_num_from_adc_input(adc_input_t *adc_input) {
    return _adc1_channel_to_gpio_num(adc_input->channel);
}

// Configure ADC1
static esp_err_t _configure_adc1(
    adc1_channel_t channel,
    esp_adc_cal_characteristics_t *chars
) {
    static const char *TAG = "(Distance Sensor Utils) Configure ADC1";
    static bool is_init = false;
    esp_err_t err;

    // Width
    if ( !is_init) {
        if ( DISTANCE_SENSOR_ADC_WIDTH == -1 ) {
            ESP_LOGE(TAG, "ADC width not configured");
            return ESP_ERR_INVALID_ARG;
        }
        if ( (err = adc1_config_width(DISTANCE_SENSOR_ADC_WIDTH)) ) {
            ESP_LOGE(TAG, "adc1_config_width: %s", esp_err_to_name(err));
            return err;
        }

        // Characterization
        if ( (err = esp_adc_cal_characterize(
            ADC_UNIT_1,
            DISTANCE_SENSOR_ADC_ATTEN,
            DISTANCE_SENSOR_ADC_WIDTH,
            DISTANCE_SENSOR_ADC_VREF_MV,
            chars
        )) ) {
            ESP_LOGE(TAG, "esp_adc_cal_characterize: %s", esp_err_to_name(err));
            return err;
        }

        is_init = true;
    }

    // Channel attenuation
    if ( (err = adc1_config_channel_atten(channel, DISTANCE_SENSOR_ADC_ATTEN)) ) {
        ESP_LOGE(TAG, "adc1_config_channel_atten: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}
#elif CONFIG_DISTANCE_SENSOR_ADC_UNIT_2
struct adc_input {
    adc2_channel_t channel;
};

int _channel_num_from_adc_input(adc_input_t *adc_input) {
    return _adc2_channel_to_channel_num(adc_input->channel);
}

int _gpio_num_from_adc_input(adc_input_t *adc_input) {
    return _adc2_channel_to_gpio_num(adc_input->channel);
}

// Configure ADC2
esp_err_t _configure_adc2(
    adc2_channel_t channel,
    esp_adc_cal_characteristics_t *chars
) {
    static const char *TAG = "(Distance Sensor Utils) Configure ADC2";
    static bool is_init = false;
    esp_err_t err;

    // ! Reading width of ADC2 is configured in every reading

    // Caracterization
    if ( !is_init ) {
        if ( (err = esp_adc_cal_characterize(
            ADC_UNIT_2,
            DISTANCE_SENSOR_ADC_ATTEN,
            DISTANCE_SENSOR_ADC_WIDTH,
            DISTANCE_SENSOR_ADC_VREF_MV,
            chars
        )) ) {
            ESP_LOGE(TAG, "esp_adc_cal_characterize: %s", esp_err_to_name(err));
            return err;
        }

        is_init = true;
    }

    // Channel attenuation
    if ( (err = adc2_config_channel_atten(channel, DISTANCE_SENSOR_ADC_ATTEN)) ) {
        ESP_LOGE(TAG, "adc2_config_channel_atten: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}
#endif

esp_err_t _configure_adc_input(adc_input_t *adc_input, int gpio_num, int channel_num)
{
    static const char *TAG = "(Distance Sensor Utils) Configure ADC";
  
  #ifdef CONFIG_DISTANCE_SENSOR_ADC_UNIT_1
    adc1_channel_t channel = -1;
    if (gpio_num != -1) { // use GPIO
        if ( (channel = _gpio_num_to_adc1_channel(gpio_num)) == -1 ) {
            ESP_LOGE(TAG, "Invalid GPIO number for ADC1: %d", gpio_num);
            return ESP_ERR_INVALID_ARG;
        }
    }
    else { // use channel number
        if ( (channel = _channel_num_to_adc1_channel(channel_num)) == -1 ) {
            ESP_LOGE(TAG, "Invalid channel number for ADC1: %d", channel_num);
            return ESP_ERR_INVALID_ARG;
        }
    }
    adc_input->channel = channel;
    return _configure_adc1(channel, &_adc_chars);
  #elif CONFIG_DISTANCE_SENSOR_ADC_UNIT_2
    adc2_channel_t channel = -1;
    if (gpio_num != -1) { // use GPIO
        if ( (channel = _gpio_num_to_adc2_channel(gpio_num)) == -1 ) {
            ESP_LOGE(TAG, "Invalid GPIO number for ADC2: %d", gpio_num);
            return ESP_ERR_INVALID_ARG;
        }
    }
    else { // use channel number
        if ( (channel = _channel_num_to_adc2_channel(channel_num)) == -1 ) {
            ESP_LOGE(TAG, "Invalid channel number for ADC2: %d", channel_num);
            return ESP_ERR_INVALID_ARG;
        }
    }
    adc_input->channel = channel;
    return _configure_adc2(channel, &_adc_chars);
  #else
    ESP_LOGE(TAG, "ADC unit not configured");
    return ESP_ERR_INVALID_ARG;
  #endif
}

void _deconfigure_adc_input(adc_input_t *adc_input)
{
    return;
}

int _adc_read(adc_input_t *adc_input)
{
  #ifdef CONFIG_DISTANCE_SENSOR_ADC_UNIT_1
    return adc1_get_raw(adc_input->channel);
  #elif CONFIG_DISTANCE_SENSOR_ADC_UNIT_2
    adc_reading return adc2_get_raw(adc_input->channel, DISTANCE_SENSOR_ADC_WIDTH);
  #else
    return -1;
  #endif
}

int _adc_reading_to_voltage_mv(int adc_reading)
{
    return esp_adc_cal_raw_to_voltage(adc_reading, &_adc_chars);
}


