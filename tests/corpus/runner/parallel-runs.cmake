# parallel-runs.cmake - runs of the same pass file at the same time
# (corpusrun pass, tedab run; #1020): one run starts after each delay of
# DELAYS (seconds), while the earlier ones stage and run their sessions in
# the same work= directory (a runner that emptied it under another gave a
# wrong verdict, depending on when it started: several delays hit
# several moments of the first run).  Each run must give the exit status
# WANT, the verdict of a run alone.  Exit 77 (skipped) if a run is skipped
# (no Amiberry).
#   cmake "-DCMD=RUNNER;SUB;PASS" -DWANT=N "-DDELAYS=0;1;2" -DLOG=PREFIX
#         -P parallel-runs.cmake
# One run (internal): -DONE=N -DDELAY=SEC, CMD joined by |, writes its
# output to LOG-N.log and LOG-N.err and its exit status to LOG-N.rc.
if(ONE)
	string(REPLACE "|" ";" CMD "${CMD}")
	if(DELAY)
		execute_process(COMMAND ${CMAKE_COMMAND} -E sleep ${DELAY})
	endif()
	execute_process(COMMAND ${CMD} RESULT_VARIABLE rc OUTPUT_FILE ${LOG}-${ONE}.log
		ERROR_FILE ${LOG}-${ONE}.err)
	file(WRITE ${LOG}-${ONE}.rc "${rc}")
	return()
endif()
# the commands of one execute_process run at the same time (a pipe: the
# wrappers write nothing to it)
set(cmds "")
# CMD as one argument of a list: its words joined by |
string(REPLACE ";" "|" cmdarg "${CMD}")
set(n 0)
foreach(d IN LISTS DELAYS)
	math(EXPR n "${n} + 1")
	file(REMOVE ${LOG}-${n}.rc)
	list(APPEND cmds COMMAND ${CMAKE_COMMAND} -DCMD=${cmdarg} -DLOG=${LOG} -DONE=${n} -DDELAY=${d}
		-P ${CMAKE_CURRENT_LIST_FILE})
endforeach()
execute_process(${cmds})
set(bad 0)
set(skipped 0)
foreach(i RANGE 1 ${n})
	math(EXPR k "${i} - 1")
	list(GET DELAYS ${k} d)
	if(NOT EXISTS ${LOG}-${i}.rc)
		message("run ${i}: no exit status")
		set(bad 1)
		continue()
	endif()
	file(READ ${LOG}-${i}.log log)
	file(READ ${LOG}-${i}.err errlog)
	file(READ ${LOG}-${i}.rc rc)
	message("== run ${i} (started after ${d} s): exit status ${rc}\n${log}${errlog}")
	if(rc STREQUAL "77")
		set(skipped 1)
	elseif(NOT rc STREQUAL WANT)
		message("run ${i}: exit status ${rc}, expected ${WANT}")
		set(bad 1)
	endif()
endforeach()
if(bad)
	message(FATAL_ERROR "the runs at the same time disturbed each other")
endif()
if(skipped)
	cmake_language(EXIT 77)
endif()
message("parallel-runs: ${n} runs, each as alone")
