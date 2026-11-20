# libs_tests: the emulator tests of the libraries as one m68k snitch
# program (from bb2 libs-c/tests).  The test logic is C (libtest.c,
# systest*.c, each main renamed to NAME_main); libs_tests.cpp calls it and
# reports every check to snitch.  The programs load one library set "like
# the compiler" (sysload.h), the C build (NAME.obj) or the original of the
# CD (NAME.orig) as the shell variable LIBSET says, and write their
# observations of results, registers and memory as cases (capture.c),
# which are compared with the expected data expected/NAME.txt.

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
	systest_blit systest_gfx
	systest_med systest_tracker)
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

add_executable(libs_tests libs_tests.cpp callregs.c bugorig.c selftest_keeps.c selftest_mem.c ${c_sources}
	${rwe_tests})
target_include_directories(libs_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
set_source_files_properties(bugorig.c selftest_keeps.c selftest_mem.c PROPERTIES COMPILE_DEFINITIONS BL_SNITCH)
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
# userlibs (label emulator; skipped without Amiberry).  libs_emulator runs
# the C build (LIBSET=obj, no CD needed), libs_emulator_originals (label
# originals) the originals of the CD (LIBSET=orig); the cases of each run
# (the test's test/captures/NAME.txt, kept for diffing) are compared with
# the expected data (expected/NAME.txt) by results libs-compare.
get_property(rwe_userlibs GLOBAL PROPERTY RWE_EMULATOR_LIBS)
string(REPLACE ";" "$<SEMICOLON>" rwe_userlibs "${rwe_userlibs}")
# (the two block programs, systest_med and rwe_ridisasmlib, take about
# 26 and 30 s of it on an idle machine: their 6.6 million observations
# are written raw and hashed on the host, capture.h)
set(LIBS_TESTS_TIMEOUT 900 CACHE STRING "libs_emulator: time limit of the emulator run (seconds)")
set(expected ${CMAKE_CURRENT_SOURCE_DIR}/expected)
# the machines the libs tests run on besides the default a1200
# (emulator.cmake MACHINE), each with the changes of its configuration
# libs_tests needs there (UAE; tests/README.md, "Another machine") and its
# own expected data expected-MACHINE/: the tests of a machine and the
# targets libs-capture and libs-update (LIBS_MACHINE) take both from here
set(libs_machines a500-204)
set(libs_uae_a500-204 "cpu_type=68020;cpu_model=68020;cpu_24bit_addressing=false;z3mem_size=32")
# the arguments of emulator.cmake or expected.cmake for MACHINE into OUT
function(libs_machine_args machine out)
	set(a -DMACHINE=${machine})
	if(DEFINED libs_uae_${machine})
		string(REPLACE ";" "$<SEMICOLON>" u "${libs_uae_${machine}}")
		list(APPEND a "-DUAE=${u}")
	endif()
	set(${out} "${a}" PARENT_SCOPE)
endfunction()
math(EXPR limit "${LIBS_TESTS_TIMEOUT} + 300 + ${EMULATION_QUEUE_WAIT}")
foreach(libset obj orig)
	set(name libs_emulator)
	set(labels emulator)
	set(originals -DNO_ORIGINALS=ON)
	if(libset STREQUAL "orig")
		set(name libs_emulator_originals)
		set(labels emulator originals)
		set(originals -DBLITZLIB=${BLITZLIB}
			-DORIGINALS=${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1/BLITZ2/ACIDLIBS)
	endif()
	add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
		-DAMIGA_RUN=${AMIGA_RUN} ${originals}
		-DPROGRAM=$<TARGET_FILE:libs_tests>
		-DLIBS=${CMAKE_BINARY_DIR}/libs/acidlibs
		-DUSERLIBS=${rwe_userlibs}
		-DLIBSET=${libset} -DEXPECTED=${expected} -DRESULTS=${RESULTS}
		-DWORK=${CMAKE_CURRENT_BINARY_DIR}/${name} -DTIMEOUT=${LIBS_TESTS_TIMEOUT}
		-DTEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake -P ${WITH_WORKDIR})
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
# unqualified case such a pair, or KEY [orig #N] (a case of the original
# only) where the port lacks KEY.  LIBS_CAPTURE_FILTER (a snitch filter of
# libs_tests, e.g. "libtest ldivlib") limits both to the programs it runs.
# LIBS_MACHINE (one of libs_machines) runs both on that machine with its
# UAE changes and writes its expected data expected-MACHINE/;
# LIBS_UPDATE_NEW (an issue N) makes libs-update write the data of a
# program of the filter that has none, every case of the port's run
# KEY [fixed #N] (a machine on which the original does not run).
set(LIBS_CAPTURE_FILTER "" CACHE STRING
	"libs-capture, libs-update: only the libs_tests test cases of this snitch filter (empty: all)")
set(LIBS_UPDATE_FIX "" CACHE STRING
	"libs-update: KEY#N;... - the unqualified case KEY becomes the pair KEY [orig] / KEY [fixed #N] (KEY [orig #N] if the port lacks KEY)")
list(JOIN libs_machines ", " m)
set(LIBS_MACHINE "" CACHE STRING
	"libs-capture, libs-update: the machine of the runs (${m}) and its expected data libs/tests/expected-MACHINE (empty: the a1200, libs/tests/expected)")
set(LIBS_UPDATE_NEW "" CACHE STRING
	"libs-update: N - a program of LIBS_CAPTURE_FILTER without expected data gets every case of the port's run as KEY [fixed #N]")
set(target_expected ${expected})
set(machine_args "")
if(NOT LIBS_MACHINE STREQUAL "")
	set(target_expected ${expected}-${LIBS_MACHINE})
	libs_machine_args("${LIBS_MACHINE}" machine_args)
endif()
string(REPLACE ";" "$<SEMICOLON>" machines "${libs_machines}")
foreach(mode capture update)
	set(fix "")
	set(new "")
	set(what "the original (two runs) -> ${target_expected}")
	if(mode STREQUAL "update")
		string(REPLACE ";" "$<SEMICOLON>" fix "${LIBS_UPDATE_FIX}")
		set(new "${LIBS_UPDATE_NEW}")
		set(what "the port -> the [fixed #N] cases of ${target_expected}")
		if(new)
			string(APPEND what ", new programs [fixed #${new}]")
		endif()
	endif()
	if(LIBS_MACHINE)
		string(APPEND what " (machine ${LIBS_MACHINE})")
	endif()
	if(LIBS_CAPTURE_FILTER)
		string(APPEND what " (filter ${LIBS_CAPTURE_FILTER})")
	endif()
	# the environment variables of the cache variables have no effect: refused
	add_custom_target(libs-${mode}
		COMMAND ${CMAKE_COMMAND} -DNAME=LIBS_CAPTURE_FILTER "-DVALUE=${LIBS_CAPTURE_FILTER}"
			-P ${PROJECT_SOURCE_DIR}/cmake/check-cache-variable.cmake
		COMMAND ${CMAKE_COMMAND} -DNAME=LIBS_UPDATE_FIX "-DVALUE=${LIBS_UPDATE_FIX}"
			-P ${PROJECT_SOURCE_DIR}/cmake/check-cache-variable.cmake
		COMMAND ${CMAKE_COMMAND} -DNAME=LIBS_MACHINE "-DVALUE=${LIBS_MACHINE}"
			-P ${PROJECT_SOURCE_DIR}/cmake/check-cache-variable.cmake
		COMMAND ${CMAKE_COMMAND} -DNAME=LIBS_UPDATE_NEW "-DVALUE=${LIBS_UPDATE_NEW}"
			-P ${PROJECT_SOURCE_DIR}/cmake/check-cache-variable.cmake
		COMMAND ${CMAKE_COMMAND} -DMODE=${mode} -DEMULATOR=${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake
			-DRESULTS=${RESULTS} -DEXPECTED=${target_expected} -DWORK=${CMAKE_CURRENT_BINARY_DIR}/libs-${mode}
			${machine_args} "-DMACHINES=${machines}" "-DNEW=${new}"
			-DAMIGA_RUN=${AMIGA_RUN} -DBLITZLIB=${BLITZLIB} -DPROGRAM=$<TARGET_FILE:libs_tests>
			-DLIBS=${CMAKE_BINARY_DIR}/libs/acidlibs
			-DORIGINALS=${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1/BLITZ2/ACIDLIBS
			-DUSERLIBS=${rwe_userlibs} -DTIMEOUT=${LIBS_TESTS_TIMEOUT}
			"-DFILTER=${LIBS_CAPTURE_FILTER}" "-DFIX=${fix}"
			-P ${CMAKE_CURRENT_SOURCE_DIR}/expected.cmake
		USES_TERMINAL
		COMMENT "The library tests' cases from ${what}"
		VERBATIM)
	add_dependencies(libs-${mode} libs_tests acidlibs host_tools)
endforeach()
# the refusal of an environment variable of LIBS_CAPTURE_FILTER,
# LIBS_UPDATE_FIX, LIBS_MACHINE or LIBS_UPDATE_NEW with the cache variable
# empty (cmake/check-cache-variable.cmake)
foreach(v LIBS_CAPTURE_FILTER LIBS_UPDATE_FIX LIBS_MACHINE LIBS_UPDATE_NEW)
	add_test(NAME cache_variable_env_${v} COMMAND ${CMAKE_COMMAND} -DNAME=${v}
		-DSCRIPT=${PROJECT_SOURCE_DIR}/cmake/check-cache-variable.cmake
		-P ${PROJECT_SOURCE_DIR}/cmake/check-cache-variable-test.cmake)
	set_tests_properties(cache_variable_env_${v} PROPERTIES
		PASS_REGULAR_EXPRESSION "check-cache-variable-test: all checks as expected")
endforeach()
# the orchestration of both with a fake run (no emulator): a deterministic
# record is written, a non-deterministic one refused with the expected
# data unchanged, an update makes the pair, a skipped run fails, a filter
# leaves the other files alone
add_test(NAME libs_capture_target COMMAND ${CMAKE_COMMAND} -DRESULTS=${RESULTS}
	-DSCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/expected.cmake
	-DFAKE=${CMAKE_CURRENT_SOURCE_DIR}/selftest/fake-emulator.cmake
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/libs_capture_target
	-DTEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/selftest/expected-target.cmake -P ${WITH_WORKDIR})
set_tests_properties(libs_capture_target PROPERTIES
	PASS_REGULAR_EXPRESSION "expected-target: all runs as expected")
set(LIBS_SELFTEST_TIMEOUT 60 CACHE STRING
	"libs_emulator_wrong, libs_emulator_lost, libs_keeps_wrong: time limit of the emulator run (seconds)")
math(EXPR selflimit "${LIBS_SELFTEST_TIMEOUT} + 60 + ${EMULATION_QUEUE_WAIT}")
# the check of the cases with broken input: the run's first case of
# libtest_ldivlib changed (emulator.cmake CAPTURE_CORRUPT) fails the compare
# with the expected data, naming the key.  The qualified cases of a fix
# (KEY [orig] / KEY [fixed #N]: a regressed port fails on the [fixed #N]
# case, a changed original on the [orig] case) are the judge's, tested on
# the host (tools/tests: test_libscmp.cpp, libs_compare_wrong_fixed and
# libs_compare_wrong_orig)
add_test(NAME libs_emulator_wrong COMMAND ${CMAKE_COMMAND}
	-DAMIGA_RUN=${AMIGA_RUN} -DPROGRAM=$<TARGET_FILE:libs_tests>
	-DLIBS=${CMAKE_BINARY_DIR}/libs/acidlibs -DNO_ORIGINALS=ON
	-DLIBSET=obj -DEXPECTED=${expected} -DRESULTS=${RESULTS} -DCAPTURE_CORRUPT=libtest_ldivlib
	"-DFILTER=libtest ldivlib"
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/libs_emulator_wrong -DTIMEOUT=${LIBS_SELFTEST_TIMEOUT}
	-DTEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake -DWANT_FAIL=ON -P ${WITH_WORKDIR})
set_tests_properties(libs_emulator_wrong PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
	TIMEOUT ${selflimit} PASS_REGULAR_EXPRESSION
	"libtest_ldivlib: \\|routine count\\|\\|1: expected \\$00000002, got \\$00000000\n")
# another machine (emulator.cmake MACHINE, UAE): libtest ldivlib on the
# a500-204 (Kickstart 2.04) with a 68020 and Zorro III fast memory (libs_tests
# needs more memory than a 68000 machine has) against the expected data, the
# way a library's Kickstart 2.x path is tested; and with broken input a
# machine the emulation does not know (a3000), which amiga_run refuses
# before any session, and a configuration change without KEY=
libs_machine_args(a500-204 a500_204)
add_test(NAME libs_emulator_machine COMMAND ${CMAKE_COMMAND}
	-DAMIGA_RUN=${AMIGA_RUN} -DPROGRAM=$<TARGET_FILE:libs_tests>
	-DLIBS=${CMAKE_BINARY_DIR}/libs/acidlibs -DNO_ORIGINALS=ON
	-DLIBSET=obj -DEXPECTED=${expected} -DRESULTS=${RESULTS} "-DFILTER=libtest ldivlib"
	${a500_204}
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/libs_emulator_machine -DTIMEOUT=${LIBS_SELFTEST_TIMEOUT}
	-DTEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake -P ${WITH_WORKDIR})
set_tests_properties(libs_emulator_machine PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
	TIMEOUT ${selflimit} PASS_REGULAR_EXPRESSION
	"libtest_ldivlib: set obj: [0-9]+ cases, [0-9]+ equal, 0 differ, 0 missing")
# riwbspritelib's path below graphics.library V39 (#1151, GetSprite and a
# SimpleSprite): the library test on the a500-204 (Kickstart 2.04, a 68020
# with Zorro III fast memory as above), the C build only (the original
# calls the V39 functions there and crashes), against its own expected
# data expected-a500-204/ (every case [fixed #1151])
add_test(NAME libs_emulator_riwbspritelib_a500_204 COMMAND ${CMAKE_COMMAND}
	-DAMIGA_RUN=${AMIGA_RUN} -DPROGRAM=$<TARGET_FILE:libs_tests>
	-DLIBS=${CMAKE_BINARY_DIR}/libs/acidlibs -DUSERLIBS=${rwe_userlibs} -DNO_ORIGINALS=ON
	-DLIBSET=obj -DEXPECTED=${expected}-a500-204 -DRESULTS=${RESULTS}
	"-DFILTER=rwe riwbspritelib" ${a500_204}
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/libs_emulator_riwbspritelib_a500_204 -DTIMEOUT=${LIBS_SELFTEST_TIMEOUT}
	-DTEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake -P ${WITH_WORKDIR})
set_tests_properties(libs_emulator_riwbspritelib_a500_204 PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
	TIMEOUT ${selflimit} PASS_REGULAR_EXPRESSION
	"rwe_riwbspritelib: set obj: [0-9]+ cases, [0-9]+ equal, 0 differ, 0 missing")
foreach(t unknown uae)
	set(opts -DMACHINE=a3000)
	set(re "amiga_run: unknown machine \"a3000\"(.*\n)*.*libs_emulator: amiga_run refused the run")
	if(t STREQUAL "uae")
		set(opts -DMACHINE=a500-204 -DUAE=z3mem_size)
		set(re "emulator.cmake: UAE: z3mem_size: KEY=VALUE")
	endif()
	add_test(NAME libs_emulator_machine_${t} COMMAND ${CMAKE_COMMAND}
		-DAMIGA_RUN=${AMIGA_RUN} -DPROGRAM=$<TARGET_FILE:libs_tests>
		-DLIBS=${CMAKE_BINARY_DIR}/libs/acidlibs -DNO_ORIGINALS=ON ${opts}
		-DWORK=${CMAKE_CURRENT_BINARY_DIR}/libs_emulator_machine_${t}
		-DTEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake -DWANT_FAIL=ON -P ${WITH_WORKDIR})
	set_tests_properties(libs_emulator_machine_${t} PROPERTIES PASS_REGULAR_EXPRESSION "${re}")
endforeach()
# the dump of one case (emulator.cmake CAP_DUMP, the shell variable
# LIBS_CAP_DUMP of capture.h): a bytes case of rwe riarrayslib whose key
# holds |, $ and spaces writes its bytes to the dump, shown by the run; a
# key no program writes fails the run; block 2 of libtest ldivlib is
# dumped on the host from its NAME.bin (results libs-dump), its first
# observation the one its block case names (first=)
set(dump_key "FillArray|array|form 0 type $0001 size 1 off 5 n 1 value $DE551B45|1")
string(REGEX REPLACE "([][|$().*+?^\\])" "\\\\\\1" dump_re "${dump_key}")
foreach(t right wrong block)
	set(key "${dump_key}")
	set(filter "rwe riarrayslib")
	set(want_fail "")             # the run that must fail (wrong) is no failure
	set(re "the dump of [^\n]*rwe_riarrayslib\\.dump \\(the first 7 lines\\)\n${dump_re}\tat \\$0000 [0-9A-F]+\n(.*\n)*${dump_re}\tat \\$00C0 [0-9A-F]+\n")
	if(t STREQUAL "wrong")
		set(key "FillArray|array|no such vector|1")
		set(want_fail -DWANT_FAIL=ON)
		set(re "FAIL LIBS_CAP_DUMP FillArray\\|array\\|no such vector\\|1: no program wrote a dump")
	elseif(t STREQUAL "block")
		set(key "BLOCK:2")
		set(filter "libtest ldivlib")
		set(re "the dump of BLOCK:2: [^\n]*libtest_ldivlib\\.dump \\(the first 40 lines\\)\n\\$CB00\\|d0\\|\\$FFFFF8C3 \\$D89B8344\\|4097\t\\$[0-9A-F]+\n")
	endif()
	add_test(NAME libs_emulator_dump_${t} COMMAND ${CMAKE_COMMAND}
		-DAMIGA_RUN=${AMIGA_RUN} -DPROGRAM=$<TARGET_FILE:libs_tests>
		-DLIBS=${CMAKE_BINARY_DIR}/libs/acidlibs -DNO_ORIGINALS=ON -DUSERLIBS=${rwe_userlibs}
		-DLIBSET=obj "-DFILTER=${filter}" "-DCAP_DUMP=${key}" -DRESULTS=${RESULTS}
		-DWORK=${CMAKE_CURRENT_BINARY_DIR}/libs_emulator_dump_${t} -DTIMEOUT=${LIBS_SELFTEST_TIMEOUT}
		-DTEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake ${want_fail} -P ${WITH_WORKDIR})
	set_tests_properties(libs_emulator_dump_${t} PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
		TIMEOUT ${selflimit} PASS_REGULAR_EXPRESSION "${re}")
endforeach()
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
	-DTEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake -DWANT_FAIL=ON -P ${WITH_WORKDIR})
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
		crunchlib-pp-readd1 debuglib-dout2-length debuglib-findoff-flags sortlib-word-pivot)
	string(REPLACE "-" "_" name "bug_orig_${t}")
	set(watch OFF)
	if(t STREQUAL "crunch-implode-overread" OR t STREQUAL "sortlib-word-pivot")
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
		-DTEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake -P ${WITH_WORKDIR}
		LABELS emulator bug-orig SKIP_RETURN_CODE 77 TIMEOUT ${limit})
