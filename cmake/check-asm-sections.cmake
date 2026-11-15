# A library source writes no section directive of its own (#1057).  GCC
# does not see a section switch in assembler text: if the text ends in
# another section than the one GCC believes current, GCC's next function
# or variable of that section gets no section directive and lands where
# the assembler left off (#1056: a check function in the normal code).
# The assembler of a library is in the functions of blitzlib.h
# (BLITZ_ASM_ROUTINE, BLITZ_ASM_BLOCK, BLITZ_ERRCHK_ASM, the thunks ...),
# whose section GCC chooses itself.
#
# Refused: a directive .section, .text, .data or .bss at the start of an
# assembler line in a C string literal (after the opening quote or after
# \n).  Allowed: .section .blitzlib.head, the header section, where GCC
# keeps the header and token table as data and so cannot place a
# function: dhandlerlib keeps its code there for the order of its
# relocation table (dhandlerlib.c).
#
#   include(): blitzlib_check_asm_sections(SOURCES...) - FATAL_ERROR with
#   file:line of every directive
#   cmake "-DSOURCES=a.c|b.c" -P check-asm-sections.cmake (the build step
#   of every library, cmake/BlitzLibrary.cmake; the tests with
#   cmake/asm-sections/broken.c and good.c, libs/CMakeLists.txt)
function(blitzlib_check_asm_sections)
	set(found)
	foreach(src IN LISTS ARGN)
		file(READ ${src} text)
		set(rest "${text}")
		set(offset 0)
		while(TRUE)
			string(REGEX MATCH "(\"|\\\\n)[ \t]*\\.(section[ \t]+[^ \t\",\\\\]+|text|data|bss)([^A-Za-z0-9_.]|$)"
				hit "${rest}")
			if(NOT hit)
				break()
			endif()
			string(FIND "${rest}" "${hit}" pos)
			math(EXPR offset "${offset} + ${pos}")
			if(NOT hit MATCHES "\\.section[ \t]+\\.blitzlib\\.head[^A-Za-z0-9_.]")
				string(SUBSTRING "${text}" 0 ${offset} before)
				string(REGEX REPLACE "[^\n]" "" newlines "${before}")
				string(LENGTH "${newlines}" line)
				math(EXPR line "${line} + 1")
				string(REGEX REPLACE "^(\"|\\\\n)[ \t]*" "" directive "${hit}")
				string(REGEX REPLACE "[^A-Za-z0-9_.]$" "" directive "${directive}")
				list(APPEND found "${src}:${line}: ${directive}")
			endif()
			# on behind the quote or \n only: the match may have taken the
			# start of the next directive
			math(EXPR offset "${offset} + 1")
			math(EXPR next "${pos} + 1")
			string(SUBSTRING "${rest}" ${next} -1 rest)
		endwhile()
	endforeach()
	if(found)
		list(JOIN found "\n  " lines)
		message(FATAL_ERROR "section directive in the assembler of a library "
			"(use BLITZ_ASM_ROUTINE, BLITZ_ASM_BLOCK or BLITZ_ERRCHK_ASM of "
			"blitzlib.h, #1057):\n  ${lines}")
	endif()
endfunction()

if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
	string(REPLACE "|" ";" sources "${SOURCES}")
	blitzlib_check_asm_sections(${sources})
endif()
