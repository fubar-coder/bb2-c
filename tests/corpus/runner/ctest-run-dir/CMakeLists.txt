# The small project of the ctest test runner_ctest_run_dir
# (tests/corpus/runner/ctest-run-dir.cmake): the shape of the fixtures of
# tests/corpus (a setup that publishes in the run directory, readers, a
# cleanup with `corpusrun drop`) without an emulator.
#
#   cmake -S . -B BIN -DCORPUSRUN=... -DCTEST=...             the fixtures
#   cmake -S . -B OUTER -DCORPUSRUN=... -DCTEST=... -DBIN=BIN  two ctest runs of
#                                                              BIN at the same time
cmake_minimum_required(VERSION 3.25)
project(run_dir_fixture NONE)
enable_testing()
# the variables of the call that this project does not use (no CMake warning)
set(unused "${CORPUSRUN}${CTEST}${BIN}")
set(step ${CMAKE_CURRENT_SOURCE_DIR}/step.cmake)
if(BIN)
	# two ctest runs in one build directory, each with a run directory of
	# its own (BB2C_TEST_RUN) and its own contents (TAG)
	foreach(who a b)
		add_test(NAME run_${who} COMMAND ${CMAKE_COMMAND} -E env BB2C_TEST_RUN=${CMAKE_BINARY_DIR}/run_${who}
			TAG=${who} SLEEP=2 ${CTEST} --test-dir ${BIN} --output-on-failure)
	endforeach()
else()
	set(default ${CMAKE_BINARY_DIR}/Testing/runs/default)
	add_test(NAME setup COMMAND ${CMAKE_COMMAND} -DMODE=setup -DDEFAULT=${default} -P ${step})
	add_test(NAME reader1 COMMAND ${CMAKE_COMMAND} -DMODE=read -DDEFAULT=${default} -P ${step})
	add_test(NAME reader2 COMMAND ${CMAKE_COMMAND} -DMODE=read -DDEFAULT=${default} -P ${step})
	add_test(NAME cleanup COMMAND ${CORPUSRUN} drop --run-dir ${default} corpus)
	set_tests_properties(setup PROPERTIES FIXTURES_SETUP corpus)
	set_tests_properties(reader1 reader2 PROPERTIES FIXTURES_REQUIRED corpus)
	set_tests_properties(cleanup PROPERTIES FIXTURES_CLEANUP corpus)
endif()
