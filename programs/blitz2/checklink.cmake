# cmake -P checklink.cmake MAP - does the linked blitz2 still start on
# Kickstart 1.x?  Runs after every link of blitz2 (POST_BUILD,
# CMakeLists.txt); tests/checklink/ has a map that must fail it.
#
# The original blitz2 runs on Kickstart 1.3.  The port starts with the libnix
# startup code, which opens every library whose base a linked member
# references (__initlibraries) and stops if one fails to open.  libnix20's
# members (the driver's default) need Kickstart 2.0: __udivsi3/__mulsi3 ...
# call utility.library, __nocommandline calls AllocVec, SelectInput ...
# blitz2 is therefore linked with libnix13 (the same helpers with 1.x calls
# only) in front of libnix20.  The check reads the map's "Archive member
# included" section and fails on
#   - any member of libnix20.a,
#   - any library base stub (libstubs.a) of a library that Kickstart 1.3 does
#     not have (allowed_stubs),
#   - a defined UtilityBase.
cmake_minimum_required(VERSION 3.28)

# library bases of Kickstart 1.3 (ROM and LIBS:) blitz2 may reference
set(allowed_stubs dos.o exec.o graphics.o intuition.o layers.o icon.o
	mathffp.o mathtrans.o mathieeedoubbas.o mathieeedoubtrans.o diskfont.o
	expansion.o translator.o)

if(NOT DEFINED CMAKE_ARGV3)
	message(FATAL_ERROR "usage: cmake -P checklink.cmake MAP")
endif()
file(READ ${CMAKE_ARGV3} map)

# the section: from its title to the next line starting with Discarded,
# Allocating or Memory
string(FIND "${map}" "Archive member included" start)
if(start EQUAL -1)
	message(FATAL_ERROR "checklink: no \"Archive member included\" section in the map")
endif()
string(SUBSTRING "${map}" ${start} -1 section)
string(REGEX MATCH "\n[^ \t\n]*(Discarded|Allocating|Memory)" end_line "${section}")
if(end_line)
	string(FIND "${section}" "${end_line}" end)
	string(SUBSTRING "${section}" 0 ${end} section)
endif()

# one member per line that starts with LIBRARY(MEMBER)
string(REGEX MATCHALL "\n[^ \t\n(]+\\([^)\n]+\\)" members "${section}")
set(bad)
set(seen 0)
foreach(m IN LISTS members)
	string(REGEX MATCH "\n([^ \t\n(]+)\\(([^)\n]+)\\)" _ "${m}")
	get_filename_component(lib "${CMAKE_MATCH_1}" NAME)
	set(member "${CMAKE_MATCH_2}")
	math(EXPR seen "${seen} + 1")
	if(lib STREQUAL "libnix20.a")
		list(APPEND bad "${lib}(${member}) (Kickstart 2.0 variant)")
	elseif(lib STREQUAL "libstubs.a" AND NOT member IN_LIST allowed_stubs)
		list(APPEND bad "${lib}(${member}) (library not in Kickstart 1.3)")
	endif()
endforeach()
if("${map}" MATCHES "\n[ \t]+0x[0-9a-f]+[ \t]+_?UtilityBase[ \t]*\n")
	list(APPEND bad "UtilityBase defined (a utility.library helper is linked)")
endif()

foreach(b IN LISTS bad)
	message("checklink: not allowed in blitz2: ${b}")
endforeach()
if(bad)
	message(FATAL_ERROR "checklink: failed")
endif()
message("checklink: ${seen} archive members, only Kickstart 1.x helpers")
