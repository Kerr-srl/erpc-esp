set(IDF_PATH $ENV{IDF_PATH})
set(ORIGINAL_COMPONENT_PATH ${IDF_PATH}/components/log)
set(srcs "${ORIGINAL_COMPONENT_PATH}/log.c" "log_linux.c")

idf_component_register(
    SRCS
    ${srcs}
    INCLUDE_DIRS
    "${ORIGINAL_COMPONENT_PATH}/include"
    LDFRAGMENTS
    ${ORIGINAL_COMPONENT_PATH}/linker.lf
    KCONFIG
    ${ORIGINAL_COMPONENT_PATH}/Kconfig)
