# corpusrun: the corpus passes (pass) and the CD runs (cd) in Amiberry, host
# side (README.md)
# strings and files, and the run directories (runwork.h), also for the TED
# A/B runner (tests/ted-ab/runner)
add_library(lib_runner_util STATIC util.c runwork.c rundir.c)
target_include_directories(lib_runner_util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(lib_runner_util PRIVATE -Wall -Wextra)
own_warnings(lib_runner_util)
set_target_properties(lib_runner_util PROPERTIES C_STANDARD 23)

add_library(lib_corpus STATIC corpus.c serial.c bugorig.c)
target_include_directories(lib_corpus PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Threads REQUIRED)
target_link_libraries(lib_corpus PUBLIC lib_runner_util lib_amiga_emulation lib_statline lib_previntended lib_prevcmp lib_corpusexpect
	lib_bugorigcmp Threads::Threads)
target_compile_options(lib_corpus PRIVATE -Wall -Wextra)
own_warnings(lib_corpus)
set_target_properties(lib_corpus PROPERTIES C_STANDARD 23)
# the verdict store of the bundles (verdicts.h): the entries, the key of a
# stored verdict's inputs
add_library(lib_verdicts STATIC verdicts.c)
target_include_directories(lib_verdicts PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_verdicts PUBLIC lib_runner_util lib_testconf lib_sha256)
target_compile_options(lib_verdicts PRIVATE -Wall -Wextra)
own_warnings(lib_verdicts)
set_target_properties(lib_verdicts PROPERTIES C_STANDARD 23)
# its logic without the emulator: put and claim, two claimants, a foreign and
# a malformed entry (broken input), the cleanup, the key's inputs
add_executable(test_verdicts test_verdicts.c)
target_link_libraries(test_verdicts PRIVATE lib_verdicts)
target_compile_options(test_verdicts PRIVATE -Wall -Wextra)
own_warnings(test_verdicts)
add_test(NAME corpusrun_verdicts COMMAND test_verdicts ${CMAKE_CURRENT_BINARY_DIR}/corpusrun_verdicts)
# one case of a Blitz test of the BASIC test model (caserun.h): the cells of
# a side in shared sessions, judged by lib/testjudge; the bundles of cases of
# several sources and the served case (verdicts.h)
add_library(lib_caserun STATIC caserun.c)
target_include_directories(lib_caserun PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# the warnings' names (shared/ab3job/warnlist.h, macros only)
target_include_directories(lib_caserun PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared/ab3job)
target_link_libraries(lib_caserun PUBLIC lib_corpus lib_testjudge lib_verdicts)
target_compile_options(lib_caserun PRIVATE -Wall -Wextra)
own_warnings(lib_caserun)
set_target_properties(lib_caserun PROPERTIES C_STANDARD 23)
# its logic without the emulator: the pass of a cell, the sessions, the
# outputs, the compile result, the printed configuration (broken input too)
add_executable(test_caserun test_caserun.c)
target_link_libraries(test_caserun PRIVATE lib_caserun)
target_compile_options(test_caserun PRIVATE -Wall -Wextra)
own_warnings(test_caserun)
add_test(NAME corpusrun_caserun COMMAND test_caserun ${CMAKE_CURRENT_BINARY_DIR}/corpusrun_caserun)
set(CORPUSRUN_SUBCOMMANDS pass cd drop case)
add_executable(corpusrun main.cpp cmd_pass.cpp cmd_cd.cpp cmd_drop.cpp cmd_case.cpp cd.c)
target_link_libraries(corpusrun PRIVATE lib_cli lib_corpus lib_caserun lib_cdcache lib_cdcmp lib_sha256)
target_compile_options(corpusrun PRIVATE -Wall -Wextra)
own_warnings(corpusrun)
set_target_properties(corpusrun PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)

add_executable(test_corpus test_corpus.c)
target_link_libraries(test_corpus PRIVATE lib_corpus)
target_compile_options(test_corpus PRIVATE -Wall -Wextra)
own_warnings(test_corpus)
add_test(NAME corpus_runner COMMAND test_corpus)
# the verdict of the bug-orig tests (tests/bug-orig), with broken inputs
add_executable(test_bugorig test_bugorig.c)
target_link_libraries(test_bugorig PRIVATE lib_corpus)
target_compile_options(test_bugorig PRIVATE -Wall -Wextra)
own_warnings(test_bugorig)
add_test(NAME bugorig_verdict COMMAND test_bugorig)

# corpusrun cd: every source of the CD compiled by one compiler (reference table /
# comparison with it)
# the cache of a pass (cache=)
add_library(lib_cdcache STATIC cdcache.c)
target_include_directories(lib_cdcache PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_cdcache PUBLIC lib_runner_util lib_sha256)
target_compile_options(lib_cdcache PRIVATE -Wall -Wextra)
own_warnings(lib_cdcache)
set_target_properties(lib_cdcache PROPERTIES C_STANDARD 23)
add_executable(test_cdcache test_cdcache.c)
target_link_libraries(test_cdcache PRIVATE lib_cdcache)
target_compile_options(test_cdcache PRIVATE -Wall -Wextra)
own_warnings(test_cdcache)
add_test(NAME cdrun_cache COMMAND test_cdcache)
# the comparison of a CD table with the reference table (cdcmp.h): compile
# results compared, executables only reported
add_library(lib_cdcmp STATIC cdcmp.c)
target_include_directories(lib_cdcmp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_cdcmp PUBLIC lib_previntended)
target_compile_options(lib_cdcmp PRIVATE -Wall -Wextra)
own_warnings(lib_cdcmp)
set_target_properties(lib_cdcmp PROPERTIES C_STANDARD 23)
add_executable(test_cdcmp test_cdcmp.c)
target_link_libraries(test_cdcmp PRIVATE lib_cdcmp)
target_compile_options(test_cdcmp PRIVATE -Wall -Wextra)
own_warnings(test_cdcmp)
add_test(NAME cdrun_compare COMMAND test_cdcmp)
# its source= lines with broken input (no emulator needed)
add_test(NAME cdrun_source COMMAND ${CMAKE_COMMAND} -DCORPUSRUN=$<TARGET_FILE:corpusrun>
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/cdrun_source
	-P ${CMAKE_CURRENT_SOURCE_DIR}/check-cdrun-source.cmake)

# the command lines of the bundles refused before any work (broken input,
# exit status 2, no emulator needed): a member named twice, a bundle name
# with "/", --member without --bundle, --from-bundle without --test
set(bundle_bad_twice --bundle b --member t /s.test.bas c --member t /s.test.bas c
	"the member t is named twice")
set(bundle_bad_name --bundle a/b --member t /s.test.bas c "\"a/b\" is no name of a bundle")
set(bundle_bad_member --member t /s.test.bas c "--member requires --bundle")
set(bundle_bad_served --source /s.test.bas --case c --from-bundle b "--from-bundle requires --test")
foreach(v twice name member served)
	set(args ${bundle_bad_${v}})
	list(POP_BACK args regex)
	add_test(NAME corpusrun_bundle_refused_${v} COMMAND ${CMAKE_COMMAND}
		"-DCMD=$<TARGET_FILE:corpusrun>;case;--env;/no/env.toml;${args}" -DWANT=2 "-DREGEX=${regex}"
		-DWORK=${CMAKE_CURRENT_BINARY_DIR}/corpusrun_bundle_refused_${v}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/runner-exit.cmake)
	set_tests_properties(corpusrun_bundle_refused_${v} PROPERTIES PASS_REGULAR_EXPRESSION "runner-exit: as expected")
endforeach()

host_help_test(corpusrun ${CORPUSRUN_SUBCOMMANDS})
host_readme_test(corpusrun README.md ${CORPUSRUN_SUBCOMMANDS})
# the run directories of corpusrun pass and tedab run (runwork.h): runs of
# their own, the leftovers, what a passed run leaves, the free-space check
add_executable(test_runwork test_runwork.c)
target_link_libraries(test_runwork PRIVATE lib_runner_util)
target_compile_options(test_runwork PRIVATE -Wall -Wextra)
own_warnings(test_runwork)
add_test(NAME runner_runwork COMMAND test_runwork ${CMAKE_CURRENT_BINARY_DIR}/runner_runwork)
# the run directory of the fixtures (rundir.h): RUN, an input in it, the
# cleanup, the copy across file systems
add_executable(test_rundir test_rundir.c)
target_link_libraries(test_rundir PRIVATE lib_runner_util)
target_compile_options(test_rundir PRIVATE -Wall -Wextra)
own_warnings(test_rundir)
add_test(NAME runner_rundir COMMAND test_rundir ${CMAKE_CURRENT_BINARY_DIR}/runner_rundir)
# the fixtures and the run directory with real (nested) ctest runs on a small
# project, no emulator (ctest-run-dir.cmake)
add_test(NAME runner_ctest_run_dir COMMAND ${CMAKE_COMMAND}
	-DCORPUSRUN=$<TARGET_FILE:corpusrun> -DCTEST=${CMAKE_CTEST_COMMAND}
	-DSRC=${CMAKE_CURRENT_SOURCE_DIR}/ctest-run-dir -DWORK=${CMAKE_CURRENT_BINARY_DIR}/ctest_run_dir
	-P ${CMAKE_CURRENT_SOURCE_DIR}/ctest-run-dir.cmake)
set_tests_properties(runner_ctest_run_dir PROPERTIES PASS_REGULAR_EXPRESSION "ctest-run-dir: as expected")
# corpusrun pass --observe on a bug-orig pass (orig=): refused, no report
# (no emulator needed)
add_test(NAME corpusrun_observe_orig COMMAND ${CMAKE_COMMAND} -DCORPUSRUN=$<TARGET_FILE:corpusrun>
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/corpusrun_observe_orig
	-DTEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/check-observe-orig.cmake -P ${WITH_WORKDIR})
