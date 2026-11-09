# tests of lib/, with inputs built in memory (testutil.h)
foreach(t hunk blitzlib biglib blitztokens hunkcheck amiga_emulation vasmlisting sha256 checklabels checkdata asmcmp mktestdata bb22 blitzres bblines bbdetok statline teststats
		outmatch covmap covsel hdf cmdref cdtable oracletable prevcmp previntended)
	add_executable(test_${t} test_${t}.c)
	target_link_libraries(test_${t} PRIVATE lib_${t} lib_hunk)
	target_compile_options(test_${t} PRIVATE -Wno-unused-function)
	add_test(NAME ${t} COMMAND test_${t})
endforeach()

# the sessions' supervisors (no emulator: the test is its own fake Amiberry)
add_executable(test_amiga_emulation_supervisor test_amiga_emulation_supervisor.c)
target_link_libraries(test_amiga_emulation_supervisor PRIVATE lib_amiga_emulation lib_hunk)
target_compile_options(test_amiga_emulation_supervisor PRIVATE -Wno-unused-function)
add_test(NAME amiga_emulation_supervisor COMMAND test_amiga_emulation_supervisor)

# collecting the coverage files of a session (no emulator)
add_executable(test_amiga_emulation_cov test_amiga_emulation_cov.c)
target_link_libraries(test_amiga_emulation_cov PRIVATE lib_amiga_emulation lib_hunk)
target_include_directories(test_amiga_emulation_cov PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/cov)
target_compile_options(test_amiga_emulation_cov PRIVATE -Wno-unused-function)
add_test(NAME amiga_emulation_cov COMMAND test_amiga_emulation_cov)

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
# a program that halts the emulated CPU: the session ends as halt, long
# before its limit
add_executable(test_amiga_emulation_halt test_amiga_emulation_halt.c)
target_link_libraries(test_amiga_emulation_halt PRIVATE lib_amiga_emulation lib_hunk)
target_compile_options(test_amiga_emulation_halt PRIVATE -Wno-unused-function)
add_test(NAME amiga_emulation_halt COMMAND test_amiga_emulation_halt)
math(EXPR limit "120 + ${EMULATION_QUEUE_WAIT}")
set_tests_properties(amiga_emulation_halt PROPERTIES
	LABELS emulator SKIP_RETURN_CODE 77 TIMEOUT ${limit})
# the A500 machines (Kickstart 1.3), one run after the other
add_executable(test_amiga_emulation_a500 test_amiga_emulation_a500.c)
target_link_libraries(test_amiga_emulation_a500 PRIVATE lib_amiga_emulation lib_hunk)
target_compile_options(test_amiga_emulation_a500 PRIVATE -Wno-unused-function)
add_test(NAME amiga_emulation_a500 COMMAND test_amiga_emulation_a500)
math(EXPR limit "200 + 2 * ${EMULATION_QUEUE_WAIT}")
set_tests_properties(amiga_emulation_a500 PROPERTIES
	LABELS emulator SKIP_RETURN_CODE 77 TIMEOUT ${limit})
# the snapshot mode (save, restore in a new process, a changed hardfile, a
# corrupted state, cold), in a slot directory of its own while it holds
# one of the shared slots
add_executable(test_amiga_emulation_snapshot test_amiga_emulation_snapshot.c)
target_link_libraries(test_amiga_emulation_snapshot PRIVATE lib_amiga_emulation lib_hunk)
target_compile_options(test_amiga_emulation_snapshot PRIVATE -Wno-unused-function)
get_filename_component(emulation_dir ${AMIGA_EMULATION_DIR} ABSOLUTE)
target_compile_definitions(test_amiga_emulation_snapshot PRIVATE
	TEST_EMULATION_DIR="${emulation_dir}")
add_test(NAME amiga_emulation_snapshot COMMAND test_amiga_emulation_snapshot)
math(EXPR limit "360 + ${EMULATION_QUEUE_WAIT}")
set_tests_properties(amiga_emulation_snapshot PROPERTIES
	LABELS emulator SKIP_RETURN_CODE 77 TIMEOUT ${limit})
