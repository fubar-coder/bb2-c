# ctest ted_checkdata_reference (label originals): the committed reference
# data of checkdata (tests/data/ted-checkdata.txt) against a fresh
# reference build of ted.s.  refcheck data-ref writes the data of TABLE
# from LISTING and REFERENCE into OUT, which must be DATA byte for byte.
#   cmake -DREFCHECK=... -DTABLE=checkdata.tsv -DLISTING=ref/ted.lst
#         -DREFERENCE=ref/ted -DOUT=FILE -DDATA=tests/data/ted-checkdata.txt
#         [-DNO_RUN=ON] -P checkdata-reference.cmake
# NO_RUN: OUT is there already, no refcheck run (the test with broken
# input).  Exit status 0 the same, 1 not.
cmake_minimum_required(VERSION 4.2.3)

foreach(v OUT DATA)
	if(NOT ${v})
		message(FATAL_ERROR "checkdata-reference.cmake: -D${v}=... missing")
	endif()
endforeach()
if(NOT NO_RUN)
	execute_process(COMMAND ${REFCHECK} data-ref ${TABLE} ${LISTING} ${REFERENCE} -o ${OUT}
		RESULT_VARIABLE rc)
	if(rc)
		message(FATAL_ERROR "ted_checkdata_reference: refcheck data-ref failed (${rc})")
	endif()
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${OUT} ${DATA} RESULT_VARIABLE rc)
if(rc)
	message(FATAL_ERROR "ted_checkdata_reference: the reference build gives other data than "
		"${DATA} (diff against ${OUT}; cmake --build --preset amiga --target ted-checkdata "
		"writes it)")
endif()
message(STATUS "ted_checkdata_reference: the data of ${DATA} are those of the reference build")
