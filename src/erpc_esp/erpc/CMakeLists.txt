idf_build_get_property(target IDF_TARGET)

set(PTHREAD_COMPONENT "pthread")
set(PTHREAD_LINK_LIBRARY "idf::pthread")
if(target STREQUAL "linux")
    set(PTHREAD_COMPONENT "")
endif()

idf_component_register(REQUIRES ${PTHREAD_COMPONENT} erpc_esp_target)

set(ERPC_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../external/erpc)
execute_process(COMMAND git submodule update --init --progress ${ERPC_DIR}
                WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
list(APPEND CMAKE_MODULE_PATH "${ERPC_DIR}/cmake")
find_package(ERPC REQUIRED COMPONENTS erpcgen)

add_library(eRPCConfig INTERFACE)

if(CONFIG_ERPC_THREADS_NONE)
    set(ERPC_THREADS ERPC_THREADS_NONE)
elseif(CONFIG_ERPC_THREADS_PTHREADS)
    set(ERPC_THREADS ERPC_THREADS_PTHREADS)
else()
    message(FATAL_ERROR "Unexpected ERPC_THREADS config")
endif()

if(target STREQUAL "linux")
    find_package(Threads)
    set(PTHREAD_LINK_LIBRARY "${CMAKE_THREAD_LIBS_INIT}")
endif()

target_compile_definitions(
    eRPCConfig
    INTERFACE
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
target_link_libraries(eRPCConfig INTERFACE ${PTHREAD_LINK_LIBRARY})
add_subdirectory(${ERPC_DIR}/erpc_c "erpc_c")

target_link_libraries(${COMPONENT_LIB} INTERFACE eRPC::lib)
