# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly

# if defined CMAKE_VER
if (DEFINED ENV{CMAKE_VER})
    message(STATUS "CMAKE_VER: $ENV{CMAKE_VER}")
    cmake_minimum_required(VERSION $ENV{CMAKE_VER})
else()
    cmake_minimum_required(VERSION 3.5)
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(main)