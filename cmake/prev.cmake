# The `prev` build: the results of the previous commit, to compare the
# results of this build with (CLAUDE.md "Acceptance", tests/README.md
# "The prev build").  Not part of the default configure - it builds the
# project a second time.
#
#   -DBLITZ2_PREV=ON            HEAD~1 (the first parent)
#   -DBLITZ2_PREV=<revision>    any commit: HEAD~3, a hash, a branch
#   -DBLITZ2_PREV=<directory>   an existing build directory of another
#                               commit (absolute path, not built here)
#
# A revision is built from `git archive` in build/<preset>/prev/ (the
# results in prev/blitz2/), by the ExternalProject blitz2_prev, which is
# part of ALL; it follows the revision on every build (cmake/prev-build.cmake).
#
# -DPREV_INTENDED names the differences from the prev build that are
# intended (a fix names them in its commit): OFF (none), ON (every one), or
# a ;-list of entries, each of which must really differ (a stale entry
# fails like a difference; tests/README.md "The prev build"):
#   libs/acidlibs/2dlib.obj      a library file (prev_results), path relative
#                                to the build directory
#   corpus:NAME                  the source NAME in the pass corpus, likewise
#   corpus_runerr:NAME           corpus_req, corpus_serial,
#   ...                          corpus_serial_runerr
#   ted_ab:CASE[:C5-P5|:Cc-Pc]   the test CASE of ted_ab (one of its two pairs
#                                with P5/Pc, or either)
#   corpus_cd_prev:PATH          a source of the CD (corpus_cd_ab3_prev: likewise)
#   compiler_prev:ID             a compiler test source (tests/blitz2, the compiler's
#   bug_orig_ID_prev:ID          bug-orig tests: prev_compile_test below)
#   blitz2_ID_prev:ID            a test of the BASIC test model (ID.test.bas): its
#   blitz2_ID_prev:ID-runerrs    driver without, with runtime error checking
#                                (prev_compile_model_test below)
#   site_examples_run:NAME       an example of the site (a pass of tests/site-examples)
#   ab3_cli_prev:CASE            a case of ab3_cli (tests/ab3/cli-prev.cmake)
# The entries are checked by the tools (tools/lib/previntended).
#
# Sets PREV_DIR: the directory of the prev build, whether it exists or not
# (the tests that need it are skipped without it), PREV_BLITZ2_DIR and
# PREV_RESULTS_DIR (PREV_DIR if a prev build was requested, else a directory
# that does not exist: a leftover PREV_DIR is ignored).

set(BLITZ2_PREV "" CACHE STRING
	"build of the previous commit: ON (HEAD~1), a revision, or the path of a build directory")
set(PREV_DIR ${CMAKE_BINARY_DIR}/prev)
set(PREV_REQUESTED OFF)

