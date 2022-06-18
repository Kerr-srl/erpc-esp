idf_build_get_property(target IDF_TARGET)

idf_component_register(
    SRCS
    "erpc_esp_log.c"
    REQUIRES
    erpc
    PRIV_REQUIRES
    log
    ${target}
    INCLUDE_DIRS
    include)
