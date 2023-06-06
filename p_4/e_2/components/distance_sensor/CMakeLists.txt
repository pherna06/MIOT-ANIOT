idf_component_register(
    SRCS
        "distance_sensor.c"
        "util/adc.c"
        "util/sampling.c"
        "util/readings.c"
    INCLUDE_DIRS
        "include"
        "util"
    REQUIRES "esp_adc_cal"
)
