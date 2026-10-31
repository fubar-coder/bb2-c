# tests of lib/, with inputs built in memory (testutil.h)
foreach(t hunk blitzlib biglib blitztokens hunkcheck amiga_emulation vasmlisting sha256 checklabels checkdata asmcmp mktestdata bb22 blitzres bblines)
	add_executable(test_${t} test_${t}.c)
	target_link_libraries(test_${t} PRIVATE lib_${t} lib_hunk)
	target_compile_options(test_${t} PRIVATE -Wno-unused-function)
	add_test(NAME ${t} COMMAND test_${t})
endforeach()

add_executable(test_blitzlib_compare test_blitzlib_compare.c)
target_link_libraries(test_blitzlib_compare PRIVATE lib_blitzlib lib_hunk)
target_compile_options(test_blitzlib_compare PRIVATE -Wno-unused-function)
add_test(NAME blitzlib_compare COMMAND test_blitzlib_compare)

# with the emulator (label emulator; skipped without Amiberry or the
# emulation data in amiga-emulation/): four runs, in parallel as far as
# emulator slots are free, else one after the other - four sessions and
# the one of the orphan check, each maybe waiting for a slot
add_executable(test_amiga_emulation_run test_amiga_emulation_run.c)
target_link_libraries(test_amiga_emulation_run PRIVATE lib_amiga_emulation lib_hunk)
target_compile_options(test_amiga_emulation_run PRIVATE -Wno-unused-function)
add_test(NAME amiga_emulation_run COMMAND test_amiga_emulation_run)
math(EXPR limit "300 + 5 * ${EMULATION_QUEUE_WAIT}")
set_tests_properties(amiga_emulation_run PROPERTIES
	LABELS emulator SKIP_RETURN_CODE 77 TIMEOUT ${limit})
# the A500 machines (Kickstart 1.3), one run after the other
add_executable(test_amiga_emulation_a500 test_amiga_emulation_a500.c)
target_link_libraries(test_amiga_emulation_a500 PRIVATE lib_amiga_emulation lib_hunk)
target_compile_options(test_amiga_emulation_a500 PRIVATE -Wno-unused-function)
add_test(NAME amiga_emulation_a500 COMMAND test_amiga_emulation_a500)
math(EXPR limit "200 + 2 * ${EMULATION_QUEUE_WAIT}")
set_tests_properties(amiga_emulation_a500 PROPERTIES
	LABELS emulator SKIP_RETURN_CODE 77 TIMEOUT ${limit})

# the .xtra model of shared/xtra (lib_xtra): snitch programs, one per file
foreach(t xtra_load xtra_blitz2 xtra_bb22 xtra_ab3 xtra_ab3_update)
	add_executable(test_${t} test_${t}.cpp)
	target_link_libraries(test_${t} PRIVATE lib_xtra snitch::snitch)
	target_compile_options(test_${t} PRIVATE -Wall -Wextra)
	set_target_properties(test_${t} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
	add_test(NAME ${t} COMMAND test_${t})
endforeach()

# the check of TEST:waitjob (static_assert in amiga_emulation.c) with
# waiters assembled the wrong way
add_test(NAME amiga_emulation_waiter_broken COMMAND ${CMAKE_COMMAND} -DCC=${CMAKE_C_COMPILER}
	-DVASM=${AMIGA_EMULATION_VASM} -DSRC=${CMAKE_CURRENT_SOURCE_DIR}/../lib/amiga_emulation
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/waiter-broken -P ${CMAKE_CURRENT_SOURCE_DIR}/waiter_broken.cmake)
