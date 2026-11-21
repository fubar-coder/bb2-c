# testlib.obj, the Blitz test library (openspec change basic-testlib,
# spec testing/basic-testlib): user library 81 (LIB_testlib) for
# blitz2:userlibs/testlib/ of the default variant, built with the default
# variant's numbers and checked in its build step (blitzlib check, no
# number of the default acidlibs and deflibs twice).  Test infrastructure:
# no part of acidlibs or deflibs.  testlib.c is the Amiga side, tlcore.c
# the logic (its host tests: tests/host, added by host/CMakeLists.txt);
# the entries are written by the codec of shared/basictest.
set(TESTLIB_OBJ ${BLITZ_VARIANTS_DIR}/${BLITZ_DEFAULT_VARIANT}/userlibs/testlib/testlib.obj
	CACHE INTERNAL "the Blitz test library (libs/userlibs/testlib)")
blitz_library(userlib-testlib VARIANT ${BLITZ_DEFAULT_VARIANT_DIR} OBJ ${TESTLIB_OBJ}
	${CMAKE_CURRENT_SOURCE_DIR}/testlib.c ${CMAKE_CURRENT_SOURCE_DIR}/tlcore.c
	${PROJECT_SOURCE_DIR}/shared/basictest/basictest_write.c
	CHECK ${BLITZ2_DIR}/deflibs ${BLITZ2_DIR}/acidlibs)
target_include_directories(userlib-testlib PRIVATE ${PROJECT_SOURCE_DIR}/shared/basictest
	${CMAKE_CURRENT_SOURCE_DIR})

# The macro text of the drivers (shared/basictest/macros.bas, spec
# testing/basic-test-interface) as the build gives it to the runner, next
# to the library: TESTLIB_MACROS.  In its build step no macro may be named
# like a token (shared/basictest/check-macros.cmake; a macro named like a
# token is a "Syntax Error" in the driver, design D10 of basic-testlib) of
# any library set of the build: blitz2 with the default acidlibs, deflibs
# and userlibs (literallib, testlib), blitz2-ab3's AmiBlitz3 list, and
# every variant (libs/variants, libs/tests/variants) with its acidlibs,
# deflibs and the RWE userlibs built with its numbers.  ctest
# basictest_macros_tokens_broken: a macro named like a token;
# basictest_macros_tokens_ab3 (label originals, skipped without the
# fetched files): AmiBlitz3 v3.14.0's compiler and BigLibs.
set(TESTLIB_MACROS ${BLITZ_VARIANTS_DIR}/${BLITZ_DEFAULT_VARIANT}/userlibs/testlib/macros.bas
	CACHE INTERNAL "the checked macro text of the test drivers")
set(macros_src ${PROJECT_SOURCE_DIR}/shared/basictest/macros.bas)
set(macros_check ${PROJECT_SOURCE_DIR}/shared/basictest/check-macros.cmake)
set(literal_obj ${BLITZ_VARIANTS_DIR}/${BLITZ_DEFAULT_VARIANT}/userlibs/literal/literallib.obj)
set(default_set "$<TARGET_FILE:blitz2>|${BLITZ2_DIR}/acidlibs|${BLITZ2_DIR}/deflibs|${literal_obj}|${TESTLIB_OBJ}")
set(sets "${default_set}^$<TARGET_FILE:blitz2-ab3>@80DB")
set(deps blitz2 blitz2-ab3 acidlibs deflibs userlib-literallib_obj userlib-testlib_obj
	${BLITZ2_DIR}/acidlibs ${BLITZ2_DIR}/deflibs ${literal_obj} ${TESTLIB_OBJ})
get_property(rwe_names GLOBAL PROPERTY RWE_LIB_NAMES)
foreach(build IN LISTS BLITZ_VARIANT_BUILDS)
	string(REPLACE "|" ";" build "${build}")
	list(GET build 0 dir)
	list(GET build 1 out)
	get_filename_component(v ${dir} NAME)
	set(set "$<TARGET_FILE:blitz2>|${out}/acidlibs|${out}/deflibs")
	list(APPEND deps ${v}-acidlibs ${v}-deflibs ${out}/acidlibs ${out}/deflibs)
	foreach(n IN LISTS rwe_names)
		get_property(lib GLOBAL PROPERTY RWE_LIB_${n})
		string(REPLACE "|" ";" lib "${lib}")
		list(GET lib 0 obj)
		list(GET lib 3 lv)
		if(lv STREQUAL v)
			string(APPEND set "|${obj}")
			list(APPEND deps ${v}-${n}_obj ${obj})
		endif()
	endforeach()
	string(APPEND sets "^${set}")
endforeach()
add_custom_command(OUTPUT ${TESTLIB_MACROS}
	COMMAND ${CMAKE_COMMAND} -DBBSRC=${BBSRC} -DMACROS=${macros_src} "-DSETS=${sets}"
		-DOUT=${TESTLIB_MACROS} -P ${macros_check}
	DEPENDS ${macros_src} ${macros_check} ${deps} ${BBSRC} host_tools
	COMMENT "Macro text of the test drivers: no macro named like a token"
	VERBATIM)
add_custom_target(testlib_macros ALL DEPENDS ${TESTLIB_MACROS})

# the check with deliberately broken input: a macro named like a token of
# blitz2 (TestCase passes, NPrint and loadshape do not; without case)
add_test(NAME basictest_macros_tokens_broken COMMAND ${CMAKE_COMMAND} -DBBSRC=${BBSRC}
	-DMACROS=${PROJECT_SOURCE_DIR}/shared/basictest/broken/macros-token.bas
	"-DSETS=${default_set}" -P ${macros_check})
set_tests_properties(basictest_macros_tokens_broken PROPERTIES PASS_REGULAR_EXPRESSION
	"macro[ \n]+NPrint[ \n]+is[ \n]+the[ \n]+token[ \n]+[0-9A-F]+[ \n]+NPrint[ \n]+of[^;]*;[ \n]+macro[ \n]+loadshape[ \n]+is[ \n]+the[ \n]+token[ \n]+[0-9A-F]+[ \n]+LoadShape")  # CMake wraps long messages
# AmiBlitz3's list (design D10: "only if there"): v3.14.0's compiler and
# BigLibs, as the test ab3_tokens takes them
set(ab3 ${PROJECT_SOURCE_DIR}/archive/amiblitz3/github-58a11658)
add_test(NAME basictest_macros_tokens_ab3 COMMAND ${CMAKE_COMMAND} -DBBSRC=${BBSRC}
	-DMACROS=${macros_src} -DSKIP_MISSING=ON
	"-DSETS=${ab3}/Amiblitz3|${ab3}/BlitzLibs/baseLibs|${ab3}/BlitzLibs/utilLibs|${ab3}/BlitzLibs/OsLibs|${ab3}/BlitzLibs/csLibs"
	-P ${macros_check})
set_tests_properties(basictest_macros_tokens_ab3 PROPERTIES LABELS originals SKIP_RETURN_CODE 77)
