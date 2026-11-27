# bundle-selftest.cmake - ctest corpusrun_bundle: the bundles and the served
# case (caserun.h, "The bundles") in Amiberry, on the sources of
# bundle-selftest/ copied to WORK/src:
#
#   - the bundle a of passes, hangs, fails, touched: one session of the four
#     cells, ended by the hang check in c1 (after the environment's
#     run-hang-after, far below the time limit: the check armed from c0's
#     run), the cells behind it in a second session; passes and touched
#     stored, hangs and fails not;
#   - the bundle b of open-bug with --require-deviation: REPRODUCED stored;
#   - the bundle c of ab3-one and ab3-two, cells of blitz2-ab3 compiled from
#     the command line (change ab3-test-model): one session, both stored and
#     served (ab3-one's cell of AmiBlitz3 is in no bundle);
#   - touched.test.bas changed;
#   - the case tests with --from-bundle: passes and open-bug served (exit
#     status 0 and 1, no session), hangs alone (no entry), fails alone (an
#     entry cut after its key line, written by the test: named malformed),
#     touched alone (its entry stale); every entry gone afterwards;
#   - the verdict of each member the verdict of its run alone, and the block
#     of passes and fails the same as alone.
#
# The runs use an environment file of their own (the build's with work=
# WORK/model-runs) and BB2C_TEST_RUN=WORK/run, so the failed runs alone stay
# in WORK; WORK is removed when the test passes.
#   cmake -DCORPUSRUN=FILE -DENV=FILE -DSRC=DIR -DWORK=DIR -P bundle-selftest.cmake
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${WORK}/run)
file(COPY ${SRC}/ DESTINATION ${WORK}/src)
file(READ ${ENV} env)
string(REGEX REPLACE "\nwork = \"[^\"]*\"" "\nwork = \"${WORK}/model-runs\"" env "${env}")
file(WRITE ${WORK}/env.toml "${env}")
set(ENV{BB2C_TEST_RUN} ${WORK}/run)
set(s ${WORK}/src)
set(store ${WORK}/run/verdicts)

# corpusrun case with ARGN: the output in OUT, the exit status in RC
function(run out rc)
	execute_process(COMMAND ${CORPUSRUN} case --env ${WORK}/env.toml ${ARGN}
		RESULT_VARIABLE r OUTPUT_VARIABLE o ERROR_VARIABLE e)
	message("$ corpusrun case ${ARGN}\n${o}${e}exit status ${r}\n")
	set(${out} "${o}${e}" PARENT_SCOPE)
	set(${rc} "${r}" PARENT_SCOPE)
endfunction()

function(want what text regex)
	if(NOT "${text}" MATCHES "${regex}")
		message(FATAL_ERROR "bundle-selftest: ${what}: the output does not match ${regex}")
	endif()
endfunction()

function(want_not what text regex)
	if("${text}" MATCHES "${regex}")
		message(FATAL_ERROR "bundle-selftest: ${what}: the output matches ${regex}")
	endif()
endfunction()

function(want_rc what rc wanted)
	if(NOT rc STREQUAL wanted)
		message(FATAL_ERROR "bundle-selftest: ${what}: exit status ${rc}, expected ${wanted}")
	endif()
endfunction()

# the verdict of the case one of ID in TEXT
function(verdict out id text)
	if(NOT "${text}" MATCHES "${id}\\.test\\.bas: case one: ([A-Z]+)")
		message(FATAL_ERROR "bundle-selftest: no verdict of ${id}")
	endif()
	set(${out} ${CMAKE_MATCH_1} PARENT_SCOPE)
endfunction()

# the block of the case one of ID in TEXT, without the bundle's mark
function(case_block out id text)
	if(NOT "${text}" MATCHES "== ${id}\\.test\\.bas, case one[^\n]*\n([^\n]*\n)*${id}\\.test\\.bas: case one: [A-Z]+")
		message(FATAL_ERROR "bundle-selftest: no block of ${id}")
	endif()
	set(${out} "${CMAKE_MATCH_0}" PARENT_SCOPE)
endfunction()

