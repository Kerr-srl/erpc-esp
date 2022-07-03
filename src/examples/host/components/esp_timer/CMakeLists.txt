set(IDF_PATH $ENV{IDF_PATH})
set(ORIGINAL_COMPONENT_PATH ${IDF_PATH}/components/esp_timer)

idf_component_register(SRCS esp_timer.c INCLUDE_DIRS
                       "${ORIGINAL_COMPONENT_PATH}/include")
