# tape-compare: the recordings of the original libraries made by a
# recording run (libs_emulator_originals, emulator.cmake TAPE=record) must
# be the committed ones (libs/tests/tapes):
#   cmake -DRECORDED=DIR -DCOMMITTED=DIR -P tape-compare.cmake
# Every NAME.tape of either directory must be in both, with the same
# bytes.  After an intended change (a new or changed scenario, a changed
# original) copy the recorded files into the committed directory.
#   cmake -DCOMMITTED=DIR -DWORK=DIR -DCORRUPT=ON -P tape-compare.cmake
# the test of the check (libs_tapes_compare_wrong): a copy of COMMITTED
# with one hash changed and one file left out must be refused.
cmake_minimum_required(VERSION 4.2.3)

if(CORRUPT)
	file(REMOVE_RECURSE ${WORK})
	file(MAKE_DIRECTORY ${WORK})
	file(GLOB tapes ${COMMITTED}/*.tape)
	list(SORT tapes)
	list(POP_FRONT tapes changed)
	list(POP_FRONT tapes)
	file(READ ${changed} text)
	string(REGEX REPLACE "\n(seg|end) ([0-9]+) [0-9a-f]+\n" "\n\\1 \\2 00000000\n" text "${text}")
	get_filename_component(n ${changed} NAME)
	file(WRITE ${WORK}/${n} "${text}")
	foreach(t IN LISTS tapes)
		file(COPY ${t} DESTINATION ${WORK})
	endforeach()
	set(RECORDED ${WORK})
endif()

file(GLOB rec RELATIVE ${RECORDED} ${RECORDED}/*.tape)
file(GLOB com RELATIVE ${COMMITTED} ${COMMITTED}/*.tape)
set(bad "")
foreach(n IN LISTS rec)
	if(NOT n IN_LIST com)
		string(APPEND bad "  ${n}: recorded, not committed\n")
	else()
		execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files
			${RECORDED}/${n} ${COMMITTED}/${n} RESULT_VARIABLE rc)
		if(rc)
			string(APPEND bad "  ${n}: differs\n")
		endif()
	endif()
endforeach()
foreach(n IN LISTS com)
	if(NOT n IN_LIST rec)
		string(APPEND bad "  ${n}: committed, not recorded\n")
	endif()
endforeach()
if(NOT rec)
	string(APPEND bad "  no recordings in ${RECORDED}\n")
endif()
if(bad)
	message(FATAL_ERROR "tape-compare: ${RECORDED} is not ${COMMITTED}:\n${bad}")
endif()
message(STATUS "tape-compare: the recordings are the committed ones")