# the bundles
run(a rc --bundle selftest_bundle_a --member t_passes ${s}/passes.test.bas one
	--member t_hangs ${s}/hangs.test.bas one --member t_fails ${s}/fails.test.bas one
	--member t_touched ${s}/touched.test.bas one)
if(rc STREQUAL 77)
	message("bundle-selftest: skipped (exit status 77)")
	return()
endif()
want_rc("bundle a" "${rc}" 0)
want("bundle a" "${a}" "bundle cell c0: t_passes\n[^\n]*bundle cell c1: t_hangs\n[^\n]*bundle cell c2: t_fails\n[^\n]*bundle cell c3: t_touched\ncorpusrun case: emulator session 0: c0 c1 c2 c3\n")
want("bundle a" "${a}" "session ended \\([a-z]+\\) in c1; the cells behind it run again\n")
want("bundle a" "${a}" "bundle cell c0: t_fails\n[^\n]*bundle cell c1: t_touched\ncorpusrun case: emulator session 1: c0 c1\n@stat [^\n]* test=selftest_bundle_a config=s1 ")
# the hang check ended the first session, not the time limit
if(NOT a MATCHES "@stat [^\n]* test=selftest_bundle_a config=s0 [^\n]* session=([0-9.]+) [^\n]* limit=([0-9]+) ")
	message(FATAL_ERROR "bundle-selftest: no @stat line of the session s0")
endif()
set(limit ${CMAKE_MATCH_2})
math(EXPR half "${limit} / 2")
if(CMAKE_MATCH_1 GREATER ${half})
	message(FATAL_ERROR "bundle-selftest: the session s0 took ${CMAKE_MATCH_1} s of ${limit} s: the hang check did not end it")
endif()
want("bundle a" "${a}" "passes\\.test\\.bas: case one: PASSED \\(bundle selftest_bundle_a\\)\nt_passes: PASSED stored\n")
want("bundle a" "${a}" "\nt_hangs: FAILED not stored ")
want("bundle a" "${a}" "\nt_fails: FAILED not stored ")
want("bundle a" "${a}" "touched\\.test\\.bas: case one: PASSED \\(bundle selftest_bundle_a\\)\nt_touched: PASSED stored\n")
want("bundle a" "${a}" "\nselftest_bundle_a: 4 members, 2 stored\n")
run(b rc --require-deviation --bundle selftest_bundle_b --member t_open_bug ${s}/open-bug.test.bas one)
want_rc("bundle b" "${rc}" 0)
want("bundle b" "${b}" "open-bug\\.test\\.bas: case one: REPRODUCED \\(bundle selftest_bundle_b\\)\nt_open_bug: REPRODUCED stored\n")
# the bundle c of two sources of blitz2-ab3 (change ab3-test-model D14):
# one session of both cells, compiled from the command line; ab3-one's
# cell of AmiBlitz3 is not a member (the side oracle is never bundled)
run(c rc --bundle selftest_bundle_c --member t_ab3_one ${s}/ab3-one.test.bas one
	--member t_ab3_two ${s}/ab3-two.test.bas one)
want_rc("bundle c" "${rc}" 0)
want("bundle c" "${c}" "corpusrun case: emulator session 0: c0 c1\n")
want("bundle c" "${c}" "ab3-one\\.test\\.bas: case one: PASSED \\(bundle selftest_bundle_c\\)\nt_ab3_one: PASSED stored\n")
want("bundle c" "${c}" "ab3-two\\.test\\.bas: case one: PASSED \\(bundle selftest_bundle_c\\)\nt_ab3_two: PASSED stored\n")
want_not("bundle c" "${c}" "amiblitz3")
foreach(t t_passes t_touched t_open_bug t_ab3_one t_ab3_two)
	if(NOT EXISTS ${store}/${t})
		message(FATAL_ERROR "bundle-selftest: no entry ${store}/${t}")
	endif()
endforeach()
foreach(t t_hangs t_fails)
	if(EXISTS ${store}/${t})
		message(FATAL_ERROR "bundle-selftest: an entry ${store}/${t}")
	endif()
