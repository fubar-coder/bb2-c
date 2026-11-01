# blitz2 - the C translation of the Blitz Basic 2 v1.95 compiler (blitz2.s).
# It calls the editor (blitz2:ted) by register conventions and runs the 68k
# code it generates, so it only makes sense on AmigaOS.

# chunk files in source order of blitz2.s, plus the assembler glue
set(BLITZ2_CHUNKS main compile stmts asmlibs parse eval libcall tokens1 tokens2 data)
list(TRANSFORM BLITZ2_CHUNKS PREPEND src/ OUTPUT_VARIABLE chunk_sources)
list(TRANSFORM chunk_sources APPEND .c)
file(GLOB glue_sources CONFIGURE_DEPENDS src/glue*.c)

# blitz2_program(TARGET [DEFINES...]): one compiler from the chunk sources.
# blitz2 is the translation; blitz2-ab3 (BLITZ2_AB3, the AB3 mode of
# docs/ab3/variant.md) is built from the same sources with
# the additions of the AB3 mode, which sit under #if BLITZ2_AB3 - blitz2
# itself stays byte-identical by construction.
# globals relative to a4: shorter and faster accesses, fewer relocations
# (data + bss must stay below 64 KB - the linker refuses a 16-bit a4
# offset beyond it; entry stubs in glue*.c load a4)
option(BLITZ2_BASEREL "address globals relative to a4 (-fbaserel)" ON)
function(blitz2_program target)
	add_executable(${target} ${chunk_sources} ${glue_sources})
	target_include_directories(${target} PRIVATE src)
	target_compile_definitions(${target} PRIVATE ${ARGN})
	target_compile_options(${target} PRIVATE
		-fomit-frame-pointer
		-fno-strict-aliasing                # raw W_()/L_() accesses to patched templates and loaded images
		-Wall -Wno-pointer-sign -Wno-parentheses)
	if(BLITZ2_BASEREL)
		target_compile_options(${target} PRIVATE -fbaserel)
		target_link_options(${target} PRIVATE -fbaserel)
	endif()
	# no symbol table in release builds
	target_link_options(${target} PRIVATE $<$<CONFIG:Release,MinSizeRel>:-s>
		-Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/${target}.map)
	# Kickstart 1.x like the original (#936): libnix13 in front of the driver's
	# libnix20, whose helpers need Kickstart 2.0 (__udivsi3/__umodsi3 call
	# utility.library, which the startup code then opens; __nocommandline calls
	# AllocVec ...).  libnix13 has the same helpers with 1.x calls only.
	target_link_libraries(${target} PRIVATE nix13)
	set_target_properties(${target} PROPERTIES OUTPUT_NAME ${target} SUFFIX ""
		RUNTIME_OUTPUT_DIRECTORY ${BLITZ2_DIR})

	# check after every link; if it fails, the program does not count as
	# built: no libnix20 member, no base of a library Kickstart 1.3 does not
	# have (UtilityBase) - checklink.cmake
	add_custom_command(TARGET ${target} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/checklink.cmake
			${CMAKE_CURRENT_BINARY_DIR}/${target}.map
		VERBATIM)
endfunction()
blitz2_program(blitz2)
blitz2_program(blitz2-ab3 BLITZ2_AB3=1)
# the extension points of the chunk files (src/ext.h): blitz2's handle
# nothing (BB2 mode only), blitz2-ab3's are the AB3 modules src/ab3*.c
target_sources(blitz2 PRIVATE src/ext_bb2.c)
# blitz2-ab3 decides the mode of a source with shared/xtra, compiled with
# its own options (like shared/filereq)
set(xtra_dir ${PROJECT_SOURCE_DIR}/shared/xtra)
target_sources(blitz2-ab3 PRIVATE ${xtra_dir}/xtra.c ${xtra_dir}/xtra_ab3.c ${xtra_dir}/xtra_mode.c)
target_include_directories(blitz2-ab3 PRIVATE ${xtra_dir})
# and its command line (-s SOURCE -e EXE: a compile with ab3cli as the
# editor, src/ab3main.c), parsed by shared/ab3job
set(ab3job_dir ${PROJECT_SOURCE_DIR}/shared/ab3job)
target_sources(blitz2-ab3 PRIVATE src/ab3main.c ${ab3job_dir}/ab3args.c)
target_include_directories(blitz2-ab3 PRIVATE ${ab3job_dir})
# PROGDIR:ab3cli belongs to it
add_dependencies(blitz2-ab3 ab3cli)

