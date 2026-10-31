# the command line of blitz2-ab3 (shared/ab3job/ab3job.h), compiled for the
# host: the library of the tests (tools/tests/test_ab3args)
get_filename_component(AB3JOB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared/ab3job ABSOLUTE)
add_library(lib_ab3job STATIC ${AB3JOB_DIR}/ab3args.c)
set_target_properties(lib_ab3job PROPERTIES OUTPUT_NAME ab3job C_STANDARD 99 C_EXTENSIONS OFF)
target_compile_options(lib_ab3job PRIVATE -pedantic)
target_include_directories(lib_ab3job PUBLIC ${AB3JOB_DIR})
