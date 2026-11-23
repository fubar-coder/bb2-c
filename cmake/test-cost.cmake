# The COST of the tests, so that a ctest run that does not know their
# durations (a cold run: no Testing/Temporary/CTestCostData.txt yet; the
# bug-orig tests, which fail on purpose, always) starts the long ones first
# and does not end with one of them alone.  ctest sorts by COST in a first
# run and by its measured durations (CTestCostData.txt) in later ones.
#
# The measured COST (tests/data/test-costs.txt, written by testinfo costs
# from the JUnit files of a test run; tests/README.md, "The measured COST"):
# a line "NAME COST" per test, the median of the sum of its emulator
# sessions started warm.  A test with an entry gets that COST: a CMake test
# here, a bug-orig test of the old form in cmake/bug-orig-order.cmake, a
# test of the model in tools/testreg (--costs); a measurement replaces the
# estimates (the list below, a bug-orig test's session limit times its
# sessions, the sum of a model test's cells' session limits), which stay
# for the tests without an entry; a model test's `cost` key wins over its
# entry.
#
# test_costs_load(FILE)
#   reads FILE (at the start of the top-level CMakeLists.txt; a change of it
#   configures again): TEST_COSTS_FILE, and test_cost(VAR NAME DEFAULT) the
#   COST of NAME, else DEFAULT
# long_test_costs()
#   at the end of the top-level CMakeLists.txt, after all add_test: the
#   CMake tests with an entry get its COST, the long tests of the list below
#   without one its value; then the check of the file: an entry whose test
#   does not exist (no CMake test, no recorded bug-orig test, no test of the
#   model of a registered prefix: PREFIX...__CASE, which testreg checks, no
#   test of the prev build named by test_cost_prev_only without one) stops
#   the configuration, naming the file's line
# test_cost_prev_only(NAME)
#   NAME is a test that only a prev build has (cmake/prev.cmake: the
#   comparisons NAME_prev and compiler_prev, the model tests' PREFIX_ID_prev;
#   corpus_cd_prev, corpus_cd_ab3_prev, ab3_cli_prev_N), called where it is
#   made, before the test of PREV_REQUESTED: without a prev build its entry
#   is not stale (a file refreshed from a run with the prev build names it);
#   with one it does nothing, the test exists or its entry is stale

function(test_costs_load file)
	set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${file})
	set_property(GLOBAL PROPERTY TEST_COSTS_FILE ${file})
	set(names "")
	if(EXISTS ${file})
		file(STRINGS ${file} lines)
		set(n 0)
		foreach(l IN LISTS lines)
			math(EXPR n "${n} + 1")
			if(l STREQUAL "" OR l MATCHES "^#")
				continue()
			endif()
			if(NOT l MATCHES "^([A-Za-z0-9_.-]+) ([1-9][0-9]*)$")
				message(FATAL_ERROR "${file}:${n}: a line is \"NAME COST\" (COST seconds, at least 1): ${l}")
			endif()
			set_property(GLOBAL PROPERTY TEST_COST_${CMAKE_MATCH_1} ${CMAKE_MATCH_2})
			set_property(GLOBAL PROPERTY TEST_COST_LINE_${CMAKE_MATCH_1} ${n})
			list(APPEND names ${CMAKE_MATCH_1})
		endforeach()
	endif()
	set_property(GLOBAL PROPERTY TEST_COSTS_NAMES "${names}")
endfunction()

function(test_cost var name default)
	get_property(c GLOBAL PROPERTY TEST_COST_${name})
	if("${c}" STREQUAL "")
		set(c ${default})
	endif()
	set(${var} ${c} PARENT_SCOPE)
endfunction()

function(test_cost_prev_only name)
	if(NOT PREV_REQUESTED)
		set_property(GLOBAL APPEND PROPERTY TEST_COSTS_PREV_ONLY ${name})
	endif()
endfunction()

function(long_test_costs)
	# the estimates of the long tests without a measurement: the rounded
	# durations in seconds of one cost file (a full run of the all suite on a
	# cool machine, tests in parallel); only their order matters
	set(costs
		ted_ab 280
		ab3_bb2_equivalence_runerr 100
		libs_emulator 85
		ab3_bb2_equivalence 85
		corpus_cd 60
		corpus_cd_ab3 50
		corpus_req 40
		ab3_tokenise_broken 30
		corpus_serial_runerr 30
		ab3_tokenise 30)
	set(known "")
	set(dirs ${CMAKE_SOURCE_DIR})
	while(dirs)
		list(POP_FRONT dirs dir)
		get_property(sub DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
		list(APPEND dirs ${sub})
		get_property(tests DIRECTORY ${dir} PROPERTY TESTS)
		list(APPEND known ${tests})
		foreach(t IN LISTS tests)
			list(FIND costs ${t} i)
			set(cost "")
			if(i GREATER_EQUAL 0)
				math(EXPR i "${i} + 1")
				list(GET costs ${i} cost)
			endif()
			test_cost(cost ${t} "${cost}")
			if(NOT cost STREQUAL "")
				set_property(TEST ${t} DIRECTORY ${dir} PROPERTY COST ${cost})
			endif()
		endforeach()
	endwhile()
	# the check: every entry names a test
	get_property(file GLOBAL PROPERTY TEST_COSTS_FILE)
	get_property(names GLOBAL PROPERTY TEST_COSTS_NAMES)
	get_property(recorded GLOBAL PROPERTY BUG_ORIG_TESTS)
	get_property(prefixes GLOBAL PROPERTY BASIC_TEST_PREFIXES)
	get_property(prev_only GLOBAL PROPERTY TEST_COSTS_PREV_ONLY)
	list(APPEND known ${recorded} ${prev_only})
	set(stale "")
	foreach(name IN LISTS names)
		if(name IN_LIST known)
			continue()
		endif()
		set(model FALSE)
		foreach(p IN LISTS prefixes)
			string(FIND "${name}" "${p}" at)
			string(FIND "${name}" "__" sep)
			if(at EQUAL 0 AND sep GREATER 0)
				set(model TRUE)
			endif()
		endforeach()
		if(NOT model)
			get_property(line GLOBAL PROPERTY TEST_COST_LINE_${name})
			string(APPEND stale "\n  ${file}:${line}: ${name}")
		endif()
	endforeach()
	if(stale)
		message(FATAL_ERROR "the measured COST names tests that do not exist (refresh the file, "
			"tests/README.md \"The measured COST\"):${stale}")
	endif()
endfunction()
