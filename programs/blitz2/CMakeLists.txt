# blitz2 - the C translation of the Blitz Basic 2 v1.95 compiler (blitz2.s).
# It calls the editor (blitz2:ted) by register conventions and runs the 68k
# code it generates, so it only makes sense on AmigaOS.

# chunk files in source order of blitz2.s, plus the assembler glue
set(BLITZ2_CHUNKS main compile stmts asmlibs parse eval libcall tokens1 tokens2 data)
list(TRANSFORM BLITZ2_CHUNKS PREPEND src/ OUTPUT_VARIABLE chunk_sources)
list(TRANSFORM chunk_sources APPEND .c)
file(GLOB glue_sources CONFIGURE_DEPENDS src/glue*.c)
# every file of src/, for the check of the core (checkcore.cmake)
file(GLOB blitz2_src_files CONFIGURE_DEPENDS src/*)

# globals relative to a4: shorter and faster accesses, fewer relocations
# (data + bss must stay below 64 KB - the linker refuses a 16-bit a4
# offset beyond it; entry stubs in glue*.c load a4)
option(BLITZ2_BASEREL "address globals relative to a4 (-fbaserel)" ON)
# the coverage configuration (preset amiga-cov): blitz2 and blitz2-ab3
# record the functions they execute in TEST:cov (shared/cov/cov.h)
option(BLITZ2_COVERAGE "blitz2 and blitz2-ab3 with function coverage" OFF)
include(${PROJECT_SOURCE_DIR}/shared/cov/cov.cmake)
# blitz2_compile(TARGET): the compiler options of the program's sources
function(blitz2_compile target)
	target_include_directories(${target} PRIVATE src)
	target_compile_options(${target} PRIVATE
		-fomit-frame-pointer
		-fno-strict-aliasing                # raw W_()/L_() accesses to patched templates and loaded images
		-Wall -Wno-pointer-sign -Wno-parentheses)
	if(BLITZ2_BASEREL)
		target_compile_options(${target} PRIVATE -fbaserel)
	endif()
	if(BLITZ2_COVERAGE)
		cov_instrument(${target})
	endif()
endfunction()

# blitz2_core: the chunk files and the glue, compiled once and linked by
# both programs (docs/ab3/variant.md, "Structure").  Where blitz2-ab3's
# AB3 mode differs, they call the extension points of src/ext.h, which
# each program implements: blitz2 with src/ext_bb2.c (BB2 only, nothing
# handled), blitz2-ab3 with the AB3 modules src/ab3*.c.
add_library(blitz2_core OBJECT ${chunk_sources} ${glue_sources})
blitz2_compile(blitz2_core)
if(BLITZ2_COVERAGE)
	cov_exclude(${glue_sources})        # their entry stubs load a4
endif()

# blitz2_program(TARGET SOURCES...): one compiler, the core with the
# implementation of the extension points in SOURCES
function(blitz2_program target)
	# the core's objects first, in the order of the chunks
	add_executable(${target} $<TARGET_OBJECTS:blitz2_core> ${ARGN})
	blitz2_compile(${target})
	if(BLITZ2_COVERAGE)
		cov_link(${target} ${target})
	endif()
	if(BLITZ2_BASEREL)
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
	# have (UtilityBase) - checklink.cmake; no BLITZ2_AB3 in the sources of
	# the core and its extensions, src/ - checkcore.cmake (CMake runs no
	# POST_BUILD for an object library: the programs check the core they
	# link, and relink after a change of any file of src/)
	add_custom_command(TARGET ${target} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/checklink.cmake
			${CMAKE_CURRENT_BINARY_DIR}/${target}.map
		COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/checkcore.cmake
			${blitz2_src_files}
		VERBATIM)
	set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS ${blitz2_src_files})
endfunction()
blitz2_program(blitz2 src/ext_bb2.c)
# blitz2-ab3: the AB3 modules; the mode of a source is decided with
# shared/xtra, the command line (-s SOURCE -e EXE: a compile with ab3cli
# as the editor, src/ab3main.c) parsed by shared/ab3job, AmiBlitz3's
# include names from shared/tokenizer
set(xtra_dir ${PROJECT_SOURCE_DIR}/shared/xtra)
set(ab3job_dir ${PROJECT_SOURCE_DIR}/shared/ab3job)
set(tokenizer_dir ${PROJECT_SOURCE_DIR}/shared/tokenizer)
blitz2_program(blitz2-ab3 src/ab3main.c src/ab3libs.c src/ab3res.c src/ab3inc.c src/ab3tok.c
	src/ab3dir.c src/ab3syn.c src/ab3sect.c src/ab3type.c src/ab3str.c src/ab3date.c src/ab3proc.c
	src/ab3chr.c
	${xtra_dir}/xtra.c ${xtra_dir}/xtra_ab3.c ${xtra_dir}/xtra_mode.c ${ab3job_dir}/ab3args.c
	${tokenizer_dir}/tokenizer.c)
target_include_directories(blitz2-ab3 PRIVATE ${xtra_dir} ${ab3job_dir} ${tokenizer_dir})
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
# and checkcore.cmake: a chunk file with a block of the former second
# build must fail it
add_test(NAME blitz2_checkcore_broken COMMAND ${CMAKE_COMMAND}
	-P ${CMAKE_CURRENT_SOURCE_DIR}/checkcore.cmake
	${CMAKE_CURRENT_SOURCE_DIR}/tests/checkcore/broken.c)
set_tests_properties(blitz2_checkcore_broken PROPERTIES PASS_REGULAR_EXPRESSION
	"BLITZ2_AB3 not allowed in the compiler's sources.*tests/checkcore/broken\\.c")

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
# (AMIGA-EMULATION-SETUP.md, "A500 machine").
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
