# ctest teddetok_bblibsrc: every source of the Aminet archive's
# bblibsrc.lha through teddetok --check (all tokens known).  Exit 77
# (skipped) without the archive (cmake -P cmake/fetch-originals.cmake).
#   cmake -DTEDDETOK=... -DTOKENS=... -DREPO=... -P bblibsrc.cmake
foreach(f IN LISTS REQUIRE)
	if(NOT EXISTS ${f})
		message(STATUS "no ${f}: skipped")
		cmake_language(EXIT 77)
	endif()
endforeach()
include(${CMAKE_CURRENT_LIST_DIR}/../bblibsrc_files.cmake)
bblibsrc_files(${REPO} dir files)
if(NOT files)
	message(STATUS "no ${dir} (cmake -P cmake/fetch-originals.cmake): skipped")
	cmake_language(EXIT 77)
endif()
list(LENGTH files n)
list(TRANSFORM files PREPEND ${dir}/)
execute_process(COMMAND ${TEDDETOK} ${TOKENS} --check ${files} RESULT_VARIABLE rc)
if(rc)
	message(FATAL_ERROR "teddetok: exit ${rc}")
endif()
message(STATUS "${n} sources of ${dir}: all tokens known")
