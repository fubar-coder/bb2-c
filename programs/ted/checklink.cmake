# cmake -P checklink.cmake MAP - which library members does the linked ted
# use?  Runs after every link of ted (POST_BUILD, CMakeLists.txt).
#
# TED runs without the libnix startup code (-nostartfiles): nothing opens
# utility.library, so libnix's 32 bit arithmetic helpers (__udivsi3,
# __umodsi3, __divsi3, __mulsi3 ... -> UtilityBase) would crash, and libgcc
# helpers are not wanted either (the original only uses the .w MULU/DIVU
# instructions; DIVU goes through divu_w in src/glue.c).
#
# The map's "Archive member included" section lists every library member
# the link pulled in.  Allowed: TED's own core library and the libnix
# members in allowed_libnix (plain string/setjmp code without library
# bases).  Anything else - in particular every helper of libgcc/libnix20 and
# anything that references UtilityBase - fails the check.
cmake_minimum_required(VERSION 4.2.3)

set(allowed_libnix setjmp.o longjmp.o memset.o memcpy.o memmove.o memcmp.o
	strlen.o strcmp.o strncmp.o strcpy.o strncpy.o bcopy.o bzero.o)

if(NOT DEFINED CMAKE_ARGV3)
	message(FATAL_ERROR "usage: cmake -P tools/checklink.cmake MAP")
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
	if(lib STREQUAL "libtedcore.a")
		continue()
	endif()
	if(lib STREQUAL "libnix.a" AND member IN_LIST allowed_libnix)
		continue()
	endif()
	list(APPEND bad "${lib}(${member})")
endforeach()
if("${map}" MATCHES "\n[ \t]+0x[0-9a-f]+[ \t]+_?UtilityBase[ \t]*\n")
	list(APPEND bad "UtilityBase defined (a utility.library helper is linked)")
endif()

foreach(b IN LISTS bad)
	message("checklink: not allowed in ted: ${b}")
endforeach()
if(bad)
	message(FATAL_ERROR "checklink: failed")
endif()
message("checklink: ${seen} archive members, no arithmetic/library helpers")
