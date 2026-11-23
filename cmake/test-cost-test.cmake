# test-cost-test.cmake - checks the measured COST of cmake/test-cost.cmake:
# a project in WORK with the tests ted_ab and libs_emulator (in the list of
# the long tests), t_plain and t_measured, and t_x_prev, a test only a prev
# build has; its file of measured costs gives t_measured, libs_emulator and
# t_x_prev a COST (the measurement wins over the list), and the COST of each
# test is read back with ctest --show-only=json-v1.
#
#   cmake -DCOST=test-cost.cmake -DMODE=MODE -DWORK=DIR -P test-cost-test.cmake
# MODE (the project's PREV_REQUESTED: off, or on with "prev"):
#   ok           without a prev build: t_x_prev is not made, its entry is ignored
#   ok_prev      with one: t_x_prev is made and gets its COST
#   stale        without a prev build, the file also names tests that do not
#                exist (t_removed, a model-like m_x__c): they are ignored
#   stale_prev   the same with a prev build (t_x_prev made)
#   malformed    a line that is not "NAME COST" (broken input): the
#                configuration must stop naming its line
foreach(v COST WORK MODE)
	if(NOT ${v})
		message(FATAL_ERROR "test-cost-test.cmake: ${v} missing")
	endif()
endforeach()
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${WORK}/src)
set(costs "# measured\nlibs_emulator 9\nt_measured 3\nt_x_prev 6\n")
set(prev OFF)
if(MODE MATCHES "_prev$")
	set(prev ON)
endif()
if(MODE MATCHES "^stale")
	string(APPEND costs "m_x__c 2\nt_removed 5\n")
elseif(MODE STREQUAL "malformed")
	string(APPEND costs "t_removed five\n")
endif()
file(WRITE ${WORK}/src/costs.txt "${costs}")
file(WRITE ${WORK}/src/CMakeLists.txt "
cmake_minimum_required(VERSION 3.20)
project(cost NONE)
enable_testing()
include(${COST})
test_costs_load(\${CMAKE_SOURCE_DIR}/costs.txt)
foreach(t ted_ab libs_emulator t_plain t_measured)
	add_test(NAME \${t} COMMAND \${CMAKE_COMMAND} -E true)
endforeach()
# a test of the prev comparison (cmake/prev.cmake)
if(${prev})
	add_test(NAME t_x_prev COMMAND \${CMAKE_COMMAND} -E true)
endif()
long_test_costs()
")
execute_process(COMMAND ${CMAKE_COMMAND} -S ${WORK}/src -B ${WORK}/bin
	RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
if(MODE STREQUAL "malformed")
	if(NOT rc)
		message(FATAL_ERROR "test-cost-test: a file with a malformed line configured")
	endif()
	if(NOT out MATCHES "costs\\.txt:5:[ \n]+a line is")  # CMake wraps long messages
		message(FATAL_ERROR "test-cost-test: the malformed line is not named:\n${out}")
	endif()
	message("test-cost-test: the malformed line stops the configuration")
	return()
endif()
if(rc)
	message(FATAL_ERROR "test-cost-test: configure failed:\n${out}")
endif()
if(out MATCHES "[Ww]arning|t_removed|m_x__c|t_x_prev")
	message(FATAL_ERROR "test-cost-test: the configuration reports an entry:\n${out}")
endif()
execute_process(COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${WORK}/bin --show-only=json-v1
	RESULT_VARIABLE rc OUTPUT_VARIABLE json ERROR_VARIABLE out)
if(rc)
	message(FATAL_ERROR "test-cost-test: ctest failed:\n${out}")
endif()
# the COST of each test, "-" for none
set(got "")
string(JSON n LENGTH "${json}" tests)
math(EXPR last "${n} - 1")
foreach(i RANGE ${last})
	string(JSON name GET "${json}" tests ${i} name)
	set(cost -)
	string(JSON np ERROR_VARIABLE e LENGTH "${json}" tests ${i} properties)
	if(NOT e AND np GREATER 0)
		math(EXPR plast "${np} - 1")
		foreach(p RANGE ${plast})
			string(JSON pname GET "${json}" tests ${i} properties ${p} name)
			if(pname STREQUAL "COST")
				string(JSON cost GET "${json}" tests ${i} properties ${p} value)
				string(REGEX REPLACE "\\.0+$" "" cost "${cost}")
			endif()
		endforeach()
	endif()
	string(APPEND got "${name}=${cost} ")
endforeach()
# ted_ab: the estimate of the list; libs_emulator: measured, wins over the list
set(want "ted_ab=280 libs_emulator=9 t_plain=- t_measured=3 ")
if(prev)
	string(APPEND want "t_x_prev=6 ")
endif()
if(NOT got STREQUAL want)
	message(FATAL_ERROR "test-cost-test: COST ${got}, expected ${want}")
endif()
message("test-cost-test: the measured COST is applied")
