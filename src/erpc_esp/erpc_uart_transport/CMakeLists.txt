idf_build_get_property(target IDF_TARGET)

idf_component_register(
    SRCS
    "src/uart_transport.cpp"
    "src/uart_transport_setup.cpp"
    REQUIRES
    erpc
    ${target}
    PRIV_REQUIRES
    log
    INCLUDE_DIRS
    include)
