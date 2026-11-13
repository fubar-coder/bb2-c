# record.cmake - the target corpus-expect-record (tests/corpus/CMakeLists.txt,
# OpenSpec change corpus-test-model, design D4): the expected data of the
# corpus (tests/corpus/expect/PASS) written from two runs of the original
# that must agree.  Only orchestration: the runs are corpusrun pass
# --observe ref (the second with --layout-stack: a Stack 16384 bytes
# larger, another memory layout), the judge is results corpus-accept
# (tools/lib/corpusexpect).
#
#   cmake -DLIST=FILE -DEXPECT=DIR -DRESULTS=results
#         -DCORPUSRUN=corpusrun -DWORK=DIR [-DFILTER=PASS[:NAME],...]
#         [-DPARALLEL=N] [-DFAKE=DIR] -P record.cmake
#
# LIST: one line per pass, PASS|ALLFILE|ORIGFILE: PASS the directory of
# EXPECT, ORIGFILE the pass file of its run with the original
# (NAME_originals: the configuration ref; ALLFILE is not used here).
# FILTER: only these passes, or only these programs of a pass (PASS:NAME;
# separated by , or ;); the others are neither run nor touched.  PARALLEL:
# passes recorded at the same time (both runs of each at once; the
# emulator slots limit the sessions).  FAKE: no runner, the runs are
# DIR/PASS/run1 and DIR/PASS/run2 (the test of the orchestration; a missing
# one is a failed run).  A program whose runs differ is reported ("test not
# deterministic") and its files stay as they were; the target fails at the
# end if any program was refused or a run failed.  A skipped run (77: no
# emulator, no CD) skips the pass, and the target fails as well (nothing
# was recorded).
#
# Internal: with -DONE=1 the script is one run (CORPUSRUN pass --observe ref
# --to OUT [--layout-stack] [--only NAME]... PASSFILE, its output to LOG,
# its exit status to LOG.rc; with FAKE the copy of FAKE/PASS/runK).

if(ONE)
	if(FAKE)
		file(REMOVE_RECURSE ${OUT})
		if(IS_DIRECTORY ${FAKE}/${PASS}/run${K})
			file(COPY ${FAKE}/${PASS}/run${K}/ DESTINATION ${OUT})
			file(WRITE ${LOG} "fake: ${FAKE}/${PASS}/run${K}\n")
			set(rc 0)
		else()
			file(WRITE ${LOG} "fake: no ${FAKE}/${PASS}/run${K}\n")
			set(rc 2)
		endif()
	else()
		set(args pass --observe ref --to ${OUT})
		if(K EQUAL 2)
			list(APPEND args --layout-stack)
		endif()
		string(REPLACE "," ";" only "${ONLY}")
		foreach(n IN LISTS only)
			list(APPEND args --only ${n})
		endforeach()
		execute_process(COMMAND ${CORPUSRUN} ${args} ${PASSFILE} OUTPUT_FILE ${LOG} ERROR_FILE ${LOG}
			RESULT_VARIABLE rc)
	endif()
	file(WRITE ${LOG}.rc "${rc}")
	return()
endif()

foreach(v LIST EXPECT RESULTS WORK)
	if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
		message(FATAL_ERROR "record.cmake: -D${v}=... missing")
	endif()
endforeach()
if(NOT FAKE AND (NOT DEFINED CORPUSRUN OR CORPUSRUN STREQUAL ""))
	message(FATAL_ERROR "record.cmake: -DCORPUSRUN=... missing")
endif()
if(NOT PARALLEL)
	set(PARALLEL 2)
endif()
file(MAKE_DIRECTORY ${WORK})
file(STRINGS ${LIST} PASSES)

# the filter: whole passes, and the programs of a pass (only_PASS)
set(known "")
foreach(e IN LISTS PASSES)
	string(REPLACE "|" ";" f "${e}")
	list(GET f 0 pass)
	list(APPEND known ${pass})
	set(only_${pass} "")
	set(take_${pass} FALSE)
