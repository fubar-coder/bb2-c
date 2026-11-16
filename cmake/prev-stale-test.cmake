# Test of cmake/prev.cmake: a leftover prev directory (from an earlier
# BLITZ2_PREV build) must not be compared when BLITZ2_PREV is not set.
#   cmake -DRESULTS=results -DWORK=DIR [-DBROKEN=ON] -P prev-stale-test.cmake
# BROKEN=ON hands the leftover directory itself to results prev, as the
# old setup did: the leftover differs, so this fails (the test of the test).
foreach(v RESULTS WORK)
	if(NOT ${v})
		message(FATAL_ERROR "prev-stale-test: -D${v}= missing")
	endif()
endforeach()
file(REMOVE_RECURSE ${WORK})
set(CMAKE_BINARY_DIR ${WORK}/build)
file(WRITE ${CMAKE_BINARY_DIR}/libs/acidlibs/2dlib.obj "current")
file(WRITE ${CMAKE_BINARY_DIR}/prev/libs/acidlibs/2dlib.obj "stale leftover")
set(BLITZ2_PREV "")
include(${CMAKE_CURRENT_LIST_DIR}/prev.cmake)
if(BROKEN)
	set(dir ${PREV_DIR})
else()
	set(dir ${PREV_RESULTS_DIR})
endif()
execute_process(COMMAND ${RESULTS} prev ${CMAKE_BINARY_DIR} ${dir}
	RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
message("${out}")
if(NOT rc EQUAL 77)
	message(FATAL_ERROR "prev-stale-test: results prev exited ${rc}, expected 77 (skipped)")
endif()
