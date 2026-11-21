# cmake -DNM=NM -DLIB=ARCHIVE -P check-writer.cmake - does the writer of
# the result file (basictest_write.c), compiled with the Blitz library
# conventions, stand on its own?  A Blitz library has no libc and no
# libgcc: a symbol the archive refers to but does not define (a division
# helper such as __udivsi3, memcpy, ...) fails the check, naming it.
# Runs after the archive is built (POST_BUILD, CMakeLists.txt); the ctest
# basictest_writer_check_broken gives it a writer that divides.
cmake_minimum_required(VERSION 4.3)

if(NOT NM OR NOT LIB)
	message(FATAL_ERROR "usage: cmake -DNM=NM -DLIB=ARCHIVE -P check-writer.cmake")
endif()
execute_process(COMMAND ${NM} -u ${LIB} RESULT_VARIABLE rc OUTPUT_VARIABLE out
	ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
	message(FATAL_ERROR "check-writer: ${NM} -u ${LIB} failed: ${err}")
endif()
string(REGEX MATCHALL "\n[ \t]*U[ \t]+[^ \t\n]+" undefined "\n${out}")
set(names)
foreach(u IN LISTS undefined)
	string(REGEX REPLACE "^\n[ \t]*U[ \t]+" "" u "${u}")
	list(APPEND names ${u})
endforeach()
if(names)
	list(REMOVE_DUPLICATES names)
	list(JOIN names " " names)
	message(FATAL_ERROR "check-writer: ${LIB} refers to symbols it does not define "
		"(a Blitz library has no libc and no libgcc): ${names}")
endif()