# two sessions joined in start order: the first hangs until its limit, the
# second (first-save) runs its job meanwhile; it holds two shared slots
add_executable(test_amiga_emulation_join_order test_amiga_emulation_join_order.c)
target_link_libraries(test_amiga_emulation_join_order PRIVATE lib_amiga_emulation lib_hunk)
target_compile_options(test_amiga_emulation_join_order PRIVATE -Wno-unused-function)
add_test(NAME amiga_emulation_join_order COMMAND test_amiga_emulation_join_order)
math(EXPR limit "120 + 2 * ${EMULATION_QUEUE_WAIT}")
set_tests_properties(amiga_emulation_join_order PROPERTIES
	LABELS emulator SKIP_RETURN_CODE 77 TIMEOUT ${limit})

# the .xtra model of shared/xtra (lib_xtra): snitch programs, one per file
foreach(t xtra_load xtra_blitz2 xtra_bb22 xtra_ab3 xtra_ab3_update xtra_ab3_binary
		xtra_mode)
	add_executable(test_${t} test_${t}.cpp)
	target_link_libraries(test_${t} PRIVATE lib_xtra snitch::snitch)
	target_compile_options(test_${t} PRIVATE -Wall -Wextra)
	set_target_properties(test_${t} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
	add_test(NAME ${t} COMMAND test_${t})
endforeach()

# the command line of blitz2-ab3 (shared/ab3job, lib_ab3job)
add_executable(test_ab3args test_ab3args.cpp)
target_link_libraries(test_ab3args PRIVATE lib_ab3job snitch::snitch)
target_compile_options(test_ab3args PRIVATE -Wall -Wextra)
set_target_properties(test_ab3args PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
add_test(NAME ab3args COMMAND test_ab3args)

# the tokeniser of shared/tokenizer (lib_tokenizer)
add_executable(test_tokenizer test_tokenizer.cpp)
target_link_libraries(test_tokenizer PRIVATE lib_tokenizer snitch::snitch)
target_compile_options(test_tokenizer PRIVATE -Wall -Wextra)
set_target_properties(test_tokenizer PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
add_test(NAME tokenizer COMMAND test_tokenizer)

# the check of SNAP:waitjob (static_assert in amiga_emulation.c) with
# waiters assembled the wrong way
add_test(NAME amiga_emulation_waiter_broken COMMAND ${CMAKE_COMMAND} -DCC=${CMAKE_C_COMPILER}
	-DVASM=${AMIGA_EMULATION_VASM} -DSRC=${CMAKE_CURRENT_SOURCE_DIR}/../lib/amiga_emulation
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/waiter-broken -P ${CMAKE_CURRENT_SOURCE_DIR}/waiter_broken.cmake)

# amiga_run --purge-snapshots [OWNER] (by hand): without a slot directory
# it does nothing and succeeds; without OWNER it takes
# $AMIGA_EMULATION_OWNER, unset the snapshots without owner; an empty
# OWNER or a second argument is a usage error.  The purge itself:
# test_amiga_emulation (snapshot_purge).
add_test(NAME amiga_run_purge_no_dir COMMAND amiga_run --purge-snapshots /no/such/build)
set_tests_properties(amiga_run_purge_no_dir PROPERTIES
	ENVIRONMENT XDG_RUNTIME_DIR=${CMAKE_CURRENT_BINARY_DIR}/purge-no-such-runtime-dir
	PASS_REGULAR_EXPRESSION "no snapshot directory, nothing to purge")
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/purge-runtime-dir/amiga-emulation-slots)
add_test(NAME amiga_run_purge_default_owner COMMAND amiga_run --purge-snapshots)
set_tests_properties(amiga_run_purge_default_owner PROPERTIES
	ENVIRONMENT "XDG_RUNTIME_DIR=${CMAKE_CURRENT_BINARY_DIR}/purge-runtime-dir;AMIGA_EMULATION_OWNER=/no/such/build"
	PASS_REGULAR_EXPRESSION "0 snapshots of /no/such/build removed")
add_test(NAME amiga_run_purge_no_owner COMMAND ${CMAKE_COMMAND} -E env --unset=AMIGA_EMULATION_OWNER
	XDG_RUNTIME_DIR=${CMAKE_CURRENT_BINARY_DIR}/purge-runtime-dir $<TARGET_FILE:amiga_run> --purge-snapshots)
set_tests_properties(amiga_run_purge_no_owner PROPERTIES
	PASS_REGULAR_EXPRESSION "0 snapshots of no owner removed")
add_test(NAME amiga_run_purge_empty_owner COMMAND amiga_run --purge-snapshots "")
set_tests_properties(amiga_run_purge_empty_owner PROPERTIES WILL_FAIL ON)
add_test(NAME amiga_run_purge_two_owners COMMAND amiga_run --purge-snapshots /a /b)
set_tests_properties(amiga_run_purge_two_owners PROPERTIES WILL_FAIL ON)

# covsel's tests embed the rules of testinfo select (tools/testinfo/rules.txt)
target_compile_definitions(test_covsel PRIVATE COVSEL_RULES="${CMAKE_CURRENT_SOURCE_DIR}/../testinfo/rules.txt")

# blitzlib cmdref on a file that is no library: an error, exit status 1
add_test(NAME cmdref_not_a_library COMMAND blitzlib cmdref --group acidlibs --name x
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)
set_tests_properties(cmdref_not_a_library PROPERTIES WILL_FAIL TRUE)

# results prev (the library files against the prev build) on two small build
# directories made here: cur differs from prev in libs/acidlibs/2dlib.obj
foreach(b cur prev)
	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/prevcmp/${b}/blitz2/acidlibs "acidlibs")
	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/prevcmp/${b}/libs/acidlibs/gfxlib.obj "gfxlib")
