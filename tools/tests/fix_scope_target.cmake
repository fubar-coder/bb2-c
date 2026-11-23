# The orchestration of the target fix-scope (cmake/fix-scope.cmake) on a
# throwaway git repository made in WORK: main holds expected data and two
# open bug-orig tests, the branch fix changes the data and fixes one test
# (its header "; @fixed-orig #7", its ID.expect) and turns a corpus program's
# expected output into its pair, in a commit with "Fixes #7".
# Only the fix's own cases: passes; an unqualified case changed too (in the
# working tree): fails naming the case; the other bug-orig test's
# expectation changed instead: fails naming its case; another corpus
# program's output changed instead: fails naming its case.  The expected
# data of another machine (libs/tests/expected-MACHINE): main has m1's,
# the fix adds m2's (every case [fixed #7]: passes); a changed case of
# m1, a case of another issue in m2 and m1's directory removed fail
# naming the case.
#   cmake -DRESULTS=results -DSCRIPT=fix-scope.cmake -DWORK=DIR -P fix_scope_target.cmake
foreach(v RESULTS SCRIPT WORK)
	if(NOT DEFINED ${v})
		message(FATAL_ERROR "fix_scope_target.cmake: -D${v}=... missing")
	endif()
endforeach()
find_program(GIT git REQUIRED)
set(repo ${WORK}/repo)
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${repo}/libs/tests/expected ${repo}/tests/bug-orig/compiler
	${repo}/tests/corpus/expect/corpus ${repo}/libs/tests/expected-m1 ${repo}/libs/tests/expected-m2)

function(git)
	execute_process(COMMAND ${GIT} -C ${repo} -c user.name=test -c user.email=test@localhost
		-c commit.gpgsign=false ${ARGN} RESULT_VARIABLE rc OUTPUT_QUIET ERROR_VARIABLE err)
	if(rc)
		message(FATAL_ERROR "git ${ARGN}: ${err}")
	endif()
endfunction()
function(expected text)
	file(WRITE ${repo}/libs/tests/expected/prog.txt "# capture prog 1\n${text}")
endfunction()
# the expected data of the machine M: the program mprog of m1, nprog of m2
function(machine m prog text)
	file(WRITE ${repo}/libs/tests/expected-${m}/${prog}.txt "# capture ${prog} 1\n${text}")
endfunction()
# bug-orig test ID: HEADER the header lines, EXPECT the expectation
function(bug_orig id header expect)
	set(d ${repo}/tests/bug-orig/compiler)
	file(WRITE ${d}/${id}.bas "; ${id}\n${header}Print 41\n")
	file(WRITE ${d}/${id}.expect "${expect}")
	file(WRITE ${d}/${id}.orig "# the original\n[session]\ndone\n[run]\n42\nrc=0\n")
endfunction()
function(run want)
	execute_process(COMMAND ${CMAKE_COMMAND} -DRESULTS=${RESULTS} -DSOURCE=${repo} -DBASE=main
		-DWORK=${WORK}/work -P ${SCRIPT} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
	message(STATUS "${out}")
	if(want STREQUAL "pass" AND NOT rc EQUAL 0)
		message(FATAL_ERROR "fix-scope failed on the fix's own cases")
	elseif(NOT want STREQUAL "pass" AND (rc EQUAL 0 OR NOT out MATCHES "${want}"))
		message(FATAL_ERROR "fix-scope did not fail with \"${want}\"")
	endif()
endfunction()

git(init -q -b main .)
expected("r|c||1\t$0\nr|d||1\t$1\n")
bug_orig(t1 "" "[run]\n41\nrc=0\n")
bug_orig(t2 "" "[run]\n41\nrc=0\n")
set(ce ${repo}/tests/corpus/expect/corpus)
file(WRITE ${ce}/t01_int.out "1\nrc=0\n")
file(WRITE ${ce}/t02_float.out "2\nrc=0\n")
machine(m1 mprog "r|e||1 [fixed #5]\t$1\n")
git(add .)
git(commit -q -m base)
git(checkout -q -b fix)
expected("r|c||1 [orig]\t$0\nr|c||1 [fixed #7]\t$5\nr|d||1\t$1\n")
bug_orig(t1 "; @fixed-orig #7\n" "[run]\n41\nrc=*\n")
file(RENAME ${ce}/t02_float.out ${ce}/t02_float.orig.out)
file(WRITE ${ce}/t02_float.fixed-7.out "2.5\nrc=0\n")
machine(m2 nprog "r|n||1 [fixed #7]\t$3\n")
git(add -A .)
git(commit -q -m "fix: c\n\nFixes #7")
run(pass)
expected("r|c||1 [orig]\t$0\nr|c||1 [fixed #7]\t$5\nr|d||1\t$2\n")
run("prog: r\\|d\\|\\|1: changed \\(\\$1 -> \\$2\\)")
expected("r|c||1 [orig]\t$0\nr|c||1 [fixed #7]\t$5\nr|d||1\t$1\n")
bug_orig(t2 "" "[run]\n43\nrc=0\n")
run("bug-orig: t2\\|run\\|\\|1: changed")
bug_orig(t2 "" "[run]\n41\nrc=0\n")
file(WRITE ${ce}/t01_int.out "3\nrc=0\n")
run("corpus: corpus\\|t01_int\\|out\\|1: changed")
file(WRITE ${ce}/t01_int.out "1\nrc=0\n")
machine(m1 mprog "r|e||1 [fixed #5]\t$2\n")
run("fix-scope: libs/tests/expected-m1: [^\n]*\n(.*\n)*mprog: r\\|e\\|\\|1 \\[fixed #5\\]: changed \\(\\$1 -> \\$2\\)")
machine(m1 mprog "r|e||1 [fixed #5]\t$1\n")
machine(m2 nprog "r|n||1 [fixed #7]\t$3\nr|o||1 [fixed #8]\t$4\n")
run("fix-scope: libs/tests/expected-m2: [^\n]*\n(.*\n)*nprog: r\\|o\\|\\|1 \\[fixed #8\\]: added")
machine(m2 nprog "r|n||1 [fixed #7]\t$3\n")
file(REMOVE_RECURSE ${repo}/libs/tests/expected-m1)
run("fix-scope: libs/tests/expected-m1: [^\n]*\n(.*\n)*mprog: r\\|e\\|\\|1 \\[fixed #5\\]: removed")
message(STATUS "fix_scope_target: all runs as expected")
