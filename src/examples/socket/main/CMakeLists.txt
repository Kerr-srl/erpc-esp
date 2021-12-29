idf_component_register(SRCS "main.c"
	REQUIRES nvs_flash erpc erpc_generic_transport
	)

erpc_add_idl_target(interface.erpc
	TARGET_PREFIX erpc_interface
	OUTPUT_DIR gen/
	GROUPS host target
	LANGUAGES c python
	)
target_link_libraries(${COMPONENT_LIB}
	PUBLIC
	erpc_interface::host::client
	erpc_interface::target::server
	)
