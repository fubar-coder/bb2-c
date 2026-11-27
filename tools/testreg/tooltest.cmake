# ctest testreg_tool: testreg on sample directories - the ctest include
# file (names, sides, labels and flaky, time limits, cost, fixtures), the depfile, and
# every check with broken input: a source of the wrong form, a case name
# that is a token, a bad configuration, a query of a check the source does
# not have, a case selector of a case it does not have, a test id that is
# not kebab-case, a name registered twice, a run from the editor with an
# assertion on the exit code or an announced runtime error; then a sample
# CMake project whose ctest -N lists the registered tests after the build,
# and again
# after a tests.toml changed (the step depends on the files it read);
# last the bundle tests: their include file, --no-bundle, a bundle named as
# a test of the old form, and a sample project whose ctest -N lists the
# bundles and their members, a case alone by -R and the bundle of the label
# bug-orig by -L.
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

# the samples above have a port cell of a plain key each: the cases of
# the tests below run alone (the bundles: at the end)
set(env --env ${WORK}/env.toml --tokens ${WORK}/tokens.txt --no-bundle)
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

# flaky: both sides get the label flaky behind the others (also when
# labels already names it, once); the key of a single test only
file(MAKE_DIRECTORY ${s}/flaky)
file(WRITE ${s}/flaky/flaky-one.test.bas "!TestCase{c}\n  !CheckEq{1,1,\"w\"}\n!EndTestCase\n")
file(WRITE ${s}/flaky/flaky-one.test.toml "flaky = \"rc 20 as the second cell of a session\"\n"
	"[deviations.\"libs=cd\"]\n\"check.c.w\" = \"== fail\"\n")
file(WRITE ${s}/flaky/flaky-two.test.bas "!TestCase{c}\n!EndTestCase\n")
file(WRITE ${s}/flaky/flaky-two.test.toml "flaky = \"x\"\nlabels = [\"flaky\", \"y\"]\n")
run(0 --prefix bug_orig_ --output ${WORK}/flaky.cmake ${env} --command run
	${s}/flaky/flaky-one.test.bas ${s}/flaky/flaky-two.test.bas)
file(READ ${WORK}/flaky.cmake inc)
string(REPLACE "]==]" "" inc "${inc}")
string(REPLACE "[==[" "" inc "${inc}")
foreach(want
		"set_tests_properties( bug_orig_flaky_one__c PROPERTIES LABELS bug-orig;emulator;flaky TIMEOUT 60 "
		"set_tests_properties( bug_orig_flaky_one__c_originals PROPERTIES LABELS originals;flaky TIMEOUT 120 "
		"set_tests_properties( bug_orig_flaky_two__c PROPERTIES LABELS emulator;flaky;y TIMEOUT 60 "
		"set_tests_properties( bug_orig_flaky_two__c_originals PROPERTIES LABELS originals;flaky;y TIMEOUT 120 ")
	string(FIND "${inc}" "${want}" at)
	if(at LESS 0)
		message(FATAL_ERROR "flaky: the include file lacks\n  ${want}\nin\n${inc}")
	endif()
endforeach()
file(WRITE ${s}/flaky/tests.toml "flaky = \"x\"\n")
refused("flaky/tests\\.toml:1: flaky is a key of a single test" ${s}/flaky/flaky-two.test.bas)
file(REMOVE ${s}/flaky/tests.toml)
file(WRITE ${s}/flaky/flaky-two.test.toml "flaky = \"\"\n")
refused("flaky/flaky-two\\.test\\.toml:1: flaky must be a non-empty string" ${s}/flaky/flaky-two.test.bas)
file(REMOVE_RECURSE ${s}/flaky)

# AB3 mode (change ab3-test-model): a test with the cells of blitz2-ab3
# (side port) and AmiBlitz3 (side oracle) has two tests per case, the
# oracle one with the label oracle and never emulator; a deviation of
# AmiBlitz3 with its variant gives no bug-orig; the source is read in AB3
# mode (a check continued with @@), in BB2 mode with source-mode = "bb2";
# a deviation of AmiBlitz3 without variant and !ExpectError beside a cell
# of amiblitz3 are refused
file(MAKE_DIRECTORY ${s}/ab3)
file(WRITE ${s}/ab3/tests.toml "libs = \"clib\"\n[matrix]\ncompiler = [\"ab3\", \"amiblitz3\"]\n")
file(WRITE ${s}/ab3/both.test.bas "!TestCase{a}\n  !CheckStrEq{\"x\", @@\n    \"x\",\"same\"}\n!EndTestCase\n")
file(WRITE ${s}/ab3/both.test.toml
	"[deviations.\"compiler=amiblitz3\"]\n\"check.a.same\" = \"== fail\"\nvariant = \"chr_nospace\"\n")
