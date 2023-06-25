# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly

# if defined CMAKE_VER
if (DEFINED ENV{CMAKE_VER})
    cmake_minimum_required(VERSION $ENV{CMAKE_VER})
else()
    cmake_minimum_required(VERSION 3.5)
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(main)

# get IDF_VERSION_MAJOR with idf_build_get_property
idf_build_get_property(idf_ver_major IDF_VER_MAJOR)

# if idf_ver_major geq 5, throw version error
if (NOT idf_ver_major LESS 5)
    message(FATAL_ERROR 
        "ESP-IDF version must be less than 5.0"
        " as Hall Sensor reading is not supported in"
        " ESP-IDF v5.0 and above"
    )
endif()