endforeach()
file(GLOB left LIST_DIRECTORIES true ${WORK}/model-runs/selftest_bundle_*/run-*)
if(left)
	message(FATAL_ERROR "bundle-selftest: run directories of the bundles left: ${left}")
endif()

# a source changed after the bundle
file(APPEND ${s}/touched.test.bas "; changed after the bundle\n")

# the case tests
set(case --case one --side port)
run(c rc --source ${s}/passes.test.bas ${case} --test t_passes --from-bundle selftest_bundle_a)
want_rc("t_passes" "${rc}" 0)
want("t_passes" "${c}" "passes\\.test\\.bas: case one: PASSED\ncorpusrun case: verdict from the bundle selftest_bundle_a\n@served v=1 test=t_passes bundle=selftest_bundle_a\n")
want_not("t_passes" "${c}" "@stat|emulator session")
case_block(served passes "${c}")
run(c rc --source ${s}/hangs.test.bas ${case} --test t_hangs --from-bundle selftest_bundle_a)
want_rc("t_hangs" "${rc}" 4)
want("t_hangs" "${c}" "@stat ")
want_not("t_hangs" "${c}" "@served")
verdict(v hangs "${c}")
verdict(w hangs "${a}")
want_rc("t_hangs: its verdict alone" "${v}" "${w}")
# a malformed entry (broken input, cut after its key line): named, the case runs alone
file(WRITE ${store}/t_fails "bundle-verdict v=1\nkey=0000000000000000000000000000000000000000000000000000000000000000\n")
run(c rc --source ${s}/fails.test.bas ${case} --test t_fails --from-bundle selftest_bundle_a)
want_rc("t_fails" "${rc}" 4)
want("t_fails" "${c}" "the verdict stored for t_fails is malformed \\(no bundle\\); the case runs alone\n")
want_not("t_fails" "${c}" "@served")
case_block(x fails "${c}")
case_block(y fails "${a}")
want_rc("t_fails: its block alone" "${x}" "${y}")
run(c rc --source ${s}/touched.test.bas ${case} --test t_touched --from-bundle selftest_bundle_a)
want_rc("t_touched" "${rc}" 0)
want("t_touched" "${c}" "the verdict stored for t_touched is not of these inputs; the case runs alone\n")
want("t_touched" "${c}" "@stat ")
want_not("t_touched" "${c}" "@served")
run(c rc --require-deviation --source ${s}/open-bug.test.bas ${case} --test t_open_bug
	--from-bundle selftest_bundle_b)
want_rc("t_open_bug" "${rc}" 1)
want("t_open_bug" "${c}" "open-bug\\.test\\.bas: case one: REPRODUCED\ncorpusrun case: verdict from the bundle selftest_bundle_b\n@served v=1 test=t_open_bug bundle=selftest_bundle_b\n")
want_not("t_open_bug" "${c}" "@stat")
foreach(t ab3_one ab3_two)
	string(REPLACE "_" "-" id ${t})
	run(c rc --source ${s}/${id}.test.bas ${case} --test t_${t} --from-bundle selftest_bundle_c)
	want_rc("t_${t}" "${rc}" 0)
	want("t_${t}" "${c}" "corpusrun case: verdict from the bundle selftest_bundle_c\n")
	want_not("t_${t}" "${c}" "@stat")
endforeach()
# every entry read once: the store is gone
if(EXISTS ${store})
	message(FATAL_ERROR "bundle-selftest: ${store} is left")
endif()

# the served verdicts are those of the runs alone
run(c rc --source ${s}/passes.test.bas ${case})
want_rc("passes alone" "${rc}" 0)
case_block(x passes "${c}")
want_rc("passes: the served block and the block alone" "${served}" "${x}")
case_block(y passes "${a}")
want_rc("passes: its block in the bundle and alone" "${y}" "${x}")
run(c rc --require-deviation --source ${s}/open-bug.test.bas ${case})
want_rc("open-bug alone" "${rc}" 1)
verdict(v open-bug "${c}")
verdict(w open-bug "${b}")
want_rc("open-bug: its verdict alone" "${v}" "${w}")

file(REMOVE_RECURSE ${WORK})
message("bundle-selftest: as expected")
