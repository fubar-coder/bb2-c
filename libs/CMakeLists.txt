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

# The declarations of the original libraries as committed data
# (tests/data/declarations/NAME.txt, blitzlib decls: header, token table,
# !libfin - names, argument types, help texts, numbers, no code), so the
# declaration comparisons (declarations_GROUP, rwe_declarations_NAME) run
# without the originals.  declarations_data(TEST ORIGINAL DATA [FIXTURE F]):
# the test TEST_data (label originals) checks that blitzlib decls of
# ORIGINAL gives DATA byte for byte (FIXTURE: the test that makes ORIGINAL,
# the RWE reference build); the target declarations-data writes every DATA
# from its original (after a change of an original, or a new library).
set(DECLS_DATA_DIR ${PROJECT_SOURCE_DIR}/tests/data/declarations)
set(DECLS_DATA_SCRIPT ${PROJECT_SOURCE_DIR}/tools/blitzlib/decls-data.cmake)
add_custom_target(declarations-data
	COMMENT "Declaration data of the original libraries (tests/data/declarations)")
function(declarations_data test original data)
	cmake_parse_arguments(PARSE_ARGV 3 arg "" "FIXTURE" "")
	add_test(NAME ${test}_data COMMAND ${CMAKE_COMMAND} -DBLITZLIB=${BLITZLIB}
		-DORIGINAL=${original} -DDATA=${data} -DOUT=${CMAKE_CURRENT_BINARY_DIR}/${test}_data.txt
		-P ${DECLS_DATA_SCRIPT})
	set_tests_properties(${test}_data PROPERTIES LABELS originals SKIP_RETURN_CODE 77)
	if(arg_FIXTURE)
		set_tests_properties(${test}_data PROPERTIES FIXTURES_REQUIRED ${arg_FIXTURE})
	endif()
	add_custom_target(${test}_data_write
		COMMAND ${CMAKE_COMMAND} -DBLITZLIB=${BLITZLIB} -DORIGINAL=${original} -DDATA=${data}
			-DWRITE=ON -P ${DECLS_DATA_SCRIPT}
		DEPENDS host_tools
		VERBATIM)
	add_dependencies(declarations-data ${test}_data_write)
endfunction()

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
	"-DLIBRARIES=${cmdref_libraries}" -P ${PROJECT_SOURCE_DIR}/tools/blitzlib/cmdref-complete.cmake)
list(SUBLIST cmdref_libraries 1 -1 cmdref_missing)
add_test(NAME cmdref_complete_broken COMMAND ${CMAKE_COMMAND} -DDIR=${CMDREF_SITE_DIR}
	"-DLIBRARIES=${cmdref_missing}" -P ${PROJECT_SOURCE_DIR}/tools/blitzlib/cmdref-complete.cmake)
set_tests_properties(cmdref_complete_broken PROPERTIES
	PASS_REGULAR_EXPRESSION "no library of the build")
add_test(NAME cmdref_check_broken COMMAND ${CMAKE_COMMAND}
	-DBLITZLIB=${BLITZLIB} -DLIB=${CMAKE_CURRENT_BINARY_DIR}/acidlibs/fadelib.obj
	-DGROUP=acidlibs -DNAME=fadelib -DOUT=${CMAKE_CURRENT_BINARY_DIR}/cmdref_check_broken.json
	-DCOMMITTED=${CMDREF_SITE_DIR}/acidlibs/trackerlib.json
	-P ${CMDREF_CHECK})
set_tests_properties(cmdref_check_broken PROPERTIES
	PASS_REGULAR_EXPRESSION "differs[ \n]+from[ \n]+[^ \n]*/acidlibs/trackerlib\\.json")  # CMake wraps long messages