if(BLITZ2_PREV)
	set(PREV_REQUESTED ON)
	if(IS_ABSOLUTE "${BLITZ2_PREV}" AND IS_DIRECTORY "${BLITZ2_PREV}")
		set(PREV_DIR ${BLITZ2_PREV})
	else()
		if(BLITZ2_PREV MATCHES "^(ON|TRUE|YES|Y|1)$")
			set(PREV_REV HEAD~1)
		else()
			set(PREV_REV ${BLITZ2_PREV})
		endif()
		execute_process(COMMAND git -C ${PROJECT_SOURCE_DIR} rev-parse --verify --quiet
			${PREV_REV}^{commit}
			OUTPUT_VARIABLE prev_commit OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE rc)
		if(rc OR NOT prev_commit)
			message(FATAL_ERROR "BLITZ2_PREV=${BLITZ2_PREV}: ${PREV_REV} is no commit of "
				"${PROJECT_SOURCE_DIR} (a directory is given as an absolute path)")
		endif()
		set(prev_args -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
			-DAMIGA_TOOLCHAIN_PREFIX=${AMIGA_TOOLCHAIN_PREFIX}
			-DHOST_SANITIZE=${HOST_SANITIZE})
		if(DEFINED CMAKE_C_FLAGS_DEBUG)
			list(APPEND prev_args "-DCMAKE_C_FLAGS_DEBUG=${CMAKE_C_FLAGS_DEBUG}")
		endif()
		if(BLITZ2_COVERAGE)
			list(APPEND prev_args -DBLITZ2_COVERAGE=ON)
		endif()
		# the list as one argument: ExternalProject would split a ;-list
		string(REPLACE ";" "|" prev_args "${prev_args}")
		# snitch: this build's source (the target snitch of libs/tests), with
		# the SHA256 this build pins (cmake/Snitch.cmake); prev-build.cmake
		# gives it to every configure of the prev build if that pins the same
		file(STRINGS ${PROJECT_SOURCE_DIR}/cmake/Snitch.cmake pin REGEX "URL_HASH +SHA256=")
		string(REGEX REPLACE ".*SHA256=([0-9a-fA-F]+).*" "\\1" pin "${pin}")
		# CLI11 the same way (cmake/CLI11.cmake, the source the top-level
		# configure fetched)
		file(STRINGS ${PROJECT_SOURCE_DIR}/cmake/CLI11.cmake cli11_pin REGEX "URL_HASH +SHA256=")
		string(REGEX REPLACE ".*SHA256=([0-9a-fA-F]+).*" "\\1" cli11_pin "${cli11_pin}")
		# the compiler launcher (cmake/ccache.cmake): ccache with base_dir
		# PREV_DIR (its sources and build directory), or the one this build
		# was given; | separates its entries (prev-build.cmake)
		set(prev_launcher "${CMAKE_C_COMPILER_LAUNCHER}")
		if(BB2C_CCACHE_OWN)
			bb2c_ccache_launcher(prev_launcher ${PREV_DIR} ${BB2C_CCACHE_DEBUG})
		endif()
		string(REPLACE ";" "|" prev_launcher "${prev_launcher}")
		ExternalProject_Add(blitz2_prev
			PREFIX ${CMAKE_BINARY_DIR}/prev-ep
			DOWNLOAD_COMMAND "" UPDATE_COMMAND "" CONFIGURE_COMMAND "" INSTALL_COMMAND ""
			BUILD_COMMAND ${CMAKE_COMMAND} -DREPO=${PROJECT_SOURCE_DIR} -DREV=${PREV_REV}
				-DPREV_DIR=${PREV_DIR} -DGENERATOR=${CMAKE_GENERATOR}
				"-DCONFIGURE_ARGS=${prev_args}" "-DLAUNCHER=${prev_launcher}" -DSNITCH_PIN=${pin}
				-DSNITCH_SOURCE_DIR=$<TARGET_PROPERTY:snitch,SOURCE_DIR>
				-DCLI11_PIN=${cli11_pin} -DCLI11_SOURCE_DIR=${cli11_SOURCE_DIR}
				-P ${PROJECT_SOURCE_DIR}/cmake/prev-build.cmake
			BUILD_ALWAYS ON
			USES_TERMINAL_BUILD ON
			BUILD_BYPRODUCTS ${PREV_DIR}/blitz2/blitz2 ${PREV_DIR}/blitz2/ted
				${PREV_DIR}/blitz2/acidlibs ${PREV_DIR}/blitz2/deflibs)
		message(STATUS "prev build: ${PREV_REV} = ${prev_commit} in ${PREV_DIR}")
	endif()
endif()
set(PREV_BLITZ2_DIR ${PREV_DIR}/blitz2)
# what prev_results compares: the prev build only if one was requested, so a
# leftover PREV_DIR from an earlier BLITZ2_PREV build is not compared
if(PREV_REQUESTED)
	set(PREV_RESULTS_DIR ${PREV_DIR})
else()
	set(PREV_RESULTS_DIR ${CMAKE_BINARY_DIR}/prev-not-requested)
