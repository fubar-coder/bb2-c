# blitz_library(NAME [VARIANT DIR] SOURCES...) - one Blitz Basic 2 library
# from C
#   VARIANT DIR   a variant (libs/variants/NAME): its directory comes first
#                 in the include path, so its libnums.h (the library
#                 numbers, blitzlib.h includes <libnums.h>) is taken
#                 instead of libs/include/libnums.h
#   libNAME.a     the library as static library
#   NAME.linked   the link with symbols (for disassembly and comparisons)
#   NAME.obj      the library file in Blitz 2 format (one code hunk, header
#                 + token table at offset 0), as found in blitz2:acidlibs /
#                 deflibs / userlibs: NAME.linked stripped (= ld -s).
#                 libs/blitzlib.ld already makes the layout; the relocs stay
#                 in ld's order, a library without relocs has no
#                 HUNK_RELOC32 - both fine for blitz2.s loadalib.
#                 The same build step checks it (tools/blitzlibcheck: one
#                 hunk, header, token table; warnings for references into
#                 the error check area): if the check fails, NAME.obj does
#                 not count as built and nothing that uses it is built.
# The compiler options are the Blitz library conventions (include/blitzlib.h):
# a3 string pointer, a4 locals, a5 globals of the Blitz program.
set(BLITZLIB_LD ${CMAKE_CURRENT_LIST_DIR}/../libs/blitzlib.ld)
set(BLITZLIB_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/../libs/include)

function(blitz_library name)
	cmake_parse_arguments(PARSE_ARGV 1 arg "" "VARIANT" "")
	add_library(${name} STATIC ${arg_UNPARSED_ARGUMENTS})
	if(arg_VARIANT)
		target_include_directories(${name} BEFORE PUBLIC ${arg_VARIANT})
	endif()
	target_include_directories(${name} PUBLIC ${BLITZLIB_INCLUDE})
	target_compile_options(${name} PRIVATE
		-fomit-frame-pointer
		-ffixed-a3 -ffixed-a4 -ffixed-a5   # Blitz string pointer/locals/globals (blitzlib.h)
		-fno-toplevel-reorder          # token table in definition order
		-ffreestanding -fno-tree-loop-distribute-patterns   # no libc: no memset/memcpy calls
		-Os                            # also in Debug builds: -O0 would call libgcc (__mulsi3 ...)
		-fno-strict-aliasing
		-Wall -Wno-pointer-sign)
	set(linked ${CMAKE_CURRENT_BINARY_DIR}/${name}.linked)
	set(obj ${CMAKE_CURRENT_BINARY_DIR}/${name}.obj)
	add_custom_command(OUTPUT ${obj}
		COMMAND ${CMAKE_C_COMPILER} -m${AMIGA_CPU} -noixemul -nostartfiles -nostdlib
			-Wl,-T,${BLITZLIB_LD}
			-Wl,--whole-archive $<TARGET_FILE:${name}> -Wl,--no-whole-archive -lgcc
			-o ${linked}
		COMMAND ${CMAKE_STRIP} -o ${obj} ${linked}
		COMMAND ${BLITZLIBCHECK} ${obj}
		DEPENDS ${name} ${BLITZLIB_LD} ${BLITZLIBCHECK} host_tools
		COMMENT "Blitz library ${name}.obj"
		VERBATIM)
	add_custom_target(${name}_obj ALL DEPENDS ${obj})
endfunction()

# biglib_join(TARGET OUTPUT OBJS... [CHECK FILES...]) - join library files
# into one BigLib file (blitz2:acidlibs, deflibs) with the host tool
# tools/biglib.  In the same build step: no library number twice in OUTPUT
# and the files CHECK (the other library files the compiler loads with it,
# e.g. acidlibs for deflibs; biglib check) - two libraries with one number
# would have the same tokens.
function(biglib_join target output)
	cmake_parse_arguments(PARSE_ARGV 2 arg "" "" "CHECK")
	get_filename_component(dir ${output} DIRECTORY)
	file(MAKE_DIRECTORY ${dir})
	add_custom_command(OUTPUT ${output}
		COMMAND ${BIGLIB} join ${output} ${arg_UNPARSED_ARGUMENTS}
		COMMAND ${BIGLIB} check ${output} ${arg_CHECK}
		DEPENDS ${arg_UNPARSED_ARGUMENTS} ${arg_CHECK} ${BIGLIB} host_tools
		COMMENT "BigLib ${output}"
		VERBATIM)
	add_custom_target(${target} ALL DEPENDS ${output})
endfunction()
