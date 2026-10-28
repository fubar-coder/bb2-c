# libs_tests in Amiberry (bb2 libs-c/tools/amiga_test.py):
#   cmake -DAMIGA_RUN=... -DBIGLIB=... -DPROGRAM=libs_tests -DLIBS=DIR
#         -DORIGINALS=acidlibs-of-the-CD -DWORK=DIR [-DTIMEOUT=SEC]
#         [-DFILTER=snitch-filter] -P emulator.cmake
# TEST: (WORK/test) gets libs_tests, every DIR/NAME.obj and the library
# with the same number from ORIGINALS as NAME.orig.  The exit status is
# that of libs_tests; 77 (skipped) without the CD or the emulator.
cmake_minimum_required(VERSION 3.29)

foreach(v AMIGA_RUN BIGLIB PROGRAM LIBS ORIGINALS WORK)
	if(NOT ${v})
		message(FATAL_ERROR "emulator.cmake: -D${v}=... missing")
	endif()
endforeach()
if(NOT TIMEOUT)
	set(TIMEOUT 120)
endif()
if(NOT EXISTS ${ORIGINALS})
	message(STATUS "libs_emulator: no ${ORIGINALS} - skipped (fetch-originals)")
	cmake_language(EXIT 77)
endif()

set(test ${WORK}/test)
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${test})
file(COPY_FILE ${PROGRAM} ${test}/libs_tests)
file(GLOB objs ${LIBS}/*.obj)
foreach(obj IN LISTS objs)
	get_filename_component(name ${obj} NAME_WE)
	file(COPY_FILE ${obj} ${test}/${name}.obj)
	execute_process(COMMAND ${BIGLIB} extract ${ORIGINALS} ${obj} ${test}/${name}.orig
		RESULT_VARIABLE rc)
	if(rc)
		message(FATAL_ERROR "libs_emulator: no original of ${name}")
	endif()
endforeach()

set(args "")
if(FILTER)
	set(args " \"${FILTER}\"")
endif()
file(WRITE ${WORK}/run.script "TEST:libs_tests${args} >TEST:libs_tests.txt\n"
	"Echo \"rc=$RC\" >>TEST:libs_tests.txt\n")
execute_process(COMMAND ${AMIGA_RUN} --timeout ${TIMEOUT} --dir ${WORK} ${WORK}/run.script
	RESULT_VARIABLE rc)
if(rc EQUAL 77)
	cmake_language(EXIT 77)
endif()
if(EXISTS ${test}/libs_tests.txt)
	file(READ ${test}/libs_tests.txt out)
	message("${out}")
endif()
if(rc)
	message(FATAL_ERROR "libs_emulator: the emulator run failed (${WORK})")
endif()
if(NOT out MATCHES "rc=([0-9]+)[\r\n]*$")
	message(FATAL_ERROR "libs_emulator: no result in ${test}/libs_tests.txt")
endif()
if(NOT CMAKE_MATCH_1 EQUAL 0)
	message(FATAL_ERROR "libs_emulator: libs_tests returned ${CMAKE_MATCH_1}")
endif()
