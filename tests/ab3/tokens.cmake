# ctest ab3_tokens*: the token list of blitz2-ab3 in AB3 mode (its
# AmiBlitz3 list, blitztokens -last LAST) with AmiBlitz3's libraries must
# equal the list of AmiBlitz3's own compiler (its only list) with the same
# libraries - same names at the same numbers.  Exit 77 (skipped) if a file
# is missing (AmiBlitz3's: cmake -P cmake/fetch-originals.cmake).
#   cmake -DBLITZTOKENS=... -DBLITZ2=... -DLAST=XXXX -DAB3=... "-DLIBS=A;B..."
#         -DOUT=PREFIX -P tokens.cmake
foreach(f IN LISTS BLITZ2 AB3 LIBS)
	if(NOT EXISTS ${f})
		message(STATUS "no ${f}: skipped")
		cmake_language(EXIT 77)
	endif()
endforeach()
execute_process(COMMAND ${BLITZTOKENS} -last ${LAST} -o ${OUT}.txt ${BLITZ2} ${LIBS}
	RESULT_VARIABLE rc)
if(rc)
	message(FATAL_ERROR "blitztokens ${BLITZ2}: exit ${rc}")
endif()
execute_process(COMMAND ${BLITZTOKENS} -o ${OUT}-ab3.txt ${AB3} ${LIBS} RESULT_VARIABLE rc)
if(rc)
	message(FATAL_ERROR "blitztokens ${AB3}: exit ${rc}")
endif()
file(STRINGS ${OUT}.txt ours)
list(LENGTH ours n)
execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${OUT}.txt ${OUT}-ab3.txt
	RESULT_VARIABLE diff)
if(NOT diff)
	message(STATUS "${n} tokens, equal to AmiBlitz3's")
	return()
endif()
# the first difference
file(STRINGS ${OUT}-ab3.txt theirs)
list(LENGTH theirs m)
set(i 0)
while(i LESS n AND i LESS m)
	list(GET ours ${i} a)
	list(GET theirs ${i} b)
	if(NOT a STREQUAL b)
		math(EXPR line "${i} + 1")
		message(FATAL_ERROR "${OUT}.txt differs from AmiBlitz3's (${OUT}-ab3.txt) "
			"at line ${line}: \"${a}\", AmiBlitz3 \"${b}\"")
	endif()
	math(EXPR i "${i} + 1")
endwhile()
message(FATAL_ERROR "${OUT}.txt differs from AmiBlitz3's (${OUT}-ab3.txt): "
	"${n} tokens, AmiBlitz3 ${m}")