run(0 --prefix ab3_ --output ${WORK}/ab3.cmake ${env} --command run ${s}/ab3/both.test.bas)
file(READ ${WORK}/ab3.cmake inc)
string(REPLACE "]==]" "" inc "${inc}")
string(REPLACE "[==[" "" inc "${inc}")
foreach(want
		"add_test( ab3_both__a run --source ${s}/ab3/both.test.bas --case a --side port)"
		"set_tests_properties( ab3_both__a PROPERTIES LABELS emulator TIMEOUT 60 SKIP_RETURN_CODE 77)"
		"add_test( ab3_both__a_oracle run --source ${s}/ab3/both.test.bas --case a --side oracle)"
		"set_tests_properties( ab3_both__a_oracle PROPERTIES LABELS oracle TIMEOUT 60 SKIP_RETURN_CODE 77)")
	string(FIND "${inc}" "${want}" at)
	if(at LESS 0)
		message(FATAL_ERROR "AB3 mode: the include file lacks\n  ${want}\nin\n${inc}")
	endif()
endforeach()
if(inc MATCHES "_originals|bug-orig")
	message(FATAL_ERROR "AB3 mode: a side originals or the label bug-orig:\n${inc}")
endif()
file(WRITE ${s}/ab3/both.test.toml "[deviations.\"compiler=amiblitz3\"]\n\"check.a.same\" = \"== fail\"\n")
refused("ab3/both.test.toml:2: \\[deviations.\"compiler=amiblitz3\"\\] of the cells of amiblitz3 has no variant"
	${s}/ab3/both.test.bas)
file(WRITE ${s}/ab3/both.test.toml "source-mode = \"bb2\"\nexclude = [\"compiler=amiblitz3\"]\n")
refused("ab3/both.test.bas:2: !CheckStrEq: the line ends before the closing }" ${s}/ab3/both.test.bas)
file(WRITE ${s}/ab3/err.test.bas "!TestCase{a}\n  !ExpectError{\"Overflow\"}\n!EndTestCase\n")
refused("ab3/err.test.bas:2: !ExpectError in a test with the cell amiblitz3 of compiler = \"amiblitz3\""
	${s}/ab3/err.test.bas)
file(REMOVE_RECURSE ${s}/ab3)

# --order: the bug-orig tests of the old form (cmake/bug-orig-order.cmake)
# declared with the tests of an open bug, the largest COST first and equal
# ones by name; an open bug without a configured cost gets the sum of its
# cells' session limits (60, the port side's one cell); the tests without
# the label bug-orig come first
file(MAKE_DIRECTORY ${s}/order)
file(WRITE ${s}/order/order-open.test.bas "!TestCase{c}\n  !CheckEq{1,1,\"w\"}\n!EndTestCase\n")
file(WRITE ${s}/order/order-open.test.toml "[deviations.\"libs=cd\"]\n\"check.c.w\" = \"== fail\"\n")
file(WRITE ${WORK}/order.cmake "# the old form\n"
	"# @test bug_orig_small 10\nadd_test(bug_orig_small run small)\n"
	"# @test bug_orig_big 240\nadd_test(bug_orig_big run big)\nset_tests_properties(bug_orig_big PROPERTIES COST 240)\n"
	"# @test bug_orig_a_mid 60\nadd_test(bug_orig_a_mid run mid)\n")
run(0 --prefix bug_orig_ --output ${WORK}/order-tests.cmake ${env} --command run --order ${WORK}/order.cmake
	--depfile ${WORK}/order-tests.d ${s}/order/order-open.test.bas)
