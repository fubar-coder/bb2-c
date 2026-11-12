# libs_tests: the emulator tests of the libraries as one m68k snitch
# program (from bb2 libs-c/tests).  The test logic is C (libtest.c,
# systest*.c, each main renamed to NAME_main); libs_tests.cpp calls it and
# reports every check to snitch.  The programs load every library twice,
# the C build (NAME.obj) and the original of the CD (NAME.orig), "like the
# compiler" (sysload.h), and compare results, registers and memory.
# ctest libs_emulator runs it in Amiberry (emulator.cmake) against the
# recordings of the originals' results (tape.c, tapes/), without the CD.
# The programs converted to one set (libtest.c and the programs that
# define LIBS_ONE_SET, sysload.h; openspec change libs-tests-capture) load
# NAME.obj or NAME.orig, as LIBSET says, and write their cases
# (capture.c), compared with expected/NAME.txt.

# snitch for libnix on the 68000: no exceptions, threads, all reporters,
# small limits (the defaults need 260 KB of data and over 100 KB of
# stack), no clock, no colours, no floating point to_chars
set(SNITCH_WITH_EXCEPTIONS     OFF CACHE BOOL "" FORCE)
set(SNITCH_WITH_MULTITHREADING OFF CACHE BOOL "" FORCE)
set(SNITCH_WITH_ALL_REPORTERS  OFF CACHE BOOL "" FORCE)
set(SNITCH_MAX_TEST_CASES         100 CACHE STRING "" FORCE)
set(SNITCH_MAX_COMMAND_LINE_ARGS  32  CACHE STRING "" FORCE)
set(SNITCH_MAX_UNIQUE_TAGS        32  CACHE STRING "" FORCE)
set(SNITCH_MAX_TEST_NAME_LENGTH   256 CACHE STRING "" FORCE)
set(SNITCH_MAX_EXPR_LENGTH        512 CACHE STRING "" FORCE)
set(SNITCH_MAX_MESSAGE_LENGTH     512 CACHE STRING "" FORCE)
set(SNITCH_MAX_PATH_LENGTH        256 CACHE STRING "" FORCE)
set(SNITCH_WITH_TIMINGS       OFF CACHE BOOL "" FORCE)
set(SNITCH_DEFAULT_WITH_COLOR OFF CACHE BOOL "" FORCE)
set(SNITCH_APPEND_TO_CHARS    OFF CACHE BOOL "" FORCE)
include(${PROJECT_SOURCE_DIR}/cmake/Snitch.cmake)
target_compile_options(snitch PRIVATE -fno-exceptions -fno-rtti)

set(c_tests libtest systest systest_mathstr systest_runtime systest_system systest_cmdlibs
	systest_med)