# the check itself: the map of blitz2 linked without libnix13 (before #936)
# must fail it, once for each rule
set(checklink_broken ${CMAKE_CURRENT_SOURCE_DIR}/tests/checklink/libnix20.map)
foreach(t IN ITEMS
		"libnix20|libnix20\\.a\\(__udivsi3\\.o\\) \\(Kickstart 2\\.0 variant\\)"
		"stubs|libstubs\\.a\\(utility\\.o\\) \\(library not in Kickstart 1\\.3\\)"
		"utilitybase|UtilityBase defined")
	string(REPLACE "|" ";" t "${t}")
	list(GET t 0 name)
	list(GET t 1 regex)
	add_test(NAME blitz2_checklink_broken_${name} COMMAND ${CMAKE_COMMAND}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/checklink.cmake ${checklink_broken})
	set_tests_properties(blitz2_checklink_broken_${name} PROPERTIES
		PASS_REGULAR_EXPRESSION "not allowed in blitz2: ${regex}")
endforeach()

# ------------------------------------------------------- reference build
# The original blitz2.s v1.95 (archive/sources/blitz2-1.95) assembled with
# vasm as Devpac did (-nocase as in GenAm.opts): the reference of the port,
# in build/<preset>/programs/blitz2/ref/:
#   blitz2         the original compiler
#   blitz2.lst     vasm listing (addresses + opcodes)
#   blitz2.map     TSV address -> file:line, bytes, source
#   blitz2.labels  TSV address, label, file:line
# The same step checks that listing and binary agree (vasmlisting verify).
# -kick1hunks: the relocations as HUNK_RELOC32 like Devpac, not vasm's
# short form (HUNK_DREL32), which LoadSeg only knows from Kickstart 2.0
# on - the reference runs on the A500 with Kickstart 1.3 as well
# (amiga-emulation/README.md, "A500").
if(NOT VASM)
	find_program(VASM vasmm68k_mot REQUIRED)
endif()
set(BLITZ2_ORIG_DIR ${PROJECT_SOURCE_DIR}/archive/sources/blitz2-1.95)
set(BLITZ2_REF_DIR ${CMAKE_CURRENT_BINARY_DIR}/ref)
set(BLITZ2_REF_INPUTS blitz2.s myoffs wbheader options2.src.new asmmac typereq
	options.src calcreq obj.info)
list(TRANSFORM BLITZ2_REF_INPUTS PREPEND ${BLITZ2_ORIG_DIR}/)
set(ref ${BLITZ2_REF_DIR}/blitz2)
add_custom_command(
	OUTPUT ${ref} ${ref}.lst ${ref}.map ${ref}.labels
	COMMAND ${CMAKE_COMMAND} -E make_directory ${BLITZ2_REF_DIR}
	COMMAND ${VASM} -quiet -devpac -nocase -m68000 -Fhunkexe -kick1hunks
		-nowarn=62 -nowarn=40 -nowarn=41 -nowarn=51
		-o ${ref} -L ${ref}.lst -Lall blitz2.s > /dev/null
	COMMAND ${VASMLISTING} ${ref}.lst map > ${ref}.map
	COMMAND ${VASMLISTING} ${ref}.lst labels > ${ref}.labels
	COMMAND ${VASMLISTING} ${ref}.lst verify ${ref}
	DEPENDS ${BLITZ2_REF_INPUTS} ${VASMLISTING} host_tools
	WORKING_DIRECTORY ${BLITZ2_ORIG_DIR}
	COMMENT "Assembling the original blitz2.s (reference)"
	VERBATIM)
add_custom_target(blitz2_ref ALL DEPENDS ${ref})
