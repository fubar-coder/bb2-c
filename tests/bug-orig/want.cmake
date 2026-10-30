# want.cmake - a bug-orig pass whose verdict is known: passes if the runner
# (CORPUSRUN: corpusrun, or tedab for TED) returns WANT (the self test: 3 =
# broken test), 77 = skipped.
#   cmake -DCORPUSRUN=... -DPASS=... -DWANT=N -P want.cmake
execute_process(COMMAND ${CORPUSRUN} ${PASS} RESULT_VARIABLE rc)
if(rc EQUAL 77)
	cmake_language(EXIT 77)
endif()
if(NOT rc EQUAL WANT)
	message(FATAL_ERROR "${CORPUSRUN} returned ${rc}, expected ${WANT}")
endif()
