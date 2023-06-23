// INCLUDES --------------------------------------------------------------------

#include "stdint.h"     // uint8_t
#include "stdbool.h"    // bool
#include "esp_timer.h"  // esp_timer_t

// DEFINITIONS -----------------------------------------------------------------

#define APP_LEDS_NUM /* 6 */ CONFIG_APP_LEDS_NUM

#define APP_LEDS_GPIO_LED_0 /* 32 */  CONFIG_APP_LEDS_GPIO_LED_0
#define APP_LEDS_GPIO_LED_1 /* 33 */  CONFIG_APP_LEDS_GPIO_LED_1
#define APP_LEDS_GPIO_LED_2 /* 25 */  CONFIG_APP_LEDS_GPIO_LED_2
#define APP_LEDS_GPIO_LED_3 /* 26 */ CONFIG_APP_LEDS_GPIO_LED_3
#define APP_LEDS_GPIO_LED_4 /* 27 */ CONFIG_APP_LEDS_GPIO_LED_4
#define APP_LEDS_GPIO_LED_5 /* 14 */ CONFIG_APP_LEDS_GPIO_LED_5
#define APP_LEDS_GPIO_LED_6 /* 12 */ CONFIG_APP_LEDS_GPIO_LED_6
#define APP_LEDS_GPIO_LED_7 /* 13 */ CONFIG_APP_LEDS_GPIO_LED_7

#define APP_LEDS_BLINK_PERIOD_MS /* 250 */ CONFIG_APP_LEDS_BLINK_PERIOD_MS

// -----------------------------------------------------------------------------

// FUNCTIONS -------------------------------------------------------------------

esp_err_t app_leds_init(void);
esp_err_t app_leds_deinit(void);

esp_err_t app_leds_all_off(void);
esp_err_t app_leds_all_on(void);

esp_err_t app_leds_set_meter(uint8_t level);

esp_err_t app_leds_start_blink(void);
esp_err_t app_leds_stop_blink(void);

// -----------------------------------------------------------------------------
