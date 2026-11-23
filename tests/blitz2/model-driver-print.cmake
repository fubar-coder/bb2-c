# model-driver-print.cmake - the driver of all cases that `corpusrun case
# --print-driver` prints without --case is the text the build step writes
# (corpusrun case --write-driver, its NAME.bas): CMD's stdout must be the
# file DRIVER, byte for byte.
#   cmake "-DCMD=CORPUSRUN;case;--env;ENV;--source;SRC;--print-driver" -DDRIVER=NAME.bas
#         -DOUT=FILE -P model-driver-print.cmake
execute_process(COMMAND ${CMD} RESULT_VARIABLE rc OUTPUT_FILE ${OUT} ERROR_VARIABLE err)
if(NOT rc STREQUAL "0")
	message(FATAL_ERROR "${CMD}: exit status ${rc}\n${err}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${OUT} ${DRIVER} RESULT_VARIABLE diff)
if(NOT diff STREQUAL "0")
	message(FATAL_ERROR "the printed driver ${OUT} differs from ${DRIVER}")
endif()
file(REMOVE ${OUT})
message("${DRIVER}: printed the same")
