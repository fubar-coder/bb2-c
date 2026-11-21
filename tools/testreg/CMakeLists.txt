# testreg: the ctest tests of the Blitz test sources of the model
# (ID.test.bas) at build time (README.md)
add_executable(testreg main.cpp)
target_link_libraries(testreg PRIVATE lib_cli lib_testreg)
set_target_properties(testreg PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
host_help_test(testreg)
host_readme_test(testreg README.md)

# the tool on sample directories: the include file, the depfile, every
# check with broken input, and a sample CMake project whose ctest -N lists
# the registered tests after the build and again after a tests.toml changed
add_test(NAME testreg_tool COMMAND ${CMAKE_COMMAND} -DTESTREG=$<TARGET_FILE:testreg>
	-DCMAKE_GENERATOR=${CMAKE_GENERATOR} -DCTEST=${CMAKE_CTEST_COMMAND}
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/tooltest
	-DTEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/tooltest.cmake -P ${WITH_WORKDIR})
