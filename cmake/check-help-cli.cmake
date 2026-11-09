# The help of a host program on CLI11 (tools/lib/cli, docs/development.md,
# "Host programs").
#   cmake -DPROGRAM=... [-DSUBCOMMANDS=a,b,...] -P check-help-cli.cmake
# PROGRAM --help and -h: exit status 0, stdout starts with "Usage: NAME"
# (NAME the file name of PROGRAM), stderr empty; PROGRAM with an unknown
# option: exit status 2 and a message on stderr.  With SUBCOMMANDS the same
# for PROGRAM SUB --help, -h ("Usage: NAME SUB") and an unknown option of
# each, and every subcommand of the SUBCOMMANDS: section of the program's
# help is in the list.
get_filename_component(name ${PROGRAM} NAME)
string(REPLACE "," ";" SUBCOMMANDS "${SUBCOMMANDS}")
string(REGEX REPLACE "([][.*+?^$(){}|\\])" "\\\\\\1" name_re "${name}")

function(check_help label expect)
	foreach(opt --help -h)
		execute_process(COMMAND ${PROGRAM} ${ARGN} ${opt} RESULT_VARIABLE rc
			OUTPUT_VARIABLE out ERROR_VARIABLE err)
		if(NOT rc EQUAL 0)
			message(FATAL_ERROR "${label} ${opt}: exit status ${rc}, not 0")
		endif()
		if(NOT out MATCHES "^Usage: ${expect}")
			message(FATAL_ERROR "${label} ${opt}: no usage text on stdout:\n${out}")
		endif()
		if(NOT err STREQUAL "")
			message(FATAL_ERROR "${label} ${opt}: output on stderr:\n${err}")
		endif()
	endforeach()
	execute_process(COMMAND ${PROGRAM} ${ARGN} --no-such-option RESULT_VARIABLE rc
		OUTPUT_VARIABLE out ERROR_VARIABLE err)
	if(NOT rc EQUAL 2)
		message(FATAL_ERROR "${label} --no-such-option: exit status ${rc}, not 2")
	endif()
	if(err STREQUAL "")
		message(FATAL_ERROR "${label} --no-such-option: no message on stderr")
	endif()
	set(help "${out}" PARENT_SCOPE)
endfunction()

check_help(${name} "${name_re}( |\n|$)")
execute_process(COMMAND ${PROGRAM} -h OUTPUT_VARIABLE program_help)
foreach(sub IN LISTS SUBCOMMANDS)
	check_help("${name} ${sub}" "${name_re} ${sub}( |\n|$)" ${sub})
endforeach()
# the subcommands the program's help names: the lines of its SUBCOMMANDS:
# section up to the next empty line
if(program_help MATCHES "\nSUBCOMMANDS:\n(([^\n]+\n?)*)")
	string(REGEX MATCHALL "(^|\n)  [a-z0-9][-a-z0-9]*" lines "${CMAKE_MATCH_1}")
	foreach(line IN LISTS lines)
		string(STRIP "${line}" sub)
		if(NOT sub IN_LIST SUBCOMMANDS)
			message(FATAL_ERROR "${name} --help: subcommand ${sub} has no help test (not in SUBCOMMANDS)")
		endif()
	endforeach()
elseif(SUBCOMMANDS)
	message(FATAL_ERROR "${name} --help: no SUBCOMMANDS: section:\n${program_help}")
endif()
