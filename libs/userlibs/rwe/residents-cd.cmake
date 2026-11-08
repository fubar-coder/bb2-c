# ctest rwe_residents_cd (residents.cmake): the corpusrun pass PASS lets
# blitz2 2.10 of the CD (ref) and blitz2-c (port) save the residents NAMES
# of their Blitz sources with Create Resident (WORK/TAG/vol/out/NAME.res);
# each must have the contents of RESIDENTS/NAME.res, the one blitzres
# wrote (CHECK: tools/blitzres/check.cmake).  Exit 77 (skipped) if the
# pass is skipped (no Amiberry, no CD).
#   cmake -DCORPUSRUN=... -DPASS=... -DWORK=... -DBLITZRES=... -DRESIDENTS=...
#         -DNAMES=a;b -DCHECK=... -P residents-cd.cmake
execute_process(COMMAND ${CORPUSRUN} pass ${PASS} RESULT_VARIABLE rc)
if(rc EQUAL 77)
	cmake_language(EXIT 77)
endif()
if(NOT rc EQUAL 0)
	message(FATAL_ERROR "corpusrun pass ${PASS}: exit ${rc} (report ${WORK}.txt)")
endif()
set(bad 0)
foreach(tag ref port)
	foreach(n IN LISTS NAMES)
		set(made ${WORK}/${tag}/vol/out/${n}.res)
		if(NOT EXISTS ${made})
			message("${tag}: no ${n}.res - Create Resident failed (${WORK}/${tag}/vol/out/session.log)")
			set(bad 1)
			continue()
		endif()
		execute_process(COMMAND ${CMAKE_COMMAND} -DBLITZRES=${BLITZRES}
			-DRES=${RESIDENTS}/${n}.res -DEXPECT=${made} -P ${CHECK} RESULT_VARIABLE rc)
		if(rc)
			set(bad 1)
		else()
			message("${tag}: ${n}.res has the contents of blitzres's")
		endif()
	endforeach()
endforeach()
if(bad)
	message(FATAL_ERROR "the residents differ from those of Create Resident")
endif()
