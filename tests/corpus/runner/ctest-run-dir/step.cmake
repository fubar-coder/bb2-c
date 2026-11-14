# A step of the project of runner_ctest_run_dir: RUN is the environment
# variable BB2C_TEST_RUN, else DEFAULT; the setup publishes the fixture
# "corpus" there, with the contents $ENV{TAG}, a reader finds it with these
# contents.  $ENV{SLEEP} seconds in a reader make parallel runs overlap;
# $ENV{READ_TAG}: the contents the reader expects (default TAG); $ENV{FAIL}: the reader fails after marking the fixture as a runner does
# (rundir.h: RUN/NAME.keep).
#   cmake -DMODE=setup|read -DDEFAULT=DIR -P step.cmake
if(DEFINED ENV{BB2C_TEST_RUN} AND NOT "$ENV{BB2C_TEST_RUN}" STREQUAL "")
	set(run $ENV{BB2C_TEST_RUN})
else()
	set(run ${DEFAULT})
endif()
set(file ${run}/corpus/clib/vol/out/x)
if(MODE STREQUAL "setup")
	file(WRITE ${file} "$ENV{TAG}")
else()
	if($ENV{SLEEP})
		execute_process(COMMAND ${CMAKE_COMMAND} -E sleep $ENV{SLEEP})
	endif()
	if(NOT EXISTS ${file})
		message(FATAL_ERROR "no fixture data ${file}")
	endif()
	file(READ ${file} got)
	if(DEFINED ENV{READ_TAG})
		set(want $ENV{READ_TAG})
	else()
		set(want $ENV{TAG})
	endif()
	if(NOT got STREQUAL want)
		message(FATAL_ERROR "${file}: '${got}', not the data of this run ('${want}')")
	endif()
	if("$ENV{FAIL}")
		file(WRITE ${run}/corpus.keep "failed\n")
		message(FATAL_ERROR "the reader fails")
	endif()
endif()
