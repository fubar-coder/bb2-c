# siteexamples: the Blitz Basic examples of the website, checked against the
# manifest of tests/site-examples and written as sources (README.md)
set(SITEEXAMPLES_SUBCOMMANDS check extract)
add_executable(siteexamples main.cpp)
target_link_libraries(siteexamples PRIVATE lib_cli lib_siteexamples)
set_target_properties(siteexamples PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
host_help_test(siteexamples ${SITEEXAMPLES_SUBCOMMANDS})
host_readme_test(siteexamples README.md ${SITEEXAMPLES_SUBCOMMANDS})

# the tool on fake site trees: a block without an entry, a stale entry, a
# block without its closing fence, a bad manifest, an unreadable manifest
# and the files written
add_test(NAME siteexamples_tool COMMAND ${CMAKE_COMMAND} -DSITEEXAMPLES=$<TARGET_FILE:siteexamples>
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/tooltest
	-DTEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/tooltest.cmake -P ${WITH_WORKDIR})
