# ctest ab3tok_tool: ab3tok on small files.
#   cmake -DAB3TOK=... -DWORK=DIR -P tooltest.cmake
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${WORK})
file(WRITE ${WORK}/tokens.txt "8020 NPrint\n8021 Print\n9999 NPrint\n")
file(WRITE ${WORK}/a.ab3 "; XTRA\n; /XTRA\nNPrint \"Print\"\nnprint 1\n")
file(WRITE ${WORK}/other.ab3 "NPrint \"Print\"\nnprint 2\n")

function(run expect_rc)
	execute_process(COMMAND ${AB3TOK} ${ARGN} RESULT_VARIABLE rc ERROR_VARIABLE err)
	if(NOT rc EQUAL expect_rc)
		message(FATAL_ERROR "ab3tok ${ARGN}: exit ${rc}, expected ${expect_rc}: ${err}")
	endif()
	set(err "${err}" PARENT_SCOPE)
endfunction()

run(0 ${WORK}/tokens.txt ${WORK}/a.ab3 ${WORK}/a.bb2)
# two lines without the header, NPrint with its first entry
file(READ ${WORK}/a.bb2 hex HEX)
if(NOT hex STREQUAL "802020225072696e7422006e7072696e74203100")
	message(FATAL_ERROR "a.bb2: ${hex}")
endif()
run(0 -cmp ${WORK}/tokens.txt ${WORK}/a.ab3 ${WORK}/a.bb2)
# another source: line 2 differs
run(1 -cmp ${WORK}/tokens.txt ${WORK}/other.ab3 ${WORK}/a.bb2)
if(NOT err MATCHES "line 2 differs from AmiBlitz3's .*ours:      nprint 2\n  AmiBlitz3: nprint 1")
	message(FATAL_ERROR "ab3tok -cmp: ${err}")
endif()
# a file with fewer lines
run(1 -cmp ${WORK}/tokens.txt ${WORK}/a.ab3 ${WORK}/tokens.txt)
# broken: arguments, a token list, a missing file
run(2 ${WORK}/tokens.txt ${WORK}/a.ab3)
file(WRITE ${WORK}/bad.txt "8020 NPrint\nPrint\n")
run(2 ${WORK}/bad.txt ${WORK}/a.ab3 ${WORK}/x.bb2)
if(NOT err MATCHES "bad.txt:2: not \"XXXX name\"")
	message(FATAL_ERROR "ab3tok bad.txt: ${err}")
endif()
run(2 ${WORK}/tokens.txt ${WORK}/missing.ab3 ${WORK}/x.bb2)
