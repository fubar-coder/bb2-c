# ctest tokens_*: the token list bbsrc tokens makes from a compiler and its
# libraries must equal EXPECT (tests/data/ted-tokens.txt, made by the
# original TED under blitz2 1.95 with the CD's libraries).  Exit 77
# (skipped) if a file is missing (the CD: cmake -P
# cmake/fetch-originals.cmake).
#   cmake -DBBSRC=... -DEXPECT=... -DOUT=... "-DFILES=BLITZ2;LIB..."
#         [-DADDITION=FILE] -P tokens-check.cmake
# ADDITION: lines the list has besides EXPECT (each once, anywhere)
foreach(f IN LISTS FILES)
	if(NOT EXISTS ${f})
		message(STATUS "no ${f}: skipped")
		cmake_language(EXIT 77)
	endif()
endforeach()
execute_process(COMMAND ${BBSRC} tokens -o ${OUT} ${FILES} RESULT_VARIABLE rc)
if(rc)
	message(FATAL_ERROR "bbsrc tokens: exit ${rc}")
endif()
if(DEFINED ADDITION)
	# the lines of ADDITION are tokens of this repository's own (literallib,
	# #1018): each must be in the list once, and the list without them must
	# be EXPECT
	file(READ ${OUT} have)
	file(READ ${ADDITION} add)
	file(READ ${EXPECT} want)
	string(REGEX MATCHALL "[^\n]*\n" addlines "${add}")
	set(have "\n${have}")
	foreach(l IN LISTS addlines)
		string(FIND "${have}" "\n${l}" at)
		if(at LESS 0)
			message(FATAL_ERROR "${OUT}: the addition's token ${l} is missing")
		endif()
		string(REPLACE "\n${l}" "\n" have "${have}")
	endforeach()
	string(REGEX REPLACE "^\n" "" have "${have}")
	if(NOT have STREQUAL want)
		message(FATAL_ERROR "${OUT} without the lines of ${ADDITION} differs from ${EXPECT}")
	endif()
	list(LENGTH addlines nadd)
	message(STATUS "${nadd} tokens of ${ADDITION}")
else()
	execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${EXPECT} ${OUT} RESULT_VARIABLE diff)
	if(diff)
		message(FATAL_ERROR "${OUT} differs from ${EXPECT}")
	endif()
endif()
file(STRINGS ${OUT} lines)
list(LENGTH lines n)
message(STATUS "${n} tokens, equal to ${EXPECT}")
