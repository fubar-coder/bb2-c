# Tests of blitz2's own behaviour: tests of the BASIC test model (ID.test.bas, tests/README.md
# "The test model"), registered below, and the self tests of the model (selftest/).
# A test of an extension of the port, where the original refuses or does something else, has
# `extension = N` in its ID.test.toml and the original's behaviour as declared deviations.
# literal-include.inc (text) is the include file of literal-include.test.bas: it is tokenised
# like TED does (tedsrc tok) into the test data (@TESTDATA@, tests/bug-orig/CMakeLists.txt)
# and staged as CORPUS:src/literal-include.inc by its `files`.
set(ted_tokens ${PROJECT_SOURCE_DIR}/tests/data/ted-tokens.txt)
set(blitz2_test_data ${CMAKE_BINARY_DIR}/tests/bug-orig/testdata/literal-include.inc)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/literal-include.inc)
add_custom_command(OUTPUT ${blitz2_test_data}
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/tests/bug-orig/testdata
	COMMAND ${TEDSRC} tok ${ted_tokens} ${CMAKE_CURRENT_SOURCE_DIR}/literal-include.inc ${blitz2_test_data}
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/literal-include.inc ${ted_tokens} ${TEDSRC} host_tools
	COMMENT "Tokenising tests/blitz2/literal-include.inc"
	VERBATIM)
add_custom_target(blitz2_test_sources ALL DEPENDS ${blitz2_test_data})
# the tests of the BASIC test model (ID.test.bas, cmake/basic-test-model.cmake): ctest
# blitz2_ID__CASE (and _originals for the cells with original material), registered by the build;
# with a prev build each is compared with it (blitz2_ID_prev, cmake/prev.cmake)
basic_test_register(blitz2_ ${CMAKE_CURRENT_SOURCE_DIR} "*.test.bas" PREV ".")
# the self test of the model: a text that does not compile on purpose is reported at its
# line of the test source (selftest/model-compile-error.test.bas, line 7), not at the line
# of the driver
math(EXPR limit "${BASIC_TEST_TIMEOUT} + 60 + ${EMULATION_QUEUE_WAIT}")
add_test(NAME blitz2_selftest_model_compile_error COMMAND ${CORPUSRUN} case --env ${BASIC_TEST_ENV}
	--source ${CMAKE_CURRENT_SOURCE_DIR}/selftest/model-compile-error.test.bas --case broken)
set_tests_properties(blitz2_selftest_model_compile_error PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
	TIMEOUT ${limit} PASS_REGULAR_EXPRESSION
	"compile error: model-compile-error\\.test\\.bas:7: [^\n]*\n.*model-compile-error\\.test\\.bas: case broken: FAILED")
# the self test of the checks of the compiled code of the intrinsic tests (intrinsic-*.test.bas
# count the JSR instructions around a statement): a wrong expectation (selftest/
# intrinsic-selftest-wrong.test.bas) must come out FAILED, the report naming the check with its
# expected and actual value
add_test(NAME blitz2_selftest_intrinsic_selftest_wrong COMMAND ${CORPUSRUN} case --env ${BASIC_TEST_ENV}
	--source ${CMAKE_CURRENT_SOURCE_DIR}/selftest/intrinsic-selftest-wrong.test.bas --case int_float)
set_tests_properties(blitz2_selftest_intrinsic_selftest_wrong PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
	TIMEOUT ${limit} PASS_REGULAR_EXPRESSION
	"check int_float int_f_calls \\(intrinsic-selftest-wrong\\.test\\.bas:23\\) failed: jsrs[^\n]*: expected 0 [^\n]*, actual 1 [^\n]*\nintrinsic-selftest-wrong\\.test\\.bas: case int_float: FAILED\n")
