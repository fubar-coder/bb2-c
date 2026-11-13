# record.cmake - the target bug-orig-record (tests/bug-orig/CMakeLists.txt,
# OpenSpec change bug-orig-test-model, design D3): the recorded original
# ID.orig of the bug-orig tests, written from two recording runs of the
# original that must agree.  Only orchestration: the runs are corpusrun
# pass / tedab run with the pass key record=write (the second Blitz run
# with layout=pad: another memory layout), the judge is results
# bug-orig-accept (tools/lib/bugorigcmp).
#
#   cmake -DLIST=FILE -DRESULTS=results -DCORPUSRUN=corpusrun -DTEDAB=tedab
#         -DWORK=DIR [-DFILTER=ID,...] [-DPARALLEL=N] [-DFAKE=DIR] -P record.cmake
#
# LIST: one line per test, ID|KIND|PASS1|PASS2|RUN1|RUN2|ORIG (KIND blitz
# or ted; PASSk the pass files of the two runs, RUNk the observations they
# write, ORIG the test's ID.orig).  FILTER: only these tests (IDs or globs
# with * and ?, separated by , or ;); the others are neither run nor
# touched.  PARALLEL: tests recorded at the same time (both runs of each at
# once; the emulator slots limit the sessions).  FAKE: no runner, the runs
# are DIR/ID.run1 and DIR/ID.run2 (the test of the orchestration; a
# missing one is a failed run).  A test whose runs differ is reported
# ("test not deterministic") and its ID.orig stays as it was; the target
# fails at the end if any test was refused or failed.  A skipped run (77:
# no emulator, a file of the pass missing) skips the test.
#
# Internal: with -DONE=1 the script is one run (RUNNER SUB PASS, its output
# to LOG, its exit status to LOG.rc; with FAKE the copy of FAKE/ID.runK).

if(ONE)
	if(FAKE)
		if(EXISTS ${FAKE}/${ID}.run${K})
			file(COPY_FILE ${FAKE}/${ID}.run${K} ${OUT})
			set(rc 0)
		else()
			file(WRITE ${LOG} "fake: no ${FAKE}/${ID}.run${K}\n")
			set(rc 2)
		endif()
	else()
		execute_process(COMMAND ${RUNNER} ${SUB} ${PASS} OUTPUT_FILE ${LOG} ERROR_FILE ${LOG}
			RESULT_VARIABLE rc)
	endif()
	file(WRITE ${LOG}.rc "${rc}")
	return()
endif()

foreach(v LIST RESULTS WORK)
	if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
		message(FATAL_ERROR "record.cmake: -D${v}=... missing")
	endif()
endforeach()
if(NOT PARALLEL)
	set(PARALLEL 4)
endif()
file(MAKE_DIRECTORY ${WORK})

# the filter as regular expressions
set(patterns "")
string(REPLACE "," ";" filter "${FILTER}")
foreach(f IN LISTS filter)
	string(STRIP "${f}" f)
	if(f STREQUAL "")
		continue()
	endif()
	string(REGEX REPLACE "([][.+^$(){}|\\])" "\\\\\\1" re "${f}")
	string(REPLACE "*" ".*" re "${re}")
	string(REPLACE "?" "." re "${re}")
	list(APPEND patterns "^${re}$")
endforeach()

file(STRINGS ${LIST} entries)
set(selected "")
foreach(e IN LISTS entries)
	string(REPLACE "|" ";" f "${e}")
	list(GET f 0 id)
	set(take TRUE)
	if(patterns)
		set(take FALSE)
		foreach(re IN LISTS patterns)
			if(id MATCHES "${re}")
				set(take TRUE)
			endif()
		endforeach()
	endif()
	if(take)
		list(APPEND selected "${e}")
	endif()
endforeach()
if(NOT selected)
	message(FATAL_ERROR "bug-orig-record: no test matches the filter \"${FILTER}\"")
endif()
list(LENGTH selected n)
message(STATUS "bug-orig-record: ${n} tests, ${PARALLEL} at a time")

