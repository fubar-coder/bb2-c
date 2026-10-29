# ctest: file A must equal file B (byte for byte); exit 77 (skipped) if B is
# missing (an original of the CD: cmake -P cmake/fetch-originals.cmake).
#   cmake -DA=... -DB=... -P compare-or-skip.cmake
if(NOT EXISTS ${B})
	message(STATUS "no ${B}: skipped")
	cmake_language(EXIT 77)
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${A} ${B} RESULT_VARIABLE diff)
if(diff)
	message(FATAL_ERROR "${A} differs from ${B}")
endif()
message(STATUS "${A} equals ${B}")
