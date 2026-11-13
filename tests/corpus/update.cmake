# update.cmake - the target corpus-expect-update (tests/corpus/CMakeLists.txt,
# OpenSpec change corpus-test-model, design D4): the expected data of the
# corpus (tests/corpus/expect/PASS) written from the port.  Only
# orchestration: the work is corpusrun pass --update-expect
# (tools/lib/corpusexpect).
#
#   cmake -DCORPUSRUN=corpusrun -DLIST=FILE
#         [-DFIX=PASS:NAME#N,...] -P update.cmake
#
# LIST: one line per pass, PASS|ALLFILE|ORIGFILE (PASS the directory of
# tests/corpus/expect, ALLFILE the pass file of ctest NAME, ORIGFILE that of
# NAME_originals).  For every pass: corpusrun pass --update-expect clib ALLFILE (the pass of
# ctest NAME: the outputs of blitz2-c with the C libraries) - a new program
# gets its files, a [fixed #N] file is rewritten, a changed unqualified
# file is refused unless FIX names the program (then it becomes the pair
# NAME.orig.EXT / NAME.fixed-N.EXT); then corpusrun pass --update-expect
# port ORIGFILE (the pass of NAME_originals: the executables of ref and
# port, only for the programs with an exe pair or a FIX; nothing to do: no
# session).  The second needs the CD: without it (77) it is skipped, and
# that fails the target only if FIX names a program of the pass.  The
# target fails at the end if a program was refused or a run failed.

foreach(v CORPUSRUN LIST)
	if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
		message(FATAL_ERROR "update.cmake: -D${v}=... missing")
	endif()
endforeach()
file(STRINGS ${LIST} PASSES)
set(fixargs "")
set(fixpasses "")
string(REPLACE "," ";" fixes "${FIX}")
set(known "")
foreach(e IN LISTS PASSES)
	string(REPLACE "|" ";" f "${e}")
	list(GET f 0 pass)
	list(APPEND known ${pass})
endforeach()
foreach(x IN LISTS fixes)
	string(STRIP "${x}" x)
	if(x STREQUAL "")
		continue()
	endif()
	if(NOT x MATCHES "^([A-Za-z0-9_-]+):[A-Za-z0-9_-]+#[1-9][0-9]*$")
		message(FATAL_ERROR "corpus-expect-update: FIX entry ${x}: not PASS:NAME#N")
	endif()
	if(NOT CMAKE_MATCH_1 IN_LIST known)
		message(FATAL_ERROR "corpus-expect-update: FIX entry ${x}: no pass ${CMAKE_MATCH_1} (${known})")
	endif()
	list(APPEND fixargs --fix ${x})
	list(APPEND fixpasses ${CMAKE_MATCH_1})
endforeach()

set(bad "")
foreach(e IN LISTS PASSES)
	string(REPLACE "|" ";" f "${e}")
	list(GET f 0 pass)
	list(GET f 1 allfile)
	list(GET f 2 origfile)
	foreach(step clib port)
		if(step STREQUAL "clib")
			set(file ${allfile})
		else()
			set(file ${origfile})
		endif()
		message(STATUS "corpus-expect-update: ${pass}: ${step}")
		execute_process(COMMAND ${CORPUSRUN} pass --update-expect ${step} ${fixargs} ${file}
			RESULT_VARIABLE rc)
		if(rc EQUAL 77 AND step STREQUAL "port" AND NOT pass IN_LIST fixpasses)
			message(STATUS "corpus-expect-update: ${pass}: the executables skipped (no CD or no "
				"emulator)")
		elseif(NOT rc EQUAL 0)
			list(APPEND bad "${pass} ${step} (${rc})")
		endif()
	endforeach()
endforeach()
if(bad)
	list(JOIN bad ", " bad)
	message(FATAL_ERROR "corpus-expect-update: refused or failed: ${bad} - see above; a refused "
		"program's files are unchanged")
endif()
