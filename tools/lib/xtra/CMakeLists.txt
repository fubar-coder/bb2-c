# the .xtra model of shared/xtra (xtra.h), compiled for the host: the
# library of the tool xtra and of the tests (tools/tests/test_xtra_*)
get_filename_component(XTRA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared/xtra ABSOLUTE)
add_library(lib_xtra STATIC ${XTRA_DIR}/xtra.c ${XTRA_DIR}/xtra_ab3.c)
set_target_properties(lib_xtra PROPERTIES OUTPUT_NAME xtra C_STANDARD 99 C_EXTENSIONS OFF)
target_compile_options(lib_xtra PRIVATE -pedantic)
target_include_directories(lib_xtra PUBLIC ${XTRA_DIR})
