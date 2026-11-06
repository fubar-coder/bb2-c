# ctest inject_fill*: inject's option fill=XX (tests/bug-orig/amiga/inject.c)
# in Amiberry.  memfill (a C program that prints what a block of AllocMem
# holds, without and with MEMF_CLEAR) runs without inject, under inject with
# several patterns (upper and lower case digits), with pad (the 64 bytes
# behind stay zeros), with patterns that are refused (too short, too long,
# not hex), and once more without inject.
#   cmake -DAMIGA_RUN=... -DOUTMATCH=... -DINJECT=... -DMEMFILL=...
#         -DEXPECT=FILE -DWORK=DIR [-DTIMEOUT=SEC] -P inject-fill.cmake
# The output (TEST:out.txt) is compared with EXPECT by outmatch.
# Exit status 0 if it matches; 77 (skipped) without the emulator.
cmake_minimum_required(VERSION 4.2.3)

foreach(v AMIGA_RUN OUTMATCH INJECT MEMFILL EXPECT WORK)
	if(NOT ${v})
		message(FATAL_ERROR "inject-fill.cmake: -D${v}=... missing")
	endif()
endforeach()
if(NOT TIMEOUT)
	set(TIMEOUT 120)
endif()

set(test ${WORK}/test)
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${test})
file(COPY_FILE ${INJECT} ${test}/inject)
file(COPY_FILE ${MEMFILL} ${test}/memfill)

set(script "FailAt 21\nEcho \"[plain]\" >TEST:out.txt\nTEST:memfill >>TEST:out.txt\n")
foreach(o "fill=a5" "fill=00" "fill=Ff" "fill=5a pad" "fill=" "fill=a" "fill=a5a" "fill=g0"
		"fill=-1")
	if(o STREQUAL "fill=5a pad")
		set(arg " behind")
	else()
		set(arg "")
	endif()
	string(APPEND script "Echo \"[${o}]\" >>TEST:out.txt\n"
		"TEST:inject ${o} TEST:memfill${arg} >>TEST:out.txt\n"
		"Echo \"rc=$RC\" >>TEST:out.txt\n")
endforeach()
string(APPEND script "Echo \"[after]\" >>TEST:out.txt\nTEST:memfill >>TEST:out.txt\n")
file(WRITE ${WORK}/run.script "${script}")
execute_process(COMMAND ${AMIGA_RUN} --timeout ${TIMEOUT} --dir ${WORK} ${WORK}/run.script
	RESULT_VARIABLE rc)
if(rc EQUAL 77)
	cmake_language(EXIT 77)
endif()
if(rc)
	message(STATUS "inject-fill.cmake: the emulator session did not end (${WORK})")
endif()
if(NOT EXISTS ${test}/out.txt)
	message(FATAL_ERROR "inject-fill.cmake: no output (${WORK})")
endif()

execute_process(COMMAND ${OUTMATCH} ${EXPECT} ${test}/out.txt
	RESULT_VARIABLE r ERROR_VARIABLE msg)
string(STRIP "${msg}" msg)
if(r)
	message(FATAL_ERROR "inject-fill.cmake: FAIL: ${msg}")
endif()
message(STATUS "ok   inject fill (outputs in ${test}/out.txt)")