endforeach()

# the checks of KEEPS (sysload.h, selftest_keeps.c) with broken input, no
# library and no CD: a set that is no register set, a routine stated
# twice, a call of a routine without a stated set, a function that loses
# a6, a word function that changes the upper word of d0 it states (d0u);
# each fails with its message, and a statement that changes d1 and a0
# passes (the register rule; the snitch case [.selftest] fails on purpose)
add_test(NAME libs_keeps_wrong COMMAND ${CMAKE_COMMAND}
	-DAMIGA_RUN=${AMIGA_RUN} -DPROGRAM=$<TARGET_FILE:libs_tests>
	-DLIBS=${CMAKE_BINARY_DIR}/libs/acidlibs -DNO_ORIGINALS=ON "-DFILTER=keeps wrong"
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/libs_keeps_wrong -DTIMEOUT=${LIBS_SELFTEST_TIMEOUT}
	-DTEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake -DWANT_FAIL=ON -P ${WITH_WORKDIR})
set_tests_properties(libs_keeps_wrong PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
	TIMEOUT ${selflimit} PASS_REGULAR_EXPRESSION
	"FAIL KEEPS\\(\"bad\", d2\\|x9\\): not a register set\n.*FAIL KEEPS\\(\"twice\", d3\\): stated twice\n.*FAIL KEEPS: no kept set stated for \"unstated\"\n.*FAIL function: lost a6\n.*FAIL word function: lost d0u\n.*keeps wrong: 9 checks, 5 failures\n")