list(TRANSFORM c_tests APPEND .c OUTPUT_VARIABLE c_sources)
# the tests of the RWE userlibs: rwe/NAME.c (main renamed rwe_NAME_main;
# argv[1] = the directory with NAME.obj and NAME.orig, see sysload.h),
# one snitch test case each ("rwe NAME", tag [rwe]) through the generated
# rwe_tests.inc; their library files come with USERLIBS below.  A test
# needs its library libs/userlibs/rwe/NAME.c.
file(GLOB rwe_all CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/rwe/*.c)
set(rwe_tests)
set(rwe_inc "")
foreach(t IN LISTS rwe_all)
	get_filename_component(n ${t} NAME_WE)
	if(NOT EXISTS ${PROJECT_SOURCE_DIR}/libs/userlibs/rwe/${n}.c)
		message(FATAL_ERROR "libs/tests/rwe/${n}.c: no libs/userlibs/rwe/${n}.c")
	endif()
	list(APPEND rwe_tests ${t})
	set_source_files_properties(${t} PROPERTIES COMPILE_DEFINITIONS "main=rwe_${n}_main;BL_SNITCH")
	string(APPEND rwe_inc "RWE_TEST(${n})\n")
endforeach()
file(CONFIGURE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/rwe_tests.inc CONTENT "${rwe_inc}" @ONLY)

# capture.c: the case writer of the programs that observe one set
# (capture.h; its system calls in capture_amiga.c, its host test in
# host/), compiled here for the m68k and linked into libs_tests
add_library(libs_capture OBJECT capture.c capture_amiga.c)
target_compile_options(libs_capture PRIVATE -Wall -Wextra -fomit-frame-pointer)

add_executable(libs_tests libs_tests.cpp callregs.c tape.c bugorig.c selftest_fixed.c ${c_sources}
	${rwe_tests})
target_include_directories(libs_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
set_source_files_properties(bugorig.c selftest_fixed.c PROPERTIES COMPILE_DEFINITIONS BL_SNITCH)
set_target_properties(libs_tests PROPERTIES SUFFIX "" CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_compile_options(libs_tests PRIVATE -Wall -fomit-frame-pointer
	$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti>)
# each C test program's main becomes NAME_main; checks go to snitch
foreach(t IN LISTS c_tests)
	set_source_files_properties(${t}.c PROPERTIES COMPILE_DEFINITIONS "main=${t}_main;BL_SNITCH")
endforeach()
# -u pulls libnix swapstack.o, which honours __stack at startup
target_link_options(libs_tests PRIVATE -Wl,-u,___stkinit)
target_link_libraries(libs_tests PRIVATE libs_capture snitch::snitch)

# in Amiberry: libs_tests with every acidlib of this build and the RWE
# userlibs (label emulator; skipped without Amiberry).  libs_emulator
# compares them with the recordings of the originals' results
# (tapes/NAME.tape, tape.c), without the CD; libs_emulator_originals
# (label originals) compares them with the originals of the CD live and
# records again: the recordings must be the committed ones.  A program
# that observes one set (capture.h) runs with LIBSET=obj in libs_emulator
# and LIBSET=orig in libs_emulator_originals, and its cases (the test's
# test/captures/NAME.txt, kept for diffing) are compared with the expected
# data (expected/NAME.txt) by results libs-compare.
get_property(rwe_userlibs GLOBAL PROPERTY RWE_EMULATOR_LIBS)
string(REPLACE ";" "$<SEMICOLON>" rwe_userlibs "${rwe_userlibs}")
set(LIBS_TESTS_TIMEOUT 300 CACHE STRING "libs_emulator: time limit of the emulator run (seconds)")
set(tapes ${CMAKE_CURRENT_SOURCE_DIR}/tapes)
set(expected ${CMAKE_CURRENT_SOURCE_DIR}/expected)
math(EXPR limit "600 + ${EMULATION_QUEUE_WAIT}")
foreach(mode replay record)
	set(name libs_emulator)
	set(labels emulator)
	set(libset obj)
	if(mode STREQUAL "record")
		set(name libs_emulator_originals)
		set(labels emulator originals)
		set(libset orig)
	endif()
	add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
		-DAMIGA_RUN=${AMIGA_RUN} -DBLITZLIB=${BLITZLIB}
		-DPROGRAM=$<TARGET_FILE:libs_tests>
		-DLIBS=${CMAKE_BINARY_DIR}/libs/acidlibs
		-DORIGINALS=${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1/BLITZ2/ACIDLIBS
		-DUSERLIBS=${rwe_userlibs} -DTAPE=${mode} -DTAPES=${tapes}
		-DLIBSET=${libset} -DEXPECTED=${expected} -DRESULTS=${RESULTS}
		-DWORK=${CMAKE_CURRENT_BINARY_DIR}/${name} -DTIMEOUT=${LIBS_TESTS_TIMEOUT}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake)
	set_tests_properties(${name} PROPERTIES LABELS "${labels}" SKIP_RETURN_CODE 77
		TIMEOUT ${limit})
endforeach()
# a USERLIBS original made by a test (the reference build of an RWE
# source, libs/userlibs/rwe/refbuild.cmake): its fixture
get_property(rwe_fixtures GLOBAL PROPERTY RWE_EMULATOR_FIXTURES)
if(rwe_fixtures)
	list(REMOVE_DUPLICATES rwe_fixtures)
	set_tests_properties(libs_emulator_originals PROPERTIES FIXTURES_REQUIRED "${rwe_fixtures}")
endif()

# writing the expected data (expected.cmake; not ALL, reviewed with
# git diff libs/tests/expected): libs-capture records it from the original
# (two runs with LIBSET=orig, the second with another memory layout,
# written only if both agree; needs Amiberry, the CD and the reference
# build of the RWE sources, ctest --preset originals -R rwe_refbuild);
# libs-update takes the [fixed #N] cases from the port (one run with
# LIBSET=obj, no CD), and LIBS_UPDATE_FIX (KEY#N;...) makes an
# unqualified case such a pair.  LIBS_CAPTURE_FILTER (a snitch filter of
# libs_tests, e.g. "libtest ldivlib") limits both to the programs it runs.
set(LIBS_CAPTURE_FILTER "" CACHE STRING
	"libs-capture, libs-update: only the libs_tests test cases of this snitch filter (empty: all)")
set(LIBS_UPDATE_FIX "" CACHE STRING
	"libs-update: KEY#N;... - the unqualified case KEY becomes the pair KEY [orig] / KEY [fixed #N]")
foreach(mode capture update)
	set(fix "")
	set(what "the original (two runs) -> ${expected}")
	if(mode STREQUAL "update")
		string(REPLACE ";" "$<SEMICOLON>" fix "${LIBS_UPDATE_FIX}")
		set(what "the port -> the [fixed #N] cases of ${expected}")
	endif()
	if(LIBS_CAPTURE_FILTER)
		string(APPEND what " (filter ${LIBS_CAPTURE_FILTER})")
	endif()
	add_custom_target(libs-${mode}
		COMMAND ${CMAKE_COMMAND} -DMODE=${mode} -DEMULATOR=${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake
			-DRESULTS=${RESULTS} -DEXPECTED=${expected} -DWORK=${CMAKE_CURRENT_BINARY_DIR}/libs-${mode}
			-DAMIGA_RUN=${AMIGA_RUN} -DBLITZLIB=${BLITZLIB} -DPROGRAM=$<TARGET_FILE:libs_tests>
			-DLIBS=${CMAKE_BINARY_DIR}/libs/acidlibs
			-DORIGINALS=${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1/BLITZ2/ACIDLIBS
			-DUSERLIBS=${rwe_userlibs} -DTAPES=${tapes} -DTIMEOUT=${LIBS_TESTS_TIMEOUT}
			"-DFILTER=${LIBS_CAPTURE_FILTER}" "-DFIX=${fix}"
			-P ${CMAKE_CURRENT_SOURCE_DIR}/expected.cmake
		USES_TERMINAL
		COMMENT "The library tests' cases from ${what}"
		VERBATIM)
	add_dependencies(libs-${mode} libs_tests acidlibs host_tools)
endforeach()
# the orchestration of both with a fake run (no emulator): a deterministic
# record is written, a non-deterministic one refused with the expected
# data unchanged, an update makes the pair, a skipped run fails, a filter
# leaves the other files alone
add_test(NAME libs_capture_target COMMAND ${CMAKE_COMMAND} -DRESULTS=${RESULTS}
	-DSCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/expected.cmake
	-DFAKE=${CMAKE_CURRENT_SOURCE_DIR}/selftest/fake-emulator.cmake
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/libs_capture_target
	-P ${CMAKE_CURRENT_SOURCE_DIR}/selftest/expected-target.cmake)
set_tests_properties(libs_capture_target PROPERTIES
	PASS_REGULAR_EXPRESSION "expected-target: all runs as expected")
# the checks of the recordings with broken input: a recording with a wrong
# hash fails the replay (one program, in the emulator); a recording run
# whose files are not the committed ones fails (tape-compare.cmake, no
# emulator)
set(LIBS_SELFTEST_TIMEOUT 60 CACHE STRING
	"libs_expect_fixed*, libs_emulator_tape_wrong, libs_emulator_wrong, libs_emulator_lost: time limit of the emulator run (seconds)")
math(EXPR selflimit "${LIBS_SELFTEST_TIMEOUT} + 60 + ${EMULATION_QUEUE_WAIT}")
add_test(NAME libs_emulator_tape_wrong COMMAND ${CMAKE_COMMAND}
	-DAMIGA_RUN=${AMIGA_RUN} -DPROGRAM=$<TARGET_FILE:libs_tests>
	-DLIBS=${CMAKE_BINARY_DIR}/libs/acidlibs -DTAPE=replay -DTAPES=${tapes}
	-DTAPE_CORRUPT=systest_runtime -DFILTER=[runtime]
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/libs_emulator_tape_wrong -DTIMEOUT=${LIBS_SELFTEST_TIMEOUT}
	-P ${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake)
set_tests_properties(libs_emulator_tape_wrong PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
	TIMEOUT ${selflimit} PASS_REGULAR_EXPRESSION
	"FAIL tape systest_runtime: observations 0\\.\\.[0-9]+ differ from the original's recording")
add_test(NAME libs_tapes_compare_same COMMAND ${CMAKE_COMMAND}
	-DRECORDED=${tapes} -DCOMMITTED=${tapes} -P ${CMAKE_CURRENT_SOURCE_DIR}/tape-compare.cmake)
add_test(NAME libs_tapes_compare_wrong COMMAND ${CMAKE_COMMAND}
	-DCOMMITTED=${tapes} -DCORRUPT=ON -DWORK=${CMAKE_CURRENT_BINARY_DIR}/tapes_compare_wrong
	-P ${CMAKE_CURRENT_SOURCE_DIR}/tape-compare.cmake)
set_tests_properties(libs_tapes_compare_wrong PROPERTIES PASS_REGULAR_EXPRESSION
	"rwe_bb_blitlib.tape: differs\n *rwe_blitzbomberslibtwo.tape: committed, not recorded")
# the check of the cases with broken input: the run's first case of
# libtest_ldivlib changed (emulator.cmake CAPTURE_CORRUPT) fails the compare
# with the expected data, naming the key
add_test(NAME libs_emulator_wrong COMMAND ${CMAKE_COMMAND}
	-DAMIGA_RUN=${AMIGA_RUN} -DPROGRAM=$<TARGET_FILE:libs_tests>
	-DLIBS=${CMAKE_BINARY_DIR}/libs/acidlibs -DTAPE=replay -DTAPES=${tapes}
	-DLIBSET=obj -DEXPECTED=${expected} -DRESULTS=${RESULTS} -DCAPTURE_CORRUPT=libtest_ldivlib
	"-DFILTER=libtest ldivlib"
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/libs_emulator_wrong -DTIMEOUT=${LIBS_SELFTEST_TIMEOUT}
	-P ${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake)
set_tests_properties(libs_emulator_wrong PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
	TIMEOUT ${selflimit} PASS_REGULAR_EXPRESSION
	"libtest_ldivlib: \\|routine count\\|\\|1: expected \\$00000002, got \\$00000000\n")
# the check of the kept registers with broken input: datalib with thunks
# that change d2 (lost/datalib.c, a library file of this directory only)
# in place of the C build's; systest states d2 for both datalib routines
# (KEEPS), writes "lost d2" for their calls, and the compare fails on the
# first of them
blitz_library(libs_lost_datalib OBJ ${CMAKE_CURRENT_BINARY_DIR}/lost/datalib.obj lost/datalib.c)
add_test(NAME libs_emulator_lost COMMAND ${CMAKE_COMMAND}
	-DAMIGA_RUN=${AMIGA_RUN} -DPROGRAM=$<TARGET_FILE:libs_tests>
	-DLIBS=${CMAKE_BINARY_DIR}/libs/acidlibs -DNO_ORIGINALS=ON
	-DOBJ_VARIANTS=${CMAKE_CURRENT_BINARY_DIR}/lost/datalib.obj
	-DLIBSET=obj -DEXPECTED=${expected} -DRESULTS=${RESULTS} -DFILTER=[sys]
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/libs_emulator_lost -DTIMEOUT=${LIBS_SELFTEST_TIMEOUT}
	-P ${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake)
set_tests_properties(libs_emulator_lost PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
	TIMEOUT ${selflimit} PASS_REGULAR_EXPRESSION
	"systest: datalib init\\|kept\\|\\|1: expected kept, got lost d2\n")

# reproduced original bugs (bugorig.c, hidden snitch tests [.bug-orig]):
# each checks the correct behaviour and FAILS as long as the C build
# reproduces the original; label bug-orig, only the test preset bug-orig
# runs them.  One emulator run each (a corrupt decrunch may crash it),
# only the C build (no CD needed).  ILLEGAL_WATCH: Amiberry logs accesses
# to unmapped memory, emulator.cmake checks the ranges the program names.
# Their own time limit: a run takes seconds, a hang (the bug, e.g.
# crunchlib-crm2-readtab) needs no more.
set(LIBS_BUG_ORIG_TIMEOUT 60 CACHE STRING
	"libs bug-orig tests: time limit of the emulator run (seconds)")
foreach(t cliargs-parpath-cli crunch-implode-overread crunch-decrunch-corrupt
		crunchlib-crm-minsecdist crunchlib-crm-overlap-4gb crunchlib-crm2-readtab
		crunchlib-pp-readd1 debuglib-findoff-flags)
	string(REPLACE "-" "_" name "bug_orig_${t}")
	set(watch OFF)
	if(t STREQUAL "crunch-implode-overread")
		set(watch ON)
	endif()
	math(EXPR limit "${LIBS_BUG_ORIG_TIMEOUT} + 60 + ${EMULATION_QUEUE_WAIT}")
	# declared with the other bug-orig tests, in the order of their COST
	# (ctest learns the cost only from passing tests, these fail on
	# purpose): the limit of the one session (cmake/bug-orig-order.cmake)
	bug_orig_add_test(${name} COST ${LIBS_BUG_ORIG_TIMEOUT} COMMAND ${CMAKE_COMMAND}
		-DAMIGA_RUN=${AMIGA_RUN} -DPROGRAM=$<TARGET_FILE:libs_tests>
		-DLIBS=${CMAKE_BINARY_DIR}/libs/acidlibs -DNO_ORIGINALS=ON
		-DILLEGAL_WATCH=${watch} -DFILTER=${t}
		-DWORK=${CMAKE_CURRENT_BINARY_DIR}/${name} -DTIMEOUT=${LIBS_BUG_ORIG_TIMEOUT}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake
		LABELS emulator bug-orig SKIP_RETURN_CODE 77 TIMEOUT ${limit})
endforeach()

# expect_fixed (sysload.h, selftest_fixed.c) with results from memory, no
# CD needed: a fixed value passes (libs_expect_fixed), a wrong value and a
# stale exemption fail with their message (the snitch cases [.selftest]
# fail on purpose, the tests pass on the report of the failure)
foreach(t pass wrong stale)
	set(name libs_expect_fixed_${t})
	set(filter "expect_fixed ${t}")
	if(t STREQUAL "pass")
		set(name libs_expect_fixed)
		set(filter "expect_fixed passes")
	endif()
	add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
		-DAMIGA_RUN=${AMIGA_RUN} -DPROGRAM=$<TARGET_FILE:libs_tests>
		-DLIBS=${CMAKE_BINARY_DIR}/libs/acidlibs -DNO_ORIGINALS=ON -DFILTER=${filter}
		-DWORK=${CMAKE_CURRENT_BINARY_DIR}/${name} -DTIMEOUT=${LIBS_SELFTEST_TIMEOUT}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake)
	set_tests_properties(${name} PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
		TIMEOUT ${selflimit})
endforeach()
# the test case ran and passed; the failure of the check with its values,
# the other checks unchanged
set_tests_properties(libs_expect_fixed PROPERTIES PASS_REGULAR_EXPRESSION
	"expect_fixed passes: 3 checks, 0 failures\n.*success: all tests passed \\(1 test cases")
set_tests_properties(libs_expect_fixed_wrong PROPERTIES PASS_REGULAR_EXPRESSION
	"FAIL result: expect_fixed #99999: the C library gives \\$00000004, expected \\$00000002: orig \\$00000003, C \\$00000004\n.*result \\(check_fixed\\): expect_fixed #99999: the C library gives \\$00000004.*\nexpect_fixed wrong: 3 checks, 2 failures\n")
set_tests_properties(libs_expect_fixed_stale PROPERTIES PASS_REGULAR_EXPRESSION
	"FAIL result: expect_fixed #99999 stale: the C library gives the original's value \\$00000003 \\(expected \\$00000002\\): orig \\$00000003, C \\$00000003\n.*result \\(check_fixed\\): expect_fixed #99999 stale.*\nexpect_fixed stale: 3 checks, 2 failures\n")

# the test variants (variants/selftest-*, built by ../CMakeLists.txt to
# variants/NAME here): one with the numbers of bb2-2.10 unchanged must give
# the files of the default build (if bb2-2.10 is the default variant); one
# with fadelib (157) and inputoutputlib (164) moved to 3 and 4 must move
# their tokens, and debuglib's call of inputoutputlib (#inputoutputlib<<7+4)
# with them
set(vcheck ${CMAKE_CURRENT_SOURCE_DIR}/variants/check.cmake)
if(BLITZ_DEFAULT_VARIANT STREQUAL "bb2-2.10")
	add_test(NAME variant_selftest_same COMMAND ${CMAKE_COMMAND} -DMODE=same
		-DVARIANT=${CMAKE_CURRENT_BINARY_DIR}/variants/selftest-same -DDEFAULT=${BLITZ2_DIR}
		-P ${vcheck})
	add_test(NAME variant_selftest_renumbered COMMAND ${CMAKE_COMMAND} -DMODE=renumbered
		-DVARIANT=${CMAKE_CURRENT_BINARY_DIR}/variants/selftest-renumbered
		-DBBSRC=${BBSRC} -DBLITZ2=$<TARGET_FILE:blitz2>
		-DTOKENS=${PROJECT_SOURCE_DIR}/tests/data/ted-tokens.txt -DMAP=157:3$<SEMICOLON>164:4
		-DOBJ=${CMAKE_BINARY_DIR}/libs/acidlibs/variants/selftest-renumbered-debuglib.obj
		-DCALL=80000204 -DOLDCALL=80005204
		-DOUT=${CMAKE_CURRENT_BINARY_DIR}/variant_renumbered_tokens.txt -P ${vcheck})
	# rwe-1999-eval (libs/variants): polylib 121 -> 87, the rest as
	# bb2-2.10 (rwe-1999 adds numbers only); no library calls polylib
	add_test(NAME variant_rwe_1999_eval COMMAND ${CMAKE_COMMAND} -DMODE=renumbered
		-DVARIANT=${BLITZ_VARIANTS_DIR}/rwe-1999-eval
		-DBBSRC=${BBSRC} -DBLITZ2=$<TARGET_FILE:blitz2>
		-DTOKENS=${PROJECT_SOURCE_DIR}/tests/data/ted-tokens.txt -DMAP=121:87
		-DOUT=${CMAKE_CURRENT_BINARY_DIR}/variant_rwe_1999_eval_tokens.txt -P ${vcheck})
	# rwe-1999-ahi: rwe-1999's numbers (only risoundlib2 instead of
	# risoundlib), so rwe-1999's acidlibs and deflibs
	add_test(NAME variant_rwe_1999_ahi COMMAND ${CMAKE_COMMAND} -DMODE=same
		-DVARIANT=${BLITZ_VARIANTS_DIR}/rwe-1999-ahi -DDEFAULT=${BLITZ_VARIANTS_DIR}/rwe-1999
		-P ${vcheck})
endif()
