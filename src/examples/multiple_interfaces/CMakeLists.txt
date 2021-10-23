cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS
	${CMAKE_SOURCE_DIR}/src
	)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# The auto generated sdkconfig goes into the build directory
set(SDKCONFIG ${CMAKE_BINARY_DIR}/sdkconfig)
# The sdkconfig files that contain overrides of the default settings
set(SDKCONFIG_DEFAULTS
	${CMAKE_SOURCE_DIR}/sdkconfig.defaults
	)
set(EXTRA_COMPONENT_DIRS
	${CMAKE_CURRENT_LIST_DIR}/../../components/
	)

set(COMPONENTS main esptool_py)
project(multiple_interface)