# the check "all memory back" of the library tests (rwemem.h rwe_mem_back(),
# selftest_mem.c) with broken input, no library and no CD: a block not
# freed, a free of a foreign block, a window without an allocation; each
# fails, the window that frees what it allocates passes
add_test(NAME libs_mem_wrong COMMAND ${CMAKE_COMMAND}
	-DAMIGA_RUN=${AMIGA_RUN} -DPROGRAM=$<TARGET_FILE:libs_tests>
	-DLIBS=${CMAKE_BINARY_DIR}/libs/acidlibs -DNO_ORIGINALS=ON "-DFILTER=mem wrong"
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/libs_mem_wrong -DTIMEOUT=${LIBS_SELFTEST_TIMEOUT}
	-DTEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake -DWANT_FAIL=ON -P ${WITH_WORKDIR})
set_tests_properties(libs_mem_wrong PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
	TIMEOUT ${selflimit} PASS_REGULAR_EXPRESSION
	"FAIL leak: all memory back\n.*FAIL free of a foreign block: all memory back\n.*FAIL no allocation: all memory back\n.*FAIL mark: block not freed: freed\n.*FAIL mark: no allocation: old one freed\n.*FAIL mark: alloc and free: nothing allocated\n.*FAIL mark: no measurement: freed\n.*mem wrong: 11 checks, 7 failures\n")

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
