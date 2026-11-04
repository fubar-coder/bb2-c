# ctest inject_datestamp*: inject's option datestamp=YYYY-MM-DD
# (tests/bug-orig/amiga/inject.c) in Amiberry.  dsdays (a C program that
# prints what DateStamp gives) runs without inject, under inject with
# several dates (the first day, a leap day, a day in 2026, the start of a
# century that is a leap year), under inject with dates that are refused
# (a day that does not exist, before 1978, a wrong form), and once more
# without inject: the patch is gone.
#   cmake -DAMIGA_RUN=... -DOUTMATCH=... -DINJECT=... -DDSDAYS=...
#         -DEXPECT=FILE -DWORK=DIR [-DTIMEOUT=SEC] -P inject-datestamp.cmake
# The output (TEST:out.txt) is compared with EXPECT by outmatch, and the
# date before and after the patch must be the same day (the system's).
# Exit status 0 if it matches; 77 (skipped) without the emulator.
cmake_minimum_required(VERSION 3.29)

foreach(v AMIGA_RUN OUTMATCH INJECT DSDAYS EXPECT WORK)
	if(NOT ${v})
		message(FATAL_ERROR "inject-datestamp.cmake: -D${v}=... missing")
	endif()
endforeach()
if(NOT TIMEOUT)
	set(TIMEOUT 120)
endif()

set(test ${WORK}/test)
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${test})
file(COPY_FILE ${INJECT} ${test}/inject)
file(COPY_FILE ${DSDAYS} ${test}/dsdays)

set(script "FailAt 21\nEcho \"[plain]\" >TEST:out.txt\nTEST:dsdays >>TEST:out.txt\n")
foreach(d 1978-01-01 2024-02-29 2000-02-29 2100-03-01 2026-10-25
		2026-02-29 1977-12-31 2026-13-01 2026-1-05 2026-10-25x 2026-00-10)
	string(APPEND script "Echo \"[${d}]\" >>TEST:out.txt\n"
		"TEST:inject datestamp=${d} TEST:dsdays >>TEST:out.txt\n"
		"Echo \"rc=$RC\" >>TEST:out.txt\n")
endforeach()
string(APPEND script "Echo \"[after]\" >>TEST:out.txt\nTEST:dsdays >>TEST:out.txt\n")
file(WRITE ${WORK}/run.script "${script}")
execute_process(COMMAND ${AMIGA_RUN} --timeout ${TIMEOUT} --dir ${WORK} ${WORK}/run.script
	RESULT_VARIABLE rc)
if(rc EQUAL 77)
	cmake_language(EXIT 77)
endif()
if(rc)
	message(STATUS "inject-datestamp.cmake: the emulator session did not end (${WORK})")
endif()
if(NOT EXISTS ${test}/out.txt)
	message(FATAL_ERROR "inject-datestamp.cmake: no output (${WORK})")
endif()

execute_process(COMMAND ${OUTMATCH} ${EXPECT} ${test}/out.txt
	RESULT_VARIABLE r ERROR_VARIABLE msg)
string(STRIP "${msg}" msg)
if(r)
	message(FATAL_ERROR "inject-datestamp.cmake: FAIL: ${msg}")
endif()
# the patch is gone afterwards: the day without inject is the same before
# and after (and is not one of the patched ones, which outmatch cannot say)
file(READ ${test}/out.txt text)
string(REGEX MATCH "\\[plain\\]\r?\ndays=([0-9]+) " m "${text}")
set(plain ${CMAKE_MATCH_1})
string(REGEX MATCH "\\[after\\]\r?\ndays=([0-9]+) " m "${text}")
set(after ${CMAKE_MATCH_1})
if(NOT plain STREQUAL after)
	message(FATAL_ERROR "inject-datestamp.cmake: FAIL: the day was ${plain} before and ${after} after inject")
endif()
message(STATUS "ok   inject datestamp (outputs in ${test}/out.txt)")
