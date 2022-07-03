set(TINYPROT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../../external/tinyproto)
file(GLOB_RECURSE SOURCE_FILES ${TINYPROT_DIR}/src/*.cpp
     ${TINYPROT_DIR}/src/*.c)
file(GLOB_RECURSE HEADER_FILES ${TINYPROT_DIR}/src/*.h)
idf_component_register(SRCS ${SOURCE_FILES} INCLUDE_DIRS "${TINYPROT_DIR}/src" REQUIRES esp_timer)

target_compile_options(${COMPONENT_LIB} PRIVATE "-includeesp_timer.h")

target_compile_definitions(
    ${COMPONENT_LIB} PRIVATE # Force ESP32 HAL port to be used
                             __XTENSA__)
