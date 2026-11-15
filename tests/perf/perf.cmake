# perf.cmake - the benchmark of the libraries (tests/perf/CMakeLists.txt):
# the configuration TAG of the pass PASS in one emulator session
# (corpusrun pass --observe TAG), then the table of its cycles per call
# against the budgets BUDGETS (results perf; without BUDGETS only the
# table: the original's values).  With PROGRAM only the budget lines of that
# program (PROGRAM.*) are judged: the pass has that program only, and a
# program without a budget line fails.  With OBS the observations of that
# directory are judged instead of a run (the checks of the budgets, no
# emulator).  Run through cmake/with-workdir.cmake: WORK is the run's
# directory.  Exit status 77 (skipped) without Amiberry or the emulation
# data; a command over its budget fails with its name, the measured and
# the budgeted cycles.
#   cmake -DCORPUSRUN=... -DRESULTS=... -DPASS=... -DTAG=clib|orig
#         [-DBUDGETS=budgets.txt [-DPROGRAM=NAME]] [-DOBS=DIR] -DWORK=DIR -P perf.cmake
foreach(v RESULTS WORK)
	if(NOT ${v})
		message(FATAL_ERROR "perf.cmake: ${v} missing")
	endif()
endforeach()
if(NOT OBS)
	foreach(v CORPUSRUN PASS TAG)
		if(NOT ${v})
			message(FATAL_ERROR "perf.cmake: ${v} missing")
		endif()
	endforeach()
	file(MAKE_DIRECTORY ${WORK})
	set(OBS ${WORK}/obs)
	execute_process(COMMAND ${CORPUSRUN} pass --observe ${TAG} --to ${OBS} ${PASS}
		RESULT_VARIABLE rc)
	# without Amiberry or the emulation data (or, for orig, the CD): skipped
	if(rc EQUAL 77)
		cmake_language(EXIT 77)
	endif()
	if(NOT rc EQUAL 0)
		message(FATAL_ERROR "perf: corpusrun pass --observe ${TAG} returned ${rc}")
	endif()
endif()
set(budgets)
if(BUDGETS)
	set(budgets --budgets ${BUDGETS})
	if(PROGRAM)
		# the lines of this program (comments and the other programs dropped)
		# line by line with string(), not as a list: the loops have ";" and "[" in them
		file(READ ${BUDGETS} rest)
		set(mine "")
		while(NOT rest STREQUAL "")
			string(FIND "${rest}" "\n" eol)
			if(eol LESS 0)
				set(l "${rest}")
				set(rest "")
			else()
				string(SUBSTRING "${rest}" 0 ${eol} l)
				math(EXPR eol "${eol} + 1")
				string(SUBSTRING "${rest}" ${eol} -1 rest)
			endif()
			if(l MATCHES "^${PROGRAM}\\.")
				string(APPEND mine "${l}\n")
			endif()
		endwhile()
		if(NOT mine)
			message(FATAL_ERROR "perf: the budgets have no line of the program ${PROGRAM}")
		endif()
		file(MAKE_DIRECTORY ${WORK})
		file(WRITE ${WORK}/budgets-${PROGRAM}.txt "${mine}")
		set(budgets --budgets ${WORK}/budgets-${PROGRAM}.txt)
	endif()
endif()
execute_process(COMMAND ${RESULTS} perf ${budgets} ${OBS} RESULT_VARIABLE rc)
if(rc EQUAL 1)
	message(FATAL_ERROR "perf: a command over its budget, without a budget or not measured "
		"(the lines above)")
elseif(NOT rc EQUAL 0)
	message(FATAL_ERROR "perf: results perf returned ${rc}")
endif()
