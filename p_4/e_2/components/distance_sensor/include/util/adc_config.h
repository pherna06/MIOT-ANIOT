#ifndef __DISTANCE_SENSOR_UTIL_ADC_CONFIG_H__
#define __DISTANCE_SENSOR_UTIL_ADC_CONFIG_H__

// MENUCONFIG //

// ADC Width
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

// ADC Attenuation
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

// ADC VRef
#define DISTANCE_SENSOR_ADC_VREF_MV CONFIG_DISTANCE_SENSOR_ADC_VREF_MV

#endif // __DISTANCE_SENSOR_UTIL_ADC_CONFIG_H__