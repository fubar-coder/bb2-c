# teddetok: a source as TED shows it, through the C core of TED (host side)
add_executable(teddetok main.c)
target_link_libraries(teddetok PRIVATE "$<LINK_GROUP:RESCAN,tedhost,tedcore_host>")
target_compile_options(teddetok PRIVATE -Wall)
set_target_properties(teddetok PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)

# tests: tests/*.bb2 (tokenised, written byte by byte), the expected text
# next to them; unknown token and half token must fail without output
get_filename_component(repo ${CMAKE_CURRENT_SOURCE_DIR}/../../../.. ABSOLUTE)
set(tokens ${repo}/tests/data/ted-tokens.txt)
set(t ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(run -DTEDDETOK=$<TARGET_FILE:teddetok> -DTOKENS=${tokens})
# good: tokens, TAB (tabstop 2), empty line, a text line (LF: tokenised on
# loading), last line
add_test(NAME teddetok_good COMMAND ${CMAKE_COMMAND} ${run} -DIN=${t}/good.bb2
	-DOUT=${CMAKE_CURRENT_BINARY_DIR}/good.txt -DEXPECT=${t}/good.txt -P ${t}/run.cmake)
# long: a line of 135 characters is two lines in TED (127, 3 lost, 5)
add_test(NAME teddetok_long COMMAND ${CMAKE_COMMAND} ${run} -DIN=${t}/long.bb2
	-DOUT=${CMAKE_CURRENT_BINARY_DIR}/long.txt -DEXPECT=${t}/long.txt
	"-DNOTE=2 lines in the file, 3 in TED" -P ${t}/run.cmake)
# broken: a token TED does not know, a token byte at the end of the file
add_test(NAME teddetok_unknown COMMAND ${CMAKE_COMMAND} ${run} -DIN=${t}/unknown.bb2
	-DOUT=${CMAKE_CURRENT_BINARY_DIR}/unknown.txt "-DERROR=unknown.bb2:2: unknown token \\$9001"
	-P ${t}/run.cmake)
add_test(NAME teddetok_half COMMAND ${CMAKE_COMMAND} ${run} -DIN=${t}/half.bb2
	-DOUT=${CMAKE_CURRENT_BINARY_DIR}/half.txt "-DERROR=half.bb2:1: token byte \\$DD without"
	-P ${t}/run.cmake)
add_test(NAME teddetok_missing COMMAND ${CMAKE_COMMAND} ${run} -DIN=${t}/missing.bb2
	-DOUT=${CMAKE_CURRENT_BINARY_DIR}/missing.txt "-DERROR=missing.bb2" -P ${t}/run.cmake)
# the whole bblibsrc set (skipped without the Aminet archive)
add_test(NAME teddetok_bblibsrc COMMAND ${CMAKE_COMMAND} ${run} -DREPO=${repo}
	-P ${t}/bblibsrc.cmake)
set_tests_properties(teddetok_bblibsrc PROPERTIES SKIP_RETURN_CODE 77)
