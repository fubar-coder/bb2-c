# test-cost-test.cmake - checks the measured COST of cmake/test-cost.cmake:
# a project in WORK with the tests ted_ab and libs_emulator (in the list of
# the long tests), t_plain and t_measured, a recorded bug-orig test and a
# test of the model of a registered prefix; its file of measured costs gives
# t_measured and libs_emulator a COST (the measurement wins over the list),
# and the COST of each test is read back with ctest --show-only=json-v1.
#
#   cmake -DCOST=test-cost.cmake [-DBROKEN=ON] -DWORK=DIR -P test-cost-test.cmake
# BROKEN=ON (the test of the check): the file names a test that does not
# exist, so the configuration must stop naming its line.
foreach(v COST WORK)
	if(NOT ${v})
		message(FATAL_ERROR "test-cost-test.cmake: ${v} missing")
	endif()
endforeach()
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${WORK}/src)
set(costs "# measured\nbug_orig_old 4\nlibs_emulator 9\nm_x__c 2\nt_measured 3\n")
if(BROKEN)
	string(APPEND costs "t_removed 5\n")
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
# a recorded bug-orig test of the old form, a registered prefix of the model
set_property(GLOBAL PROPERTY BUG_ORIG_TESTS bug_orig_old)
set_property(GLOBAL PROPERTY BASIC_TEST_PREFIXES m_)
long_test_costs()
")
execute_process(COMMAND ${CMAKE_COMMAND} -S ${WORK}/src -B ${WORK}/bin
	RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
if(BROKEN)
	if(NOT rc)
		message(FATAL_ERROR "test-cost-test: a file with a stale entry configured")
	endif()
	if(NOT out MATCHES "costs\\.txt:6: t_removed")
		message(FATAL_ERROR "test-cost-test: the stale entry is not named:\n${out}")
	endif()
	message("test-cost-test: the stale entry stops the configuration")
	return()
endif()
if(rc)
	message(FATAL_ERROR "test-cost-test: configure failed:\n${out}")
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
if(NOT got STREQUAL want)
	message(FATAL_ERROR "test-cost-test: COST ${got}, expected ${want}")
endif()
message("test-cost-test: the measured COST is applied")
