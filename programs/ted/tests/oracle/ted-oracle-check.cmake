# ctest ted_oracle_reference (label originals): the committed oracle data
# of the TED tests against the original.  ted-oracle.cmake (the target
# ted-oracle) writes into CHECKDIR, which starts with a copy of the
# committed tests/data/ted-oracle.sha256 (its note rows stay, their note
# must name the original's hash); ted-oracle.sha256 and ted-tokens.txt of
# CHECKDIR must be those of DATA byte for byte.
#   cmake -DDATA=tests/data -DCHECKDIR=DIR [-DNO_RUN=ON]
#         (and the -D of ted-oracle.cmake but DATA) -P ted-oracle-check.cmake
# NO_RUN: CHECKDIR holds the files already, no emulator run (the test with
# broken input).  Exit status 0 the same, 1 not, 77 skipped (no CD or no
# Amiberry).
cmake_minimum_required(VERSION 4.2.3)

foreach(v DATA CHECKDIR)
	if(NOT ${v})
		message(FATAL_ERROR "ted-oracle-check.cmake: -D${v}=... missing")
	endif()
endforeach()
if(NOT NO_RUN)
	if(NOT IS_DIRECTORY "${CD}/BLITZ2")
		message(STATUS "ted_oracle_reference: no CD ${CD} - skipped")
		cmake_language(EXIT 77)
	endif()
	file(REMOVE_RECURSE ${CHECKDIR})
	file(MAKE_DIRECTORY ${CHECKDIR})
	file(COPY_FILE ${DATA}/ted-oracle.sha256 ${CHECKDIR}/ted-oracle.sha256)
	execute_process(COMMAND ${CMAKE_COMMAND} -DAMIGA_RUN=${AMIGA_RUN} -DRESULTS=${RESULTS}
		-DBLITZ2=${BLITZ2} -DTED=${TED} -DORACLE=${ORACLE} -DCD=${CD} -DWORK=${WORK}
		-DDATA=${CHECKDIR} -P ${CMAKE_CURRENT_LIST_DIR}/ted-oracle.cmake
		RESULT_VARIABLE rc)
	if(rc EQUAL 77)
		cmake_language(EXIT 77)
	elseif(rc)
		message(FATAL_ERROR "ted_oracle_reference: ted-oracle.cmake failed (${rc})")
	endif()
endif()
set(bad)
foreach(f ted-oracle.sha256 ted-tokens.txt)
	execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${CHECKDIR}/${f} ${DATA}/${f}
		RESULT_VARIABLE rc)
	if(rc)
		list(APPEND bad ${f})
	endif()
endforeach()
if(bad)
	message(FATAL_ERROR "ted_oracle_reference: the original's run differs from ${DATA} in "
		"${bad} (diff against ${CHECKDIR}; cmake --build --preset amiga --target ted-oracle "
		"writes it)")
endif()
message(STATUS "ted_oracle_reference: the oracle data of ${DATA} are the original's")
