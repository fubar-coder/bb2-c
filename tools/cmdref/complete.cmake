# ctest cmdref_complete: the committed command tables of the website
# (site/data/libraries/GROUP/NAME.json) are exactly those of the libraries
# the build checks (blitz_library CMDREF GROUP/NAME): no file without a
# library, no library without a file.
#   cmake -DDIR=site/data/libraries "-DLIBRARIES=GROUP/NAME;..." -P complete.cmake
cmake_minimum_required(VERSION 4.2.3)
file(GLOB_RECURSE files RELATIVE ${DIR} ${DIR}/*)
set(bad)
foreach(f IN LISTS files)
	string(REGEX REPLACE "\\.json$" "" lib "${f}")
	if(NOT lib IN_LIST LIBRARIES)
		list(APPEND bad "${DIR}/${f}: no library of the build (remove the file)")
	endif()
endforeach()
foreach(lib IN LISTS LIBRARIES)
	if(NOT EXISTS ${DIR}/${lib}.json)
		list(APPEND bad "${DIR}/${lib}.json: missing (build the target cmdref_update)")
	endif()
endforeach()
if(bad)
	list(JOIN bad "\n" bad)
	message(FATAL_ERROR "${bad}")
endif()
list(LENGTH LIBRARIES n)
message(STATUS "${n} command tables, one per library")