file(READ ${WORK}/order-tests.cmake inc)
string(REPLACE "]==]" "" inc "${inc}")
string(REPLACE "[==[" "" inc "${inc}")
set(last -1)
foreach(want "add_test( bug_orig_order_open__c_originals"
		"# the tests of the label bug-orig, the largest COST first"
		"add_test(bug_orig_big run big)\nset_tests_properties(bug_orig_big PROPERTIES COST 240)\n"
		"add_test(bug_orig_a_mid run mid)\n"
		"set_tests_properties( bug_orig_order_open__c PROPERTIES LABELS bug-orig;emulator TIMEOUT 60 SKIP_RETURN_CODE 77 COST 60)"
		"add_test(bug_orig_small run small)\n")
	string(FIND "${inc}" "${want}" at)
	if(at LESS 0 OR NOT at GREATER last)
		message(FATAL_ERROR "--order: the include file lacks, or has out of order,\n  ${want}\nin\n${inc}")
	endif()
	set(last ${at})
endforeach()
file(READ ${WORK}/order-tests.d dep)
string(FIND "${dep}" "${WORK}/order.cmake" at)
if(at LESS 0)
	message(FATAL_ERROR "--order: the depfile lacks the order file:\n${dep}")
endif()
# --costs: a measured COST replaces the sum of the cells' session limits
# (order-open: 60 -> 7); the configuration's cost wins over a measurement
# (shape-frames: cost = 3 in libs/, both sides measured); an entry of
# another prefix or without "__" is not testreg's to check
file(WRITE ${WORK}/costs.txt "# measured\nbug_orig_old 5\nbug_orig_order_open__c 7\n"
	"bug_orig_shape_frames__behind 44\nbug_orig_shape_frames__behind_originals 50\nregression_x__c 3\n")
run(0 --prefix bug_orig_ --output ${WORK}/cost-tests.cmake ${env} --command run --order ${WORK}/order.cmake
	--costs ${WORK}/costs.txt --depfile ${WORK}/cost-tests.d ${s}/order/order-open.test.bas
	${s}/libs/shape-frames.test.bas)
file(READ ${WORK}/cost-tests.cmake inc)
string(REPLACE "]==]" "" inc "${inc}")
string(REPLACE "[==[" "" inc "${inc}")
set(last -1)
foreach(want "set_tests_properties( bug_orig_shape_frames__behind_originals PROPERTIES LABELS originals;libs TIMEOUT 120 SKIP_RETURN_CODE 77 COST 3 "
		"add_test(bug_orig_a_mid run mid)\n"
		"add_test(bug_orig_small run small)\n"
		"set_tests_properties( bug_orig_order_open__c PROPERTIES LABELS bug-orig;emulator TIMEOUT 60 SKIP_RETURN_CODE 77 COST 7)"
		"set_tests_properties( bug_orig_shape_frames__behind PROPERTIES LABELS bug-orig;emulator;libs TIMEOUT 60 SKIP_RETURN_CODE 77 COST 3 ")
	string(FIND "${inc}" "${want}" at)
	if(at LESS 0 OR NOT at GREATER last)
		message(FATAL_ERROR "--costs: the include file lacks, or has out of order,\n  ${want}\nin\n${inc}")
	endif()
	set(last ${at})
endforeach()
file(READ ${WORK}/cost-tests.d dep)
string(FIND "${dep}" "${WORK}/costs.txt" at)
if(at LESS 0)
	message(FATAL_ERROR "--costs: the depfile lacks the file:\n${dep}")
endif()
# --test-env: the script is included first and called after the properties
# of each registered test (also of the label bug-orig), not for an entry of
# --order (it carries its own environment); without it, no call
if(inc MATCHES "basic_test_environment")
	message(FATAL_ERROR "without --test-env: a call of basic_test_environment in\n${inc}")
endif()
run(0 --prefix bug_orig_ --output ${WORK}/env-tests.cmake ${env} --command run --order ${WORK}/order.cmake
	--test-env /x/test-env.cmake ${s}/order/order-open.test.bas)
