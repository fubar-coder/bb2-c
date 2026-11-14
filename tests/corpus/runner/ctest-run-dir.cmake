# The fixtures and the run directory with real ctest runs (runner_ctest_run_dir,
# no emulator): a small project (ctest-run-dir/) of the shape of tests/corpus
# - setup that publishes in the run directory, readers, cleanup with
# `corpusrun drop` - run by a nested ctest:
#   * without BB2C_TEST_RUN: setup, readers and cleanup find the default
#     directory (Testing/runs/default); a passed run leaves nothing
#   * with BB2C_TEST_RUN: the caller's directory is used, only the fixture
#     is removed from it (a file of the caller stays)
#   * a reader that fails: its fixture stays (marked), the cleanup keeps it
#   * two ctest runs at the same time in one build directory, each with a
#     directory of its own: each reader sees the data of its own setup
#   * the check of the readers can fail: a reader with other data than the
#     setup wrote (as if the two runs shared a directory) fails the run
#   cmake -DCORPUSRUN=... -DCTEST=... -DSRC=... -DWORK=DIR -P ctest-run-dir.cmake
function(run_ctest result dir)
	execute_process(COMMAND ${CMAKE_COMMAND} -E env ${ARGN} ${CTEST} -V --test-dir ${dir}
		RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
	set(${result} ${rc} PARENT_SCOPE)
	set(log "${out}${err}" PARENT_SCOPE)
endfunction()
# cond: a condition of if(), as text
function(expect cond text)
	set(failed OFF)
	cmake_language(EVAL CODE "if(NOT (${cond}))\nset(failed ON)\nendif()")
	if(failed)
		message(FATAL_ERROR "ctest-run-dir: ${text}\n${log}")
	endif()
endfunction()

file(REMOVE_RECURSE ${WORK})
set(bin ${WORK}/bin)
set(outer ${WORK}/outer)
foreach(args "bin|" "outer|-DBIN=${bin}")
	string(REPLACE "|" ";" args "${args}")
	list(GET args 0 d)
	list(GET args 1 extra)
	execute_process(COMMAND ${CMAKE_COMMAND} -S ${SRC} -B ${WORK}/${d} -DCORPUSRUN=${CORPUSRUN}
		-DCTEST=${CTEST} ${extra} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
	set(log "${out}${err}")
	expect("rc EQUAL 0" "cannot configure ${d}")
endforeach()

# without the variable: the default directory, gone afterwards
run_ctest(rc ${bin} TAG=d)
expect("rc EQUAL 0" "the run in the default directory failed")
expect("NOT EXISTS ${bin}/Testing/runs" "the passed run left ${bin}/Testing/runs")

# broken: a reader that finds other data than its setup wrote (as if two
# runs shared a directory) fails the run
run_ctest(rc ${bin} TAG=x READ_TAG=y)
expect("NOT rc EQUAL 0" "a reader with other data than the setup's passed")
expect("NOT EXISTS ${bin}/Testing/runs/default/corpus.keep" "unexpected mark")

# with the variable: the caller's directory, only the fixture removed
file(MAKE_DIRECTORY ${WORK}/mine)
file(WRITE ${WORK}/mine/other.txt "the caller's file")
run_ctest(rc ${bin} TAG=m BB2C_TEST_RUN=${WORK}/mine)
expect("rc EQUAL 0" "the run in the caller's directory failed")
expect("EXISTS ${WORK}/mine/other.txt" "the caller's file was removed")
expect("NOT EXISTS ${WORK}/mine/corpus" "the fixture is still in the caller's directory")
expect("NOT EXISTS ${bin}/Testing/runs" "the default directory was used")
file(REMOVE_RECURSE ${WORK}/mine)

# a reader that fails: the fixture stays, marked
run_ctest(rc ${bin} TAG=f FAIL=1 BB2C_TEST_RUN=${WORK}/kept)
expect("NOT rc EQUAL 0" "the failing reader did not fail the run")
expect("EXISTS ${WORK}/kept/corpus/clib/vol/out/x" "the fixture was removed after a failure")
expect("EXISTS ${WORK}/kept/corpus.keep" "no mark")
expect("log MATCHES \"kept [^\\n]*corpus\"" "the cleanup does not say that it kept the data")
file(REMOVE_RECURSE ${WORK}/kept)

# two ctest runs at the same time in one build directory (the readers wait
# 2 s: the runs overlap)
execute_process(COMMAND ${CTEST} --test-dir ${outer} -j2 --output-on-failure
	RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
set(log "${out}${err}")
expect("rc EQUAL 0" "the two runs at the same time failed")
expect("log MATCHES \"100% tests passed[^\\n]* out of 2\"" "not both runs passed")
foreach(who a b)
	expect("NOT EXISTS ${outer}/run_${who}/corpus" "the fixture of run ${who} stays")
endforeach()
expect("NOT EXISTS ${bin}/Testing/runs" "the default directory was used by the parallel runs")

file(REMOVE_RECURSE ${WORK})
message("ctest-run-dir: as expected")