# the declarations of every library (header, entries, !libfin; code
# offsets only set or not) against the originals of the CD, same libraries
# in the same order (the compiler's token list order), as committed data
# (tests/data/declarations/GROUP.txt; declarations_GROUP_data, preset
# originals, checks them against the CD's BLITZ2/GROUP).
# GROUP/declarations-intended.txt, if there: the intended differences
# (fixed original bugs), each of them must occur
foreach(group acidlibs deflibs)
	string(TOUPPER ${group} orig)
	set(intended)
	if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${group}/declarations-intended.txt)
		set(intended ${CMAKE_CURRENT_SOURCE_DIR}/${group}/declarations-intended.txt)
	endif()
	add_test(NAME declarations_${group} COMMAND ${BLITZLIB} compare
		${DECLS_DATA_DIR}/${group}.txt ${BLITZ2_DIR}/${group} ${intended})
	declarations_data(declarations_${group}
		${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1/BLITZ2/${orig}
		${DECLS_DATA_DIR}/${group}.txt)
endforeach()
# with broken input: the data with one declaration byte changed (the first
# name of acidlibs) gives a difference; the check of the data fails on it
set(decls_wrong ${CMAKE_CURRENT_BINARY_DIR}/declarations-wrong.txt)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${DECLS_DATA_DIR}/acidlibs.txt)
file(READ ${DECLS_DATA_DIR}/acidlibs.txt decls_text)
# (the name Anim of entry 0 of lib 94 becomes Bnim)
string(REPLACE "\nentry\t0\tAnim\t" "\nentry\t0\tBnim\t" decls_text "${decls_text}")
file(WRITE ${decls_wrong} "${decls_text}")
add_test(NAME declarations_data_broken COMMAND ${BLITZLIB} compare ${decls_wrong}
	${BLITZ2_DIR}/acidlibs)
set_tests_properties(declarations_data_broken PROPERTIES
	PASS_REGULAR_EXPRESSION "lib 94: entry 0 Bnim \\+\\$C: \\$42 != \\$41")
add_test(NAME declarations_data_check_wrong COMMAND ${CMAKE_COMMAND} -DNO_RUN=ON
	-DOUT=${decls_wrong} -DDATA=${DECLS_DATA_DIR}/acidlibs.txt -P ${DECLS_DATA_SCRIPT})
set_tests_properties(declarations_data_check_wrong PROPERTIES WILL_FAIL ON)
add_test(NAME declarations_data_check_right COMMAND ${CMAKE_COMMAND} -DNO_RUN=ON
	-DOUT=${DECLS_DATA_DIR}/acidlibs.txt -DDATA=${DECLS_DATA_DIR}/acidlibs.txt
	-P ${DECLS_DATA_SCRIPT})
set_tests_properties(declarations_data_check_right PROPERTIES
	PASS_REGULAR_EXPRESSION "are those of the original")
# the token list of compiler and libraries (bbsrc tokens) against the one
# the original TED got (tests/data/ted-tokens.txt): with the CD's compiler
# and libraries (skipped without the CD), and with the ones built here
set(cd_blitz2 ${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1/BLITZ2)
set(tokens_check -DBBSRC=${BBSRC}
	-DEXPECT=${PROJECT_SOURCE_DIR}/tests/data/ted-tokens.txt
	-P ${PROJECT_SOURCE_DIR}/tools/bbsrc/tokens-check.cmake)
add_test(NAME tokens_cd COMMAND ${CMAKE_COMMAND} -DOUT=${CMAKE_CURRENT_BINARY_DIR}/tokens_cd.txt
	"-DFILES=${cd_blitz2}/BLITZ2;${cd_blitz2}/ACIDLIBS;${cd_blitz2}/DEFLIBS" ${tokens_check})
# the port's list has literallib's 127 tokens besides the original's
# (#1018: tests/data/literallib-tokens.txt, named as an addition)
set(literal_tokens ${PROJECT_SOURCE_DIR}/tests/data/literallib-tokens.txt)
add_test(NAME tokens_port COMMAND ${CMAKE_COMMAND} -DOUT=${CMAKE_CURRENT_BINARY_DIR}/tokens_port.txt
	"-DFILES=$<TARGET_FILE:blitz2>;${BLITZ2_DIR}/acidlibs;${BLITZ2_DIR}/deflibs"
	-DADDITION=${literal_tokens} ${tokens_check})
# the addition with its token $90DF left out (written at configure time):
# the list then has a token neither file names, the test must fail
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${literal_tokens})
file(READ ${literal_tokens} literal_text)
string(REGEX REPLACE "\n90DF [^\n]*\n" "\n" literal_text "${literal_text}")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/literallib-tokens-broken.txt "${literal_text}")
add_test(NAME tokens_port_addition_broken COMMAND ${CMAKE_COMMAND}
	-DOUT=${CMAKE_CURRENT_BINARY_DIR}/tokens_port_broken.txt
	"-DFILES=$<TARGET_FILE:blitz2>;${BLITZ2_DIR}/acidlibs;${BLITZ2_DIR}/deflibs"
	-DADDITION=${CMAKE_CURRENT_BINARY_DIR}/literallib-tokens-broken.txt ${tokens_check})
