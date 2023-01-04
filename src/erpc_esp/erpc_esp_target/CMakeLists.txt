idf_build_get_property(target IDF_TARGET)
idf_build_get_property(CURRENT_MAJOR_VERSION IDF_VERSION_MAJOR)

set(TARGET_COMPONENT "")

if(CURRENT_MAJOR_VERSION LESS 4)
    message(FATAL_ERROR "ESP-IDF 3.x is not supported")
elseif(CURRENT_MAJOR_VERSION LESS 5)
    set(TARGET_COMPONENT ${target})
else()
    # Starting from ESP-IDF 5.0, components such as `esp32`, `esp32s2`, etc.
    # don't exist anymore.
    set(TARGET_COMPONENT "")
endif()

idf_component_register(REQUIRES ${TARGET_COMPONENT})