# the run of all cases of a source (corpusrun case without --case;
# selftest/model-all-cases.test.bas): each case's line, the cases judged on their own, the case
# whose verdict on its own differs from the shared run's, and the summary line; and the exit
# status of the worst verdict, failed (4: runner-exit.cmake, whose check for run directories
# left looks into a directory of its own, as the failed run stays in the model's work directory)
math(EXPR limit "6 * ${BASIC_TEST_TIMEOUT} + 60 + ${EMULATION_QUEUE_WAIT}")
set(model_all_cases ${CORPUSRUN} case --env ${BASIC_TEST_ENV} --source
	${CMAKE_CURRENT_SOURCE_DIR}/selftest/model-all-cases.test.bas --side port)
add_test(NAME blitz2_selftest_model_all_cases COMMAND ${model_all_cases})
set_tests_properties(blitz2_selftest_model_all_cases PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
	TIMEOUT ${limit} PASS_REGULAR_EXPRESSION
	"shared-0: cell port,clib did not end cleanly, the case ends open\n.*model-all-cases\\.test\\.bas: case pass1: PASSED\n.*model-all-cases\\.test\\.bas: case fails: FAILED \\(alone\\)\n.*model-all-cases\\.test\\.bas: case ends: PASSED \\(alone\\)\n.*model-all-cases\\.test\\.bas: case behind: PASSED\n.*model-all-cases\\.test\\.bas: case victim: PASSED \\(alone\\)\ncase victim: PASSED alone \\(FAILED in the run shared with pass1, fails, behind\\)\n.*model-all-cases\\.test\\.bas: case prints: PASSED \\(alone\\)\nmodel-all-cases\\.test\\.bas: 6 cases, side port: FAILED\n")
add_test(NAME blitz2_selftest_model_all_cases_exit COMMAND ${CMAKE_COMMAND} "-DCMD=${model_all_cases}" -DWANT=4
	"-DREGEX=model-all-cases\\.test\\.bas: 6 cases, side port: FAILED\n"
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/model-all-cases-exit
	-P ${PROJECT_SOURCE_DIR}/tests/corpus/runner/runner-exit.cmake)
set_tests_properties(blitz2_selftest_model_all_cases_exit PROPERTIES LABELS emulator TIMEOUT ${limit}
	PASS_REGULAR_EXPRESSION "runner-exit: as expected" SKIP_REGULAR_EXPRESSION "exit status 77, expected")
# the same on the side originals, where isolate = ["libs=cd"] gives port,cd and orig195,cd a
# session of their own: the cases share the runs in orig195,clib, each runs on its own in the
# isolated cells and is judged over all three, and a run on its own covers every cell
math(EXPR limit "24 * ${BASIC_TEST_TIMEOUT} + 60 + ${EMULATION_QUEUE_WAIT}")
add_test(NAME blitz2_selftest_model_all_cases_originals COMMAND ${CORPUSRUN} case --env ${BASIC_TEST_ENV}
	--source ${CMAKE_CURRENT_SOURCE_DIR}/selftest/model-all-cases.test.bas --side originals)
set_tests_properties(blitz2_selftest_model_all_cases_originals PROPERTIES LABELS originals SKIP_RETURN_CODE 77
	TIMEOUT ${limit} FIXTURES_REQUIRED originals_refbuild PASS_REGULAR_EXPRESSION
	"shared-0: cell orig195,clib did not end cleanly, the case ends open\n.*isolated-pass1: the case pass1 in the cells of a session of their own\ncorpusrun case: emulator session [0-9]+: c0\n@stat [^\n]*\ncorpusrun case: emulator session [0-9]+: c1\n.*model-all-cases\\.test\\.bas, case pass1, side originals: port,cd \\(isolated-pass1/c0\\) orig195,cd \\(isolated-pass1/c1\\) orig195,clib \\(shared-1/c0\\)\nmodel-all-cases\\.test\\.bas: case pass1: PASSED\n.*model-all-cases\\.test\\.bas: case fails: FAILED \\(alone\\)\n.*model-all-cases\\.test\\.bas: case ends: PASSED \\(alone\\)\n.*model-all-cases\\.test\\.bas: case behind: PASSED\n.*side originals: port,cd \\(alone-victim/c0\\) orig195,cd \\(alone-victim/c1\\) orig195,clib \\(alone-victim/c2\\)\nmodel-all-cases\\.test\\.bas: case victim: PASSED \\(alone\\)\ncase victim: PASSED alone \\(FAILED in the run shared with pass1, fails, behind\\)\n.*model-all-cases\\.test\\.bas: case prints: PASSED \\(alone\\)\nmodel-all-cases\\.test\\.bas: 6 cases, side originals: FAILED\n")
