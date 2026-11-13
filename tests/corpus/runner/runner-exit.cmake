# runner-exit.cmake - a runner (corpusrun pass, tedab run) on a pass file
# that must end with the exit status WANT and an output matching REGEX,
# leaving no run directory in WORK (runwork.h: a run without a session
# removes its own).  The free-space check (min_free=) with a work directory
# that never has enough room.
#   cmake "-DCMD=RUNNER;SUB;PASS" -DWANT=2 -DREGEX=... -DWORK=DIR -P runner-exit.cmake
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
message("runner-exit: as expected")
