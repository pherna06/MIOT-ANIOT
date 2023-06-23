#include "app_leds.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "stdio.h"
#include "string.h"

#include "esp_log.h"
const static char *TAG = "APP LEDS";

// STATIC VARIABLES ------------------------------------------------------------

/* LED GPIOs Array */
const static gpio_num_t _gpio_led[] = {
    APP_LEDS_GPIO_LED_0,
    APP_LEDS_GPIO_LED_1,
    APP_LEDS_GPIO_LED_2,
    APP_LEDS_GPIO_LED_3,
    APP_LEDS_GPIO_LED_4,
    APP_LEDS_GPIO_LED_5,
    APP_LEDS_GPIO_LED_6,
    APP_LEDS_GPIO_LED_7,
};

/* APP LEDS init */
static bool _initialized = false;

/* Blink Timer */
static esp_timer_handle_t _blink_timer = NULL;
static uint8_t _blink = 0;

/* GPIO Log Level */
static esp_log_level_t _gpio_default_log_level;

// -----------------------------------------------------------------------------

// MACROS ----------------------------------------------------------------------

//// APP LEDS INIT --------------------------------------------------------------

#define CHECK_IF_APP_LEDS_NOT_INIT()                                           \
    if (_initialized) {                                                        \
        ESP_LOGE(TAG, "APP LEDS already initialized");                         \
        return ESP_FAIL;                                                       \
    }

#define CHECK_IF_APP_LEDS_INIT()                                               \
    if (!_initialized) {                                                       \
        ESP_LOGE(TAG, "APP LEDS not initialized");                             \
        return ESP_FAIL;                                                       \
    }

//// ---------------------------------------------------------------------------

//// GPIO ERRORS --------------------------------------------------------------

#define CHECK_ERR_GPIO_CONFIG(err)                                             \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Could not configure GPIOs: %s", esp_err_to_name(err));  \
        return err;                                                            \
    }

#define LOG_WARN_REPEATED_GPIO(gpio)                                           \
    ESP_LOGW(TAG, "Repeated GPIO %d", gpio);

#define LOG_WARN_GPIO_SET_LEVEL(err, gpio)                                     \
    if (err)                                                                   \
    ESP_LOGW(TAG, "Error setting LED %d: %s",                                  \
        gpio, esp_err_to_name(err));

#define LOG_WARN_GPIO_RESET_PIN(err, gpio)                                     \
    if (err)                                                                   \
        ESP_LOGW(TAG, "Error resetting LED %d: %s",                            \
            gpio, esp_err_to_name(err));

//// ---------------------------------------------------------------------------

//// TIMER ERRORS --------------------------------------------------------------

#define CHECK_ERR_CREATE_TIMER(err)                                            \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Error creating timer: %s", esp_err_to_name(err));       \
        return err;                                                            \
    }

#define CHECK_WARN_DELETE_TIMER(err)                                           \
    if (err) ESP_LOGW(TAG, "Error deleting timer: %s", esp_err_to_name(err));

#define CHECK_ERR_START_TIMER(err)                                             \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Error starting timer: %s", esp_err_to_name(err));       \
        return err;                                                            \
    }

#define CHECK_ERR_STOP_TIMER(err)                                              \
    if (err) {                                                                 \
        ESP_LOGE(TAG, "Error stopping timer: %s", esp_err_to_name(err));       \
        return err;                                                            \
    }

#define CHECK_IF_BLINK_TIMER_ACTIVE()                                          \
    if (esp_timer_is_active(_blink_timer)) {                                   \
        ESP_LOGW(TAG, "Blink timer already active.");                          \
        return ESP_OK;                                                         \
    }

#define CHECK_IF_BLINK_TIMER_NOT_ACTIVE()                                      \
    if (!esp_timer_is_active(_blink_timer)) {                                  \
        ESP_LOGW(TAG, "Blink timer already not active.");                      \
        return ESP_OK;                                                         \
    }


// -----------------------------------------------------------------------------

// PRIVATE FUNCTIONS -----------------------------------------------------------

//// TEMP MODE -----------------------------------------------------------------

/* Blink Mode Timer Callback */
static void _blink_timer_cb(void *arg)
{
    if (_blink) app_leds_all_off();
    else app_leds_all_on();

    _blink ^= 1;
}

/* Get GPIO mask */
static uint64_t _get_gpio_mask(void)
{
    uint64_t gpio_mask = 0;
    for (int i = 0; i < APP_LEDS_NUM; i++) {
        if ( gpio_mask & (1ULL << _gpio_led[i]) )
            LOG_WARN_REPEATED_GPIO(_gpio_led[i]);
        gpio_mask |= (1ULL << _gpio_led[i]);
    }
    return gpio_mask;
}

/* Deinit GPIOs */
static esp_err_t _deinit_gpio(void)
{
    esp_err_t err;

    // Reset GPIOs
    int err_count = 0;
    for (int i = 0; i < APP_LEDS_NUM; i++) {
        err = gpio_reset_pin(_gpio_led[i]);
        if (err) err_count++;
        LOG_WARN_GPIO_RESET_PIN(err, _gpio_led[i]);
    }
    
    return err_count ? ESP_FAIL : ESP_OK;
}

// -----------------------------------------------------------------------------

// PUBLIC FUNCTIONS ------------------------------------------------------------