# isolate = "on" (selftest/model-isolate.test.bas): the two cells of the side port, which share
# the session settings, run in two sessions, one per cell
math(EXPR limit "2 * ${BASIC_TEST_TIMEOUT} + 60 + ${EMULATION_QUEUE_WAIT}")
add_test(NAME blitz2_selftest_model_isolate COMMAND ${CORPUSRUN} case --env ${BASIC_TEST_ENV}
	--source ${CMAKE_CURRENT_SOURCE_DIR}/selftest/model-isolate.test.bas --case own)
set_tests_properties(blitz2_selftest_model_isolate PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
	TIMEOUT ${limit} PASS_REGULAR_EXPRESSION
	"emulator session 0: c0\n@stat [^\n]* config=s0 [^\n]*\ncorpusrun case: emulator session 1: c1\n@stat [^\n]* config=s1 [^\n]*\n== model-isolate\\.test\\.bas, case own, side port: off \\(c0\\) on \\(c1\\)\nmodel-isolate\\.test\\.bas: case own: PASSED\n")
# a cell whose end varies (selftest/model-varying-end.test.bas): one cell returns, one hangs, and
# with both ends declared as alternatives each cell passes by its own; with one alternative
# removed (model-varying-end-one) the hanging cell fails, naming it
math(EXPR limit "${BASIC_TEST_TIMEOUT} + 60 + ${EMULATION_QUEUE_WAIT}")
foreach(variant "" -one)
	string(REPLACE "-" "_" v "${variant}")
	add_test(NAME blitz2_selftest_model_varying_end${v} COMMAND ${CORPUSRUN} case --env ${BASIC_TEST_ENV}
		--source ${CMAKE_CURRENT_SOURCE_DIR}/selftest/model-varying-end${variant}.test.bas --case varies)
endforeach()
set_tests_properties(blitz2_selftest_model_varying_end PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
	TIMEOUT ${limit} PASS_REGULAR_EXPRESSION
	"cell return [^\n]*\n  the end of \\[ends\\.\"compiler=port\"\\] [^\n]*: the alternative returns holds\ncase varies, cell hang [^\n]*\n  the end of \\[ends\\.\"compiler=port\"\\] [^\n]*: the alternative hangs holds\nmodel-varying-end\\.test\\.bas: case varies: REPRODUCED\n")
set_tests_properties(blitz2_selftest_model_varying_end_one PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
	TIMEOUT ${limit} PASS_REGULAR_EXPRESSION
	"cell hang [^\n]*\n  no alternative end of \\[ends\\.\"compiler=port\"\\] [^\n]* holds:\n    the alternative returns:\n      assertion session == done [^\n]* does not hold: \"timeout\"\n.*model-varying-end-one\\.test\\.bas: case varies: FAILED\n")
# an announced runtime error judged like a check (selftest/model-expect-error.test.bas): the
# port shows the announced error, the CD's libraries another one, which a deviation declares;
# both sides pass, and without the deviation (model-expect-error-removed) the side originals
# fails naming the announcement
math(EXPR limit "${BASIC_TEST_TIMEOUT} + 60 + ${EMULATION_QUEUE_WAIT}")
foreach(t "port|" "originals|_originals" "originals-removed|_removed")
	string(REPLACE "|" ";" t "${t}")
	list(GET t 0 side)
	list(GET t 1 suffix)
	set(src model-expect-error)
	if(side STREQUAL "originals-removed")
		set(side originals)
		set(src model-expect-error-removed)
	endif()
	add_test(NAME blitz2_selftest_model_expect_error${suffix} COMMAND ${CORPUSRUN} case --env ${BASIC_TEST_ENV}
		--source ${CMAKE_CURRENT_SOURCE_DIR}/selftest/${src}.test.bas --case depth_zero --side ${side})
	if(side STREQUAL "port")
		set(label emulator)
	else()
		set(label originals)
	endif()
	set_tests_properties(blitz2_selftest_model_expect_error${suffix} PROPERTIES LABELS ${label} SKIP_RETURN_CODE 77
		TIMEOUT ${limit})
