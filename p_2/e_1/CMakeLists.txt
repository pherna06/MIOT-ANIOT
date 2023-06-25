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

# Get IDF version (vA.*)
idf_build_get_property(idf_ver IDF_VER)
if (NOT idf_ver)
    message(WARNING "IDF version is not detected")
else()
  message(STATUS "IDF_VER: ${idf_ver}")
  # Get IDF version
  string(REGEX MATCH "(\\.?[0-9]+)+" idf_version "${idf_ver}")
  if (NOT idf_version)
      message(WARNING "Could not match IDF version")
  else()
    message(STATUS "IDF version: ${idf_version}")
    # If version is 5.0 or greater, throw error
    if (idf_version GREATER_EQUAL 5)
      message(FATAL_ERROR "IDF version 5.0 or greater is not supported")
    endif()
  endif()
endif()

project(main)