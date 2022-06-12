idf_build_get_property(target IDF_TARGET)

idf_component_register(
    SRCS
    "src/erpc_esp_log_transport.cpp"
    "src/erpc_esp_log_transport_setup.cpp"
    "src/common.c"
    REQUIRES
    erpc
    PRIV_REQUIRES
    log
    ${target}
    INCLUDE_DIRS
    include)