endforeach()
string(REPLACE "," ";" filter "${FILTER}")
set(any FALSE)
foreach(f IN LISTS filter)
	string(STRIP "${f}" f)
	if(f STREQUAL "")
		continue()
	endif()
	set(any TRUE)
	string(FIND "${f}" ":" colon)
	if(colon EQUAL -1)
		set(pass "${f}")
		set(name "")
	else()
		string(SUBSTRING "${f}" 0 ${colon} pass)
		math(EXPR colon "${colon} + 1")
		string(SUBSTRING "${f}" ${colon} -1 name)
	endif()
	if(NOT pass IN_LIST known)
		message(FATAL_ERROR "corpus-expect-record: ${f}: no pass ${pass} (${known})")
	endif()
	set(take_${pass} TRUE)
	if(name STREQUAL "")
		set(whole_${pass} TRUE)
	else()
		list(APPEND only_${pass} ${name})
	endif()
endforeach()
set(selected "")
foreach(e IN LISTS PASSES)
	string(REPLACE "|" ";" f "${e}")
	list(GET f 0 pass)
	if(NOT any OR take_${pass})
		if(whole_${pass})
			set(only_${pass} "")
		endif()
		list(APPEND selected "${e}")
	endif()
endforeach()
list(LENGTH selected n)
message(STATUS "corpus-expect-record: ${n} passes, ${PARALLEL} at a time")

set(accepted "")
set(refused "")
set(failed "")
set(skipped "")
while(selected)
	# a batch: both runs of each pass at the same time (a pipeline of
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
		list(GET f 0 pass)
		list(GET f 2 passfile)
		string(REPLACE ";" "," only "${only_${pass}}")
		file(MAKE_DIRECTORY ${WORK}/${pass})
		foreach(k 1 2)
			list(APPEND cmds COMMAND ${CMAKE_COMMAND} -DONE=1 -DK=${k} -DPASS=${pass}
				-DPASSFILE=${passfile} -DCORPUSRUN=${CORPUSRUN} "-DONLY=${only}"
				-DOUT=${WORK}/${pass}/run${k} -DLOG=${WORK}/${pass}/run${k}.log
				"-DFAKE=${FAKE}" -P ${CMAKE_CURRENT_LIST_FILE})
		endforeach()
	endforeach()
	execute_process(${cmds} RESULTS_VARIABLE ignored)
	foreach(e IN LISTS batch)
		string(REPLACE "|" ";" f "${e}")
		list(GET f 0 pass)
		set(bad "")
		set(skip FALSE)
		foreach(k 1 2)
			set(rc "")
			if(EXISTS ${WORK}/${pass}/run${k}.log.rc)
				file(READ ${WORK}/${pass}/run${k}.log.rc rc)
			endif()
			if(rc STREQUAL "77")
				set(skip TRUE)
			elseif(NOT rc STREQUAL "0")
				list(APPEND bad "run ${k}: ${rc}")
			endif()
		endforeach()
		if(skip)
			message(STATUS "corpus-expect-record: ${pass}: skipped (${WORK}/${pass}/run1.log)")
			list(APPEND skipped ${pass})
			continue()
		endif()
		if(bad)
			message(STATUS "corpus-expect-record: ${pass}: the runs failed (${bad}; "
				"${WORK}/${pass}/run1.log, run2.log)")
			list(APPEND failed ${pass})
			continue()
		endif()
		set(args "")
		foreach(n IN LISTS only_${pass})
			list(APPEND args --only ${n})
		endforeach()
		execute_process(COMMAND ${RESULTS} corpus-accept ${WORK}/${pass}/run1 ${WORK}/${pass}/run2
			${EXPECT}/${pass} ${args} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
		string(STRIP "${out}${err}" text)
		message(STATUS "${text}")
		if(rc EQUAL 0)
			list(APPEND accepted ${pass})
		elseif(rc EQUAL 1)
			list(APPEND refused ${pass})
		else()
			list(APPEND failed ${pass})
		endif()
	endforeach()
endwhile()

list(LENGTH accepted na)
list(LENGTH refused nr)
list(LENGTH failed nf)
list(LENGTH skipped ns)
message(STATUS "corpus-expect-record: ${na} passes accepted, ${nr} with a program refused "
	"(not deterministic or no output), ${nf} failed, ${ns} skipped")
if(refused OR failed OR skipped)
	message(FATAL_ERROR "corpus-expect-record: refused: ${refused}; failed: ${failed}; "
		"skipped: ${skipped} - their programs' files are unchanged")
endif()
