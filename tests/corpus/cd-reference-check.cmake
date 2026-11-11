# ctest blitz2_cd_reference (label originals): the committed CD result
# table against the original.  The CD through the original blitz2 2.10
# (corpusrun cd PASS, as the target blitz2-cd-reference), the table made
# from that run and the committed one (results cd-reference: the note rows
# stay, their note must name the original's result), and the table so made
# must be the committed one byte for byte.
#   cmake -DCORPUSRUN=corpusrun -DRESULTS=results -DPASS=cd-reference.pass
#         -DFRESH=cd-reference.tsv -DTABLE=tests/data/blitz2-cd.tsv
#         -DOUT=FILE [-DNO_RUN=ON] -P cd-reference-check.cmake
# NO_RUN: FRESH is given, no emulator run (the test with broken input).
# Exit status 0 the same, 1 not, 77 skipped (no CD or no Amiberry).
cmake_minimum_required(VERSION 4.2.3)

foreach(v RESULTS FRESH TABLE OUT)
	if(NOT ${v})
		message(FATAL_ERROR "cd-reference-check.cmake: -D${v}=... missing")
	endif()
endforeach()
if(NOT NO_RUN)
	execute_process(COMMAND ${CORPUSRUN} cd ${PASS} RESULT_VARIABLE rc)
	if(rc EQUAL 77)
		cmake_language(EXIT 77)
	elseif(rc)
		message(FATAL_ERROR "blitz2_cd_reference: corpusrun cd failed (${rc})")
	endif()
endif()
execute_process(COMMAND ${RESULTS} cd-reference ${FRESH} ${TABLE} ${OUT} RESULT_VARIABLE rc)
if(rc)
	message(FATAL_ERROR "blitz2_cd_reference: the table does not fit the original's run "
		"(results cd-reference)")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${OUT} ${TABLE} RESULT_VARIABLE rc)
if(rc)
	message(FATAL_ERROR "blitz2_cd_reference: the original's run differs from ${TABLE} "
		"(diff ${TABLE} ${OUT}; cmake --build --preset amiga --target blitz2-cd-reference "
		"writes it)")
endif()
message(STATUS "blitz2_cd_reference: ${TABLE} is the original's")
