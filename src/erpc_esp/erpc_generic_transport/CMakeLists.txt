idf_build_get_property(target IDF_TARGET)

idf_component_register(
    SRCS
    "src/generic_transport.cpp"
    "src/generic_transport_setup.cpp"
    REQUIRES
    erpc
    INCLUDE_DIRS
    include)
