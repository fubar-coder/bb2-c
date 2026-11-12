# One test of tedsrc tok (ctest, see ../CMakeLists.txt):
#   cmake -DTEDSRC=... -DTOKENS=... -DIN=... -DOUT=... -DEXPECT=FILE -P tok.cmake
#       IN (text) must give exactly EXPECT (the tokenised file)
file(REMOVE ${OUT})
execute_process(COMMAND ${TEDSRC} tok ${TOKENS} ${IN} ${OUT}
	RESULT_VARIABLE rc ERROR_VARIABLE err)
message(STATUS "tedsrc tok: exit ${rc}\n${err}")
if(NOT rc EQUAL 0)
	message(FATAL_ERROR "exit status ${rc}, 0 expected")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${OUT} ${EXPECT} RESULT_VARIABLE diff)
if(diff)
	message(FATAL_ERROR "${OUT} differs from ${EXPECT}")
endif()
