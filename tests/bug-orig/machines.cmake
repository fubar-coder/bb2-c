# machines.cmake - the machines of a bug-orig test's header line
# `; @machine NAME...` (tests/bug-orig/CMakeLists.txt, tests/README.md
# "bug-orig tests"): one or more names separated by blanks, each of
# [a-z0-9-] (whether the emulation knows it is checked by corpusrun,
# machine=), none twice.
#
#   bug_orig_machines(BAS TEXT OUT)   OUT: the list of names; a bad name or
#                                     one given twice stops the configure
#
# As a script, for its test (selftest/machines-test.cmake):
#   cmake -DBAS=FILE -DTEXT=NAMES -P machines.cmake   prints "machines: A;B"
function(bug_orig_machines bas text out)
	separate_arguments(names UNIX_COMMAND "${text}")
	if(NOT names)
		message(FATAL_ERROR "bug-orig: ${bas}: @machine without a name")
	endif()
	set(seen "")
	foreach(n IN LISTS names)
		if(NOT n MATCHES "^[a-z0-9-]+$")
			message(FATAL_ERROR "bug-orig: ${bas}: @machine ${n}: not a machine name ([a-z0-9-])")
		endif()
		if(n IN_LIST seen)
			message(FATAL_ERROR "bug-orig: ${bas}: @machine ${n} twice")
		endif()
		list(APPEND seen ${n})
	endforeach()
	set(${out} ${seen} PARENT_SCOPE)
endfunction()

if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
	bug_orig_machines("${BAS}" "${TEXT}" m)
	message(STATUS "machines: ${m}")
endif()
