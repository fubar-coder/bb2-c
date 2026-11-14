# runner-exit.cmake - a runner (corpusrun pass, tedab run) on a pass file
# that must end with the exit status WANT and an output matching REGEX,
# leaving no run directory in WORK (runwork.h: a run without a session
# removes its own).  The free-space check (min_free=) with a work directory
# that never has enough room.  RUN_ENV=DIR: the runner gets BB2C_TEST_RUN=DIR
# (an empty directory it makes); without it the variable is not set.
#   cmake "-DCMD=RUNNER;SUB;PASS" -DWANT=2 -DREGEX=... -DWORK=DIR [-DRUN_ENV=DIR]
#         -P runner-exit.cmake
if(RUN_ENV)
	file(REMOVE_RECURSE ${RUN_ENV})
	file(MAKE_DIRECTORY ${RUN_ENV})
	set(ENV{BB2C_TEST_RUN} ${RUN_ENV})
else()
	unset(ENV{BB2C_TEST_RUN})
endif()
execute_process(COMMAND ${CMD} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
message("${out}${err}")
if(NOT rc STREQUAL WANT)
	message(FATAL_ERROR "${CMD}: exit status ${rc}, expected ${WANT}")
endif()
if(NOT "${out}${err}" MATCHES "${REGEX}")
	message(FATAL_ERROR "the output does not match: ${REGEX}")
endif()
file(GLOB left LIST_DIRECTORIES true ${WORK}/run-*)
if(left)
	message(FATAL_ERROR "run directories left in ${WORK}: ${left}")
endif()
if(RUN_ENV)
	file(REMOVE_RECURSE ${RUN_ENV})
endif()
message("runner-exit: as expected")
