# bbsrc: Blitz Basic 2 sources and tokens without TED (README.md).
# tokens-check.cmake: the tokens_* tests (libs/CMakeLists.txt);
# detok-archive-text.cmake: the build target archive_text (included by the
# top-level CMakeLists.txt)
set(BBSRC_SUBCOMMANDS tokens detok comment bb22 bb22-info ab3 ab3-cmp)
add_executable(bbsrc main.cpp cmd_tokens.cpp cmd_detok.cpp cmd_comment.cpp cmd_bb22.cpp cmd_ab3.cpp)
target_link_libraries(bbsrc PRIVATE lib_cli lib_blitztokens lib_bbdetok lib_bblines lib_bb22
	lib_tokenizer)
set_target_properties(bbsrc PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
host_help_test(bbsrc ${BBSRC_SUBCOMMANDS})
host_readme_test(bbsrc README.md ${BBSRC_SUBCOMMANDS})

# detok on files: a text tokenised by ab3 and back (the same text), a
# token missing from the list ({L:N}, --unknown, --strict), broken
# arguments, token lists and files
add_test(NAME bbdetok_tool COMMAND ${CMAKE_COMMAND} -DBBSRC=$<TARGET_FILE:bbsrc>
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/detok-tooltest
	-DTEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/detok-tooltest.cmake -P ${WITH_WORKDIR})

# ab3 and ab3-cmp on files: a source tokenised, its output compared with
# itself (equal) and with other bytes (the first line that differs),
# broken arguments and token lists refused
add_test(NAME ab3tok_tool COMMAND ${CMAKE_COMMAND} -DBBSRC=$<TARGET_FILE:bbsrc>
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/ab3-tooltest -DTEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/ab3-tooltest.cmake
	-P ${WITH_WORKDIR})

# bb22 on the real files: the 9 BB22 sources of rwelibsrc converted and
# read by tedsrc detok (skipped without the Aminet archive or the CD)
get_filename_component(repo ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)
add_test(NAME bb22_rwelibsrc COMMAND ${CMAKE_COMMAND} -DBBSRC=$<TARGET_FILE:bbsrc>
	-DTEDSRC=$<TARGET_FILE:tedsrc> -DREPO=${repo} -DOUTDIR=${CMAKE_CURRENT_BINARY_DIR}/rwelibsrc
	-P ${CMAKE_CURRENT_SOURCE_DIR}/bb22-rwelibsrc.cmake)
set_tests_properties(bb22_rwelibsrc PROPERTIES SKIP_RETURN_CODE 77)
