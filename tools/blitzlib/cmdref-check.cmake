# The check of the command reference in a library's build step
# (blitz_library CMDREF, cmake/BlitzLibrary.cmake): the command table
# blitzlib cmdref generates from the library file equals the committed one of the
# website.
#   cmake -DBLITZLIB=blitzlib -DLIB=NAME.obj -DGROUP=G -DNAME=N -DOUT=generated.json
#         -DCOMMITTED=site/data/libraries/G/N.json -P cmdref-check.cmake
cmake_minimum_required(VERSION 4.2.3)
execute_process(COMMAND ${BLITZLIB} cmdref -o ${OUT} --group ${GROUP} --name ${NAME} ${LIB}
	RESULT_VARIABLE rc)
if(rc)
	message(FATAL_ERROR "blitzlib cmdref: exit ${rc}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${OUT} ${COMMITTED}
	RESULT_VARIABLE diff)
if(diff)
	message(FATAL_ERROR "the command table ${OUT} differs from ${COMMITTED}: "
		"if the change of the library is intended, build the target cmdref_update "
		"and commit the file")
endif()