set_tests_properties(tokens_port_addition_broken PROPERTIES WILL_FAIL TRUE)
set_tests_properties(tokens_cd tokens_port PROPERTIES SKIP_RETURN_CODE 77)
# the CD's list is a comparison with the originals (preset originals)
set_tests_properties(tokens_cd PROPERTIES LABELS originals)
# the check itself: without deflibs the list differs, the test must fail
add_test(NAME tokens_broken COMMAND ${CMAKE_COMMAND} -DOUT=${CMAKE_CURRENT_BINARY_DIR}/tokens_broken.txt
	"-DFILES=$<TARGET_FILE:blitz2>;${BLITZ2_DIR}/acidlibs" ${tokens_check})
set_tests_properties(tokens_broken PROPERTIES WILL_FAIL TRUE)
# the check in the build (blitzlib numbers, biglib_join CHECK): a library number
# twice must fail - fadelib once more besides acidlibs
add_test(NAME biglib_check_clash COMMAND ${BLITZLIB} numbers ${BLITZ2_DIR}/acidlibs
	${CMAKE_CURRENT_BINARY_DIR}/acidlibs/fadelib.obj)
set_tests_properties(biglib_check_clash PROPERTIES WILL_FAIL TRUE)
# the reservation of libraries 32 and 33 (#1018; blitzlib_reserved,
# tools/tests/test_blitzlib.c with libraries in memory): literallib once
# passes (the build step), twice is a clash; RIDosLib (33, rwe-1999) is
# refused without --allow-33 by check and numbers, and passes with it
add_test(NAME blitzlib_literallib_twice COMMAND ${BLITZLIB} numbers ${BLITZ2_DIR}/acidlibs
	${CMAKE_CURRENT_BINARY_DIR}/acidlibs/literallib.obj)
set_tests_properties(blitzlib_literallib_twice PROPERTIES WILL_FAIL TRUE)
set(ridos_obj ${BLITZ_VARIANTS_DIR}/rwe-1999/userlibs/rwe/ridoslib.obj)
add_test(NAME blitzlib_reserved_33_check COMMAND ${BLITZLIB} check ${ridos_obj})
add_test(NAME blitzlib_reserved_33_numbers COMMAND ${BLITZLIB} numbers ${BLITZ2_DIR}/deflibs
	${ridos_obj})
set_tests_properties(blitzlib_reserved_33_check blitzlib_reserved_33_numbers PROPERTIES
	WILL_FAIL TRUE)
add_test(NAME blitzlib_reserved_33_allowed COMMAND ${BLITZLIB} check --allow-33 ${ridos_obj})
# blitzlib compare on single library files (the RWE userlibs against their
# originals): a file against itself, and two different libraries (must fail)
add_test(NAME blitzlibcompare_single COMMAND ${BLITZLIB} compare
	${CMAKE_CURRENT_BINARY_DIR}/acidlibs/fadelib.obj ${CMAKE_CURRENT_BINARY_DIR}/acidlibs/fadelib.obj)
add_test(NAME blitzlibcompare_single_differs COMMAND ${BLITZLIB} compare
	${CMAKE_CURRENT_BINARY_DIR}/acidlibs/fadelib.obj ${CMAKE_CURRENT_BINARY_DIR}/acidlibs/trackerlib.obj)
set_tests_properties(blitzlibcompare_single_differs PROPERTIES WILL_FAIL TRUE)
# the list of intended differences (declarations-intended.txt): an entry
# that does not occur is a difference (must fail)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/blitzlibcompare_stale.txt
	"# a stale entry\nlib 65450: entry 0 Fade +$0C: $01 != $00\n")
add_test(NAME blitzlibcompare_single_stale COMMAND ${BLITZLIB} compare
	${CMAKE_CURRENT_BINARY_DIR}/acidlibs/fadelib.obj ${CMAKE_CURRENT_BINARY_DIR}/acidlibs/fadelib.obj
	${CMAKE_CURRENT_BINARY_DIR}/blitzlibcompare_stale.txt)
set_tests_properties(blitzlibcompare_single_stale PROPERTIES
	PASS_REGULAR_EXPRESSION "intended difference not found: lib 65450: entry 0 Fade")
add_subdirectory(tests)
