# machines-test.cmake - the test of machines.cmake (the header line
# `; @machine NAME...` of a bug-orig test) with broken input: a name with
# other characters, a name given twice and a line without a name stop the
# configure naming the file and the name; several names pass in their
# order.  PASSES and MACHINES: the pass files of
# regression-selftest-machines and their machines, each pass with its one
# machine= line and its own work directory.
#   cmake -DSCRIPT=machines.cmake -DPASSES=A.pass;B.pass -DMACHINES=a;b
#         -P machines-test.cmake
foreach(v SCRIPT PASSES MACHINES)
	if(NOT ${v})
		message(FATAL_ERROR "machines-test.cmake: -D${v}=... missing")
	endif()
endforeach()
# TEXT: the names after @machine; WANT: the regular expression of the output
function(check text want)
	execute_process(COMMAND ${CMAKE_COMMAND} -DBAS=x.bas "-DTEXT=${text}" -P ${SCRIPT}
		RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
	if(want MATCHES "^machines: ")
		if(NOT rc EQUAL 0 OR NOT out MATCHES "${want}")
			message(FATAL_ERROR "machines-test: \"${text}\" did not pass as ${want}: ${out}")
		endif()
	elseif(rc EQUAL 0 OR NOT out MATCHES "${want}")
		message(FATAL_ERROR "machines-test: \"${text}\" did not fail with ${want}: ${out}")
	endif()
endfunction()
check("a500" "machines: a500\n")
check("a500 a500-204  a1200" "machines: a500;a500-204;a1200\n")
check("a500 A500" "bug-orig: x\\.bas: @machine A500: not a machine name")
check("a500-204 a1200 a500-204" "bug-orig: x\\.bas: @machine a500-204 twice")
check(" " "bug-orig: x\\.bas: @machine without a name")
# the pass files of a test with several machines
set(works "")
foreach(p m IN ZIP_LISTS PASSES MACHINES)
	file(READ ${p} text)
	string(REGEX MATCHALL "\nmachine=[^\n]*" lines "${text}")
	if(NOT lines STREQUAL "\nmachine=${m}")
		message(FATAL_ERROR "machines-test: ${p}: the machine lines are \"${lines}\", "
			"expected one machine=${m}")
	endif()
	if(NOT text MATCHES "\nwork=([^\n]*)\n")
		message(FATAL_ERROR "machines-test: ${p} has no work=")
	endif()
	if(CMAKE_MATCH_1 IN_LIST works)
		message(FATAL_ERROR "machines-test: ${p}: work directory ${CMAKE_MATCH_1} twice")
	endif()
	list(APPEND works ${CMAKE_MATCH_1})
endforeach()
message(STATUS "machines-test: all checks as expected")
