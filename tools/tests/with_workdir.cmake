# The wrapper cmake/with-workdir.cmake: a test script runs in a work
# directory of its own, with every -D of the call (a value with ";" too); a
# script that passes or is skipped (77) leaves nothing (the directory, and the
# base directory if empty), one that fails keeps its directory and prints
# where; with BB2C_TEST_RUN the directory is below the caller's, which keeps
# what is not the test's.
#   cmake -DWRAPPER=cmake/with-workdir.cmake -DWORK=DIR -P with_workdir.cmake
foreach(v WRAPPER WORK)
	if(NOT ${v})
		message(FATAL_ERROR "with_workdir.cmake: ${v} missing")
	endif()
endforeach()
unset(ENV{BB2C_TEST_RUN})
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${WORK})

# the script of the cases: prints its WORK and P, makes WORK, and passes,
# fails (FAIL=1) or is skipped (SKIP=1)
file(WRITE ${WORK}/case.cmake "message(\"case: WORK=\${WORK} P=\${P}\")
file(MAKE_DIRECTORY \${WORK})
file(WRITE \${WORK}/file \"x\")
if(FAIL)
	message(FATAL_ERROR \"case: failed on purpose\")
endif()
if(SKIP)
	cmake_language(EXIT 77)
endif()
")

# runs the wrapper on the case with the arguments (and the environment
# variables of ENVS), sets rc and out
function(wrap name)
	execute_process(COMMAND ${CMAKE_COMMAND} -E env ${ENVS} ${CMAKE_COMMAND} ${ARGN}
		-DTEST_SCRIPT=${WORK}/case.cmake -DWORK=${WORK}/base/${name} -P ${WRAPPER}
		RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
	set(rc ${rc} PARENT_SCOPE)
	set(out "${out}${err}" PARENT_SCOPE)
endfunction()
function(fail text)
	message(FATAL_ERROR "with_workdir: ${text}\n${out}")
endfunction()

# a passing script: its directory is below the base, with the arguments, and
# gone afterwards together with the base
wrap(pass "-DP=a\;b\;c")   # one argument with ";" (escaped in the list)
if(NOT rc EQUAL 0)
	fail("a passing script failed")
endif()
if(NOT out MATCHES "case: WORK=[^\n]*/base/pass/run-[a-z0-9]+ P=a;b;c")
	fail("no run directory below WORK, or the value with ; lost")
endif()
if(EXISTS ${WORK}/base/pass)
	fail("a passed run left ${WORK}/base/pass")
endif()

# a failing script (the check itself fails on broken input): the wrapper
# fails, keeps the directory and says where
wrap(fail -DFAIL=1)
if(rc EQUAL 0)
	fail("a failing script passed")
endif()
if(NOT out MATCHES "the files of the run: [^\n]*/base/fail/run-[a-z0-9]+")
	fail("the kept directory is not named")
endif()
file(GLOB kept ${WORK}/base/fail/run-*/file)
if(NOT kept)
	fail("the files of a failed run are gone")
endif()
file(REMOVE_RECURSE ${WORK}/base)

# a skipped script (77): the status is passed on, nothing stays
wrap(skip -DSKIP=1)
if(NOT rc EQUAL 77)
	fail("the skip (77) was not passed on: ${rc}")
endif()
if(EXISTS ${WORK}/base/skip)
	fail("a skipped run left ${WORK}/base/skip")
endif()

# WANT_FAIL: a script that fails as it must leaves nothing and its status
# stays; one that passes is kept
wrap(wantfail -DFAIL=1 -DWANT_FAIL=ON)
if(rc EQUAL 0)
	fail("the status of the failing script is lost")
endif()
if(EXISTS ${WORK}/base/wantfail)
	fail("an expected failure left ${WORK}/base/wantfail")
endif()
wrap(wantpass -DWANT_FAIL=ON)
if(NOT out MATCHES "the files of the run: [^\n]*/base/wantpass/run-")
	fail("a script that had to fail but passed is not kept")
endif()
file(REMOVE_RECURSE ${WORK}/base)

# the caller's directory: the run is below it, removed after the pass, the
# caller's other files stay and the base is not used
file(MAKE_DIRECTORY ${WORK}/mine)
file(WRITE ${WORK}/mine/other.txt "the caller's")
set(ENVS BB2C_TEST_RUN=${WORK}/mine)
wrap(env)
if(NOT rc EQUAL 0)
	fail("a passing script failed with BB2C_TEST_RUN")
endif()
if(NOT out MATCHES "case: WORK=[^\n]*/mine/env-[a-z0-9]+ ")
	fail("the run is not below BB2C_TEST_RUN")
endif()
file(GLOB left ${WORK}/mine/env-*)
if(left)
	fail("the run directory stays in the caller's directory")
endif()
if(NOT EXISTS ${WORK}/mine/other.txt)
	fail("the caller's file is gone")
endif()
wrap(envfail -DFAIL=1)
if(rc EQUAL 0)
	fail("a failing script passed with BB2C_TEST_RUN")
endif()
file(GLOB left ${WORK}/mine/envfail-*/file)
if(NOT left)
	fail("the files of a failed run are gone from the caller's directory")
endif()

file(REMOVE_RECURSE ${WORK})
message("with_workdir: all cases as expected")
