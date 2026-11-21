# ctest inject_race*: inject's patch removal (tests/bug-orig/amiga/inject.c)
# in Amiberry.  inject's patches (AllocMem, FreeMem, OpenLibrary, Open, ...
# are system-wide) run from a block of memory that is never freed, and the
# removal waits until no task is inside any more (tests/corpus/amiga/
# patchblock.h).  Option racehold holds a helper process inside a patch
# while the patches are removed; segwrite (writes nothing) is the COMMAND.
# With RACE=racehold the removal must wait: no call inside after it, the
# helper returns.  With RACE=racehold=nowait the removal does not wait: the
# check must fail (1 call inside), as the broken input of the mechanism.
#   cmake -DAMIGA_RUN=... -DRESULTS=... -DINJECT=... -DSEGWRITE=...
#         -DEXPECT=FILE -DRACE=racehold[=nowait] -DWORK=DIR [-DTIMEOUT=SEC]
#         -P inject-race.cmake
# The output (TEST:out.txt) is compared with EXPECT by results match.
# Exit status 0 if it matches; 77 (skipped) without the emulator.
cmake_minimum_required(VERSION 4.3)

foreach(v AMIGA_RUN RESULTS INJECT SEGWRITE EXPECT RACE WORK)
	if(NOT ${v})
		message(FATAL_ERROR "inject-race.cmake: -D${v}=... missing")
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

file(WRITE ${WORK}/run.script "FailAt 21\n"
	"TEST:inject ${RACE} TEST:segwrite 0 0 >TEST:out.txt\n"
	"Echo \"rc=$RC\" >>TEST:out.txt\n")
execute_process(COMMAND ${AMIGA_RUN} --timeout ${TIMEOUT} --dir ${WORK} ${WORK}/run.script
	RESULT_VARIABLE rc)
if(rc EQUAL 77)
	cmake_language(EXIT 77)
endif()
if(rc)
	message(STATUS "inject-race.cmake: the emulator session did not end (${WORK})")
endif()
if(NOT EXISTS ${test}/out.txt)
	message(FATAL_ERROR "inject-race.cmake: no output (${WORK})")
endif()

execute_process(COMMAND ${RESULTS} match ${EXPECT} ${test}/out.txt
	RESULT_VARIABLE r ERROR_VARIABLE msg)
string(STRIP "${msg}" msg)
if(r)
	message(FATAL_ERROR "inject-race.cmake: FAIL: ${msg}")
endif()
message(STATUS "ok   inject race (outputs in ${test}/out.txt)")
