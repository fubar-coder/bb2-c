# want-report.cmake - a corpus pass whose result is known: passes if
# corpusrun pass PASS returns WANT and its output matches REGEX (77 =
# skipped).  The self tests of the notes (tests/corpus/CMakeLists.txt).
#   cmake -DRUNNER=... -DPASS=... -DWANT=N -DREGEX=... -P want-report.cmake
execute_process(COMMAND ${RUNNER} pass ${PASS} RESULT_VARIABLE rc OUTPUT_VARIABLE out
	ERROR_VARIABLE err)
if(rc EQUAL 77)
	cmake_language(EXIT 77)
endif()
message("${out}${err}")
if(NOT rc EQUAL WANT)
	message(FATAL_ERROR "${RUNNER} pass returned ${rc}, expected ${WANT}")
endif()
if(NOT "${out}${err}" MATCHES "${REGEX}")
	message(FATAL_ERROR "the output does not match: ${REGEX}")
endif()
