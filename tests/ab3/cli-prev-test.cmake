# cli-prev.cmake with broken input (tests/ab3/CMakeLists.txt, ab3_cli_prev_*): one case (atoi), two sessions.
#   cmake -DMODE=same|differs|intended|stale|second|unreadable ... (the arguments of cli-prev.cmake except
#         BLITZ2_DIR, PREV_BLITZ2_DIR, INTENDED) -DBLITZ2_DIR=DIR -DWORK=DIR -P cli-prev-test.cmake
# MODE
#   same      prev is this build's directory: passes, nothing differs
#   differs   prev's blitz2-ab3 is another program (ab3cli): the compiler's output differs and
#             there is no executable: fails, naming the case and the line to quote in the commit
#   intended  the same, the case listed (ab3_cli_prev:atoi): passes
#   stale     prev is this build's directory and the case is listed: fails, STALE
#   second    prev is this build's directory, a second case from the block's other sources: passes, and
#             that case was compiled in both sessions (the cases of the other sources
#             were never compiled, #1200)
#   unreadable  prev is this build's directory, a case's source does not exist: fails, naming the
#             case (two sessions that cannot read it must not count as the same)
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
if(MODE STREQUAL "same" OR MODE STREQUAL "intended" OR MODE STREQUAL "second")
	if(rc)
		message(FATAL_ERROR "cli-prev-test: ${MODE}: exit ${rc}, expected 0")
	endif()
	if(MODE STREQUAL "intended" AND NOT "${out}${err}" MATCHES "atoi: [^\n]*\\(intended\\)")
		message(FATAL_ERROR "cli-prev-test: intended: the difference is not reported as intended")
	endif()
	if(MODE STREQUAL "second")
		# compiled (its error is expected), not just "the same": the unfixed script gave both
		# sessions only the first source
		foreach(w cur prev)
			set(o ${WORK}/run/${w}/test/out/chr_toolong.txt)
			set(text)
			if(EXISTS ${o})
				file(READ ${o} text)
			endif()
			if(NOT text MATCHES "chr_toolong.ab3:5: Syntax Error")
				message(FATAL_ERROR "cli-prev-test: second: chr_toolong was not compiled in the ${w} session")
			endif()
		endforeach()
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
	if(MODE STREQUAL "unreadable" AND NOT "${out}${err}" MATCHES "FAIL no_such_source: the cur session cannot read the source")
		message(FATAL_ERROR "cli-prev-test: unreadable: the case is not reported")
	endif()
endif()
message(STATUS "cli-prev-test: ${MODE} as expected")
