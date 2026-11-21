# ctest wbrun_timeout*: wbrun's option timeout=SEC (tests/bug-orig/amiga/
# wbrun.c) in Amiberry.  wbtool (a C program without C startup, which takes
# the WBStartup message itself) replies at once, never, after 2 s or not
# before it ends after 4 s, as the name of its first project says; wbrun
# starts it under timeout= and must report: a reply in time (no extra line,
# `cd` shows the normal end), no reply and the process ended, no reply within
# the time and the process ended later (after replying or without), and
# refuse a timeout outside 1 to 60.  A start without timeout= and a start
# without a project (the tool ends at once without replying: the timeout is
# what ends the wait) are in it too.
#   cmake -DAMIGA_RUN=... -DRESULTS=... -DWBRUN=... -DWBTOOL=...
#         -DEXPECT=FILE -DWORK=DIR [-DTIMEOUT=SEC] -P wbrun-timeout.cmake
# The output (TEST:out.txt) is compared with EXPECT by results match.
# Exit status 0 if it matches; 77 (skipped) without the emulator.
cmake_minimum_required(VERSION 4.3)

foreach(v AMIGA_RUN RESULTS WBRUN WBTOOL EXPECT WORK)
	if(NOT ${v})
		message(FATAL_ERROR "wbrun-timeout.cmake: -D${v}=... missing")
	endif()
endforeach()
if(NOT TIMEOUT)
	set(TIMEOUT 120)
endif()

set(test ${WORK}/test)
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${test})
file(COPY_FILE ${WBRUN} ${test}/wbrun)
file(COPY_FILE ${WBTOOL} ${test}/wbtool)

# label|wbrun options and arguments
set(script "FailAt 21\n")
set(redir ">")
foreach(c
	"plain, no timeout|cd TEST:wbtool TEST:reply"
	"reply|timeout=3 cd TEST:wbtool TEST:reply"
	"silent|timeout=3 TEST:wbtool TEST:silent"
	"no project|timeout=3 TEST:wbtool"
	"late, in time|timeout=5 cd TEST:wbtool TEST:late"
	"late, too late|timeout=1 TEST:wbtool TEST:late"
	"slow|timeout=1 TEST:wbtool TEST:slow"
	"timeout 0|timeout=0 TEST:wbtool TEST:reply"
	"timeout 61|timeout=61 TEST:wbtool TEST:reply"
	"timeout x|timeout=x TEST:wbtool TEST:reply")
	string(REPLACE "|" ";" c "${c}")
	list(GET c 0 label)
	list(GET c 1 args)
	string(APPEND script "Echo \"[${label}]\" ${redir}TEST:out.txt\n"
		"TEST:wbrun ${args} >>TEST:out.txt\n"
		"Echo \"rc=$RC\" >>TEST:out.txt\n")
	set(redir ">>")
endforeach()
file(WRITE ${WORK}/run.script "${script}")
execute_process(COMMAND ${AMIGA_RUN} --timeout ${TIMEOUT} --dir ${WORK} ${WORK}/run.script
	RESULT_VARIABLE rc)
if(rc EQUAL 77)
	cmake_language(EXIT 77)
endif()
if(rc)
	message(STATUS "wbrun-timeout.cmake: the emulator session did not end (${WORK})")
endif()
if(NOT EXISTS ${test}/out.txt)
	message(FATAL_ERROR "wbrun-timeout.cmake: no output (${WORK})")
endif()

execute_process(COMMAND ${RESULTS} match ${EXPECT} ${test}/out.txt
	RESULT_VARIABLE r ERROR_VARIABLE msg)
string(STRIP "${msg}" msg)
if(r)
	message(FATAL_ERROR "wbrun-timeout.cmake: FAIL: ${msg}")
endif()
message(STATUS "ok   wbrun timeout (outputs in ${test}/out.txt)")
