# the codec of the Blitz test library's result file (shared/basictest,
# basictest.h), compiled for the host: the library of the tests
# (tools/tests/test_basictest*)
get_filename_component(BASICTEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared/basictest ABSOLUTE)
add_library(lib_basictest STATIC ${BASICTEST_DIR}/basictest_write.c ${BASICTEST_DIR}/basictest_read.c)
set_target_properties(lib_basictest PROPERTIES OUTPUT_NAME basictest C_STANDARD 99 C_EXTENSIONS OFF)
target_compile_options(lib_basictest PRIVATE -pedantic)
target_include_directories(lib_basictest PUBLIC ${BASICTEST_DIR})
