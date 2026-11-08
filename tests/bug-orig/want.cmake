# want.cmake - a bug-orig pass whose verdict is known: passes if the runner
# (RUNNER SUB: corpusrun, or tedab run for TED) returns WANT (the self test:
# 3 = broken test), 77 = skipped.
#   cmake -DRUNNER=... [-DSUB=...] -DPASS=... -DWANT=N -P want.cmake
execute_process(COMMAND ${RUNNER} ${SUB} ${PASS} RESULT_VARIABLE rc)
if(rc EQUAL 77)
	cmake_language(EXIT 77)
endif()
if(NOT rc EQUAL WANT)
	message(FATAL_ERROR "${RUNNER} ${SUB} returned ${rc}, expected ${WANT}")
endif()
