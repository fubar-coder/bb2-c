# Host tests of the library test framework (libs/tests), built with the
# native compiler by host/CMakeLists.txt:
#   libs_capture_format  the case writer (capture.c, design D4/D6 of the
#                        openspec change libs-tests-capture) with files in
#                        memory: the text it writes for known cases, and its
#                        raw block file read by the judge (tools/lib/libscmp)
set(LIBS_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(libs_capture_host STATIC ${LIBS_TESTS_DIR}/capture.c)
set_target_properties(libs_capture_host PROPERTIES C_STANDARD 99 C_EXTENSIONS OFF)
target_compile_options(libs_capture_host PRIVATE -Wall -Wextra -pedantic)
target_include_directories(libs_capture_host PUBLIC ${LIBS_TESTS_DIR})

add_executable(test_capture_format test_capture_format.cpp)
target_link_libraries(test_capture_format PRIVATE libs_capture_host lib_libscmp snitch::snitch)
target_compile_options(test_capture_format PRIVATE -Wall -Wextra)
set_target_properties(test_capture_format PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
add_test(NAME libs_capture_format COMMAND test_capture_format)
