# Tests of known defects of the port (label bug-port; README, "Tests"): the
# test checks the correct behaviour and fails until the defect is fixed;
# the fix removes the label (the BUG flag of bug_port_lsclear).  The
# presets host, emulator and all exclude the label, only the preset
# bug-orig (with its -L bug-port) runs them; with no open defect the label
# has no test (ctest: "No tests were found", success).  The label stays
# for future tests.
#
# #963  (fixed) Kickstart 1.x's LoadSeg does not clear the tail of a
#       CODE/DATA hunk beyond the stored block, where the baserel programs
#       ted and ab3cli (no C startup) had their .bss; now
#       cmake/baserel-nostart.ld stores it.  lsclear (amiga/lsclear.c)
#       dirties the memory of the a500 machine (Kickstart 1.3), loads the
#       program with LoadSeg and checks that every byte between the stored
#       block and the hunk size is 0.
#
# The checker itself (no label: it must pass): lsclear -gen writes three
# load files in the emulator, one with an unstored tail, one with the tail
# stored as zeros, one with a HUNK_BSS; on the a500 the first must come out
# FAIL with counted bytes, the others PASS; on the a1200 (Kickstart 3.2,
# whose LoadSeg clears everything) all PASS.

add_executable(lsclear amiga/lsclear.c)
target_compile_options(lsclear PRIVATE -Wall -Wno-pointer-sign -ffreestanding -fno-builtin
	-fomit-frame-pointer)
target_link_options(lsclear PRIVATE -nostartfiles -nostdlib)
set_target_properties(lsclear PROPERTIES SUFFIX "")

set(BUG_PORT_TIMEOUT 120 CACHE STRING "bug-port tests: time limit of an emulator run (seconds)")
math(EXPR bug_port_limit "${BUG_PORT_TIMEOUT} + 60 + ${EMULATION_QUEUE_WAIT}")

function(bug_port_lsclear name)
	cmake_parse_arguments(PARSE_ARGV 1 arg "BUG;GEN;WILL_FAIL" "MACHINE" "FILES;EXPECT")
	add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
		-DAMIGA_RUN=${AMIGA_RUN} -DLSCLEAR=$<TARGET_FILE:lsclear>
		-DMACHINE=${arg_MACHINE} -DWORK=${CMAKE_CURRENT_BINARY_DIR}/${name}
		"-DFILES=${arg_FILES}" "-DEXPECT=${arg_EXPECT}" -DGEN=${arg_GEN}
		-DTIMEOUT=${BUG_PORT_TIMEOUT} -P ${CMAKE_CURRENT_SOURCE_DIR}/run.cmake)
	if(arg_BUG)
		set(labels "bug-port;emulator")
	else()
		set(labels emulator)
	endif()
	set_tests_properties(${name} PROPERTIES LABELS "${labels}" SKIP_RETURN_CODE 77 WILL_FAIL ${arg_WILL_FAIL}
		TIMEOUT ${bug_port_limit})
endfunction()

# #963: the programs on the a500 (Kickstart 1.3)
bug_port_lsclear(bug_port_963_ted MACHINE a500 FILES $<TARGET_FILE:ted>)
bug_port_lsclear(bug_port_963_ab3cli MACHINE a500 FILES $<TARGET_FILE:ab3cli>)

# the checker's self test
bug_port_lsclear(lsclear_selftest_a500 GEN MACHINE a500
	EXPECT "RAM:t_tail=FAIL|RAM:t_zero=PASS|RAM:t_bss=PASS")
bug_port_lsclear(lsclear_selftest_a1200 GEN MACHINE a1200
	EXPECT "RAM:t_tail=PASS|RAM:t_zero=PASS|RAM:t_bss=PASS")
# the defect test on the a1200 (control: LoadSeg of Kickstart 3.2 clears)
bug_port_lsclear(lsclear_ted_a1200 MACHINE a1200 FILES $<TARGET_FILE:ted>)
# deliberately wrong expectations must fail (the verdict comparison and
# the checker's counts are checked): a tail that is counted cannot be PASS
# on the a500, a clean one cannot be FAIL
bug_port_lsclear(lsclear_selftest_wrong_pass_a500 GEN WILL_FAIL MACHINE a500
	EXPECT "RAM:t_tail=PASS")
bug_port_lsclear(lsclear_selftest_wrong_fail_a500 GEN WILL_FAIL MACHINE a500
	EXPECT "RAM:t_zero=FAIL")
bug_port_lsclear(lsclear_selftest_wrong_name_a500 GEN WILL_FAIL MACHINE a500
	EXPECT "RAM:t_none=PASS")