endif()

set(PREV_INTENDED OFF CACHE STRING
	"differences from the prev build that are intended: ON (all) or a ;-list of library paths, corpus:NAME, ted_ab:CASE[:PAIR], corpus_cd_prev:PATH, compiler_prev:ID, ab3_cli_prev:CASE ...")
# for the runners: PREV_INTENDED_LINES = "intended=ENTRY\n" per entry of the
# corpus and ted_ab kind (with a colon) and ON, to put in a pass file;
# PREV_INTENDED_PREV_ARGS = the arguments of results prev (it also takes the
# entries with a colon, only to check their syntax)
set(PREV_INTENDED_LINES)
set(PREV_INTENDED_PREV_ARGS)
if(PREV_INTENDED STREQUAL "ON")
	set(PREV_INTENDED_LINES "intended=ON\n")
	set(PREV_INTENDED_PREV_ARGS --all)
elseif(PREV_INTENDED AND NOT PREV_INTENDED STREQUAL "OFF")
	foreach(f IN LISTS PREV_INTENDED)
		list(APPEND PREV_INTENDED_PREV_ARGS --intended ${f})
		if(f MATCHES ":")
			string(APPEND PREV_INTENDED_LINES "intended=${f}\n")
		endif()
	endforeach()
endif()

# ----------------------------------- the compile-only comparisons with prev
# The sources of tests/blitz2 and the bug-orig tests of the compiler (tests/bug-orig/
# compiler, the compiler's port regression tests) are compiled by the previous
# commit's compiler as well and its executable (or its error) must be the one this
# build's compiler writes: a corpus pass with run=off, the configurations clib and
# prev (exe:clib), no expectation.  What the program does is the test's own check;
# this one is for the executable and the compile result.
#
# prev_compile_test(NAME SOURCE [RUNTIME_ERRORS on|off] [CLIB_DIR DIR] [TIMEOUT SEC]
#                   [EXTRA_VAR VARIABLE]):  NAME = the ctest name of the test it belongs to,
#   SOURCE = the tokenised source (ID.bb2; ID is the name PREV_INTENDED entries use),
#   CLIB_DIR the libraries of this build (default BLITZ2_DIR), EXTRA_VAR a variable with lines of the test's
#   pass file that matter for a compile (wrap=, jobline=, file=, machine= ...: the ones
#   for a program run are dropped, a file= for clib is given to prev too).  A test
#   without extras and without runtime error checking joins the pass compiler_prev (one
#   session per configuration for all of them: prev_compile_plain_pass, called once at the
#   end), the others get NAME_prev.
# Without a prev build nothing is configured, as in the other functions here.
function(prev_compile_test name src)
	cmake_parse_arguments(PARSE_ARGV 2 arg "" "RUNTIME_ERRORS;CLIB_DIR;TIMEOUT;EXTRA_VAR" "")
	if(NOT arg_RUNTIME_ERRORS)
		set(arg_RUNTIME_ERRORS off)
	endif()
	if(NOT arg_CLIB_DIR)
		set(arg_CLIB_DIR ${BLITZ2_DIR})
	endif()
	if(NOT arg_TIMEOUT)
		set(arg_TIMEOUT 120)
	endif()
	# the lines for a compile
	set(extra "")
	set(mask_traps "")
	set(arg_EXTRA "")
	if(arg_EXTRA_VAR)
		set(arg_EXTRA "${${arg_EXTRA_VAR}}")
	endif()
	string(REPLACE ";" "<semicolon>" arg_EXTRA "${arg_EXTRA}")
	string(REPLACE "\n" ";" lines "${arg_EXTRA}")
	foreach(l IN LISTS lines)
		string(REPLACE "<semicolon>" ";" l "${l}")
		# a test with a Compile/Run job compiles with runtime error checking on (blitz2's own start
		# options, its defaults cannot be saved), the pass does not say so: the trap blocks of the
		# executables hold addresses of the compiler's memory and are masked (mask_traps=on)
		if(l MATCHES "^jobline=run=")
			set(mask_traps "mask_traps=on\n")
		endif()
		if(l STREQUAL "" OR l MATCHES "^(launcher|runwrap|runargs|launcherargs|hang_after|hang_from|gate|serial|rcput|expect|run)=")
			continue()
		endif()
		if(l MATCHES "^file=[^|]*\\|[^|]*\\|(ref|port)$")
			continue()
		endif()
		string(APPEND extra "${l}\n")
		if(l MATCHES "^(file=[^|]*\\|[^|]*)\\|clib$")
			string(APPEND extra "${CMAKE_MATCH_1}|prev\n")
		endif()
	endforeach()
	string(APPEND extra "${mask_traps}")
	get_filename_component(id ${src} NAME_WE)
	if(extra STREQUAL "" AND arg_RUNTIME_ERRORS STREQUAL "off" AND arg_CLIB_DIR STREQUAL BLITZ2_DIR)
		set_property(GLOBAL APPEND PROPERTY PREV_PLAIN_SOURCES ${src})
		return()
	endif()
	set(test ${name}_prev)
	if(NOT PREV_REQUESTED)
		return()
	endif()
	prev_register(${test})
	set(work ${CMAKE_BINARY_DIR}/prev-compile/${test})
	file(GENERATE OUTPUT ${work}.pass CONTENT
"# ${test}: generated by cmake/prev.cmake
test=${test}
work=${work}
report=${work}.txt
runtime_errors=${arg_RUNTIME_ERRORS}
timeout=${arg_TIMEOUT}
parallel=${CORPUS_PARALLEL}
ted=$<TARGET_FILE:tedstub>
rcput=$<TARGET_FILE:rcput>
run=off
config=clib|ref|$<TARGET_FILE:blitz2>|${arg_CLIB_DIR}/deflibs|${arg_CLIB_DIR}/acidlibs
config=prev|exe:clib|${PREV_BLITZ2_DIR}/blitz2|${PREV_BLITZ2_DIR}/deflibs|${PREV_BLITZ2_DIR}/acidlibs
${extra}source=${src}
${PREV_INTENDED_LINES}")
	add_test(NAME ${test} COMMAND ${CORPUSRUN} pass ${work}.pass)
	# two sessions, maybe one after the other, each maybe waiting for a slot
	math(EXPR limit "${arg_TIMEOUT} * 2 + 60 + 2 * ${EMULATION_QUEUE_WAIT}")
	set_tests_properties(${test} PROPERTIES LABELS emulator SKIP_RETURN_CODE 77 TIMEOUT ${limit})
endfunction()

# the pass compiler_prev of the sources prev_compile_test collected (after the last call)
set(PREV_COMPILE_TIMEOUT 600 CACHE STRING
	"compiler_prev: time limit of one emulator session (all the plain sources, seconds)")
function(prev_compile_plain_pass)
	get_property(sources GLOBAL PROPERTY PREV_PLAIN_SOURCES)
	if(NOT sources)
		return()
	endif()
	if(NOT PREV_REQUESTED)
		return()
	endif()
	set(work ${CMAKE_BINARY_DIR}/prev-compile/compiler_prev)
	string(CONCAT text "# compiler_prev: generated by cmake/prev.cmake\ntest=compiler_prev\nwork=${work}\n"
		"report=${work}.txt\nruntime_errors=off\ntimeout=${PREV_COMPILE_TIMEOUT}\n"
		"parallel=${CORPUS_PARALLEL}\nted=$<TARGET_FILE:tedstub>\nrun=off\n"
		"config=clib|ref|$<TARGET_FILE:blitz2>|${BLITZ2_DIR}/deflibs|${BLITZ2_DIR}/acidlibs\n"
		"config=prev|exe:clib|${PREV_BLITZ2_DIR}/blitz2|${PREV_BLITZ2_DIR}/deflibs|"
		"${PREV_BLITZ2_DIR}/acidlibs\n")
	foreach(s IN LISTS sources)
		string(APPEND text "source=${s}\n")
	endforeach()
	string(APPEND text "${PREV_INTENDED_LINES}")
	file(GENERATE OUTPUT ${work}.pass CONTENT "${text}")
	prev_register(compiler_prev)
	add_test(NAME compiler_prev COMMAND ${CORPUSRUN} pass ${work}.pass)
	math(EXPR limit "${PREV_COMPILE_TIMEOUT} * 2 + 60 + 2 * ${EMULATION_QUEUE_WAIT}")
	set_tests_properties(compiler_prev PROPERTIES LABELS emulator SKIP_RETURN_CODE 77 TIMEOUT ${limit})
endfunction()

# The comparison of a test of the BASIC test model (ID.test.bas, cmake/basic-test-model.cmake;
# change basic-test-runner D10): the driver of all its cases, which the build writes, compiled by
# this build's compiler and by the previous commit's, once per value of runtime-errors among the
# cells without original material, the executables (or compile results) the same: corpusrun case
# --prev (tests/corpus/runner/caserun.h), passes of corpus.h with run=off and the configurations
# clib and prev (exe:clib).  The compiles are named ID and ID-runerrs, the entries of PREV_INTENDED
# NAME:ID and NAME:ID-runerrs; an entry of NAME that names no compile of the test is stale.
#
# prev_compile_model_test(NAME SOURCE DRIVER):  NAME the ctest name (PREFIX_ID_prev), SOURCE
#   ID.test.bas, DRIVER its driver (basic_test_driver).  Without a prev build nothing is configured.
# prev_model_test_command(VAR NAME SOURCE DRIVER PREV_BLITZ2 PREV_LIBS): the command (also for the
#   self tests with another prev compiler, tests/blitz2/CMakeLists.txt), with the PREV_INTENDED
#   entries
function(prev_model_test_command var name src driver prev_blitz2 prev_libs)
	set(cmd ${CORPUSRUN} case --env ${BASIC_TEST_ENV} --source ${src} --prev --driver ${driver}
		--test ${name} --prev-blitz2 ${prev_blitz2} --prev-libs ${prev_libs})
	if(PREV_INTENDED STREQUAL "ON")
		list(APPEND cmd --intended ON)
	elseif(PREV_INTENDED AND NOT PREV_INTENDED STREQUAL "OFF")
		foreach(f IN LISTS PREV_INTENDED)
			if(f MATCHES ":")
				list(APPEND cmd --intended ${f})
			endif()
		endforeach()
	endif()
	set(${var} ${cmd} PARENT_SCOPE)
endfunction()
function(prev_compile_model_test name src driver)
	if(NOT PREV_REQUESTED)
		return()
	endif()
	prev_register(${name})
	prev_model_test_command(cmd ${name} ${src} ${driver} ${PREV_BLITZ2_DIR}/blitz2 ${PREV_BLITZ2_DIR})
	add_test(NAME ${name} COMMAND ${cmd})
	# two compiles (runtime error checking off and on), each two sessions, maybe one after the
	# other, each maybe waiting for a slot
	math(EXPR limit "${BASIC_TEST_TIMEOUT} * 4 + 60 + 4 * ${EMULATION_QUEUE_WAIT}")
	set_tests_properties(${name} PROPERTIES LABELS emulator SKIP_RETURN_CODE 77 TIMEOUT ${limit})
endfunction()

# The tests that compare with the prev build and take PREV_INTENDED entries (not the corpus
# passes and ted_ab, which the runners know): prev_register(NAME) where they are made;
# prev_check_entries (prev-check-entries.cmake) at the end of the top-level CMakeLists.txt
include(${CMAKE_CURRENT_LIST_DIR}/prev-check-entries.cmake)
function(prev_register name)
	set_property(GLOBAL APPEND PROPERTY PREV_TESTS ${name})
endfunction()
