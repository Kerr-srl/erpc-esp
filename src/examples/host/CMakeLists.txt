cmake_minimum_required(VERSION 3.5)

set(IDF_TARGET "linux")
list(
    APPEND
    EXTRA_COMPONENT_DIRS
    # From
    # https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32c3/api-guides/build-system.html#multiple-components-with-the-same-name
    # we know that when multiple components with the same name are found, the
    # last one is taken. The components in these extra components dir will
    # override the original components and they provide mocks of the original
    # component.s
    "$ENV{IDF_PATH}/tools/mocks"
    # We want to override the original freertos component with the one we
    # provide, which uses the POSIX port of FreeRTOS
    "${CMAKE_CURRENT_LIST_DIR}/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# The auto generated sdkconfig goes into the build directory
set(SDKCONFIG ${CMAKE_BINARY_DIR}/sdkconfig)
# The sdkconfig files that contain overrides of the default settings
set(SDKCONFIG_DEFAULTS ${CMAKE_SOURCE_DIR}/sdkconfig.defaults)

set(TINYPROTO_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../external/tinyproto/)
execute_process(COMMAND git submodule update --init --progress ${TINYPROTO_DIR}
                WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
list(APPEND EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/../../erpc_esp/
     ${TINYPROTO_DIR})

set(COMPONENTS main)

project(host)
