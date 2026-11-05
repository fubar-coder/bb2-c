# libs_tests: the emulator tests of the libraries as one m68k snitch
# program (from bb2 libs-c/tests).  The test logic is C (libtest.c,
# systest*.c, each main renamed to NAME_main); libs_tests.cpp calls it and
# reports every check to snitch.  The programs load every library twice,
# the C build (NAME.obj) and the original of the CD (NAME.orig), "like the
# compiler" (sysload.h), and compare results, registers and memory.
# ctest libs_emulator runs it in Amiberry (emulator.cmake).

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

add_executable(libs_tests libs_tests.cpp callregs.c bugorig.c ${c_sources} ${rwe_tests})
target_include_directories(libs_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
set_source_files_properties(bugorig.c PROPERTIES COMPILE_DEFINITIONS BL_SNITCH)
set_target_properties(libs_tests PROPERTIES SUFFIX "" CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_compile_options(libs_tests PRIVATE -Wall -fomit-frame-pointer
	$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti>)
# each C test program's main becomes NAME_main; checks go to snitch
foreach(t IN LISTS c_tests)
	set_source_files_properties(${t}.c PROPERTIES COMPILE_DEFINITIONS "main=${t}_main;BL_SNITCH")
endforeach()
# -u pulls libnix swapstack.o, which honours __stack at startup
target_link_options(libs_tests PRIVATE -Wl,-u,___stkinit)
target_link_libraries(libs_tests PRIVATE snitch::snitch)

# in Amiberry: libs_tests with every acidlib of this build and its original,
# and the RWE userlibs with theirs (label emulator; skipped without Amiberry
# or the CD)
get_property(rwe_userlibs GLOBAL PROPERTY RWE_EMULATOR_LIBS)
string(REPLACE ";" "$<SEMICOLON>" rwe_userlibs "${rwe_userlibs}")
set(LIBS_TESTS_TIMEOUT 300 CACHE STRING "libs_emulator: time limit of the emulator run (seconds)")
add_test(NAME libs_emulator COMMAND ${CMAKE_COMMAND}
	-DAMIGA_RUN=${AMIGA_RUN} -DBIGLIB=${BIGLIB}
	-DPROGRAM=$<TARGET_FILE:libs_tests>
	-DLIBS=${CMAKE_BINARY_DIR}/libs/acidlibs
	-DORIGINALS=${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1/BLITZ2/ACIDLIBS
	-DUSERLIBS=${rwe_userlibs}
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/emulator -DTIMEOUT=${LIBS_TESTS_TIMEOUT}
	-P ${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake)
math(EXPR limit "600 + ${EMULATION_QUEUE_WAIT}")
set_tests_properties(libs_emulator PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
	TIMEOUT ${limit})
# a USERLIBS original made by a test (the reference build of an RWE
# source, libs/userlibs/rwe/refbuild.cmake): its fixture
get_property(rwe_fixtures GLOBAL PROPERTY RWE_EMULATOR_FIXTURES)
if(rwe_fixtures)
	list(REMOVE_DUPLICATES rwe_fixtures)
	set_tests_properties(libs_emulator PROPERTIES FIXTURES_REQUIRED "${rwe_fixtures}")
endif()

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
		-DBLITZTOKENS=${BLITZTOKENS} -DBLITZ2=$<TARGET_FILE:blitz2>
		-DTOKENS=${PROJECT_SOURCE_DIR}/tests/data/ted-tokens.txt -DMAP=157:3$<SEMICOLON>164:4
		-DOBJ=${CMAKE_BINARY_DIR}/libs/acidlibs/variants/selftest-renumbered-debuglib.obj
		-DCALL=80000204 -DOLDCALL=80005204
		-DOUT=${CMAKE_CURRENT_BINARY_DIR}/variant_renumbered_tokens.txt -P ${vcheck})
	# rwe-1999-eval (libs/variants): polylib 121 -> 87, the rest as
	# bb2-2.10 (rwe-1999 adds numbers only); no library calls polylib
	add_test(NAME variant_rwe_1999_eval COMMAND ${CMAKE_COMMAND} -DMODE=renumbered
		-DVARIANT=${BLITZ_VARIANTS_DIR}/rwe-1999-eval
		-DBLITZTOKENS=${BLITZTOKENS} -DBLITZ2=$<TARGET_FILE:blitz2>
		-DTOKENS=${PROJECT_SOURCE_DIR}/tests/data/ted-tokens.txt -DMAP=121:87
		-DOUT=${CMAKE_CURRENT_BINARY_DIR}/variant_rwe_1999_eval_tokens.txt -P ${vcheck})
	# rwe-1999-ahi: rwe-1999's numbers (only risoundlib2 instead of
	# risoundlib), so rwe-1999's acidlibs and deflibs
	add_test(NAME variant_rwe_1999_ahi COMMAND ${CMAKE_COMMAND} -DMODE=same
		-DVARIANT=${BLITZ_VARIANTS_DIR}/rwe-1999-ahi -DDEFAULT=${BLITZ_VARIANTS_DIR}/rwe-1999
		-P ${vcheck})
endif()
