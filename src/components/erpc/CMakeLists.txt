idf_build_get_property(target IDF_TARGET)

idf_component_register(SRCS
  "src/uart_transport.cpp"
  "src/uart_transport_setup.cpp"
  REQUIRES pthread ${target}
  PRIV_REQUIRES log
  INCLUDE_DIRS include
  )

set(ERPC_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../external/erpc)
execute_process(COMMAND git submodule update --init --progress ${ERPC_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
list(APPEND CMAKE_MODULE_PATH "${ERPC_DIR}/cmake")
find_package(ERPC REQUIRED COMPONENTS erpcgen)

add_library(eRPCConfig INTERFACE)

if (CONFIG_ERPC_THREADS_NONE)
  set(ERPC_THREADS ERPC_THREADS_NONE)
elseif(CONFIG_ERPC_THREADS_PTHREADS)
  set(ERPC_THREADS ERPC_THREADS_PTHREADS)
else()
  message(FATAL_ERROR "Unexpected ERPC_THREADS config")
endif()

target_compile_definitions(eRPCConfig
  INTERFACE
  ERPC_THREADS=${ERPC_THREADS}
  ERPC_DEFAULT_BUFFER_SIZE=${CONFIG_ERPC_DEFAULT_BUFFER_SIZE}
  ERPC_DEFAULT_BUFFERS_COUNT=${CONFIG_ERPC_DEFAULT_BUFFERS_COUNT}
  # Force dynamic allocation policy
  # Static allocation policy is not fully supported yet.
  # See https://github.com/EmbeddedRPC/erpc#known-issues-and-limitations
  # and https://github.com/EmbeddedRPC/erpc/pull/169#pullrequestreview-760504606
  ERPC_ALLOCATION_POLICY=ERPC_ALLOCATION_POLICY_DYNAMIC
  #ERPC_NOEXCEPT=${CONFIG_ERPC_NOEXCEPT}
  #ERPC_NESTED_CALLS=${CONFIG_ERPC_NESTED_CALLS}
  #ERPC_NESTED_CALLS_DETECTION=${CONFIG_ERPC_NESTED_CALLS_DETECTION}
  #ERPC_MESSAGE_LOGGING=${CONFIG_ERPC_MESSAGE_LOGGING}
  #ERPC_TRANSPORT_MU_USE_MCMGR=${CONFIG_ERPC_TRANSPORT_MU_USE_MCMGR}
  #ERPC_PRE_POST_ACTION=${CONFIG_ERPC_PRE_POST_ACTION}
  #ERPC_PRE_POST_ACTION_DEFAULT=${CONFIG_ERPC_PRE_POST_ACTION_DEFAULT}
  )
target_link_libraries(eRPCConfig
  INTERFACE
  idf::pthread
  )
add_subdirectory(${ERPC_DIR}/erpc_c "erpc_c")

target_link_libraries(${COMPONENT_LIB} PUBLIC eRPC::lib)
