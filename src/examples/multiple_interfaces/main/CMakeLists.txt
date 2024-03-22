idf_component_register(
    SRCS
    main.c
    greet/greet.c
    farewell/farewell.c
    REQUIRES
    erpc
    driver
    erpc_uart_transport
    PRIV_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR})

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
    python
    SERVER_DEPENDS
    ${COMPONENT_LIB})
target_link_libraries(${COMPONENT_LIB} PUBLIC erpc_interface::host::client
                                              erpc_interface::target::server)
