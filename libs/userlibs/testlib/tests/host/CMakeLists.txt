# Host tests of the Blitz test library (libs/userlibs/testlib), built with
# the native compiler by host/CMakeLists.txt:
#   testlib_core  the logic (tlcore.c) with the codec (lib_basictest):
#                 entries and their order, verdicts, the entries kept in
#                 Blitz mode, attribution, preconditions, the batch init,
#                 the end, calls out of order, a damaged state
set(TESTLIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

add_library(testlib_core_host STATIC ${TESTLIB_DIR}/tlcore.c)
set_target_properties(testlib_core_host PROPERTIES C_STANDARD 99 C_EXTENSIONS OFF)
target_compile_options(testlib_core_host PRIVATE -Wall -Wextra -pedantic)
target_include_directories(testlib_core_host PUBLIC ${TESTLIB_DIR})
target_link_libraries(testlib_core_host PUBLIC lib_basictest)

add_executable(test_tlcore test_tlcore.cpp)
target_link_libraries(test_tlcore PRIVATE testlib_core_host snitch::snitch)
target_compile_options(test_tlcore PRIVATE -Wall -Wextra)
set_target_properties(test_tlcore PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
add_test(NAME testlib_core COMMAND test_tlcore)
