idf_build_get_property(target IDF_TARGET)
set(ERPC_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../external/erpc)

set(COMPONENT_REQUIRES "freertos")

if(NOT target STREQUAL "linux")
    list(APPEND COMPONENT_REQUIRES "pthread")
endif()

idf_component_register(
    REQUIRES
    ${COMPONENT_REQUIRES}
    erpc_esp_target
    INCLUDE_DIRS
    ${ERPC_DIR}/erpc_c/config/
    ${ERPC_DIR}/erpc_c/infra/
    ${ERPC_DIR}/erpc_c/port/
    ${ERPC_DIR}/erpc_c/setup/
    ${ERPC_DIR}/erpc_c/transports/
    SRCS
    ${ERPC_DIR}/erpc_c/infra/erpc_arbitrated_client_manager.cpp
    ${ERPC_DIR}/erpc_c/infra/erpc_basic_codec.cpp
    ${ERPC_DIR}/erpc_c/infra/erpc_client_manager.cpp
    ${ERPC_DIR}/erpc_c/infra/erpc_crc16.cpp
    ${ERPC_DIR}/erpc_c/infra/erpc_framed_transport.cpp
    ${ERPC_DIR}/erpc_c/infra/erpc_message_buffer.cpp
    ${ERPC_DIR}/erpc_c/infra/erpc_message_loggers.cpp
    ${ERPC_DIR}/erpc_c/infra/erpc_server.cpp
    ${ERPC_DIR}/erpc_c/infra/erpc_simple_server.cpp
    ${ERPC_DIR}/erpc_c/infra/erpc_transport_arbitrator.cpp
    ${ERPC_DIR}/erpc_c/infra/erpc_pre_post_action.cpp
    ${ERPC_DIR}/erpc_c/infra/erpc_utils.cpp
    # Use heap allocation functions from stdlib
    ${ERPC_DIR}/erpc_c/port/erpc_port_stdlib.cpp
    ${ERPC_DIR}/erpc_c/setup/erpc_arbitrated_client_setup.cpp
    ${ERPC_DIR}/erpc_c/setup/erpc_client_setup.cpp
    ${ERPC_DIR}/erpc_c/setup/erpc_setup_mbf_dynamic.cpp
    ${ERPC_DIR}/erpc_c/setup/erpc_setup_mbf_static.cpp
    ${ERPC_DIR}/erpc_c/setup/erpc_server_setup.cpp)

execute_process(COMMAND git submodule update --init --progress ${ERPC_DIR}
                WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

include(${CMAKE_CURRENT_LIST_DIR}/erpc_utils.cmake)
erpc_find_erpcgen()

if(CONFIG_ERPC_THREADS_NONE)
    set(ERPC_THREADS ERPC_THREADS_NONE)
elseif(CONFIG_ERPC_THREADS_PTHREADS)
    set(ERPC_THREADS ERPC_THREADS_PTHREADS)
    target_sources(${COMPONENT_LIB}
                   PRIVATE ${ERPC_DIR}/erpc_c/port/erpc_threading_pthreads.cpp)
else()
    message(FATAL_ERROR "Unexpected ERPC_THREADS config")
endif()

if(target STREQUAL "linux")
    find_package(Threads)
    target_link_libraries(${COMPONENT_LIB} PUBLIC "${CMAKE_THREAD_LIBS_INIT}")
else()
    target_link_libraries(${COMPONENT_LIB} PUBLIC "idf::pthread")
endif()

target_compile_definitions(
    ${COMPONENT_LIB}
    PUBLIC
        ERPC_THREADS=${ERPC_THREADS}
        ERPC_DEFAULT_BUFFER_SIZE=${CONFIG_ERPC_DEFAULT_BUFFER_SIZE}
        ERPC_DEFAULT_BUFFERS_COUNT=${CONFIG_ERPC_DEFAULT_BUFFERS_COUNT}
        # Force dynamic allocation policy Static allocation policy is not fully
        # supported yet. See
        # https://github.com/EmbeddedRPC/erpc#known-issues-and-limitations and
        # https://github.com/EmbeddedRPC/erpc/pull/169#pullrequestreview-760504606
        ERPC_ALLOCATION_POLICY=ERPC_ALLOCATION_POLICY_DYNAMIC
        # ERPC_NOEXCEPT=${CONFIG_ERPC_NOEXCEPT}
        # ERPC_NESTED_CALLS=${CONFIG_ERPC_NESTED_CALLS}
        # ERPC_NESTED_CALLS_DETECTION=${CONFIG_ERPC_NESTED_CALLS_DETECTION}
        # ERPC_MESSAGE_LOGGING=${CONFIG_ERPC_MESSAGE_LOGGING}
        # ERPC_TRANSPORT_MU_USE_MCMGR=${CONFIG_ERPC_TRANSPORT_MU_USE_MCMGR}
        # ERPC_PRE_POST_ACTION=${CONFIG_ERPC_PRE_POST_ACTION}
        # ERPC_PRE_POST_ACTION_DEFAULT=${CONFIG_ERPC_PRE_POST_ACTION_DEFAULT}
)

target_include_directories(${COMPONENT_LIB} PUBLIC ${COMPONENT_DIR}/include)
target_compile_options(${COMPONENT_LIB} PUBLIC "-includeerpc_config_override.h")
