# The README of a host program against its -h (docs/development.md, "Host
# programs"): every synopsis line of PROGRAM -h - a line that starts with
# "usage: NAME " or seven spaces and "NAME " (NAME the file name of
# PROGRAM) - occurs verbatim as a line of README, and every such line of
# README is a line of the -h output.
#   cmake -DPROGRAM=... -DREADME=.../README.md -P check-readme.cmake
get_filename_component(name ${PROGRAM} NAME)
string(REGEX REPLACE "([][.*+?^$(){}|\\])" "\\\\\\1" name_re "${name}")
execute_process(COMMAND ${PROGRAM} -h RESULT_VARIABLE rc OUTPUT_VARIABLE help ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
	message(FATAL_ERROR "${name} -h: exit status ${rc}, not 0")
endif()
if(NOT EXISTS ${README})
	message(FATAL_ERROR "${README}: no such file")
endif()
file(READ ${README} readme)

# every synopsis line of TEXT (no CMake list: the lines may hold ';' or '[')
# must be a line of OTHER; a missing one is reported as WHAT
function(synopsis_in text other what)
	set(missing "")
	set(rest "${text}\n")
	set(lines "\n${other}\n")
	while(NOT rest STREQUAL "")
		string(FIND "${rest}" "\n" nl)
		string(SUBSTRING "${rest}" 0 ${nl} line)
		math(EXPR nl "${nl} + 1")
		string(SUBSTRING "${rest}" ${nl} -1 rest)
		string(REGEX REPLACE "\r$" "" line "${line}")
		if(line MATCHES "^(usage: |       )${name_re}( |$)")
			string(FIND "${lines}" "\n${line}\n" at)
			if(at EQUAL -1)
				string(APPEND missing "\n  ${line}")
			endif()
		endif()
	endwhile()
	if(NOT missing STREQUAL "")
		message(SEND_ERROR "${what}:${missing}")
		set(failed ON PARENT_SCOPE)
	endif()
endfunction()

string(REPLACE "\r\n" "\n" readme "${readme}")
set(failed OFF)
synopsis_in("${help}" "${readme}" "${README}: synopsis lines of ${name} -h missing in the README")
synopsis_in("${readme}" "${help}" "${README}: synopsis lines no longer printed by ${name} -h")
if(failed)
	message(FATAL_ERROR "${README} does not match ${name} -h")
endif()
