# PROGRAM --help (and -h): exit status 0, the usage text on stdout,
# nothing on stderr (TODO.md: -h / --help for all callable programs).
#   cmake -DPROGRAM=... -P check-help.cmake
foreach(opt --help -h)
	execute_process(COMMAND ${PROGRAM} ${opt} RESULT_VARIABLE rc
		OUTPUT_VARIABLE out ERROR_VARIABLE err)
	if(NOT rc EQUAL 0)
		message(FATAL_ERROR "${PROGRAM} ${opt}: exit status ${rc}, not 0")
	endif()
	if(NOT out MATCHES "^usage: ")
		message(FATAL_ERROR "${PROGRAM} ${opt}: no usage text on stdout:\n${out}")
	endif()
	if(NOT err STREQUAL "")
		message(FATAL_ERROR "${PROGRAM} ${opt}: output on stderr:\n${err}")
	endif()
endforeach()
