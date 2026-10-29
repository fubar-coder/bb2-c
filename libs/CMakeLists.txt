# The libraries blitz2 reads at startup: blitz2:acidlibs (command
# libraries) and blitz2:deflibs (AmigaOS call declarations), each one BigLib
# file joined from one library file per C source (cmake/BlitzLibrary.cmake).
# Variants (libs/variants/NAME/libnums.h): the same libraries with other
# library numbers, to BLITZ_VARIANTS_DIR/NAME/acidlibs and deflibs (the
# default build stays blitz2/acidlibs, deflibs with libs/include/libnums.h)
set(BLITZ_VARIANTS_DIR ${CMAKE_BINARY_DIR}/variants)
file(GLOB variant_headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/variants/*/libnums.h)
set(BLITZ_VARIANTS)
foreach(h IN LISTS variant_headers)
	get_filename_component(dir ${h} DIRECTORY)
	get_filename_component(v ${dir} NAME)
	list(APPEND BLITZ_VARIANTS ${v})
endforeach()
list(SORT BLITZ_VARIANTS)

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
# the variants used as checks (libs/variants/selftest-*): one with the
# numbers unchanged must give the default files; one with fadelib (157)
# and inputoutputlib (164) moved to 3 and 4 must move their tokens, and
# debuglib's call of inputoutputlib (#inputoutputlib<<7+4) with them
set(vcheck ${CMAKE_CURRENT_SOURCE_DIR}/variants/check.cmake)
if(selftest-same IN_LIST BLITZ_VARIANTS)
	add_test(NAME variant_selftest_same COMMAND ${CMAKE_COMMAND} -DMODE=same
		-DVARIANT=${BLITZ_VARIANTS_DIR}/selftest-same -DDEFAULT=${BLITZ2_DIR} -P ${vcheck})
endif()
if(selftest-renumbered IN_LIST BLITZ_VARIANTS)
	add_test(NAME variant_selftest_renumbered COMMAND ${CMAKE_COMMAND} -DMODE=renumbered
		-DVARIANT=${BLITZ_VARIANTS_DIR}/selftest-renumbered -DBLITZTOKENS=${BLITZTOKENS}
		-DBLITZ2=$<TARGET_FILE:blitz2> -DTOKENS=${PROJECT_SOURCE_DIR}/tests/data/ted-tokens.txt
		-DMAP=157:3$<SEMICOLON>164:4
		-DOBJ=${CMAKE_CURRENT_BINARY_DIR}/acidlibs/selftest-renumbered-debuglib.obj
		-DCALL=80000204 -DOLDCALL=80005204
		-DOUT=${CMAKE_CURRENT_BINARY_DIR}/variant_renumbered_tokens.txt -P ${vcheck})
endif()
# the check in the build (biglib check, biglib_join CHECK): a library number
# twice must fail - fadelib once more besides acidlibs
add_test(NAME biglib_check_clash COMMAND ${BIGLIB} check ${BLITZ2_DIR}/acidlibs
	${CMAKE_CURRENT_BINARY_DIR}/acidlibs/fadelib.obj)
set_tests_properties(biglib_check_clash PROPERTIES WILL_FAIL TRUE)
add_subdirectory(tests)
