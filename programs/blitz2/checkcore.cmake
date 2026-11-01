# cmake -P checkcore.cmake FILE... - are the compiler's sources free of a
# build switch for the AB3 mode?  Runs after every link of blitz2 and
# blitz2-ab3 (POST_BUILD, CMakeLists.txt; they relink after a change of
# any file of src/) over every file of src/; tests/checkcore/ has a file
# that must fail it.
#
# The chunk files are compiled once for blitz2 and blitz2-ab3; where AB3
# mode differs they call an extension point (src/ext.h), implemented by
# ext_bb2.c for blitz2 and by the AB3 modules ab3*.c for blitz2-ab3
# (docs/ab3/variant.md, "Structure").  The define of the former second
# build, BLITZ2_AB3, must not come back in any of them.
cmake_minimum_required(VERSION 3.28)

math(EXPR last "${CMAKE_ARGC} - 1")
if(last LESS 3)
	message(FATAL_ERROR "usage: cmake -P checkcore.cmake FILE...")
endif()
set(bad "")
foreach(i RANGE 3 ${last})
	set(f "${CMAKE_ARGV${i}}")
	file(STRINGS "${f}" hits REGEX "BLITZ2_AB3")
	if(hits)
		list(APPEND bad "${f}")
	endif()
endforeach()
if(bad)
	list(JOIN bad "\n  " bad)
	message(FATAL_ERROR "BLITZ2_AB3 not allowed in the compiler's sources "
		"(AB3 mode is an extension, src/ext.h):\n  ${bad}")
endif()
