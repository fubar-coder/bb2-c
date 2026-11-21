# ctest inject_openwindow*: inject's option openwindowfail=N[/COUNT]
# (tests/bug-orig/amiga/inject.c) in Amiberry.  winopen (a C program that
# opens four windows, three with OpenWindow and the last with
# OpenWindowTagList, and says which failed) runs without inject, under
# inject with the first, a middle and the last window failing (the last is
# the OpenWindowTagList one), with two in a row, with a number past the
# windows asked for, and with options that are refused (0, a count of 0, a
# wrong form), and once more without inject: the patch is gone.
#   cmake -DAMIGA_RUN=... -DRESULTS=... -DINJECT=... -DWINOPEN=...
#         -DEXPECT=FILE -DWORK=DIR [-DTIMEOUT=SEC] -P inject-openwindow.cmake
# The output (TEST:out.txt) is compared with EXPECT by results match.
# Exit status 0 if it matches; 77 (skipped) without the emulator.
cmake_minimum_required(VERSION 4.3)

foreach(v AMIGA_RUN RESULTS INJECT WINOPEN EXPECT WORK)
	if(NOT ${v})
		message(FATAL_ERROR "inject-openwindow.cmake: -D${v}=... missing")
	endif()
endforeach()
if(NOT TIMEOUT)
	set(TIMEOUT 120)
endif()

set(test ${WORK}/test)
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${test})
file(COPY_FILE ${INJECT} ${test}/inject)
file(COPY_FILE ${WINOPEN} ${test}/winopen)

set(script "FailAt 21\nEcho \"[plain]\" >TEST:out.txt\nTEST:winopen >>TEST:out.txt\n")
foreach(o 1 2 4 2/2 3/9 5 0 2/0 x 2/x 2/1x)
	string(APPEND script "Echo \"[${o}]\" >>TEST:out.txt\n"
		"TEST:inject openwindowfail=${o} TEST:winopen >>TEST:out.txt\n"
		"Echo \"rc=$RC\" >>TEST:out.txt\n")
endforeach()
string(APPEND script "Echo \"[after]\" >>TEST:out.txt\nTEST:winopen >>TEST:out.txt\n")
file(WRITE ${WORK}/run.script "${script}")
execute_process(COMMAND ${AMIGA_RUN} --timeout ${TIMEOUT} --dir ${WORK} ${WORK}/run.script
	RESULT_VARIABLE rc)
if(rc EQUAL 77)
	cmake_language(EXIT 77)
endif()
if(rc)
	message(STATUS "inject-openwindow.cmake: the emulator session did not end (${WORK})")
endif()
if(NOT EXISTS ${test}/out.txt)
	message(FATAL_ERROR "inject-openwindow.cmake: no output (${WORK})")
endif()

execute_process(COMMAND ${RESULTS} match ${EXPECT} ${test}/out.txt
	RESULT_VARIABLE r ERROR_VARIABLE msg)
string(STRIP "${msg}" msg)
if(r)
	message(FATAL_ERROR "inject-openwindow.cmake: FAIL: ${msg}")
endif()
message(STATUS "ok   inject openwindowfail (outputs in ${test}/out.txt)")
