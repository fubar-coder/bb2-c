# The comparison of one block of the command line tests of blitz2-ab3 with the build of the
# previous commit (tests/ab3/CMakeLists.txt, ab3_cli_prev_N; tests/README.md "The prev build"):
#   cmake -DAMIGA_RUN=... -DRESULTS=... -DREQTRAP=... -DINJECT=TOOL -DCASES=FILE
#         -DBLITZ2_DIR=DIR -DPREV_BLITZ2_DIR=DIR -DAB3=DIR -DSOURCES=FILE;...
#         -DUSERLIB=OBJ -DWORK=DIR [-DTIMEOUT=SEC] [-DINTENDED=ENTRY;...] -P cli-prev.cmake
# cli.cmake with COMPILE_ONLY runs the block's cases twice, with blitz2-ab3 of BLITZ2_DIR (WORK/cur)
# and of the previous commit (WORK/prev); the two sessions run at the same time (one
# execute_process with two commands: the first writes nothing to its standard output, which is
# the second's standard input).  The executable (results exe-same: the parameter blocks of the traps
# masked) and the console output of every case must be the same.  A case that differs fails unless INTENDED names it (ab3_cli_prev:CASE, or ON: the
# PREV_INTENDED of the build; entries of other tests are ignored); a listed case of this block
# that does not differ fails as well.  Exit status 0 = as listed, 77 = skipped (no emulator, no
# AmiBlitz3 files, the previous commit has no such directory).
cmake_minimum_required(VERSION 4.3)

foreach(v AMIGA_RUN RESULTS REQTRAP INJECT CASES BLITZ2_DIR PREV_BLITZ2_DIR AB3 WORK)
	if(NOT ${v})
		message(FATAL_ERROR "cli-prev.cmake: -D${v}=... missing")
	endif()
endforeach()
foreach(f blitz2-ab3 ab3cli acidlibs deflibs)
	if(NOT EXISTS ${PREV_BLITZ2_DIR}/${f})
		message(STATUS "cli-prev.cmake: no ${PREV_BLITZ2_DIR}/${f} - skipped")
		cmake_language(EXIT 77)
	endif()
endforeach()
if(NOT TIMEOUT)
	set(TIMEOUT 600)
endif()
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${WORK})

set(common -DAMIGA_RUN=${AMIGA_RUN} -DRESULTS=${RESULTS} -DREQTRAP=${REQTRAP} -DINJECT=${INJECT}
	-DCONFIG=exe -DCASES=${CASES} -DAB3=${AB3} "-DSOURCES=${SOURCES}" -DUSERLIB=${USERLIB}
	-DTIMEOUT=${TIMEOUT} -DCOMPILE_ONLY=ON)
execute_process(
	COMMAND ${CMAKE_COMMAND} ${common} -DBLITZ2_DIR=${BLITZ2_DIR} -DWORK=${WORK}/cur
		-P ${CMAKE_CURRENT_LIST_DIR}/cli.cmake
	COMMAND ${CMAKE_COMMAND} ${common} -DBLITZ2_DIR=${PREV_BLITZ2_DIR} -DWORK=${WORK}/prev
		-P ${CMAKE_CURRENT_LIST_DIR}/cli.cmake
	RESULTS_VARIABLE rcs ERROR_VARIABLE msg OUTPUT_QUIET)
string(STRIP "${msg}" msg)
foreach(rc IN LISTS rcs)
	if(rc EQUAL 77)
		cmake_language(EXIT 77)
	endif()
endforeach()
foreach(rc IN LISTS rcs)
	if(rc)
		message(FATAL_ERROR "cli-prev.cmake: a compile session failed (${rcs}): ${msg}")
	endif()
endforeach()

# the entries of this test
set(all OFF)
set(entries)
foreach(e IN LISTS INTENDED)
	if(e STREQUAL "ON")
		set(all ON)
	elseif(e MATCHES "^ab3_cli_prev:(.+)$")
		list(APPEND entries ${CMAKE_MATCH_1})
	endif()
endforeach()

file(STRINGS ${CASES} cases)
set(differ)
set(errors 0)
set(case_names)
foreach(c IN LISTS cases)
	string(REPLACE "|" ";" c "${c}")
	list(GET c 0 name)
	list(APPEND case_names ${name})
	set(what)
	set(exe_a ${WORK}/cur/test/out/${name}.exe)
	set(exe_b ${WORK}/prev/test/out/${name}.exe)
	if(EXISTS ${exe_a} AND EXISTS ${exe_b})
		# the trap blocks (a program with debugging or runtime error checking) hold addresses of the
		# compiler's memory: masked
		execute_process(COMMAND ${RESULTS} exe-same ${exe_a} ${exe_b} RESULT_VARIABLE same
			ERROR_VARIABLE same_msg)
		if(same EQUAL 2)
			message(FATAL_ERROR "cli-prev.cmake: ${same_msg}")
		elseif(same)
			list(APPEND what "the executables differ")
		endif()
	elseif(EXISTS ${exe_a} OR EXISTS ${exe_b})
		list(APPEND what "an executable only of the ${name} compile of one build")
	endif()
	foreach(w cur prev)
		if(EXISTS ${WORK}/${w}/test/out/${name}.txt)
			file(READ ${WORK}/${w}/test/out/${name}.txt text_${w})
		else()
			set(text_${w} "no output")
		endif()
	endforeach()
	if(NOT text_cur STREQUAL text_prev)
		list(APPEND what "the compiler's output differs")
	endif()
	if(what)
		list(JOIN what ", " what)
		if(name IN_LIST entries OR all)
			message(STATUS "${name}: ${what} (intended)")
		else()
			message(STATUS "FAIL ${name}: ${what}")
			math(EXPR errors "${errors} + 1")
		endif()
		list(APPEND differ ab3_cli_prev:${name})
	else()
		message(STATUS "ok   ${name}")
	endif()
endforeach()
foreach(e IN LISTS entries)
	if(e IN_LIST case_names AND NOT "ab3_cli_prev:${e}" IN_LIST differ)
		message(STATUS "STALE ab3_cli_prev:${e}: intended, but prev does not differ in it")
		math(EXPR errors "${errors} + 1")
	endif()
endforeach()
if(differ)
	list(JOIN differ ";" differ)
	message(STATUS "prev differs in: PREV_INTENDED=\"${differ}\"")
endif()
if(errors)
	message(FATAL_ERROR "cli-prev.cmake: ${errors} difference(s) from the prev build that are not intended "
		"(${WORK})")
endif()
