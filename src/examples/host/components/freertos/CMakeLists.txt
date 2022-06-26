idf_component_register(INCLUDE_DIRS include)

set(FREERTOS_CONFIG_FILE_DIRECTORY
    ${CMAKE_CURRENT_LIST_DIR}
    CACHE STRING "")
set(FREERTOS_PORT
    GCC_POSIX
    CACHE STRING "")
add_subdirectory(../../../../../external/FreeRTOS-Kernel "FreeRTOS-Kernel")

target_link_libraries(${COMPONENT_LIB} INTERFACE freertos_kernel)
