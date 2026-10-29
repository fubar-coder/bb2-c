# tedtok: a source through the C core of TED (host side)
add_executable(tedtok main.c)
target_link_libraries(tedtok PRIVATE "$<LINK_GROUP:RESCAN,tedhost,tedcore_host>")
set_target_properties(tedtok PROPERTIES C_STANDARD 23 C_EXTENSIONS ON)

# the line check: 129 characters pass, 130 are refused (TED would cut)
set(tokens ${CMAKE_CURRENT_SOURCE_DIR}/../../../../tests/data/ted-tokens.txt)
foreach(n 129 130)
	add_test(NAME tedtok_line${n} COMMAND tedtok ${tokens}
		${CMAKE_CURRENT_SOURCE_DIR}/tests/line${n}.bas ${CMAKE_CURRENT_BINARY_DIR}/line${n}.bb2)
endforeach()
set_tests_properties(tedtok_line130 PROPERTIES WILL_FAIL TRUE)

# the token list made from the CD's compiler and libraries: the same file
# as with the list file (skipped without the CD)
set(cd ${CMAKE_CURRENT_SOURCE_DIR}/../../../../archive/cd/ultimate-blitz-basic-2.1/BLITZ2)
add_test(NAME tedtok_libs COMMAND ${CMAKE_COMMAND} -DTEDTOK=$<TARGET_FILE:tedtok>
	-DTOKENS=${tokens} -DCD=${cd} -DIN=${CMAKE_CURRENT_SOURCE_DIR}/tests/tokens.bas
	-DOUT=${CMAKE_CURRENT_BINARY_DIR}/tokens -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/libs.cmake)
set_tests_properties(tedtok_libs PROPERTIES SKIP_RETURN_CODE 77)
