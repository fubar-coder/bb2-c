# blitz_library(NAME [VARIANT DIR] [OBJ PATH] [OPTIMIZE PROFILE] SOURCES...
#               [CHECK FILES...])
# - one Blitz Basic 2 library from C
#   OPTIMIZE PROFILE  the optimisation flags (BLITZLIB_OPTIMIZE_PROFILE,
#                 below): size (default) or speed.  A caller that builds
#                 a library in several variants passes the same profile to
#                 each of them
#   VARIANT DIR   the variant (libs/variants/NAME, a test variant in
#                 libs/tests/variants): its directory comes first in the
#                 include path, so its libnums.h (the library numbers,
#                 blitzlib.h includes <libnums.h>) is taken; default
#                 BLITZ_DEFAULT_VARIANT
#   OBJ PATH      the library file (default NAME.obj in the build
#                 directory), e.g. in the userlibs directory of a variant
#   CHECK FILES   in the same step: its number is in none of FILES (the
#                 acidlibs and deflibs it is loaded with; blitzlib numbers)
#   ALLOW33       the library may have the number 33, which is reserved for
#                 literallib (#1018): RIDosLib of the RWE variants
#                 (blitzlib check/numbers --allow-33)
#   CMDREF G/N    in the same step: the command table of the library
#                 (blitzlib cmdref, CMDREF_DIR/G/N.json) equals the committed
#                 site/data/libraries/G/N.json, the data of the website's
#                 command reference (tools/blitzlib/cmdref-check.cmake: cmake -E
#                 compare_files).  After an
#                 intended change: build the target cmdref_update, which
#                 copies the generated files into the site
#   libNAME.a     the library as static library
#   NAME.linked   the link with symbols (for disassembly and comparisons)
#   NAME.obj      the library file in Blitz 2 format (one code hunk, header
#                 + token table at offset 0), as found in blitz2:acidlibs /
#                 deflibs / userlibs: NAME.linked stripped (= ld -s).
#                 libs/blitzlib.ld already makes the layout; the relocs stay
#                 in ld's order, a library without relocs has no
#                 HUNK_RELOC32 - both fine for blitz2.s loadalib.
#                 The same build step checks it (blitzlib check: one
#                 hunk, header, token table, no reference from the normal
#                 code into the error check area): if the check fails, NAME.obj does
#                 not count as built and nothing that uses it is built.
# The compiler options are the Blitz library conventions (include/blitzlib.h):
# a3 string pointer, a4 locals, a5 globals of the Blitz program.
set(BLITZLIB_LD ${CMAKE_CURRENT_LIST_DIR}/../libs/blitzlib.ld)
set(BLITZLIB_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/../libs/include)

# the optimisation profiles of blitz_library(OPTIMIZE ...); every profile
# optimises, also in Debug builds: -O0 would call libgcc (__mulsi3 ...)
# - size: -Os, the default; blitz2 copies a whole library into every
#   program that uses it, so its size counts in every executable
# - speed: -O2 for the compute libraries whose loops -Os makes slow (a
#   byte copy loop of about 50 instead of 22 cycles on the 68000).
#   -fno-tree-scev-cprop: the final value replacement of -O2 turns a sum
#   over a loop into a 32-bit multiply, a libgcc call (__mulsi3) on the
#   68000; -fno-inline-small-functions -fno-inline-functions: inlining
#   is most of -O2's growth and little of its gain (static inline
#   functions are still inlined)
set(BLITZLIB_OPTIMIZE_size -Os)
set(BLITZLIB_OPTIMIZE_speed -O2 -fno-tree-scev-cprop
	-fno-inline-small-functions -fno-inline-functions)

# the variant of the libraries built to blitz2/acidlibs and deflibs (and
# checked against the CD): libs/variants/NAME/libnums.h
set(BLITZ_DEFAULT_VARIANT bb2-2.10 CACHE STRING
	"variant of the libraries (libs/variants/NAME) built to blitz2/acidlibs, deflibs")
get_filename_component(BLITZ_VARIANTS_SOURCE ${CMAKE_CURRENT_LIST_DIR}/../libs/variants ABSOLUTE)
set(BLITZ_DEFAULT_VARIANT_DIR ${BLITZ_VARIANTS_SOURCE}/${BLITZ_DEFAULT_VARIANT})
if(NOT EXISTS ${BLITZ_DEFAULT_VARIANT_DIR}/libnums.h)
	message(FATAL_ERROR "BLITZ_DEFAULT_VARIANT ${BLITZ_DEFAULT_VARIANT}: no ${BLITZ_DEFAULT_VARIANT_DIR}/libnums.h")
endif()

# the command tables of the libraries with CMDREF, as blitzlib cmdref
# generates them (site/data/libraries holds the committed ones)
set(CMDREF_DIR ${CMAKE_BINARY_DIR}/cmdref)
set(CMDREF_SITE_DIR ${CMAKE_CURRENT_LIST_DIR}/../site/data/libraries)
cmake_path(NORMAL_PATH CMDREF_SITE_DIR)
# the variant whose acidlibs and deflibs the committed files describe (its
# library numbers): with another BLITZ_DEFAULT_VARIANT they are not checked
set(CMDREF_VARIANT bb2-2.10)
set(CMDREF_CHECK ${CMAKE_CURRENT_LIST_DIR}/../tools/blitzlib/cmdref-check.cmake)
cmake_path(NORMAL_PATH CMDREF_CHECK)