/* APP LEDS Init */
esp_err_t app_leds_init(void)
{
    CHECK_IF_APP_LEDS_NOT_INIT();
    esp_err_t err = ESP_OK;

    // Set gpio log level to warn
    _gpio_default_log_level = esp_log_level_get("gpio");
    esp_log_level_set("gpio", ESP_LOG_WARN);

    // Configure GPIOs as outputs
    gpio_config_t gpio_conf = {
        .pin_bit_mask = _get_gpio_mask()      ,
        .mode         = GPIO_MODE_OUTPUT      ,
        .pull_up_en   = GPIO_PULLUP_DISABLE   ,
        .pull_down_en = GPIO_PULLDOWN_DISABLE ,
        .intr_type    = GPIO_INTR_DISABLE   };
    err = gpio_config(&gpio_conf);
    CHECK_ERR_GPIO_CONFIG(err);

    // Create blink timer
    esp_timer_create_args_t blink_timer_args = {
        .callback        = &_blink_timer_cb ,
        .arg             = NULL             ,
        .dispatch_method = ESP_TIMER_TASK   ,
        .name            = "blink_timer"   };
    err = esp_timer_create(&blink_timer_args, &_blink_timer);
    if (err) _deinit_gpio();
    CHECK_ERR_CREATE_TIMER(err);

    // Log LEDs gpios
    char gpio_str[256] = "GPIOs:\n";
    char sub_str[32];
    for (int i = 0; i < APP_LEDS_NUM; i++) {
        sprintf(sub_str, "  - LED %d : %d\n", i, _gpio_led[i]);
        strcat(gpio_str, sub_str);
    }
    ESP_LOGI(TAG, "%s", gpio_str);

    // Set initialized to true
    _initialized = true;

    return err;
}

/* APP LEDS Deinit */
esp_err_t app_leds_deinit(void)
{
    CHECK_IF_APP_LEDS_INIT();
    esp_err_t err;
    int err_count = 0;

    // Delete blink timer
    err = esp_timer_delete(_blink_timer);
    if (err) err_count++;
    CHECK_WARN_DELETE_TIMER(err);

    // Deinit GPIOs
    err = _deinit_gpio();
    if (err) err_count++;

    // Set gpio log level to default
    esp_log_level_set("gpio", _gpio_default_log_level);

    return err_count ? ESP_FAIL : ESP_OK;
}

/* APP LEDS All Off */
esp_err_t app_leds_all_off(void)
{
    CHECK_IF_APP_LEDS_INIT();
    esp_err_t err;
    int err_count = 0;
    
    // Set all LEDS Off
    for (int i = 0; i < APP_LEDS_NUM; i++)
        if ( (err = gpio_set_level(_gpio_led[i], 0)) ) {
            LOG_WARN_GPIO_SET_LEVEL(err, _gpio_led[i]);
            err_count++;
        }

    return err_count ? ESP_FAIL : ESP_OK;
}

/* APP LEDS All On */
esp_err_t app_leds_all_on(void)
{
    CHECK_IF_APP_LEDS_INIT();
    esp_err_t err;
    int err_count = 0;
    
    // Set all LEDS On
    for (int i = 0; i < APP_LEDS_NUM; i++)
        if ( (err = gpio_set_level(_gpio_led[i], 1)) ) {
            LOG_WARN_GPIO_SET_LEVEL(err, _gpio_led[i]);
            err_count++;
        }

    return err_count ? ESP_FAIL : ESP_OK;
}

/* APP LEDS Set Meter */
esp_err_t app_leds_set_meter(uint8_t level)
{
    CHECK_IF_APP_LEDS_INIT();
    esp_err_t err;
    int err_count = 0;

    if (level > APP_LEDS_NUM) level = APP_LEDS_NUM;

    // Set ON up to <level>
    for (int i = 0; i < level; i++)
        if ( (err = gpio_set_level(_gpio_led[i], 1)) ) {
            LOG_WARN_GPIO_SET_LEVEL(err, _gpio_led[i]);
            err_count++;
        }
    
    // Set OFF from <level> to end
    for (int i = level; i < APP_LEDS_NUM; i++)
        if ( (err = gpio_set_level(_gpio_led[i], 0)) ) {
            LOG_WARN_GPIO_SET_LEVEL(err, _gpio_led[i]);
            err_count++;
        }

    return err_count ? ESP_FAIL : ESP_OK;
}

/* APP LEDS Start Blink */
esp_err_t app_leds_start_blink(void)
{
    CHECK_IF_APP_LEDS_INIT();
    CHECK_IF_BLINK_TIMER_ACTIVE();
    esp_err_t err = ESP_OK;

    // Start blink timer
    err = esp_timer_start_periodic(_blink_timer,
        APP_LEDS_BLINK_PERIOD_MS * 1000);
    CHECK_ERR_START_TIMER(err);

    return err;
}

/* APP LEDS Stop Blink */
esp_err_t app_leds_stop_blink(void)
{
    CHECK_IF_APP_LEDS_INIT();
    CHECK_IF_BLINK_TIMER_NOT_ACTIVE();
    esp_err_t err = ESP_OK;

    // Stop blink timer
    err = esp_timer_stop(_blink_timer);
    CHECK_ERR_STOP_TIMER(err);

    // Set all LEDS Off
    err = app_leds_all_off();
    if (err) return err;

    return err;
}

// -----------------------------------------------------------------------------