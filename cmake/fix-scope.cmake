# The target fix-scope (design D12 of the OpenSpec change
# libs-tests-capture): the cases of the expected data a branch changes
# beyond the issues its commits fix.  Only orchestration: the check is
# results fix-scope (tools/lib/libscmp).
#   cmake -DRESULTS=results -DSOURCE=CHECKOUT -DBASE=main -DWORK=DIR -P fix-scope.cmake
# The base files are those of the merge base of BASE and HEAD, exported
# with git archive into WORK/base (the checkout is not touched); the
# branch's are those of the checkout (the working tree).  The issues are
# the "Fixes #N" lines of the commits BASE..HEAD.  The directories of
# expected data: DIRS (libs/tests/expected; tests/bug-orig, the bug-orig
# tests, each projected onto cases: results fix-scope --bug-orig, OpenSpec
# change bug-orig-test-model, design D6).

foreach(v RESULTS SOURCE BASE WORK)
	if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
		message(FATAL_ERROR "fix-scope.cmake: -D${v}=... missing")
	endif()
endforeach()
set(DIRS libs/tests/expected tests/bug-orig)
set(OPTS_libs/tests/expected "")
set(OPTS_tests/bug-orig --bug-orig)
file(MAKE_DIRECTORY ${WORK})

execute_process(COMMAND git -C ${SOURCE} merge-base ${BASE} HEAD
	OUTPUT_VARIABLE base OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE rc ERROR_VARIABLE err)
if(rc OR NOT base)
	message(FATAL_ERROR "fix-scope: no merge base of ${BASE} and HEAD in ${SOURCE}: ${err}")
endif()
execute_process(COMMAND git -C ${SOURCE} log --format=%B ${base}..HEAD
	OUTPUT_FILE ${WORK}/log.txt RESULT_VARIABLE rc ERROR_VARIABLE err)
if(rc)
	message(FATAL_ERROR "fix-scope: git log ${base}..HEAD failed: ${err}")
endif()

file(REMOVE_RECURSE ${WORK}/base ${WORK}/empty)
file(MAKE_DIRECTORY ${WORK}/base ${WORK}/empty)
set(failed "")
foreach(d IN LISTS DIRS)
	# the base's files, if the base has the directory
	execute_process(COMMAND git -C ${SOURCE} ls-tree -d --name-only ${base} -- ${d}
		OUTPUT_VARIABLE have OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE rc)
	set(b ${WORK}/empty)
	if(NOT rc AND have)
		execute_process(COMMAND git -C ${SOURCE} archive --format=tar ${base} -- ${d}
			COMMAND tar -x -C ${WORK}/base
			RESULTS_VARIABLE rcs ERROR_VARIABLE err)
		list(GET rcs 0 rc1)
		list(GET rcs 1 rc2)
		if(rc1 OR rc2)
			message(FATAL_ERROR "fix-scope: git archive ${base} -- ${d} failed: ${err}")
		endif()
		set(b ${WORK}/base/${d})
	endif()
	set(r ${SOURCE}/${d})
	if(NOT IS_DIRECTORY ${r})
		set(r ${WORK}/empty)
	endif()
	message(STATUS "fix-scope: ${d}: ${BASE} (${base}) against the checkout")
	execute_process(COMMAND ${RESULTS} fix-scope ${OPTS_${d}} --log ${WORK}/log.txt ${b} ${r}
		RESULT_VARIABLE rc)
	if(NOT rc EQUAL 0)
		list(APPEND failed "${d} (${rc})")
	endif()
endforeach()
if(failed)
	list(JOIN failed ", " failed)
	message(FATAL_ERROR "fix-scope: cases beyond the issues of the branch, or an error: ${failed} "
		"- review each case and name it in the commit with its reason (design D12)")
endif()
