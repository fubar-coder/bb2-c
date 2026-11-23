# ctest testreg_tool: testreg on sample directories - the ctest include
# file (names, sides, labels, time limits, cost, fixtures), the depfile, and
# every check with broken input: a source of the wrong form, a case name
# that is a token, a bad configuration, a query of a check the source does
# not have, a case selector of a case it does not have, a test id that is
# not kebab-case, a name registered twice; then a sample CMake project
# whose ctest -N lists the registered tests after the build, and again
# after a tests.toml changed (the step depends on the files it read).
#   cmake -DTESTREG=... -DCMAKE_GENERATOR=... -DCTEST=... -DWORK=DIR -P tooltest.cmake
file(REMOVE_RECURSE ${WORK})
set(s ${WORK}/sample)
file(MAKE_DIRECTORY ${s}/libs/sub ${s}/other)
file(WRITE ${WORK}/env.toml "top = \"${s}\"\n[placeholders]\nTESTDATA = \"/td\"\n")
file(WRITE ${WORK}/tokens.txt "8001 NEWTYPE\n8002 End\n\n# a comment\nC001 Free\n")
file(WRITE ${s}/tests.toml "root = true\ntimeout = 60\nexclude = [\"compiler=orig195,libs=clib\"]\n"
	"[matrix]\ncompiler = [\"port\", \"orig195\"]\nlibs = [\"cd\", \"clib\"]\n")
file(WRITE ${s}/libs/tests.toml "labels = [\"libs\"]\ncost = 3\nfixtures = [\"testdata\"]\n")
# an open bug: case behind has a declared deviation, case plain has none
file(WRITE ${s}/libs/shape-frames.test.bas
	"!TestCase{behind}\n  !CheckEq{1,1,\"width\"}\n!EndTestCase\n!TestCase{plain}\n  !Check{1}\n!EndTestCase\n")
file(WRITE ${s}/libs/shape-frames.test.toml
	"[deviations.\"libs=cd\"]\n\"check.behind.width\" = \"== fail\"\n")
# a fixed bug of a test without cells of the original
file(WRITE ${s}/libs/sub/fixed-one.test.bas "!TestCase{a}\n!EndTestCase\n")
file(WRITE ${s}/libs/sub/fixed-one.test.toml
	"fixed-orig = 12\ncompiler = \"port\"\nlibs = \"clib\"\ntimeout = 20\n"
	"[deviations.\"libs=cd\"]\n\"end.case\" = \"not exists\"\n")
# the test of an extension of the port: labelled like a fixed one (no bug-orig)
file(WRITE ${s}/libs/sub/ext-one.test.bas "!TestCase{a}\n  !CheckEq{1,1,\"loaded\"}\n!EndTestCase\n")
file(WRITE ${s}/libs/sub/ext-one.test.toml
	"extension = 13\n[deviations.\"libs=cd\"]\n\"check.a.loaded\" = \"== fail\"\n")

