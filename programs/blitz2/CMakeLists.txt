# blitz2 - the C translation of the Blitz Basic 2 v1.95 compiler (blitz2.s).
# It calls the editor (blitz2:ted) by register conventions and runs the 68k
# code it generates, so it only makes sense on AmigaOS.

# chunk files in source order of blitz2.s, plus the assembler glue
set(BLITZ2_CHUNKS main compile stmts asmlibs parse eval libcall tokens1 tokens2 data)
list(TRANSFORM BLITZ2_CHUNKS PREPEND src/ OUTPUT_VARIABLE chunk_sources)
list(TRANSFORM chunk_sources APPEND .c)
file(GLOB glue_sources CONFIGURE_DEPENDS src/glue*.c)

add_executable(blitz2 ${chunk_sources} ${glue_sources})
target_include_directories(blitz2 PRIVATE src)
target_compile_options(blitz2 PRIVATE
	-fomit-frame-pointer
	-fno-strict-aliasing                # raw W_()/L_() accesses to patched templates and loaded images
	-Wall -Wno-pointer-sign -Wno-parentheses)
# globals relative to a4: shorter and faster accesses, fewer relocations
# (data + bss must stay below 64 KB; entry stubs in glue*.c load a4)
option(BLITZ2_BASEREL "address globals relative to a4 (-fbaserel)" ON)
if(BLITZ2_BASEREL)
	target_compile_options(blitz2 PRIVATE -fbaserel)
	target_link_options(blitz2 PRIVATE -fbaserel)
endif()
# no symbol table in release builds
target_link_options(blitz2 PRIVATE $<$<CONFIG:Release,MinSizeRel>:-s>)
set_target_properties(blitz2 PROPERTIES OUTPUT_NAME blitz2 SUFFIX ""
	RUNTIME_OUTPUT_DIRECTORY ${BLITZ2_DIR})

# ------------------------------------------------------- reference build
# The original blitz2.s v1.95 (archive/sources/blitz2-1.95) assembled with
# vasm as Devpac did (-nocase as in GenAm.opts): the reference of the port,
# in build/<preset>/programs/blitz2/ref/:
#   blitz2         the original compiler
#   blitz2.lst     vasm listing (addresses + opcodes)
#   blitz2.map     TSV address -> file:line, bytes, source
#   blitz2.labels  TSV address, label, file:line
# The same step checks that listing and binary agree (vasmlisting verify).
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
	COMMAND ${VASM} -quiet -devpac -nocase -m68000 -Fhunkexe
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
