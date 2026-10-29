# ctest tokens_*: the token list blitztokens makes from a compiler and its
# libraries must equal EXPECT (tests/data/ted-tokens.txt, made by the
# original TED under blitz2 1.95 with the CD's libraries).  Exit 77
# (skipped) if a file is missing (the CD: cmake -P
# cmake/fetch-originals.cmake).
#   cmake -DBLITZTOKENS=... -DEXPECT=... -DOUT=... "-DFILES=BLITZ2;LIB..."
#         -P check.cmake
foreach(f IN LISTS FILES)
	if(NOT EXISTS ${f})
		message(STATUS "no ${f}: skipped")
		cmake_language(EXIT 77)
	endif()
endforeach()
execute_process(COMMAND ${BLITZTOKENS} -o ${OUT} ${FILES} RESULT_VARIABLE rc)
if(rc)
	message(FATAL_ERROR "blitztokens: exit ${rc}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${EXPECT} ${OUT} RESULT_VARIABLE diff)
if(diff)
	message(FATAL_ERROR "${OUT} differs from ${EXPECT}")
endif()
file(STRINGS ${OUT} lines)
list(LENGTH lines n)
message(STATUS "${n} tokens, equal to ${EXPECT}")
