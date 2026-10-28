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

set(c_tests libtest systest systest_mathstr systest_runtime systest_system systest_cmdlibs)
list(TRANSFORM c_tests APPEND .c OUTPUT_VARIABLE c_sources)
add_executable(libs_tests libs_tests.cpp callregs.c bugorig.c ${c_sources})
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

# in Amiberry: libs_tests with every acidlib of this build and its original
# (label emulator; skipped without Amiberry or the CD)
set(LIBS_TESTS_TIMEOUT 120 CACHE STRING "libs_emulator: time limit of the emulator run (seconds)")
add_test(NAME libs_emulator COMMAND ${CMAKE_COMMAND}
	-DAMIGA_RUN=${AMIGA_RUN} -DBIGLIB=${BIGLIB}
	-DPROGRAM=$<TARGET_FILE:libs_tests>
	-DLIBS=${CMAKE_BINARY_DIR}/libs/acidlibs
	-DORIGINALS=${PROJECT_SOURCE_DIR}/archive/cd/ultimate-blitz-basic-2.1/BLITZ2/ACIDLIBS
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/emulator -DTIMEOUT=${LIBS_TESTS_TIMEOUT}
	-P ${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake)
set_tests_properties(libs_emulator PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
	TIMEOUT 600)

# reproduced original bugs (bugorig.c, hidden snitch tests [.bug-orig]):
# each checks the correct behaviour and FAILS as long as the C build
# reproduces the original; label bug-orig, only the test preset bug-orig
# runs them.  One emulator run each (a corrupt decrunch may crash it),
# only the C build (no CD needed).  ILLEGAL_WATCH: Amiberry logs accesses
# to unmapped memory, emulator.cmake checks the ranges the program names.
foreach(t cliargs-parpath-cli crunch-implode-overread crunch-decrunch-corrupt
		crunchlib-crm-minsecdist crunchlib-crm-overlap-4gb crunchlib-crm2-readtab
		crunchlib-pp-readd1 debuglib-findoff-flags)
	string(REPLACE "-" "_" name "bug_orig_${t}")
	set(watch OFF)
	if(t STREQUAL "crunch-implode-overread")
		set(watch ON)
	endif()
	add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
		-DAMIGA_RUN=${AMIGA_RUN} -DPROGRAM=$<TARGET_FILE:libs_tests>
		-DLIBS=${CMAKE_BINARY_DIR}/libs/acidlibs -DNO_ORIGINALS=ON
		-DILLEGAL_WATCH=${watch} -DFILTER=${t}
		-DWORK=${CMAKE_CURRENT_BINARY_DIR}/${name} -DTIMEOUT=${LIBS_TESTS_TIMEOUT}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/emulator.cmake)
	set_tests_properties(${name} PROPERTIES LABELS "emulator;bug-orig" SKIP_RETURN_CODE 77
		TIMEOUT 300)
endforeach()