function(blitz_library name)
	cmake_parse_arguments(PARSE_ARGV 1 arg "ALLOW33" "VARIANT;OBJ;CMDREF;OPTIMIZE" "CHECK")
	if(NOT arg_OPTIMIZE)
		set(arg_OPTIMIZE size)
	endif()
	if(NOT DEFINED BLITZLIB_OPTIMIZE_${arg_OPTIMIZE})
		message(FATAL_ERROR "blitz_library(${name}): no OPTIMIZE profile ${arg_OPTIMIZE} (size, speed)")
	endif()
	add_library(${name} STATIC ${arg_UNPARSED_ARGUMENTS})
	if(NOT arg_VARIANT)
		set(arg_VARIANT ${BLITZ_DEFAULT_VARIANT_DIR})
	endif()
	target_include_directories(${name} BEFORE PUBLIC ${arg_VARIANT})
	target_include_directories(${name} PUBLIC ${BLITZLIB_INCLUDE})
	target_compile_options(${name} PRIVATE
		-fomit-frame-pointer
		-ffixed-a3 -ffixed-a4 -ffixed-a5   # Blitz string pointer/locals/globals (blitzlib.h)
		-fno-toplevel-reorder          # token table in definition order
		-ffreestanding -fno-tree-loop-distribute-patterns   # no libc: no memset/memcpy calls
		${BLITZLIB_OPTIMIZE_${arg_OPTIMIZE}}   # the profile (above)
		-fno-strict-aliasing
		-Wall -Wno-pointer-sign)
	set(linked ${CMAKE_CURRENT_BINARY_DIR}/${name}.linked)
	set(obj ${CMAKE_CURRENT_BINARY_DIR}/${name}.obj)
	if(arg_OBJ)
		set(obj ${arg_OBJ})
		get_filename_component(dir ${obj} DIRECTORY)
		file(MAKE_DIRECTORY ${dir})
	endif()
	set(check)
	set(allow33)
	set(allow33_numbers)
	if(arg_ALLOW33)
		set(allow33 --allow-33)
		set(allow33_numbers --allow-33 ${obj})
	endif()
	if(arg_CHECK)
		set(check COMMAND ${BLITZLIB} numbers ${allow33_numbers} ${obj} ${arg_CHECK})
	endif()
	set(cmdref)
	set(cmdref_files)
	set(cmdref_deps)
	if(arg_CMDREF)
		string(REGEX MATCH "^([^/]+)/([^/]+)$" m "${arg_CMDREF}")
		if(NOT m)
			message(FATAL_ERROR "blitz_library(${name}): CMDREF ${arg_CMDREF} is not GROUP/NAME")
		endif()
		set(generated ${CMDREF_DIR}/${arg_CMDREF}.json)
		set(committed ${CMDREF_SITE_DIR}/${arg_CMDREF}.json)
		file(MAKE_DIRECTORY ${CMDREF_DIR}/${CMAKE_MATCH_1})
		set(cmdref COMMAND ${CMAKE_COMMAND} -DBLITZLIB=${BLITZLIB} -DLIB=${obj}
			-DGROUP=${CMAKE_MATCH_1} -DNAME=${CMAKE_MATCH_2} -DOUT=${generated}
			-DCOMMITTED=${committed} -P ${CMDREF_CHECK})
		set(cmdref_files ${generated})
		set_property(GLOBAL APPEND PROPERTY CMDREF_LIBRARIES ${arg_CMDREF})
		# a new library has no committed file yet: the check fails until
		# cmdref_update has copied it (then configure again)
		set(cmdref_deps ${CMDREF_CHECK})
		if(EXISTS ${committed})
			list(APPEND cmdref_deps ${committed})
		endif()
	endif()
	add_custom_command(OUTPUT ${obj}
		BYPRODUCTS ${cmdref_files}
		COMMAND ${CMAKE_C_COMPILER} -m${AMIGA_CPU} -noixemul -nostartfiles -nostdlib
			-Wl,-T,${BLITZLIB_LD}
			-Wl,--whole-archive $<TARGET_FILE:${name}> -Wl,--no-whole-archive -lgcc
			-o ${linked}
		COMMAND ${CMAKE_STRIP} -o ${obj} ${linked}
		COMMAND ${BLITZLIB} check ${allow33} ${obj}
		${check}
		${cmdref}
		DEPENDS ${name} ${BLITZLIB_LD} ${BLITZLIB} ${arg_CHECK} ${cmdref_deps} host_tools
		COMMENT "Blitz library ${name}.obj"
		VERBATIM)
	add_custom_target(${name}_obj ALL DEPENDS ${obj})
endfunction()

# biglib_join(TARGET OUTPUT OBJS... [CHECK FILES...]) - join library files
# into one BigLib file (blitz2:acidlibs, deflibs) with the host tool
# blitzlib join (tools/blitzlib).  In the same build step: no library number twice in OUTPUT
# and the files CHECK (the other library files the compiler loads with it,
# e.g. acidlibs for deflibs; blitzlib numbers) - two libraries with one number
# would have the same tokens.
function(biglib_join target output)
	cmake_parse_arguments(PARSE_ARGV 2 arg "" "" "CHECK")
	get_filename_component(dir ${output} DIRECTORY)
	file(MAKE_DIRECTORY ${dir})
	add_custom_command(OUTPUT ${output}
		COMMAND ${BLITZLIB} join ${output} ${arg_UNPARSED_ARGUMENTS}
		COMMAND ${BLITZLIB} numbers ${output} ${arg_CHECK}
		DEPENDS ${arg_UNPARSED_ARGUMENTS} ${arg_CHECK} ${BLITZLIB} host_tools
		COMMENT "BigLib ${output}"
		VERBATIM)
	add_custom_target(${target} ALL DEPENDS ${output})
endfunction()
