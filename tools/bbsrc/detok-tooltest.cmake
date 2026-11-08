# ctest bbdetok_tool: bbsrc detok on small files - a text tokenised by
# bbsrc ab3 and back, a token the list does not have, broken input.
#   cmake -DBBSRC=... -DWORK=DIR -P detok-tooltest.cmake
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${WORK})
file(WRITE ${WORK}/tokens.txt "8020 NPrint\n8021 Print\nB201 Foo\n")
file(WRITE ${WORK}/short.txt "8020 NPrint\n8021 Print\n")
set(text "NPrint \"Print\"\n\nFoo 1 ;Print 2\nx=1\n")
file(WRITE ${WORK}/a.ab2 "${text}")

function(run expect_rc)
	execute_process(COMMAND ${ARGN} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
	if(NOT rc EQUAL expect_rc)
		message(FATAL_ERROR "${ARGN}: exit ${rc}, expected ${expect_rc}: ${err}")
	endif()
	set(out "${out}" PARENT_SCOPE)
	set(err "${err}" PARENT_SCOPE)
endfunction()

# tokenised as AmiBlitz3 does it (bbsrc ab3), detokenised: the same text,
# in OUT and on stdout
run(0 ${BBSRC} ab3 ${WORK}/tokens.txt ${WORK}/a.ab2 ${WORK}/a.bb2)
file(READ ${WORK}/a.bb2 hex HEX)
if(NOT hex MATCHES "^8020")
	message(FATAL_ERROR "a.bb2 is not tokenised: ${hex}")
endif()
run(0 ${BBSRC} detok --tokens ${WORK}/tokens.txt ${WORK}/a.bb2 ${WORK}/a.txt)
file(READ ${WORK}/a.txt got)
if(NOT got STREQUAL text OR NOT err STREQUAL "")
	message(FATAL_ERROR "a.txt:\n${got}\nstderr: ${err}")
endif()
run(0 ${BBSRC} detok --tokens ${WORK}/tokens.txt ${WORK}/a.bb2)
if(NOT out STREQUAL text)
	message(FATAL_ERROR "stdout:\n${out}")
endif()

# a list without Foo: {100:1}, counted; --unknown lists it; --strict fails
# and leaves no OUT
run(0 ${BBSRC} detok --tokens ${WORK}/short.txt --unknown ${WORK}/a.bb2)
if(NOT out MATCHES "\n\\{100:1\\} 1 ;Print 2\n"
		OR NOT err MATCHES "a.bb2: 1 of 2 tokens unknown \\(1 different numbers\\)\n\\{100:1\\} \\$B201 1\n$")
	message(FATAL_ERROR "unknown token:\n${out}\nstderr: ${err}")
endif()
run(1 ${BBSRC} detok --strict --tokens ${WORK}/short.txt ${WORK}/a.bb2 ${WORK}/a.txt)
if(EXISTS ${WORK}/a.txt)
	message(FATAL_ERROR "--strict: a.txt written")
endif()

# broken: a token byte at the end of the file (but in a string AmiBlitz's
# rules take it as a character), arguments, token lists, files
string(ASCII 120 61 49 178 half)              # x=1 $B2
file(WRITE ${WORK}/half.bb2 "${half}")
run(1 ${BBSRC} detok --tokens ${WORK}/tokens.txt ${WORK}/half.bb2)
if(NOT err MATCHES "half.bb2: line 1: token byte \\$B2 at the end of the file")
	message(FATAL_ERROR "half.bb2: ${err}")
endif()
string(ASCII 34 178 str)                      # "$B2
file(WRITE ${WORK}/str.bb2 "${str}")
run(0 ${BBSRC} detok --tokens ${WORK}/tokens.txt ${WORK}/str.bb2)
if(NOT err MATCHES "note: the last line has no 0 byte")
	message(FATAL_ERROR "str.bb2: ${err}")
endif()
run(1 ${BBSRC} detok --ted --tokens ${WORK}/tokens.txt ${WORK}/str.bb2)
run(2 ${BBSRC} detok ${WORK}/a.bb2)                               # no token list
run(2 ${BBSRC} detok --tokens ${WORK}/tokens.txt)                  # no IN
run(2 ${BBSRC} detok --tokens ${WORK}/tokens.txt ${WORK}/a.bb2 x y)
run(2 ${BBSRC} detok --what --tokens ${WORK}/tokens.txt ${WORK}/a.bb2)
run(2 ${BBSRC} detok --last 0 --blitz2 ${WORK}/a.bb2 ${WORK}/a.bb2)
run(2 ${BBSRC} detok --tokens ${WORK}/tokens.txt --last 80DB ${WORK}/a.bb2)   # no --blitz2 after it
file(WRITE ${WORK}/bad.txt "8020 NPrint\nPrint\n")
run(1 ${BBSRC} detok --tokens ${WORK}/bad.txt ${WORK}/a.bb2)
if(NOT err MATCHES "bad.txt: line 2: not \"XXXX name\"")
	message(FATAL_ERROR "bad.txt: ${err}")
endif()
run(1 ${BBSRC} detok --blitz2 ${WORK}/tokens.txt ${WORK}/a.bb2)
if(NOT err MATCHES "tokens.txt: not a load file")
	message(FATAL_ERROR "--blitz2 tokens.txt: ${err}")
endif()
run(1 ${BBSRC} detok --lib ${WORK}/tokens.txt ${WORK}/a.bb2)
run(1 ${BBSRC} detok --tokens ${WORK}/tokens.txt ${WORK}/missing.bb2)
run(1 ${BBSRC} detok --tokens ${WORK}/missing.txt ${WORK}/a.bb2)
run(1 ${BBSRC} detok --tokens ${WORK}/tokens.txt ${WORK}/a.bb2 ${WORK}/no/dir/a.txt)