endforeach()
set_tests_properties(blitz2_selftest_model_expect_error blitz2_selftest_model_expect_error_originals PROPERTIES
	PASS_REGULAR_EXPRESSION "model-expect-error\\.test\\.bas: case depth_zero: PASSED\n")
set_tests_properties(blitz2_selftest_model_expect_error_removed PROPERTIES PASS_REGULAR_EXPRESSION
	"case depth_zero, cell cd:\n.*  the announced runtime error \"Illegal Shape Depth\" \\(model-expect-error-removed\\.test\\.bas:[0-9]+\\) did not come\nmodel-expect-error-removed\\.test\\.bas: case depth_zero: FAILED\n")
# the test's own data (selftest/model-local-data.test.bas and its directory
# model-local-data.test/, change basic-test-local-data): a copied file and one made by mktestdata
# that a case reads, a data source compiled with each cell's build configuration and one with a
# fixed option (a compile part of its own in the cell without runtime error checking), both
# started by a case; a data source that does not compile (model-local-data-broken) makes the
# case BROKEN, the report naming the line of the data source
math(EXPR limit "${BASIC_TEST_TIMEOUT} + 60 + ${EMULATION_QUEUE_WAIT}")
add_test(NAME blitz2_selftest_model_local_data COMMAND ${CORPUSRUN} case --env ${BASIC_TEST_ENV}
	--source ${CMAKE_CURRENT_SOURCE_DIR}/selftest/model-local-data.test.bas --side port)
add_test(NAME blitz2_selftest_model_local_data_broken COMMAND ${CORPUSRUN} case --env ${BASIC_TEST_ENV}
	--source ${CMAKE_CURRENT_SOURCE_DIR}/selftest/model-local-data-broken.test.bas --case child)
set_tests_properties(blitz2_selftest_model_local_data blitz2_selftest_model_local_data_broken PROPERTIES
	LABELS emulator SKIP_RETURN_CODE 77 TIMEOUT ${limit})
set_tests_properties(blitz2_selftest_model_local_data PROPERTIES PASS_REGULAR_EXPRESSION
	"model-local-data\\.test\\.bas: case files: PASSED\n.*model-local-data\\.test\\.bas: case children: PASSED\nmodel-local-data\\.test\\.bas: 2 cases, side port: PASSED\n")
set_tests_properties(blitz2_selftest_model_local_data_broken PROPERTIES PASS_REGULAR_EXPRESSION
	"the data source model-local-data-broken\\.test/child\\.bas does not compile \\([^\n]*model-local-data-broken\\.test\\.toml:3\\): model-local-data-broken\\.test/child\\.bas:3: No Terminating Quote\nmodel-local-data-broken\\.test\\.bas: case child: BROKEN\n")