set(env --env ${WORK}/env.toml --tokens ${WORK}/tokens.txt)
function(run expect_rc)
	execute_process(COMMAND ${TESTREG} ${ARGN} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
	if(NOT rc EQUAL expect_rc)
		message(FATAL_ERROR "testreg ${ARGN}: exit ${rc}, expected ${expect_rc}:\n${out}${err}")
	endif()
	set(err "${err}" PARENT_SCOPE)
endfunction()
# refused(REGEX ARGS...): exit 1 with a message matching REGEX, no include file
function(refused regex)
	file(REMOVE ${WORK}/bad.cmake)
	run(1 --prefix bug_orig_ --output ${WORK}/bad.cmake ${env} --command run ${ARGN})
	if(NOT err MATCHES "${regex}")
		message(FATAL_ERROR "expected ${regex}, got:\n${err}")
	endif()
	if(EXISTS ${WORK}/bad.cmake)
		message(FATAL_ERROR "an include file after an error")
	endif()
endfunction()

# the include file and the depfile
run(0 --prefix bug_orig_ --output ${WORK}/tests.cmake --depfile ${WORK}/tests.d ${env}
	--timeout-extra 30 --command /x/corpusrun --command case
	${s}/libs/shape-frames.test.bas ${s}/libs/sub/fixed-one.test.bas ${s}/libs/sub/ext-one.test.bas)
file(READ ${WORK}/tests.cmake inc)
string(REPLACE "]==]" "" inc "${inc}")
string(REPLACE "[==[" "" inc "${inc}")
foreach(want
		"add_test( bug_orig_shape_frames__behind /x/corpusrun case --source ${s}/libs/shape-frames.test.bas --case behind --side port)"
		"set_tests_properties( bug_orig_shape_frames__behind PROPERTIES LABELS bug-orig;emulator;libs TIMEOUT 90 SKIP_RETURN_CODE 77 COST 3 FIXTURES_REQUIRED testdata)"
		"set_tests_properties( bug_orig_shape_frames__behind_originals PROPERTIES LABELS originals;libs TIMEOUT 150 SKIP_RETURN_CODE 77 COST 3 FIXTURES_REQUIRED testdata)"
		"set_tests_properties( bug_orig_shape_frames__plain PROPERTIES LABELS emulator;libs TIMEOUT 90"
		"--case plain --side originals)"
		"set_tests_properties( bug_orig_fixed_one__a PROPERTIES LABELS emulator;libs TIMEOUT 50 SKIP_RETURN_CODE 77 COST 3"
		"set_tests_properties( bug_orig_ext_one__a PROPERTIES LABELS emulator;libs TIMEOUT 90"
		"set_tests_properties( bug_orig_ext_one__a_originals PROPERTIES LABELS originals;libs")
	string(FIND "${inc}" "${want}" at)
	if(at LESS 0)
		message(FATAL_ERROR "the include file lacks\n  ${want}\nin\n${inc}")
	endif()
endforeach()
# no side without cells
string(FIND "${inc}" "fixed_one__a_originals" at)
if(NOT at LESS 0)
	message(FATAL_ERROR "a side without cells is registered:\n${inc}")
endif()
file(READ ${WORK}/tests.d dep)
foreach(want "${WORK}/tests.cmake:" "${s}/libs/shape-frames.test.bas" "${s}/libs/shape-frames.test.toml"
		"${s}/libs/tests.toml" "${s}/tests.toml" "${s}/libs/sub/fixed-one.test.toml" "${WORK}/env.toml"
		"${WORK}/tokens.txt")
	string(FIND "${dep}" "${want}" at)
	if(at LESS 0)
		message(FATAL_ERROR "the depfile lacks ${want}:\n${dep}")
	endif()
endforeach()

# --cell-fixture: the side with a cell of the selector requires the fixture
# (shape-frames' originals side has orig195,cd; fixed-one has no cell of it)
run(0 --prefix bug_orig_ --output ${WORK}/fx.cmake ${env} --command run
	--cell-fixture "compiler=orig195|originals_refbuild" --cell-fixture "libs=clib|clib_fixture"
	${s}/libs/shape-frames.test.bas ${s}/libs/sub/fixed-one.test.bas)
file(READ ${WORK}/fx.cmake inc)
string(REPLACE "]==]" "" inc "${inc}")
string(REPLACE "[==[" "" inc "${inc}")
foreach(want
		"set_tests_properties( bug_orig_shape_frames__behind_originals PROPERTIES LABELS originals;libs TIMEOUT 120 SKIP_RETURN_CODE 77 COST 3 FIXTURES_REQUIRED testdata;originals_refbuild)"
		"set_tests_properties( bug_orig_shape_frames__behind PROPERTIES LABELS bug-orig;emulator;libs TIMEOUT 60 SKIP_RETURN_CODE 77 COST 3 FIXTURES_REQUIRED testdata;clib_fixture)"
		"set_tests_properties( bug_orig_fixed_one__a PROPERTIES LABELS emulator;libs TIMEOUT 20 SKIP_RETURN_CODE 77 COST 3 FIXTURES_REQUIRED testdata;clib_fixture)")
	string(FIND "${inc}" "${want}" at)
	if(at LESS 0)
		message(FATAL_ERROR "--cell-fixture: the include file lacks\n  ${want}\nin\n${inc}")
	endif()
endforeach()
refused("--cell-fixture compiler=orig195 is not SELECTOR\\|FIXTURE" --cell-fixture "compiler=orig195"
	${s}/libs/sub/fixed-one.test.bas)
refused("--cell-fixture nokey=1\\|f: " --cell-fixture "nokey=1|f" ${s}/libs/sub/fixed-one.test.bas)

# a source given by a relative path: the test names it by its absolute path
execute_process(COMMAND ${TESTREG} --prefix p_ --output ${WORK}/rel.cmake ${env} --command run
	libs/sub/fixed-one.test.bas WORKING_DIRECTORY ${s} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
if(NOT rc EQUAL 0)
	message(FATAL_ERROR "a relative source: exit ${rc}\n${out}")
endif()
file(READ ${WORK}/rel.cmake inc)
string(FIND "${inc}" "[==[--source]==] [==[${s}/libs/sub/fixed-one.test.bas]==]" at)
if(at LESS 0)
	message(FATAL_ERROR "a relative source is not named by its absolute path:\n${inc}")
endif()

# broken input
file(WRITE ${s}/other/form.test.bas "!TestCase{a}\nx=1\n!TestCase{b}:y=5\n!EndTestCase\n")
refused("other/form.test.bas:3: the case a \\(line 1\\) has no !EndTestCase" ${s}/other/form.test.bas)
refused("other/form.test.bas:3: !TestCase must stand on a line of its own" ${s}/other/form.test.bas)
file(WRITE ${s}/other/macro.test.bas "Macro M\n  !CheckEq{a,1}\nEnd Macro\n!TestCase{a}\n!EndTestCase\n")
refused("macro.test.bas:2: !CheckEq inside the macro M \\(line 1\\)" ${s}/other/macro.test.bas)
file(WRITE ${s}/other/token.test.bas "!TestCase{free}\n!EndTestCase\n")
refused("token.test.bas:1: the case name free is the token Free" ${s}/other/token.test.bas)
file(WRITE ${s}/other/conf.test.bas "!TestCase{a}\n!EndTestCase\n")
file(WRITE ${s}/other/conf.test.toml "timout = 60\n")
refused("other/conf.test.toml:1: unknown key timout" ${s}/other/conf.test.bas)
file(WRITE ${s}/other/conf.test.toml "[asserts]\n\"check.a.width\" = \"== ok\"\n")
refused("other/conf.test.toml:2: check.a.width: the source has no check named width in a" ${s}/other/conf.test.bas)
file(WRITE ${s}/other/conf.test.toml
	"[ends.\"libs=cd\"]\nreason = \"r\"\nhangs = [\"session == timeout\", \"check.a.width == not-run\"]\n")
refused("other/conf.test.toml:3: check.a.width: the source has no check named width in a" ${s}/other/conf.test.bas)
file(WRITE ${s}/other/conf.test.toml "[ends.\"case=zz\"]\nreason = \"r\"\nhangs = [\"session == timeout\"]\n")
refused("the cell selector \"case=zz\" names the case zz, which the source does not have" ${s}/other/conf.test.bas)
file(WRITE ${s}/other/conf.test.toml "[asserts.\"case=zz\"]\nrc = \"== 1\"\n")
refused("the cell selector \"case=zz\" names the case zz, which the source does not have" ${s}/other/conf.test.bas)
file(WRITE ${s}/other/conf.test.toml "[matrix]\nasserts = [\"x\"]\n")
refused("other/conf.test.toml:2: asserts is no run setting and cannot be an axis" ${s}/other/conf.test.bas)
file(REMOVE ${s}/other/conf.test.toml)
file(WRITE ${s}/other/Bad_Name.test.bas "!TestCase{a}\n!EndTestCase\n")
refused("the test id Bad_Name is not lower-case kebab-case" ${s}/other/Bad_Name.test.bas)
file(MAKE_DIRECTORY ${s}/twice)
file(WRITE ${s}/twice/fixed-one.test.bas "!TestCase{a}\n!EndTestCase\n")
refused("the test bug_orig_fixed_one__a is registered twice"
	${s}/libs/sub/fixed-one.test.bas ${s}/twice/fixed-one.test.bas)
refused("cannot read" ${s}/other/missing.test.bas)
run(1 --prefix Bug --output ${WORK}/bad.cmake --command run ${s}/other/conf.test.bas)
if(NOT err MATCHES "the prefix \"Bug\" holds other characters")
	message(FATAL_ERROR "a bad prefix: ${err}")
endif()
file(REMOVE_RECURSE ${s}/twice ${s}/other)
# usage errors
run(2 --output ${WORK}/x.cmake --command run ${s}/libs/sub/fixed-one.test.bas)
run(2 --prefix p_ --output ${WORK}/x.cmake ${s}/libs/sub/fixed-one.test.bas)

# a sample CMake project: the include file of a build step, ctest -N lists
# the tests after the build, and the step reruns when a tests.toml changes
set(p ${WORK}/project)
file(MAKE_DIRECTORY ${p})
file(WRITE ${p}/CMakeLists.txt "cmake_minimum_required(VERSION 3.20)
project(sample NONE)
enable_testing()
file(GLOB sources CONFIGURE_DEPENDS ${s}/*/*.test.bas ${s}/*/*/*.test.bas)
set(inc \${CMAKE_BINARY_DIR}/blitz-tests.cmake)
add_custom_command(OUTPUT \${inc}
	COMMAND ${TESTREG} --prefix regression_ --output \${inc} --depfile \${inc}.d
		--env ${WORK}/env.toml --command run \${sources}
	DEPFILE \${inc}.d)
add_custom_target(blitz_tests ALL DEPENDS \${inc})
set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES \${inc})
")
execute_process(COMMAND ${CMAKE_COMMAND} -S ${p} -B ${p}/build -G "${CMAKE_GENERATOR}"
	RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
if(NOT rc EQUAL 0)
	message(FATAL_ERROR "the sample project does not configure:\n${out}")
endif()
function(build_and_list want_count)
	execute_process(COMMAND ${CMAKE_COMMAND} --build ${p}/build RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
	if(NOT rc EQUAL 0)
		message(FATAL_ERROR "the sample project does not build:\n${out}")
	endif()
	execute_process(COMMAND ${CTEST} -N ${ARGN} WORKING_DIRECTORY ${p}/build OUTPUT_VARIABLE list ERROR_VARIABLE list)
	if(NOT list MATCHES "Total Tests: ${want_count}\n")
		message(FATAL_ERROR "ctest -N ${ARGN}: not ${want_count} tests:\n${list}")
	endif()
	set(list "${list}" PARENT_SCOPE)
endfunction()
build_and_list(7)
foreach(name regression_shape_frames__behind regression_shape_frames__behind_originals
		regression_shape_frames__plain regression_shape_frames__plain_originals regression_fixed_one__a
		regression_ext_one__a regression_ext_one__a_originals)
	if(NOT list MATCHES "Test +#[0-9]+: ${name}\n")
		message(FATAL_ERROR "ctest -N lacks ${name}:\n${list}")
	endif()
endforeach()
build_and_list(0 -L sample)
file(WRITE ${s}/libs/tests.toml "labels = [\"libs\", \"sample\"]\ncost = 3\n")
build_and_list(7 -L sample)
file(REMOVE_RECURSE ${WORK})
message("testreg_tool: all cases as expected")
