# xtra: the companion settings of a Blitz source (README.md)
set(XTRA_SUBCOMMANDS dump check convert)
add_executable(xtra main.cpp cmd_xtra.cpp)
target_link_libraries(xtra PRIVATE lib_cli lib_xtra)
set_target_properties(xtra PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
host_help_test(xtra ${XTRA_SUBCOMMANDS})
host_readme_test(xtra README.md ${XTRA_SUBCOMMANDS})

# the tool on files: an AmiBlitz3 header converted to a .xtra and back,
# dumped and checked; broken inputs refused with their message
add_test(NAME xtra_tool COMMAND ${CMAKE_COMMAND} -DXTRA=$<TARGET_FILE:xtra>
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/tooltest -P ${CMAKE_CURRENT_SOURCE_DIR}/tooltest.cmake)

# the real files (skipped without them, cmake/fetch-originals.cmake): the
# CD's and rwelibsrc's .xtra, the BB22 sources; AmiBlitz3 sources
get_filename_component(repo ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)
add_test(NAME xtra_originals COMMAND ${CMAKE_COMMAND} -DXTRA=$<TARGET_FILE:xtra> -DREPO=${repo}
	-P ${CMAKE_CURRENT_SOURCE_DIR}/originals.cmake)
add_test(NAME xtra_ab3_examples COMMAND ${CMAKE_COMMAND} -DXTRA=$<TARGET_FILE:xtra> -DREPO=${repo}
	-P ${CMAKE_CURRENT_SOURCE_DIR}/ab3examples.cmake)
set_tests_properties(xtra_originals xtra_ab3_examples PROPERTIES SKIP_RETURN_CODE 77)