# its comparison with the prev build: the driver and the data sources (model-local-data/child.bas,
# model-local-data/child.bas-runerrs, model-local-data/errs.bas-runerrs) where the driver is compiled
set(model_ld_src ${CMAKE_CURRENT_SOURCE_DIR}/selftest/model-local-data.test.bas)
set(model_ld_drv ${CMAKE_CURRENT_BINARY_DIR}/model-drivers/model-local-data.bb2)
file(GLOB_RECURSE model_ld_data CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/selftest/model-local-data.test/*)
basic_test_driver(${model_ld_src} ${model_ld_drv} ${CMAKE_CURRENT_SOURCE_DIR}/selftest/model-local-data.test.toml
	${model_ld_data})
add_custom_target(blitz2_selftest_model_local_data_driver ALL DEPENDS ${model_ld_drv})
prev_compile_model_test(blitz2_selftest_model_local_data_prev ${model_ld_src} ${model_ld_drv})
# and with broken input: hangstub as the prev compiler (every compile differs) and the driver's
# compiles and errs.bas named in PREV_INTENDED: the test fails naming the data source child.bas
set(name blitz2_selftest_model_local_data_differs_prev)
add_test(NAME ${name} COMMAND ${CORPUSRUN} case --env ${BASIC_TEST_ENV} --source ${model_ld_src} --prev
	--driver ${model_ld_drv} --test ${name} --prev-blitz2 $<TARGET_FILE:hangstub> --prev-libs ${BLITZ2_DIR}
	--intended ${name}:model-local-data --intended ${name}:model-local-data-runerrs
	--intended ${name}:model-local-data/errs.bas-runerrs)
math(EXPR limit "30 * 2 + 120 + 2 * ${EMULATION_QUEUE_WAIT}")
set_tests_properties(${name} PROPERTIES LABELS emulator SKIP_RETURN_CODE 77 TIMEOUT ${limit}
	PASS_REGULAR_EXPRESSION "model-local-data/child\\.bas compiles; prev: RESULT DIFFERS [^\n]*, NO EXE\n.*model-local-data/child\\.bas-runerrs compiles; prev: RESULT DIFFERS [^\n]*, NO EXE\n.*model-local-data: the comparison with the prev build: DIFFERS\n")
# a case starts a program (selftest/model-run-program.test.bas, change basic-test-local-data):
# its return code, output (cut at 1024 bytes) and the one result of two runs checked in BASIC, a
# child's requester and a child that hangs judged by the configuration; every case runs on its own,
# with the three compilers (port,clib on the side port, the CD's libraries on the side originals)
math(EXPR limit "8 * 60 + 60 + ${EMULATION_QUEUE_WAIT}")
foreach(side port originals)
	set(name blitz2_selftest_model_run_program)
	set(label emulator)
	set(fixture "")
	if(side STREQUAL "originals")
		string(APPEND name _originals)
		set(label originals)
		set(fixture FIXTURES_REQUIRED originals_refbuild)
	endif()
	add_test(NAME ${name} COMMAND ${CORPUSRUN} case --env ${BASIC_TEST_ENV}
		--source ${CMAKE_CURRENT_SOURCE_DIR}/selftest/model-run-program.test.bas --side ${side})
	set_tests_properties(${name} PROPERTIES LABELS ${label} SKIP_RETURN_CODE 77 TIMEOUT ${limit} ${fixture}
		PASS_REGULAR_EXPRESSION
		"model-run-program\\.test\\.bas: case returns: PASSED \\(alone\\)\n.*case long: PASSED \\(alone\\)\n.*case twice: PASSED \\(alone\\)\n.*case req: PASSED \\(alone\\)\n.*case hang: PASSED \\(alone\\)\nmodel-run-program\\.test\\.bas: 5 cases, side ${side}: PASSED\n")
endforeach()
# a program run from the editor (selftest/model-editor-run.test.bas, change basic-test-editor-run):
# a case that passes from its result file and its console output, and a case whose program
# hangs, ended by the hang check armed on the console; inject's report of the compiler, the
# query compile.inject (selftest/model-compile-inject.test.bas)
math(EXPR limit "90 + 60 + ${EMULATION_QUEUE_WAIT}")
foreach(t "prints|" "hangs|_hang")
	string(REPLACE "|" ";" t "${t}")
	list(GET t 0 case)
	list(GET t 1 suffix)
	add_test(NAME blitz2_selftest_model_editor_run${suffix} COMMAND ${CORPUSRUN} case --env ${BASIC_TEST_ENV}
		--source ${CMAKE_CURRENT_SOURCE_DIR}/selftest/model-editor-run.test.bas --case ${case})
	set_tests_properties(blitz2_selftest_model_editor_run${suffix} PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
		TIMEOUT ${limit} PASS_REGULAR_EXPRESSION "model-editor-run\\.test\\.bas: case ${case}: PASSED\n")
endforeach()
math(EXPR limit "${BASIC_TEST_TIMEOUT} + 60 + ${EMULATION_QUEUE_WAIT}")
add_test(NAME blitz2_selftest_model_compile_inject COMMAND ${CORPUSRUN} case --env ${BASIC_TEST_ENV}
	--source ${CMAKE_CURRENT_SOURCE_DIR}/selftest/model-compile-inject.test.bas --case tracked)
set_tests_properties(blitz2_selftest_model_compile_inject PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
	TIMEOUT ${limit} PASS_REGULAR_EXPRESSION "model-compile-inject\\.test\\.bas: case tracked: PASSED\n")
# the example of tests/README.md, "Tests that judge themselves": its source and configuration
# run as a test of the model, every case on the port side (readme-model-example.cmake)
add_test(NAME blitz2_model_readme_example COMMAND ${CMAKE_COMMAND} -DREADME=${PROJECT_SOURCE_DIR}/tests/README.md
	-DCORPUSRUN=${CORPUSRUN} -DENV=${BASIC_TEST_ENV} -DWORK=${CMAKE_CURRENT_BINARY_DIR}/readme-model-example
	-DSIDE=port -P ${CMAKE_CURRENT_SOURCE_DIR}/readme-model-example.cmake)
math(EXPR limit "2 * (${BASIC_TEST_TIMEOUT} + 60) + ${EMULATION_QUEUE_WAIT}")
set_tests_properties(blitz2_model_readme_example PROPERTIES LABELS emulator SKIP_RETURN_CODE 77
	TIMEOUT ${limit} PASS_REGULAR_EXPRESSION "readme-model-example: every case passed")
# The comparison of a test of the model with the prev build (prev_compile_model_test, corpusrun
# case --prev) with broken input, in a build without a prev build: the driver of
# selftest/model-prev.test.bas (one compile, model-prev, without runtime error checking) compiled
# by this build's compiler as "prev", or by hangstub (tests/corpus/amiga/hangstub.c, never ends: a
# prev compiler whose result differs; the session's time limit is the test's, 30 s):
#   blitz2_selftest_model_same_prev      the same compiler: passes
#   blitz2_selftest_model_differs_prev   hangstub: fails, naming the compile
#   blitz2_selftest_model_intended_prev  hangstub, the compile named: passes, "(intended)"
#   blitz2_selftest_model_stale_prev     the same compiler, the compile named: fails, STALE
#   blitz2_selftest_model_unknown_prev   the same compiler, model-prev-runerrs named (the test
#                                        has no cell with runtime error checking): fails, STALE
set(model_prev_src ${CMAKE_CURRENT_SOURCE_DIR}/selftest/model-prev.test.bas)
set(model_prev_drv ${CMAKE_CURRENT_BINARY_DIR}/model-drivers/model-prev.bb2)
basic_test_driver(${model_prev_src} ${model_prev_drv} ${CMAKE_CURRENT_SOURCE_DIR}/selftest/model-prev.test.toml)
add_custom_target(blitz2_selftest_model_prev_driver ALL DEPENDS ${model_prev_drv})
function(model_prev_selftest kind compiler intended regex)
	set(name blitz2_selftest_model_${kind}_prev)
	set(cmd ${CORPUSRUN} case --env ${BASIC_TEST_ENV} --source ${model_prev_src} --prev
		--driver ${model_prev_drv} --test ${name} --prev-blitz2 ${compiler} --prev-libs ${BLITZ2_DIR})
	if(intended)
		list(APPEND cmd --intended ${name}:${intended})
	endif()
	add_test(NAME ${name} COMMAND ${cmd})
	math(EXPR limit "30 * 2 + 120 + 2 * ${EMULATION_QUEUE_WAIT}")
	set_tests_properties(${name} PROPERTIES LABELS emulator SKIP_RETURN_CODE 77 TIMEOUT ${limit}
		PASS_REGULAR_EXPRESSION "${regex}")
endfunction()
model_prev_selftest(same $<TARGET_FILE:blitz2> ""
	"model-prev[^\n]*prev: result same, exe identical\n.*model-prev: the comparison with the prev build: the same\n")
model_prev_selftest(differs $<TARGET_FILE:hangstub> ""
	"model-prev[^\n]*no RESULT[^\n]*\nprev differs in: PREV_INTENDED=\"blitz2_selftest_model_differs_prev:model-prev\"\n.*model-prev: the comparison with the prev build: DIFFERS\n")
model_prev_selftest(intended $<TARGET_FILE:hangstub> model-prev
	"model-prev[^\n]*no RESULT[^\n]*\\(intended\\)\n.*model-prev: the comparison with the prev build: the same\n")
model_prev_selftest(stale $<TARGET_FILE:blitz2> model-prev
	"STALE blitz2_selftest_model_stale_prev:model-prev: intended, but prev does not differ.*model-prev: the comparison with the prev build: DIFFERS\n")
model_prev_selftest(unknown $<TARGET_FILE:blitz2> model-prev-runerrs
	"STALE blitz2_selftest_model_unknown_prev:model-prev-runerrs: intended, but the test has no such compile \\(its compiles: model-prev\\)\nmodel-prev: the comparison with the prev build: DIFFERS\n")
# the driver the build writes for the comparison calls every case (model-numbering: first and
# second, between the batch init and end), and the command of that build step fails on a
# source whose driver cannot be written (selftest/model-driver-broken.test.bas)
add_test(NAME blitz2_model_driver_all_cases COMMAND ${CMAKE_COMMAND}
	-DDRIVER=${CMAKE_CURRENT_BINARY_DIR}/model-drivers/model-numbering.bas
	"-DWANT=\n\\.tc__skip\nGosub tc__init\nGosub tc_first\nGosub tc_second\nGosub tc__final\nEnd\n$"
	-P ${CMAKE_CURRENT_SOURCE_DIR}/model-driver-check.cmake)
# the driver of all cases printed (--print-driver without --case) is the text that build step
# writes, byte for byte; with the driver of another source the check fails (_broken)
foreach(variant "" _broken)
	if(variant STREQUAL "")
		set(want ${CMAKE_CURRENT_BINARY_DIR}/model-drivers/model-numbering.bas)
	else()
		set(want ${CMAKE_CURRENT_BINARY_DIR}/model-drivers/model-prev.bas)
	endif()
	add_test(NAME blitz2_model_driver_printed${variant} COMMAND ${CMAKE_COMMAND}
		"-DCMD=${CORPUSRUN};case;--env;${BASIC_TEST_ENV};--source;${CMAKE_CURRENT_SOURCE_DIR}/model-numbering.test.bas;--print-driver"
		-DDRIVER=${want} -DOUT=${CMAKE_CURRENT_BINARY_DIR}/model-driver-printed${variant}.bas
		-P ${CMAKE_CURRENT_SOURCE_DIR}/model-driver-print.cmake)
endforeach()
set_tests_properties(blitz2_model_driver_printed PROPERTIES PASS_REGULAR_EXPRESSION "printed the same")
set_tests_properties(blitz2_model_driver_printed_broken PROPERTIES WILL_FAIL TRUE)
add_test(NAME blitz2_selftest_model_driver_broken COMMAND ${CORPUSRUN} case --env ${BASIC_TEST_ENV}
	--source ${CMAKE_CURRENT_SOURCE_DIR}/selftest/model-driver-broken.test.bas
	--write-driver ${CMAKE_CURRENT_BINARY_DIR}/model-drivers-broken/model-driver-broken.bb2)
set_tests_properties(blitz2_selftest_model_driver_broken PROPERTIES PASS_REGULAR_EXPRESSION
	"model-driver-broken\\.test\\.bas:5: [^\n]*@NOSUCH@")
# the sources of the compiler that need nothing special, compiled by the prev build (the
# compiler's bug-orig tests, tests/bug-orig, were added before)
prev_compile_plain_pass()
