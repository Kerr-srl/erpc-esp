idf_component_register(SRCS "main.c" REQUIRES erpc erpc_uart_transport)

erpc_add_idl_target(
    interface.erpc
    TARGET_PREFIX
    erpc_interface
    OUTPUT_DIR
    gen/
    GROUPS
    host
    target
    LANGUAGES
    c
    python)

if(HOST)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC HOST=1)
    target_link_libraries(
        ${COMPONENT_LIB} PUBLIC erpc_interface::host::server
                                erpc_interface::target::client)
else()
    target_compile_definitions(${COMPONENT_LIB} PUBLIC HOST=0)
    target_link_libraries(
        ${COMPONENT_LIB} PUBLIC erpc_interface::host::client
                                erpc_interface::target::server)
endif()
