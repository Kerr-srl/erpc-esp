idf_build_get_property(target IDF_TARGET)

idf_component_register(SRCS
  "src/tinyproto_transport.cpp"
  "src/tinyproto_transport_setup.cpp"
  REQUIRES ${target} erpc tinyproto
  INCLUDE_DIRS include
  )