file(READ ${WORK}/env-tests.cmake inc)
string(REPLACE "]==]" "" inc "${inc}")
string(REPLACE "[==[" "" inc "${inc}")
set(last -1)
foreach(want "include( /x/test-env.cmake)\n"
		"SKIP_RETURN_CODE 77)\nbasic_test_environment( bug_orig_order_open__c_originals originals)\n"
		"add_test(bug_orig_big run big)\nset_tests_properties(bug_orig_big PROPERTIES COST 240)\nadd_test("
		"SKIP_RETURN_CODE 77 COST 60)\nbasic_test_environment( bug_orig_order_open__c bug-orig;emulator)\n"
		"add_test(bug_orig_small run small)\n")
	string(FIND "${inc}" "${want}" at)
	if(at LESS 0 OR NOT at GREATER last)
		message(FATAL_ERROR "--test-env: the include file lacks, or has out of order,\n  ${want}\nin\n${inc}")
	endif()
	set(last ${at})
endforeach()
string(REGEX MATCHALL "basic_test_environment\\(" calls "${inc}")
list(LENGTH calls ncalls)
if(NOT ncalls EQUAL 2)
	message(FATAL_ERROR "--test-env: ${ncalls} calls, not 2 (one per registered test):\n${inc}")
endif()
# an entry of a test of the model of the prefix that is not registered is
# ignored: no message, the include file as without it
file(WRITE ${WORK}/costs-stale.txt "bug_orig_gone__c 5\nbug_orig_order_open__c 7\n")
run(0 --prefix bug_orig_ --output ${WORK}/stale-tests.cmake ${env} --command run --costs ${WORK}/costs-stale.txt
	${s}/order/order-open.test.bas)
if(NOT err STREQUAL "")
	message(FATAL_ERROR "--costs: a stale entry is reported:\n${err}")
endif()
file(READ ${WORK}/stale-tests.cmake inc)
string(REPLACE "]==]" "" inc "${inc}")
string(REPLACE "[==[" "" inc "${inc}")
if(inc MATCHES "bug_orig_gone" OR NOT inc MATCHES "bug_orig_order_open__c PROPERTIES [^)]* COST 7\\)")
	message(FATAL_ERROR "--costs: with a stale entry, the include file\n${inc}")
endif()
# broken input: a malformed file
file(WRITE ${WORK}/costs-bad.txt "bug_orig_b 5\nbug_orig_a 5\n")
refused("costs-bad\\.txt:2: bug_orig_a after bug_orig_b: the file is sorted by name" --costs ${WORK}/costs-bad.txt
	${s}/order/order-open.test.bas)
# broken input: text before the first entry, a malformed entry, a test of
# both forms, no such file
file(WRITE ${WORK}/order-bad.cmake "add_test(x run)\n")
refused("order-bad\\.cmake:1: text before the first entry" --order ${WORK}/order-bad.cmake
	${s}/order/order-open.test.bas)
file(WRITE ${WORK}/order-bad.cmake "# @test bug_orig_x many\n")
refused("order-bad\\.cmake:1: an entry is \"# @test NAME COST\"" --order ${WORK}/order-bad.cmake
	${s}/order/order-open.test.bas)
file(WRITE ${WORK}/order-bad.cmake "# @test bug_orig_order_open__c 5\nadd_test(x run)\n")
refused("the test bug_orig_order_open__c is also a test of the old form" --order ${WORK}/order-bad.cmake
	${s}/order/order-open.test.bas)
refused("no-such\\.cmake: cannot read" --order ${WORK}/no-such.cmake ${s}/order/order-open.test.bas)
file(REMOVE_RECURSE ${s}/order)

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
file(WRITE ${s}/other/conf.test.toml "[asserts]\n\"run.a.child.rc\" = \"== 20\"\n")
refused("other/conf.test.toml:2: run.a.child.rc: the source has no !RunProgram named child in a"
	${s}/other/conf.test.bas)
file(WRITE ${s}/other/conf.test.toml
	"[ends.\"libs=cd\"]\nreason = \"r\"\nhangs = [\"session == timeout\", \"check.a.width == not-run\"]\n")