set(written "")
set(unchanged "")
set(refused "")
set(failed "")
set(skipped "")
while(selected)
	# a batch: both runs of each test at the same time (a pipeline of
	# processes that write nothing to stdout)
	set(batch "")
	set(cmds "")
	foreach(i RANGE 1 ${PARALLEL})
		if(NOT selected)
			break()
		endif()
		list(POP_FRONT selected e)
		list(APPEND batch "${e}")
		string(REPLACE "|" ";" f "${e}")
		list(GET f 0 id)
		list(GET f 1 kind)
		if(kind STREQUAL "ted")
			set(runner ${TEDAB})
			set(sub run)
		else()
			set(runner ${CORPUSRUN})
			set(sub pass)
		endif()
		foreach(k 1 2)
			math(EXPR pi "${k} + 1")
			math(EXPR oi "${k} + 3")
			list(GET f ${pi} pass)
			list(GET f ${oi} out)
			file(REMOVE ${out} ${WORK}/${id}.run${k}.log ${WORK}/${id}.run${k}.log.rc)
			list(APPEND cmds COMMAND ${CMAKE_COMMAND} -DONE=1 -DRUNNER=${runner} -DSUB=${sub}
				-DPASS=${pass} -DOUT=${out} -DLOG=${WORK}/${id}.run${k}.log -DID=${id} -DK=${k}
				-DFAKE=${FAKE} -P ${CMAKE_CURRENT_LIST_FILE})
		endforeach()
	endforeach()
	execute_process(${cmds} OUTPUT_QUIET)
	foreach(e IN LISTS batch)
		string(REPLACE "|" ";" f "${e}")
		list(GET f 0 id)
		list(GET f 4 run1)
		list(GET f 5 run2)
		list(GET f 6 orig)
		set(bad "")
		foreach(k 1 2)
			set(rc "none")
			if(EXISTS ${WORK}/${id}.run${k}.log.rc)
				file(READ ${WORK}/${id}.run${k}.log.rc rc)
			endif()
			if(rc STREQUAL "77")
				set(bad skipped)
			elseif(NOT rc STREQUAL "0" AND NOT bad)
				set(bad "run ${k} failed (exit status ${rc}, ${WORK}/${id}.run${k}.log)")
			endif()
		endforeach()
		if(bad STREQUAL "skipped")
			message(STATUS "${id}: skipped (no emulator, or a file of its pass is missing; "
				"${WORK}/${id}.run1.log)")
			list(APPEND skipped ${id})
			continue()
		elseif(bad)
			message(STATUS "${id}: ${bad}")
			list(APPEND failed ${id})
			continue()
		endif()
		execute_process(COMMAND ${RESULTS} bug-orig-accept --id ${id} ${run1} ${run2} ${orig}
			RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
		string(STRIP "${out}" out)
		message(STATUS "${out}")
		if(rc EQUAL 0 AND out MATCHES " unchanged$")
			list(APPEND unchanged ${id})
		elseif(rc EQUAL 0)
			list(APPEND written ${id})
		elseif(rc EQUAL 1)
			list(APPEND refused ${id})
		else()
			list(APPEND failed ${id})
		endif()
	endforeach()
endwhile()

foreach(l written unchanged refused failed skipped)
	list(LENGTH ${l} n_${l})
endforeach()
message(STATUS "bug-orig-record: ${n_written} written, ${n_unchanged} unchanged, ${n_refused} "
	"refused (not deterministic or a wildcard that does not match), ${n_failed} failed, "
	"${n_skipped} skipped")
if(skipped)
	list(JOIN skipped ", " s)
	message(STATUS "bug-orig-record: skipped: ${s}")
endif()
if(refused OR failed)
	list(JOIN refused ", " r)
	list(JOIN failed ", " x)
	message(FATAL_ERROR "bug-orig-record: not recorded - refused: ${r}; failed: ${x} (their "
		"ID.orig is unchanged)")
endif()
