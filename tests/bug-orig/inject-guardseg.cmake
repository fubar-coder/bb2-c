# ctest inject_guardseg*: inject's option guardseg (tests/bug-orig/amiga/inject.c)
# in Amiberry.  segwrite (a C program that writes zeros behind the end of its
# own hunk) runs without inject and writing nothing, under inject guardseg
# writing nothing, 4 bytes right behind the hunk and 4 bytes at the end of
# the 32 KB guardseg puts behind it (each reported), and under inject guard
# writing nothing (guard alone does not reach the program's hunk).
#   cmake -DAMIGA_RUN=... -DRESULTS=... -DINJECT=... -DSEGWRITE=...
#         -DEXPECT=FILE -DWORK=DIR [-DTIMEOUT=SEC] -P inject-guardseg.cmake
# The output (TEST:out.txt) is compared with EXPECT by results match.
# Exit status 0 if it matches; 77 (skipped) without the emulator.
cmake_minimum_required(VERSION 4.2.3)

foreach(v AMIGA_RUN RESULTS INJECT SEGWRITE EXPECT WORK)
	if(NOT ${v})
		message(FATAL_ERROR "inject-guardseg.cmake: -D${v}=... missing")
	endif()
endforeach()
if(NOT TIMEOUT)
	set(TIMEOUT 120)
endif()

set(test ${WORK}/test)
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${test})
file(COPY_FILE ${INJECT} ${test}/inject)
file(COPY_FILE ${SEGWRITE} ${test}/segwrite)

set(script "FailAt 21\nEcho \"[plain]\" >TEST:out.txt\nTEST:segwrite 0 0 >>TEST:out.txt\n")
foreach(c "guardseg|0 0" "guardseg|4 0" "guardseg|4 32764" "guard|0 0")
	string(REPLACE "|" ";" c "${c}")
	list(GET c 0 o)
	list(GET c 1 a)
	string(APPEND script "Echo \"[${o} ${a}]\" >>TEST:out.txt\n"
		"TEST:inject ${o} TEST:segwrite ${a} >>TEST:out.txt\n"
		"Echo \"rc=$RC\" >>TEST:out.txt\n")
endforeach()
file(WRITE ${WORK}/run.script "${script}")
execute_process(COMMAND ${AMIGA_RUN} --timeout ${TIMEOUT} --dir ${WORK} ${WORK}/run.script
	RESULT_VARIABLE rc)
if(rc EQUAL 77)
	cmake_language(EXIT 77)
endif()
if(rc)
	message(STATUS "inject-guardseg.cmake: the emulator session did not end (${WORK})")
endif()
if(NOT EXISTS ${test}/out.txt)
	message(FATAL_ERROR "inject-guardseg.cmake: no output (${WORK})")
endif()

execute_process(COMMAND ${RESULTS} match ${EXPECT} ${test}/out.txt
	RESULT_VARIABLE r ERROR_VARIABLE msg)
string(STRIP "${msg}" msg)
if(r)
	message(FATAL_ERROR "inject-guardseg.cmake: FAIL: ${msg}")
endif()
message(STATUS "ok   inject guardseg (outputs in ${test}/out.txt)")
