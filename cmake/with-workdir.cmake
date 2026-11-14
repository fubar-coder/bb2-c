# with-workdir.cmake - runs a test script (cmake -P) in a work directory of
# its own, so that two runs of the test at the same time (two ctest runs in
# one build directory) do not share it, and a passed test leaves nothing:
#
#   cmake -DTEST_SCRIPT=test.cmake [-DNAME=value]... -DWORK=BASE [-DWANT_FAIL=ON]
#         -P with-workdir.cmake
#
# The script runs with every -D of the call, WORK being a new directory
# BASE/run-XXXXXX, or, when the environment variable BB2C_TEST_RUN names a
# directory (the caller's, tests/corpus/runner/rundir.h), BB2C_TEST_RUN/
# NAME-XXXXXX with NAME the last component of BASE.  The directory is not
# made (the script makes what it needs; the free name is checked).  Its
# exit status is the wrapper's (77 = skipped included; its output
# is the test's output).  A script that passes (or is skipped, 77): the directory is removed, and
# BASE if it is empty; one that fails keeps it and prints
# "the files of the run: DIR".  WANT_FAIL=ON: the test is about a script that
# must fail (its output is matched by the test): a failure (not 77) counts as
# the pass, a script that passes is kept.
if(NOT TEST_SCRIPT OR NOT WORK)
	message(FATAL_ERROR "with-workdir: TEST_SCRIPT and WORK are needed")
endif()
# the name: a time stamp with microseconds and random letters; the directory
# is not there yet
get_filename_component(base_name ${WORK} NAME)
set(private "")
foreach(try RANGE 20)
	string(TIMESTAMP now "%s%f")
	string(RANDOM LENGTH 6 ALPHABET "abcdefghijklmnopqrstuvwxyz0123456789" RANDOM_SEED ${now} suffix)
	if(DEFINED ENV{BB2C_TEST_RUN} AND NOT "$ENV{BB2C_TEST_RUN}" STREQUAL "")
		set(candidate $ENV{BB2C_TEST_RUN}/${base_name}-${suffix})
	else()
		set(candidate ${WORK}/run-${suffix})
	endif()
	if(NOT EXISTS ${candidate})
		set(private ${candidate})
		break()
	endif()
endforeach()
if(NOT private)
	message(FATAL_ERROR "with-workdir: no free directory name for ${WORK}")
endif()
# the arguments of this call, every -D but WORK and TEST_SCRIPT, and WORK for the script
set(args)
math(EXPR last "${CMAKE_ARGC} - 1")
foreach(i RANGE 1 ${last})
	set(a "${CMAKE_ARGV${i}}")
	if(a STREQUAL "-P")
		break()
	endif()
	if(a MATCHES "^-D(WORK|TEST_SCRIPT)=")
		continue()
	endif()
	string(REPLACE ";" "\\;" a "${a}")
	list(APPEND args "${a}")
endforeach()
execute_process(COMMAND ${CMAKE_COMMAND} ${args} -DWORK=${private} -P ${TEST_SCRIPT}
	RESULT_VARIABLE rc)
set(ok OFF)
if(WANT_FAIL)
	if(NOT rc STREQUAL "0")
		set(ok ON)
	endif()
elseif(rc STREQUAL "0" OR rc STREQUAL "77")
	set(ok ON)
endif()
if(ok)
	file(REMOVE_RECURSE ${private})
	# the base directory if nothing else is in it (a run at the same time)
	file(GLOB left LIST_DIRECTORIES true ${WORK}/*)
	if(NOT left AND IS_DIRECTORY ${WORK})
		file(REMOVE_RECURSE ${WORK})
	endif()
	if(NOT rc STREQUAL "0" AND rc MATCHES "^[0-9]+$")
		cmake_language(EXIT ${rc})
	endif()
	return()
endif()
if(EXISTS ${private})
	message("the files of the run: ${private}")
endif()
if(rc MATCHES "^[0-9]+$")
	cmake_language(EXIT ${rc})
endif()
message(FATAL_ERROR "with-workdir: ${TEST_SCRIPT}: ${rc}")