refused("other/conf.test.toml:3: check.a.width: the source has no check named width in a" ${s}/other/conf.test.bas)
file(WRITE ${s}/other/conf.test.toml "[ends.\"case=zz\"]\nreason = \"r\"\nhangs = [\"session == timeout\"]\n")
refused("the cell selector \"case=zz\" names the case zz, which the source does not have" ${s}/other/conf.test.bas)
file(WRITE ${s}/other/conf.test.toml "[asserts.\"case=zz\"]\nrc = \"== 1\"\n")
refused("the cell selector \"case=zz\" names the case zz, which the source does not have" ${s}/other/conf.test.bas)
file(WRITE ${s}/other/conf.test.toml "[matrix]\nasserts = [\"x\"]\n")
refused("other/conf.test.toml:2: asserts is no run setting and cannot be an axis" ${s}/other/conf.test.bas)
# a run from the editor: an assertion on the exit code, an announced runtime error
file(WRITE ${s}/other/conf.test.toml "editor-run = \"on\"\n[asserts]\nrc = \"== 0\"\n")
refused("other/conf.test.toml:3: rc: an assertion on rc in the cell port,cd with editor-run = \"on\""
	${s}/other/conf.test.bas)
file(WRITE ${s}/other/editor.test.bas "!TestCase{a}\n  !ExpectError{\"Shape too Big\"}\n!EndTestCase\n")
file(WRITE ${s}/other/editor.test.toml "runtime-errors = \"on\"\n[matrix]\neditor-run = [\"off\", \"on\"]\n")
refused("other/editor.test.bas:2: !ExpectError in a test whose cell [a-z0-9,]+ runs from the editor"
	${s}/other/editor.test.bas)
file(REMOVE ${s}/other/editor.test.bas ${s}/other/editor.test.toml)
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
# test-local data (change basic-test-local-data): the files of ID.test/ are
# dependencies (and its directories, which an added file changes); an
# undeclared file and a declared one that is missing are errors
file(WRITE ${s}/other/data.test.bas "!TestCase{a}\n!EndTestCase\n")
file(WRITE ${s}/other/data.test.toml
	"[data]\n\"in.txt\" = \"copy\"\n[data.\"sub/child.bas\"]\naction = \"basic-compile\"\n")
file(WRITE ${s}/other/data.test/in.txt "x\n")
file(WRITE ${s}/other/data.test/sub/child.bas "NPrint 1\n")
run(0 --prefix p_ --output ${WORK}/data.cmake --depfile ${WORK}/data.d ${env} --command run
	${s}/other/data.test.bas)
file(READ ${WORK}/data.d dep)
foreach(want "${s}/other/data.test/in.txt" "${s}/other/data.test/sub/child.bas" "${s}/other/data.test/sub "
		"${s}/other/data.test ")
	string(FIND "${dep}" "${want}" at)
	if(at LESS 0)
		message(FATAL_ERROR "data: the depfile lacks ${want}:\n${dep}")
	endif()
endforeach()
# a check macro in a data source (basic-compile): its program has no result file
file(WRITE ${s}/other/data.test/sub/child.bas "NPrint \"@TESTDIR@\"\n  !CheckEq{a,1}\n")
refused("other/data\\.test/sub/child\\.bas:2: !CheckEq in a data source" ${s}/other/data.test.bas)
file(WRITE ${s}/other/data.test/sub/child.bas "NPrint 1\n")
file(WRITE ${s}/other/data.test/extra.bin "")
refused("other/data\\.test/extra\\.bin: a file of data\\.test/ that [^ ]*other/data\\.test\\.toml does not declare"
	${s}/other/data.test.bas)
file(REMOVE ${s}/other/data.test/extra.bin ${s}/other/data.test/in.txt)
refused("other/data\\.test\\.toml:2: data \"in\\.txt\": no file [^ ]*other/data\\.test/in\\.txt"
	${s}/other/data.test.bas)
file(REMOVE ${s}/other/data.test.toml)
refused("other/data\\.test/sub/child\\.bas: a file of data\\.test/ that [^ ]*other/data\\.test\\.toml does not declare"
	${s}/other/data.test.bas)
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
		--env ${WORK}/env.toml --no-bundle --command run \${sources}
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
# a file added to a test's ID.test/ registers again (here: an undeclared
# one, so the step fails)
file(WRITE ${s}/libs/sub/fixed-one.test.toml
	"fixed-orig = 12\ncompiler = \"port\"\nlibs = \"clib\"\ntimeout = 20\n"
	"[deviations.\"libs=cd\"]\n\"end.case\" = \"not exists\"\n[data]\n\"in.txt\" = \"copy\"\n")