endforeach()
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/prevcmp/cur/libs/acidlibs/2dlib.obj "changed")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/prevcmp/prev/libs/acidlibs/2dlib.obj "2dlib")
set(prevcmp_dirs ${CMAKE_CURRENT_BINARY_DIR}/prevcmp/cur ${CMAKE_CURRENT_BINARY_DIR}/prevcmp/prev)
# a difference that is not intended fails and names the file
add_test(NAME prevcmp_changed COMMAND results prev ${prevcmp_dirs})
set_tests_properties(prevcmp_changed PROPERTIES WILL_FAIL ON)
add_test(NAME prevcmp_changed_names COMMAND results prev ${prevcmp_dirs})
set_tests_properties(prevcmp_changed_names PROPERTIES
	PASS_REGULAR_EXPRESSION "DIFFERS libs/acidlibs/2dlib.obj\n")
# ... passes when it is intended (all, or the file named)
add_test(NAME prevcmp_intended_all COMMAND results prev --all ${prevcmp_dirs})
add_test(NAME prevcmp_intended_file
	COMMAND results prev --intended libs/acidlibs/2dlib.obj ${prevcmp_dirs})
# an intended entry for a file that is the same fails
add_test(NAME prevcmp_stale COMMAND results prev --intended libs/acidlibs/2dlib.obj
	--intended libs/acidlibs/gfxlib.obj ${prevcmp_dirs})
set_tests_properties(prevcmp_stale PROPERTIES WILL_FAIL ON)
add_test(NAME prevcmp_stale_names COMMAND results prev --intended libs/acidlibs/2dlib.obj
	--intended libs/acidlibs/gfxlib.obj ${prevcmp_dirs})
set_tests_properties(prevcmp_stale_names PROPERTIES
	PASS_REGULAR_EXPRESSION "STALE libs/acidlibs/gfxlib.obj")
# no prev build: the message (the exit status 77 skips prev_results)
add_test(NAME prevcmp_no_prev COMMAND results prev ${CMAKE_CURRENT_BINARY_DIR}/prevcmp/cur
	${CMAKE_CURRENT_BINARY_DIR}/prevcmp/no-such-build)
set_tests_properties(prevcmp_no_prev PROPERTIES PASS_REGULAR_EXPRESSION "no-such-build: no prev build")
# an entry of the corpus or ted_ab (PREV_INTENDED, lib/previntended) is
# checked for its syntax and otherwise left to those tests: it does not make
# a library file stale; a malformed one is refused
add_test(NAME prevcmp_corpus_entry COMMAND results prev --intended libs/acidlibs/2dlib.obj
	--intended corpus:t01 --intended ted_ab:open:Cc-Pc ${prevcmp_dirs})
add_test(NAME prevcmp_malformed_entry COMMAND results prev --intended corpus:
	--intended libs/acidlibs/2dlib.obj ${prevcmp_dirs})
set_tests_properties(prevcmp_malformed_entry PROPERTIES WILL_FAIL ON)
add_test(NAME prevcmp_malformed_entry_names COMMAND results prev --intended corpuss:t01 ${prevcmp_dirs})
set_tests_properties(prevcmp_malformed_entry_names PROPERTIES
	PASS_REGULAR_EXPRESSION "PREV_INTENDED entry \"corpuss:t01\": unknown test")

