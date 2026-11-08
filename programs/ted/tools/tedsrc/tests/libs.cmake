# ctest tedtok_libs: IN tokenised with the list file TOKENS and with the
# list made from the CD's compiler and libraries (CD = its BLITZ2
# directory) must give the same file.  Exit 77 (skipped) without the CD.
#   cmake -DTEDSRC=... -DTOKENS=... -DCD=... -DIN=... -DOUT=... -P libs.cmake
foreach(f BLITZ2 ACIDLIBS DEFLIBS)
	if(NOT EXISTS ${CD}/${f})
		message(STATUS "no ${CD}/${f}: skipped")
		cmake_language(EXIT 77)
	endif()
endforeach()
execute_process(COMMAND ${TEDSRC} tok ${TOKENS} ${IN} ${OUT}.list RESULT_VARIABLE rc1)
execute_process(COMMAND ${TEDSRC} tok --blitz2 ${CD}/BLITZ2 --lib ${CD}/ACIDLIBS --lib ${CD}/DEFLIBS
	${IN} ${OUT}.libs RESULT_VARIABLE rc2)
if(rc1 OR rc2)
	message(FATAL_ERROR "tedsrc tok: exit ${rc1} (list file), ${rc2} (libraries)")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${OUT}.list ${OUT}.libs RESULT_VARIABLE diff)
if(diff)
	message(FATAL_ERROR "${OUT}.libs differs from ${OUT}.list")
endif()
file(SIZE ${IN} a)
file(SIZE ${OUT}.libs b)
if(NOT b LESS a)
	message(FATAL_ERROR "${OUT}.libs (${b} bytes) not tokenised (${IN}: ${a} bytes)")
endif()
