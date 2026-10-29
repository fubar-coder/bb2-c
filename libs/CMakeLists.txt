# The libraries blitz2 reads at startup: blitz2:acidlibs (command
# libraries) and blitz2:deflibs (AmigaOS call declarations), each one BigLib
# file joined from one library file per C source (cmake/BlitzLibrary.cmake).
add_subdirectory(acidlibs)
add_subdirectory(deflibs)

# the declarations of every library (header, entries, !libfin; code
# offsets only set or not) against the originals of the CD, same libraries
# in the same order (the compiler's token list order); skipped without the
# CD (archive/cd, cmake --build --preset amiga --target fetch-originals)
foreach(group acidlibs deflibs)
	string(TOUPPER ${group} orig)
	add_test(NAME declarations_${group} COMMAND ${BLITZLIBCOMPARE}
		${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1/BLITZ2/${orig}
		${BLITZ2_DIR}/${group})
	set_tests_properties(declarations_${group} PROPERTIES SKIP_RETURN_CODE 77)
endforeach()
# the token list of compiler and libraries (blitztokens) against the one
# the original TED got (tests/data/ted-tokens.txt): with the CD's compiler
# and libraries (skipped without the CD), and with the ones built here
set(cd_blitz2 ${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1/BLITZ2)
set(tokens_check -DBLITZTOKENS=${BLITZTOKENS}
	-DEXPECT=${PROJECT_SOURCE_DIR}/tests/data/ted-tokens.txt
	-P ${PROJECT_SOURCE_DIR}/tools/blitztokens/check.cmake)
add_test(NAME tokens_cd COMMAND ${CMAKE_COMMAND} -DOUT=${CMAKE_CURRENT_BINARY_DIR}/tokens_cd.txt
	"-DFILES=${cd_blitz2}/BLITZ2;${cd_blitz2}/ACIDLIBS;${cd_blitz2}/DEFLIBS" ${tokens_check})
add_test(NAME tokens_port COMMAND ${CMAKE_COMMAND} -DOUT=${CMAKE_CURRENT_BINARY_DIR}/tokens_port.txt
	"-DFILES=$<TARGET_FILE:blitz2>;${BLITZ2_DIR}/acidlibs;${BLITZ2_DIR}/deflibs" ${tokens_check})
set_tests_properties(tokens_cd tokens_port PROPERTIES SKIP_RETURN_CODE 77)
# the check itself: without deflibs the list differs, the test must fail
add_test(NAME tokens_broken COMMAND ${CMAKE_COMMAND} -DOUT=${CMAKE_CURRENT_BINARY_DIR}/tokens_broken.txt
	"-DFILES=$<TARGET_FILE:blitz2>;${BLITZ2_DIR}/acidlibs" ${tokens_check})
set_tests_properties(tokens_broken PROPERTIES WILL_FAIL TRUE)
# the check in the build (biglib check, biglib_join CHECK): a library number
# twice must fail - fadelib once more besides acidlibs
add_test(NAME biglib_check_clash COMMAND ${BIGLIB} check ${BLITZ2_DIR}/acidlibs
	${CMAKE_CURRENT_BINARY_DIR}/acidlibs/fadelib.obj)
set_tests_properties(biglib_check_clash PROPERTIES WILL_FAIL TRUE)
add_subdirectory(tests)
