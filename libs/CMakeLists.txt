# The libraries blitz2 reads at startup: blitz2:acidlibs (command
# libraries) and blitz2:deflibs (AmigaOS call declarations), each one BigLib
# file joined from one library file per C source (cmake/BlitzLibrary.cmake).
# Variants: the same libraries with other library numbers (NAME/libnums.h).
# The default variant (BLITZ_DEFAULT_VARIANT, cmake/BlitzLibrary.cmake) is
# built to blitz2/acidlibs, deflibs; every other one of libs/variants to
# BLITZ_VARIANTS_DIR/NAME, the test variants of libs/tests/variants to
# libs/tests/variants/NAME in the build directory.  BLITZ_VARIANT_BUILDS:
# "DIR|OUTDIR" per variant besides the default one.
set(BLITZ_VARIANTS_DIR ${CMAKE_BINARY_DIR}/variants)
set(BLITZ_VARIANT_BUILDS)
foreach(kind variants tests/variants)
	file(GLOB headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${kind}/*/libnums.h)
	list(SORT headers)
	foreach(h IN LISTS headers)
		get_filename_component(dir ${h} DIRECTORY)
		get_filename_component(v ${dir} NAME)
		if(kind STREQUAL "variants")
			if(NOT v STREQUAL BLITZ_DEFAULT_VARIANT)
				list(APPEND BLITZ_VARIANT_BUILDS "${dir}|${BLITZ_VARIANTS_DIR}/${v}")
			endif()
		else()
			list(APPEND BLITZ_VARIANT_BUILDS "${dir}|${CMAKE_CURRENT_BINARY_DIR}/tests/variants/${v}")
		endif()
	endforeach()
endforeach()

add_subdirectory(acidlibs)
add_subdirectory(deflibs)
add_subdirectory(userlibs)

# the command reference of the website: the command tables the library
# build steps generate (blitz_library CMDREF) are the committed ones of
# site/data/libraries.  cmdref_update copies the generated files there
# after an intended change (the build step of a changed library fails
# until then); cmdref_complete: no committed file without a library and
# no library without one; the checks with broken input: another library's
# file (cmdref_check_broken), a library missing from the list
# (cmdref_complete_broken)
get_property(cmdref_libraries GLOBAL PROPERTY CMDREF_LIBRARIES)
add_custom_target(cmdref_update
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMDREF_DIR} ${CMDREF_SITE_DIR}
	COMMENT "Copying the generated command tables into site/data/libraries"
	VERBATIM)
add_test(NAME cmdref_complete COMMAND ${CMAKE_COMMAND} -DDIR=${CMDREF_SITE_DIR}
	"-DLIBRARIES=${cmdref_libraries}" -P ${PROJECT_SOURCE_DIR}/tools/cmdref/complete.cmake)
list(SUBLIST cmdref_libraries 1 -1 cmdref_missing)
add_test(NAME cmdref_complete_broken COMMAND ${CMAKE_COMMAND} -DDIR=${CMDREF_SITE_DIR}
	"-DLIBRARIES=${cmdref_missing}" -P ${PROJECT_SOURCE_DIR}/tools/cmdref/complete.cmake)
set_tests_properties(cmdref_complete_broken PROPERTIES
	PASS_REGULAR_EXPRESSION "no library of the build")
add_test(NAME cmdref_check_broken COMMAND ${CMAKE_COMMAND}
	-DCMDREF=${CMDREF} -DLIB=${CMAKE_CURRENT_BINARY_DIR}/acidlibs/fadelib.obj
	-DGROUP=acidlibs -DNAME=fadelib -DOUT=${CMAKE_CURRENT_BINARY_DIR}/cmdref_check_broken.json
	-DCOMMITTED=${CMDREF_SITE_DIR}/acidlibs/trackerlib.json
	-P ${CMDREF_CHECK})
set_tests_properties(cmdref_check_broken PROPERTIES
	PASS_REGULAR_EXPRESSION "differs[ \n]+from[ \n]+[^ \n]*/acidlibs/trackerlib\\.json")  # CMake wraps long messages

# the declarations of every library (header, entries, !libfin; code
# offsets only set or not) against the originals of the CD, same libraries
# in the same order (the compiler's token list order); skipped without the
# CD (archive/cd, cmake --build --preset amiga --target fetch-originals).
# GROUP/declarations-intended.txt, if there: the intended differences
# (fixed original bugs), each of them must occur
foreach(group acidlibs deflibs)
	string(TOUPPER ${group} orig)
	set(intended)
	if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${group}/declarations-intended.txt)
		set(intended ${CMAKE_CURRENT_SOURCE_DIR}/${group}/declarations-intended.txt)
	endif()
	add_test(NAME declarations_${group} COMMAND ${BLITZLIBCOMPARE}
		${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1/BLITZ2/${orig}
		${BLITZ2_DIR}/${group} ${intended})
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
# blitzlibcompare on single library files (the RWE userlibs against their
# originals): a file against itself, and two different libraries (must fail)
add_test(NAME blitzlibcompare_single COMMAND ${BLITZLIBCOMPARE}
	${CMAKE_CURRENT_BINARY_DIR}/acidlibs/fadelib.obj ${CMAKE_CURRENT_BINARY_DIR}/acidlibs/fadelib.obj)
add_test(NAME blitzlibcompare_single_differs COMMAND ${BLITZLIBCOMPARE}
	${CMAKE_CURRENT_BINARY_DIR}/acidlibs/fadelib.obj ${CMAKE_CURRENT_BINARY_DIR}/acidlibs/trackerlib.obj)
set_tests_properties(blitzlibcompare_single_differs PROPERTIES WILL_FAIL TRUE)
# the list of intended differences (declarations-intended.txt): an entry
# that does not occur is a difference (must fail)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/blitzlibcompare_stale.txt
	"# a stale entry\nlib 65450: entry 0 Fade +$0C: $01 != $00\n")
add_test(NAME blitzlibcompare_single_stale COMMAND ${BLITZLIBCOMPARE}
	${CMAKE_CURRENT_BINARY_DIR}/acidlibs/fadelib.obj ${CMAKE_CURRENT_BINARY_DIR}/acidlibs/fadelib.obj
	${CMAKE_CURRENT_BINARY_DIR}/blitzlibcompare_stale.txt)
set_tests_properties(blitzlibcompare_single_stale PROPERTIES
	PASS_REGULAR_EXPRESSION "intended difference not found: lib 65450: entry 0 Fade")
add_subdirectory(tests)
