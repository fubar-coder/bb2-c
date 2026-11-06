add_executable(bbdetok main.c)
target_link_libraries(bbdetok PRIVATE lib_bbdetok)

# the tool on files: a text tokenised by ab3tok and back (the same text),
# a token missing from the list ({L:N}, --unknown, --strict), broken
# arguments, token lists and files
add_test(NAME bbdetok_tool COMMAND ${CMAKE_COMMAND} -DBBDETOK=$<TARGET_FILE:bbdetok>
	-DAB3TOK=$<TARGET_FILE:ab3tok> -DWORK=${CMAKE_CURRENT_BINARY_DIR}/tooltest
	-P ${CMAKE_CURRENT_SOURCE_DIR}/tooltest.cmake)
