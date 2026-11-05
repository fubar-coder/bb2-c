# The order of the bug-orig tests (label bug-orig, preset bug-orig).
# ctest with more than one job runs the tests that failed in its last run
# first, in the order of their declaration, before all others and without
# regard to COST; the bug-orig tests fail on purpose (a reproduced bug), so
# from the second run on they start in declaration order.  So that the long
# ones start first and the run does not end with one of them alone, they
# are declared in one place, sorted by COST (the session limit times the
# sessions: the time the test can take), the largest first, equal ones by
# name.  The COST stays for the first run (no failures known yet), where
# ctest sorts by it itself.
#
# bug_orig_add_test(NAME COST cost COMMAND args... [LABELS ...] [TIMEOUT s]
#                   [SKIP_RETURN_CODE rc] [FIXTURES_REQUIRED ...])
#   records a test of the current directory (its working directory: the
#   current binary directory, as add_test would give it); any directory
#   that is processed before tests/bug-orig
# bug_orig_declare_tests()
#   declares the recorded tests in that order, with these properties
#   (tests/bug-orig/CMakeLists.txt, at its end; no test can be recorded
#   after it).  The tests are tests of that directory.

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

function(bug_orig_declare_tests)
	set_property(GLOBAL PROPERTY BUG_ORIG_DECLARED ON)
	get_property(names GLOBAL PROPERTY BUG_ORIG_TESTS)
	# the sort key: 9999999 - COST (seven digits, ascending = the largest
	# COST first), then the name
	set(keys "")
	foreach(name IN LISTS names)
		get_property(cost GLOBAL PROPERTY BUG_ORIG_TEST_${name}_COST)
		math(EXPR inv "9999999 - ${cost}")
		string(LENGTH "${inv}" n)
		while(n LESS 7)
			string(PREPEND inv 0)
			math(EXPR n "${n} + 1")
		endwhile()
		list(APPEND keys "${inv}|${name}")
	endforeach()
	list(SORT keys)
	foreach(key IN LISTS keys)
		string(REGEX REPLACE "^[0-9]+\\|" "" name "${key}")
		foreach(k DIR COST TIMEOUT SKIP_RETURN_CODE COMMAND LABELS FIXTURES_REQUIRED)
			get_property(${k} GLOBAL PROPERTY BUG_ORIG_TEST_${name}_${k})
		endforeach()
		add_test(NAME ${name} COMMAND ${COMMAND} WORKING_DIRECTORY ${DIR})
		set_tests_properties(${name} PROPERTIES COST ${COST} LABELS "${LABELS}")
		foreach(k TIMEOUT SKIP_RETURN_CODE FIXTURES_REQUIRED)
			if(NOT "${${k}}" STREQUAL "")
				set_tests_properties(${name} PROPERTIES ${k} "${${k}}")
			endif()
		endforeach()
	endforeach()
endfunction()
