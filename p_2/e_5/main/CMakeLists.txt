# This project uses Hall Sensor, which is not supported by IDF5.0 or higher
# VERSION CHECK:
idf_build_get_property(idf_ver IDF_VER)
if (NOT idf_ver)
  message(WARNING "IDF version is not detected")

else()
  message(STATUS "IDF_VER: ${idf_ver}")

  # Get IDF version (CMAKE format)
  string(REGEX MATCH "(\\.?[0-9]+)+" idf_version "${idf_ver}")

  if (NOT idf_version)
    message(WARNING "Could not match IDF version")

  else()
    message(STATUS "IDF version: ${idf_version}")
    # If version is 4.2 or lower, throw error
    if (idf_version LESS 4.3)
      message(FATAL_ERROR "IDF version 4.2 or lower is not supported")
    endif()
    # If version is 5.0 or greater, throw error
    if (idf_version GREATER_EQUAL 5)
      message(FATAL_ERROR "IDF version 5.0 or greater is not supported")
    endif()
  
  endif()

endif()

idf_component_register(
  SRCS
    main.c
    hall_sampling.c
    logger.c
    task_monitor.c
  INCLUDE_DIRS
    .
  REQUIRES
    driver
)
