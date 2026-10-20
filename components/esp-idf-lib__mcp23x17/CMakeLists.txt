list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/eil-cmake-utils/cmake)
include(eil_check_idf_features)
eil_check_idf_has_esp_drivers()

# List of components required by this component regardless of the ESP-IDF version
set(REQUIRED_COMPONENTS i2cdev log esp_idf_lib_helpers)

# mcp23x17.h includes <driver/gpio.h> and <driver/spi_master.h>. Before ESP-IDF 5.3
# these headers were provided by the monolithic "driver" component; since 5.3 they
# live in the dedicated esp_driver_gpio / esp_driver_spi components. In ESP-IDF 6.0
# the "driver" meta-component no longer re-exports them as public requirements, so a
# component that pulls those headers must request the split components explicitly.
if(EIL_IDF_HAS_ESP_DRIVERS)
    list(APPEND REQUIRED_COMPONENTS esp_driver_gpio esp_driver_spi)
else()
    list(APPEND REQUIRED_COMPONENTS driver)
endif()

idf_component_register(
    SRCS mcp23x17.c
    INCLUDE_DIRS .
    REQUIRES ${REQUIRED_COMPONENTS}
)

include(eil_ci)
