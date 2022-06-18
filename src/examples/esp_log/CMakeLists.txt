cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# The auto generated sdkconfig goes into the build directory
set(SDKCONFIG ${CMAKE_BINARY_DIR}/sdkconfig)
# The sdkconfig files that contain overrides of the default settings
set(SDKCONFIG_DEFAULTS
	${CMAKE_SOURCE_DIR}/sdkconfig.defaults
	)

set(TINYPROTO_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../external/tinyproto/)
execute_process(COMMAND git submodule update --init --progress ${TINYPROTO_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
set(EXTRA_COMPONENT_DIRS
	${CMAKE_CURRENT_LIST_DIR}/../../erpc_esp/
	${TINYPROTO_DIR}
	)

set(COMPONENTS main esptool_py)
project(connection-oriented)