# the command-line shell of the host programs (tools/lib/cli): cli::run
add_executable(test_cli test_cli.cpp)
target_link_libraries(test_cli PRIVATE lib_cli snitch::snitch)
target_compile_options(test_cli PRIVATE -Wall -Wextra)
set_target_properties(test_cli PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
add_test(NAME cli COMMAND test_cli)

# the checks of the help and README of the host programs
# (cmake/check-help.cmake, cmake/check-readme.cmake) on a fake
# program with the subcommands alpha and beta: correct, and broken on
# purpose - each broken case must fail with its message
add_executable(fake_cli11 fake_cli11.cpp)
target_link_libraries(fake_cli11 PRIVATE lib_cli)
target_compile_options(fake_cli11 PRIVATE -Wall -Wextra)
set_target_properties(fake_cli11 PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
add_executable(fake_cli11_broken fake_cli11.cpp)   # beta -h writes to stderr
target_link_libraries(fake_cli11_broken PRIVATE lib_cli)
target_compile_options(fake_cli11_broken PRIVATE -Wall -Wextra)
target_compile_definitions(fake_cli11_broken PRIVATE FAKE_BROKEN)
set_target_properties(fake_cli11_broken PROPERTIES OUTPUT_NAME fake_cli11
	CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fake-broken)
set(fake $<TARGET_FILE:fake_cli11>)
add_test(NAME help_check_ok COMMAND ${CMAKE_COMMAND} -DPROGRAM=${fake}
	-DSUBCOMMANDS=alpha,beta -P ${HOST_CHECK_HELP})
add_test(NAME help_check_stderr COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:fake_cli11_broken>
	-DSUBCOMMANDS=alpha,beta -P ${HOST_CHECK_HELP})
set_tests_properties(help_check_stderr PROPERTIES
	PASS_REGULAR_EXPRESSION "fake_cli11 beta --help: output on stderr")
add_test(NAME help_check_unlisted COMMAND ${CMAKE_COMMAND} -DPROGRAM=${fake}
	-DSUBCOMMANDS=alpha -P ${HOST_CHECK_HELP})
set_tests_properties(help_check_unlisted PROPERTIES
	PASS_REGULAR_EXPRESSION "subcommand beta has no help test")
add_test(NAME help_check_missing COMMAND ${CMAKE_COMMAND} -DPROGRAM=${fake}
	-DSUBCOMMANDS=alpha,beta,gamma -P ${HOST_CHECK_HELP})
set_tests_properties(help_check_missing PROPERTIES
	PASS_REGULAR_EXPRESSION "fake_cli11 gamma --help: exit status 2, not 0")
set(readme ${CMAKE_CURRENT_SOURCE_DIR}/fake-readme)
foreach(c ok missing changed stale)
	add_test(NAME readme_check_${c} COMMAND ${CMAKE_COMMAND} -DPROGRAM=${fake}
		-DREADME=${readme}/${c}.md -DSUBCOMMANDS=alpha,beta -P ${HOST_CHECK_README})
endforeach()
# (CMake wraps long messages: [ \n]+ between the words)
set_tests_properties(readme_check_missing PROPERTIES PASS_REGULAR_EXPRESSION
	"help[ \n]+of[ \n]+`fake_cli11[ \n]+beta`[ \n]+is[ \n]+missing[ \n]+or[ \n]+out[ \n]+of[ \n]+date")
set_tests_properties(readme_check_changed PROPERTIES PASS_REGULAR_EXPRESSION
	"help[ \n]+of[ \n]+`fake_cli11[ \n]+alpha`[ \n]+is[ \n]+missing.*usage[ \n]+block[ \n]+`Usage:[ \n]+fake_cli11[ \n]+alpha[ \n]+\\[OPTIONS\\][ \n]+FILE`[ \n]+is[ \n]+no[ \n]+longer")
set_tests_properties(readme_check_stale PROPERTIES PASS_REGULAR_EXPRESSION
	"usage[ \n]+block[ \n]+`Usage:[ \n]+fake_cli11[ \n]+gamma[ \n]+FILE`[ \n]+is[ \n]+no[ \n]+longer[ \n]+printed")
