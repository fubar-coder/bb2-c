# The order of the bug-orig tests (label bug-orig, preset bug-orig).
# ctest with more than one job runs the tests that failed in its last run
# first, in the order of their declaration, before all others and without
# regard to COST; the bug-orig tests fail on purpose (a reproduced bug), so
# from the second run on they start in declaration order.  So that the long
# ones start first and the run does not end with one of them alone, they
# are declared in one place, sorted by COST (the session limit times the
# sessions: the time the test can take), the largest first, equal ones by
# name.  The COST stays for the first run (no failures known yet), where
# ctest sorts by it itself.  The tests of the BASIC test model of an open
# bug (label bug-orig, a COST from tools/testreg: the sum of their cells'
# session limits) are known only to the build step that registers them, so
# that step declares all of them: the tests recorded here and its own, in
# one sorted list in its ctest include file (testreg --order FILE).
#
# bug_orig_add_test(NAME COST cost COMMAND args... [LABELS ...] [TIMEOUT s]
#                   [SKIP_RETURN_CODE rc] [FIXTURES_REQUIRED ...])
#   records a test of the current directory (its working directory: the
#   current binary directory, as add_test would give it); any directory
#   that is processed before tests/bug-orig
# bug_orig_declare_tests(FILE)
#   writes the recorded tests to FILE (at the end of the top directory, with
#   the environment emulation_test_environment gives them), the order file
#   that basic_test_register(... ORDER FILE) hands to testreg
#   (tests/bug-orig/CMakeLists.txt; no test can be recorded after it).  The
#   tests are tests of the directory of that registration.

function(bug_orig_add_test name)
	cmake_parse_arguments(PARSE_ARGV 1 arg "" "COST;TIMEOUT;SKIP_RETURN_CODE"
		"COMMAND;LABELS;FIXTURES_REQUIRED")
	get_property(declared GLOBAL PROPERTY BUG_ORIG_DECLARED)
	if(declared)
		message(FATAL_ERROR "bug_orig_add_test(${name}): after bug_orig_declare_tests")
	endif()
	if(NOT arg_COMMAND OR NOT arg_COST MATCHES "^[0-9]+$" OR arg_UNPARSED_ARGUMENTS)
		message(FATAL_ERROR "bug_orig_add_test(${name}): needs COST (seconds) and COMMAND")
	endif()
	get_property(names GLOBAL PROPERTY BUG_ORIG_TESTS)
	if(name IN_LIST names)
		message(FATAL_ERROR "bug_orig_add_test(${name}): recorded twice")
	endif()
	set_property(GLOBAL APPEND PROPERTY BUG_ORIG_TESTS ${name})
	set_property(GLOBAL PROPERTY BUG_ORIG_TEST_${name}_DIR ${CMAKE_CURRENT_BINARY_DIR})
	foreach(k COST TIMEOUT SKIP_RETURN_CODE COMMAND LABELS FIXTURES_REQUIRED)
		set_property(GLOBAL PROPERTY BUG_ORIG_TEST_${name}_${k} "${arg_${k}}")
	endforeach()
endfunction()

function(bug_orig_declare_tests file)
	set_property(GLOBAL PROPERTY BUG_ORIG_DECLARED ON)
	set_property(GLOBAL PROPERTY BUG_ORIG_DECLARE_DIR ${CMAKE_CURRENT_BINARY_DIR})
	# at the end of the top directory: after emulation_snapshot_owner and
	# emulation_coverage, whose environment the tests get as if CMake had
	# declared them
	set_property(GLOBAL PROPERTY BUG_ORIG_ORDER_FILE ${file})
	# (a deferred call evaluates its arguments when it runs: the file by a property)
	cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL _bug_orig_write_declarations)
endfunction()

# the entries of FILE: per recorded test "@test NAME COST" and its declaration
function(_bug_orig_write_declarations)
	get_property(file GLOBAL PROPERTY BUG_ORIG_ORDER_FILE)
	get_property(names GLOBAL PROPERTY BUG_ORIG_TESTS)
	get_property(bin GLOBAL PROPERTY BUG_ORIG_DECLARE_DIR)
	# sorted, so that the file is a declaration of its own too (a ctest include
	# file); the sort key: 9999999 - COST (seven digits, ascending = the
	# largest COST first), then the name
	set(keys "")
	foreach(name IN LISTS names)
		get_property(cost GLOBAL PROPERTY BUG_ORIG_TEST_${name}_COST)
		# a measured COST wins over the estimate (cmake/test-cost.cmake)
		test_cost(cost ${name} ${cost})
		set_property(GLOBAL PROPERTY BUG_ORIG_TEST_${name}_COST ${cost})
		math(EXPR inv "9999999 - ${cost}")
		string(LENGTH "${inv}" n)
		while(n LESS 7)
			string(PREPEND inv 0)
			math(EXPR n "${n} + 1")
		endwhile()
		list(APPEND keys "${inv}|${name}")
	endforeach()
	list(SORT keys)
	string(CONCAT text "# the bug-orig tests of the old form, the largest COST first (cmake/bug-orig-order.cmake);\n"
		"# testreg declares them with the tests of the model of the label bug-orig\n")
	foreach(key IN LISTS keys)
		string(REGEX REPLACE "^[0-9]+\\|" "" name "${key}")
		foreach(k DIR COST TIMEOUT SKIP_RETURN_CODE COMMAND LABELS FIXTURES_REQUIRED)
			get_property(${k} GLOBAL PROPERTY BUG_ORIG_TEST_${name}_${k})
		endforeach()
		string(APPEND text "# @test ${name} ${COST}\nadd_test([==[${name}]==]")
		foreach(w IN LISTS COMMAND)
			string(APPEND text " [==[${w}]==]")
		endforeach()
		string(APPEND text ")\nset_tests_properties([==[${name}]==] PROPERTIES WORKING_DIRECTORY [==[${DIR}]==]"
			" COST ${COST} LABELS [==[${LABELS}]==]")
		foreach(k TIMEOUT SKIP_RETURN_CODE FIXTURES_REQUIRED)
			if(NOT "${${k}}" STREQUAL "")
				string(APPEND text " ${k} [==[${${k}}]==]")
			endif()
		endforeach()
		emulation_test_environment(env ${name} "${LABELS}" ${bin})
		if(env)
			string(APPEND text " ENVIRONMENT [==[${env}]==]")
		endif()
		string(APPEND text ")\n")
	endforeach()
	file(GENERATE OUTPUT ${file} CONTENT "${text}")
endfunction()