file(WRITE ${s}/libs/sub/fixed-one.test/in.txt "x\n")
build_and_list(7 -L sample)
file(WRITE ${s}/libs/sub/fixed-one.test/extra.bin "")
execute_process(COMMAND ${CMAKE_COMMAND} --build ${p}/build RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
if(rc EQUAL 0 OR NOT out MATCHES "fixed-one\\.test/extra\\.bin: a file of fixed-one\\.test/")
	message(FATAL_ERROR "a file added to ID.test/ does not register again (exit ${rc}):\n${out}")
endif()

# bundles (change basic-test-bundle-registration): the cases of one port
# cell and a plain key share a bundle test; a case that asserts the output
# runs alone; an open bug's case is in a bundle of the label bug-orig
set(b ${WORK}/bsample)
file(MAKE_DIRECTORY ${b}/x)
file(WRITE ${WORK}/benv.toml "top = \"${b}\"\n[runner]\ntimeout = \"90\"\n")
file(WRITE ${b}/tests.toml "root = true\n")
file(WRITE ${b}/x/plain-a.test.bas
	"!TestCase{a}\n  !Check{1}\n!EndTestCase\n!TestCase{b}\n  !Check{1}\n!EndTestCase\n")
file(WRITE ${b}/x/plain-b.test.bas "!TestCase{a}\n  !Check{1}\n!EndTestCase\n")
file(WRITE ${b}/x/alone.test.bas "!TestCase{a}\n  !Check{1}\n!EndTestCase\n")
file(WRITE ${b}/x/alone.test.toml "[asserts]\noutput = \"contains \\\"x\\\"\"\n")
file(WRITE ${b}/x/open-one.test.bas "!TestCase{c}\n  !CheckEq{1,1,\"w\"}\n!EndTestCase\n")
file(WRITE ${b}/x/open-one.test.toml "[deviations.\"compiler=port\"]\n\"check.c.w\" = \"== fail\"\n")
set(bsrc ${b}/x/plain-a.test.bas ${b}/x/plain-b.test.bas ${b}/x/alone.test.bas ${b}/x/open-one.test.bas)
set(benv --env ${WORK}/benv.toml --tokens ${WORK}/tokens.txt)
run(0 --prefix bug_orig_ --output ${WORK}/bundles.cmake ${benv} --timeout-extra 30 --command run --command case
	${bsrc})
file(READ ${WORK}/bundles.cmake inc)
string(REPLACE "]==]" "" inc "${inc}")
string(REPLACE "[==[" "" inc "${inc}")
foreach(want
		"add_test( bug_orig_bundle_1 run case --bundle bug_orig_bundle_1 --member bug_orig_plain_a__a ${b}/x/plain-a.test.bas a --member bug_orig_plain_a__b ${b}/x/plain-a.test.bas b --member bug_orig_plain_b__a ${b}/x/plain-b.test.bas a)"
		"set_tests_properties( bug_orig_bundle_1 PROPERTIES LABELS emulator TIMEOUT 300 SKIP_RETURN_CODE 77)"
		"add_test( bug_orig_plain_a__b run case --source ${b}/x/plain-a.test.bas --case b --side port --test bug_orig_plain_a__b --from-bundle bug_orig_bundle_1)"
		"set_tests_properties( bug_orig_plain_a__b PROPERTIES LABELS emulator TIMEOUT 120 SKIP_RETURN_CODE 77 DEPENDS bug_orig_bundle_1)"
		"add_test( bug_orig_bundle_2 run case --bundle bug_orig_bundle_2 --member bug_orig_open_one__c ${b}/x/open-one.test.bas c)"
		"set_tests_properties( bug_orig_bundle_2 PROPERTIES LABELS bug-orig;emulator TIMEOUT 300 SKIP_RETURN_CODE 77 COST 90)"
		"set_tests_properties( bug_orig_open_one__c PROPERTIES LABELS bug-orig;emulator TIMEOUT 120 SKIP_RETURN_CODE 77 COST 90 DEPENDS bug_orig_bundle_2)"
		"add_test( bug_orig_alone__a run case --source ${b}/x/alone.test.bas --case a --side port)\n")
	string(FIND "${inc}" "${want}" at)
	if(at LESS 0)
		message(FATAL_ERROR "bundles: the include file lacks\n  ${want}\nin\n${inc}")
	endif()
endforeach()
if(inc MATCHES "alone__a [^\n]*DEPENDS")
	message(FATAL_ERROR "bundles: a case that asserts the output depends on a bundle:\n${inc}")
endif()
# --no-bundle: no bundle test, no option of a member, no DEPENDS
run(0 --prefix bug_orig_ --output ${WORK}/nobundles.cmake ${benv} --no-bundle --command run ${bsrc})
file(READ ${WORK}/nobundles.cmake inc)
if(inc MATCHES "bundle_|DEPENDS|--from-bundle|--test")
	message(FATAL_ERROR "--no-bundle: a bundle in\n${inc}")
endif()
# broken input: a bundle's name is the name of a test of the old form
file(WRITE ${WORK}/border.cmake "# @test bug_orig_bundle_1 5\nadd_test(x run)\n")
file(REMOVE ${WORK}/bad.cmake)
run(1 --prefix bug_orig_ --output ${WORK}/bad.cmake ${benv} --command run --order ${WORK}/border.cmake ${bsrc})
if(NOT err MATCHES "the test bug_orig_bundle_1 is also a test of the old form" OR EXISTS ${WORK}/bad.cmake)
	message(FATAL_ERROR "a bundle named as a test of the old form:\n${err}")
endif()
# a sample CMake project: ctest reads the include file with the bundles
set(p ${WORK}/bproject)
file(MAKE_DIRECTORY ${p})
file(WRITE ${p}/CMakeLists.txt "cmake_minimum_required(VERSION 3.20)
project(bsample NONE)
enable_testing()
set(inc \${CMAKE_BINARY_DIR}/blitz-tests.cmake)
add_custom_command(OUTPUT \${inc}
	COMMAND ${TESTREG} --prefix regression_ --output \${inc} --env ${WORK}/benv.toml --command run ${bsrc})
add_custom_target(blitz_tests ALL DEPENDS \${inc})
set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES \${inc})
")
execute_process(COMMAND ${CMAKE_COMMAND} -S ${p} -B ${p}/build -G "${CMAKE_GENERATOR}"
	RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
if(NOT rc EQUAL 0)
	message(FATAL_ERROR "the bundle project does not configure:\n${out}")
endif()
# ctest_list(COUNT [MUST NAME...] [NOT NAME...] -- ARGS...)
function(ctest_list want_count)
	cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "MUST;NOT;ARGS")
	execute_process(COMMAND ${CTEST} -N ${arg_ARGS} WORKING_DIRECTORY ${p}/build OUTPUT_VARIABLE list
		ERROR_VARIABLE list)
	if(NOT list MATCHES "Total Tests: ${want_count}\n")
		message(FATAL_ERROR "bundles: ctest -N ${arg_ARGS}: not ${want_count} tests:\n${list}")
	endif()
	foreach(name IN LISTS arg_MUST)
		if(NOT list MATCHES "Test +#[0-9]+: ${name}\n")
			message(FATAL_ERROR "bundles: ctest -N ${arg_ARGS} lacks ${name}:\n${list}")
		endif()
	endforeach()
	foreach(name IN LISTS arg_NOT)
		if(list MATCHES "Test +#[0-9]+: ${name}\n")
			message(FATAL_ERROR "bundles: ctest -N ${arg_ARGS} lists ${name}:\n${list}")
		endif()
	endforeach()
endfunction()
execute_process(COMMAND ${CMAKE_COMMAND} --build ${p}/build RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
if(NOT rc EQUAL 0)
	message(FATAL_ERROR "the bundle project does not build:\n${out}")
endif()
# the bundles and their members; a case alone (without its bundle: DEPENDS
# orders, it does not select); the bundle of the label bug-orig
ctest_list(7 MUST regression_bundle_1 regression_bundle_2 regression_plain_a__a regression_plain_a__b
	regression_plain_b__a regression_open_one__c regression_alone__a)
ctest_list(1 MUST regression_plain_a__b ARGS -R "^regression_plain_a__b$")
ctest_list(2 MUST regression_bundle_2 regression_open_one__c NOT regression_bundle_1 ARGS -L bug-orig)
file(REMOVE_RECURSE ${WORK})
message("testreg_tool: all cases as expected")
