add_executable(ab3tok main.c)
target_link_libraries(ab3tok PRIVATE lib_tokenizer)

# the tool on files: a source tokenised, its output compared with itself
# (equal) and with other bytes (the first line that differs), broken
# arguments and token lists refused
add_test(NAME ab3tok_tool COMMAND ${CMAKE_COMMAND} -DAB3TOK=$<TARGET_FILE:ab3tok>
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/tooltest -P ${CMAKE_CURRENT_SOURCE_DIR}/tooltest.cmake)
