# PROGRAM --help (and -h): exit status 0, the usage text on stdout,
# nothing on stderr (TODO.md: -h / --help for all callable programs).
#   cmake -DPROGRAM=... [-DSUBCOMMANDS=a,b,...] -P check-help.cmake
# With SUBCOMMANDS (a program with subcommands, tools/lib/subcmd): the same
# for PROGRAM SUB --help and -h of each one (stdout starts with
# "usage: NAME SUB"), the program's usage has a synopsis line of each one,
# and every subcommand of a synopsis line ("usage: NAME SUB" or seven
# spaces and "NAME SUB") is in the list.  NAME is the file name of PROGRAM.
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
		if(NOT out MATCHES "^${expect}")
			message(FATAL_ERROR "${label} ${opt}: no usage text on stdout:\n${out}")
		endif()
		if(NOT err STREQUAL "")
			message(FATAL_ERROR "${label} ${opt}: output on stderr:\n${err}")
		endif()
	endforeach()
	set(help "${out}" PARENT_SCOPE)
endfunction()

check_help(${PROGRAM} "usage: ")
if(NOT SUBCOMMANDS)
	return()
endif()
set(program_help "${help}")
foreach(sub IN LISTS SUBCOMMANDS)
	check_help("${name} ${sub}" "usage: ${name_re} ${sub}[ \n]" ${sub})
	if(NOT program_help MATCHES "(^|\n)(usage: |       )${name_re} ${sub}[ \n]")
		message(FATAL_ERROR "${name} --help: no synopsis line of ${sub}:\n${program_help}")
	endif()
endforeach()
# the subcommands of the synopsis lines of the program's usage
string(REGEX MATCHALL "\n(usage: |       )${name_re} [a-z0-9][-a-z0-9]*" lines "\n${program_help}")
foreach(line IN LISTS lines)
	string(REGEX REPLACE ".* " "" sub "${line}")
	if(NOT sub IN_LIST SUBCOMMANDS)
		message(FATAL_ERROR "${name} --help: subcommand ${sub} has no help test (not in SUBCOMMANDS)")
	endif()
endforeach()
