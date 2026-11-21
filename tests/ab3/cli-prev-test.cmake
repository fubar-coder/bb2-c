# cli-prev.cmake with broken input (tests/ab3/CMakeLists.txt, ab3_cli_prev_*): one case (atoi), two sessions.
#   cmake -DMODE=same|differs|intended|stale ... (the arguments of cli-prev.cmake except
#         BLITZ2_DIR, PREV_BLITZ2_DIR, INTENDED) -DBLITZ2_DIR=DIR -DWORK=DIR -P cli-prev-test.cmake
# MODE
#   same      prev is this build's directory: passes, nothing differs
#   differs   prev's blitz2-ab3 is another program (ab3cli): the compiler's output differs and
#             there is no executable: fails, naming the case and the line to quote in the commit
#   intended  the same, the case listed (ab3_cli_prev:atoi): passes
#   stale     prev is this build's directory and the case is listed: fails, STALE
cmake_minimum_required(VERSION 4.3)

set(other ${WORK}/other)
file(REMOVE_RECURSE ${other})
file(MAKE_DIRECTORY ${other})
foreach(f blitz2-ab3 ab3cli acidlibs deflibs)
	file(COPY_FILE ${BLITZ2_DIR}/${f} ${other}/${f})
endforeach()
file(COPY_FILE ${BLITZ2_DIR}/ab3cli ${other}/blitz2-ab3 ONLY_IF_DIFFERENT)
if(MODE STREQUAL "differs" OR MODE STREQUAL "intended")
	set(prev ${other})
else()
	set(prev ${BLITZ2_DIR})
endif()
set(intended)
if(MODE STREQUAL "intended" OR MODE STREQUAL "stale")
	set(intended "-DINTENDED=ab3_cli_prev:atoi")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -DAMIGA_RUN=${AMIGA_RUN} -DRESULTS=${RESULTS}
		-DREQTRAP=${REQTRAP} -DINJECT=${INJECT} -DCASES=${CASES} -DBLITZ2_DIR=${BLITZ2_DIR}
		-DPREV_BLITZ2_DIR=${prev} -DAB3=${AB3} "-DSOURCES=${SOURCES}" -DUSERLIB=${USERLIB}
		-DWORK=${WORK}/run ${intended} -P ${CMAKE_CURRENT_LIST_DIR}/cli-prev.cmake
	RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
message("${out}${err}")
if(rc EQUAL 77)
	cmake_language(EXIT 77)
endif()
if(MODE STREQUAL "same" OR MODE STREQUAL "intended")
	if(rc)
		message(FATAL_ERROR "cli-prev-test: ${MODE}: exit ${rc}, expected 0")
	endif()
	if(MODE STREQUAL "intended" AND NOT "${out}${err}" MATCHES "atoi: [^\n]*\\(intended\\)")
		message(FATAL_ERROR "cli-prev-test: intended: the difference is not reported as intended")
	endif()
else()
	if(NOT rc)
		message(FATAL_ERROR "cli-prev-test: ${MODE}: exit 0, a failure expected")
	endif()
	if(MODE STREQUAL "differs" AND NOT "${out}${err}" MATCHES "FAIL atoi: [^\n]*\n-- prev differs in: PREV_INTENDED=\"ab3_cli_prev:atoi\"")
		message(FATAL_ERROR "cli-prev-test: differs: the case and the entry are not named")
	endif()
	if(MODE STREQUAL "stale" AND NOT "${out}${err}" MATCHES "STALE ab3_cli_prev:atoi")
		message(FATAL_ERROR "cli-prev-test: stale: no STALE line")
	endif()
endif()
message(STATUS "cli-prev-test: ${MODE} as expected")
