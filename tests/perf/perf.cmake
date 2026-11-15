# perf.cmake - the benchmark of the libraries (tests/perf/CMakeLists.txt):
# the configuration TAG of the pass PASS in one emulator session
# (corpusrun pass --observe TAG), then the table of its cycles per call
# (results perf).  Run through cmake/with-workdir.cmake: WORK is the run's
# directory.  Exit status 77 (skipped) without Amiberry or the emulation
# data.
#   cmake -DCORPUSRUN=... -DRESULTS=... -DPASS=... -DTAG=clib|orig
#         -DWORK=DIR -P perf.cmake
foreach(v CORPUSRUN RESULTS PASS TAG WORK)
	if(NOT ${v})
		message(FATAL_ERROR "perf.cmake: ${v} missing")
	endif()
endforeach()
file(MAKE_DIRECTORY ${WORK})
execute_process(COMMAND ${CORPUSRUN} pass --observe ${TAG} --to ${WORK}/obs ${PASS}
	RESULT_VARIABLE rc)
# without Amiberry or the emulation data (or, for orig, the CD): skipped
if(rc EQUAL 77)
	cmake_language(EXIT 77)
endif()
if(NOT rc EQUAL 0)
	message(FATAL_ERROR "perf: corpusrun pass --observe ${TAG} returned ${rc}")
endif()
execute_process(COMMAND ${RESULTS} perf ${WORK}/obs RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
	message(FATAL_ERROR "perf: results perf returned ${rc}")
endif()
