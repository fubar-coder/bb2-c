# ted - the C translation of TED 1.1, the Blitz Basic 2 editor (ted.s).
# blitz2 loads it as blitz2:ted and calls hunk offset 4 (`outside`).

include(sources.cmake)

# original/: the original sources of TED 1.1 (ted.s and its includes,
# unchanged copy of bb2src.zip.dir/tedt) - for the reference build below
# and for the resources: the binary includes of ted.s (incbin) as C
# arrays, src/core/res.c embeds the original files at build time
set(TED_ORIG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/original)
set(RES_DIR ${TED_ORIG_DIR})
set(RES_FILES sleep.bin pointer.bin src.info chars.b chars8.b)
list(TRANSFORM RES_FILES PREPEND ${RES_DIR}/)
set_source_files_properties(src/core/res.c PROPERTIES
	COMPILE_OPTIONS --embed-dir=${RES_DIR}
	OBJECT_DEPENDS "${RES_FILES}")

# ------------------------------------------------------- reference build
# The original ted.s assembled with vasm as Devpac did (-nocase as in
# GenAm.opts): the reference of the port (not the CD binary, which is a
# different build), in build/<preset>/programs/ted/ref/:
#   ted         the original TED
#   ted.lst     vasm listing (addresses + opcodes)
#   ted.map     TSV address -> file:line, bytes, source
#   ted.labels  TSV address, label, file:line
# The same step checks that listing and binary agree (vasmlisting verify).
if(NOT VASM)
	find_program(VASM vasmm68k_mot REQUIRED)
endif()
set(REF_DIR ${CMAKE_CURRENT_BINARY_DIR}/ref)
set(REF_INPUTS ted.s myoffs wbheader aboutreq specialreq
	sleep.bin pointer.bin src.info rawed.obj filereq chars.b chars8.b)
list(TRANSFORM REF_INPUTS PREPEND ${TED_ORIG_DIR}/)
add_custom_command(
	OUTPUT ${REF_DIR}/ted ${REF_DIR}/ted.lst ${REF_DIR}/ted.map ${REF_DIR}/ted.labels
	COMMAND ${CMAKE_COMMAND} -E make_directory ${REF_DIR}
	COMMAND ${VASM} -quiet -devpac -nocase -m68000 -Fhunkexe
		-nowarn=62 -nowarn=40 -nowarn=41 -nowarn=51
		-o ${REF_DIR}/ted -L ${REF_DIR}/ted.lst -Lall ted.s > /dev/null
	COMMAND ${VASMLISTING} ${REF_DIR}/ted.lst map > ${REF_DIR}/ted.map
	COMMAND ${VASMLISTING} ${REF_DIR}/ted.lst labels > ${REF_DIR}/ted.labels
	COMMAND ${VASMLISTING} ${REF_DIR}/ted.lst verify ${REF_DIR}/ted
	DEPENDS ${REF_INPUTS} ${VASMLISTING} host_tools
	WORKING_DIRECTORY ${TED_ORIG_DIR}
	COMMENT "Assembling the original ted.s (reference)"
	VERBATIM)
add_custom_target(ted_ref ALL DEPENDS ${REF_DIR}/ted)

set(core_sources)
foreach(m IN LISTS TED_CORE_MODULES)
	list(APPEND core_sources src/core/${m}.c)
endforeach()
add_library(tedcore STATIC ${core_sources} src/core/res.c)
target_include_directories(tedcore PUBLIC src/core)
target_compile_options(tedcore PRIVATE -Wall -Wno-pointer-sign -Wno-parentheses)

set(amiga_sources)
foreach(m IN LISTS TED_AMIGA_MODULES)
	list(APPEND amiga_sources src/${m}.c)
endforeach()
file(GLOB glue_sources CONFIGURE_DEPENDS src/glue*.c)
list(REMOVE_ITEM glue_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/glue_start.c)

# glue_start.c must be the first object: blitz2 calls hunk offset 4
# (`outside`); no libnix startup code in front of it.  The file requester
# core shared with freqlib is compiled here with TED's options;
# src/filereq_env.h is its environment.
add_library(tedobj OBJECT src/glue_start.c ${amiga_sources} ${glue_sources}
	${FILEREQ_DIR}/filereq_core.c)
target_include_directories(tedobj PRIVATE src src/core ${FILEREQ_DIR})
target_compile_options(tedobj PRIVATE
	-fomit-frame-pointer -fno-strict-aliasing
	-Wall -Wno-pointer-sign -Wno-parentheses)
option(TED_BASEREL "address globals relative to a4 (-fbaserel)" ON)
if(TED_BASEREL)
	target_compile_options(tedcore PRIVATE -fbaserel)
	target_compile_options(tedobj PRIVATE -fbaserel)
endif()

add_executable(ted $<TARGET_OBJECTS:tedobj>)
target_link_options(ted PRIVATE -nostartfiles)
# whole archive: every core module is linked
target_link_libraries(ted PRIVATE -Wl,--whole-archive tedcore -Wl,--no-whole-archive)
if(TED_BASEREL)
	target_link_options(ted PRIVATE -fbaserel)
endif()
# no symbol table in release builds
target_link_options(ted PRIVATE $<$<CONFIG:Release,MinSizeRel>:-s>
	-Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/ted.map)
set_target_properties(ted PROPERTIES OUTPUT_NAME ted SUFFIX ""
	RUNTIME_OUTPUT_DIRECTORY ${BLITZ2_DIR})

# checks after every link; if one fails, ted does not count as built:
#   - hunk layout: blitz2 calls offset 4 of the first hunk (`outside`), DOS
#     a standalone TED at offset 0: bsr.w edstart ($6100), outside_stub
#     starts with movem.l ...,-(sp) ($48E7)
#   - no libgcc/libnix arithmetic helpers (they need UtilityBase, which TED
#     never opens) and nothing else from the libraries but string/setjmp code
add_dependencies(ted host_tools)
add_custom_command(TARGET ted POST_BUILD
	COMMAND ${HUNKCHECK} $<TARGET_FILE:ted> 0:6100 4:48e7
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/checklink.cmake
		${CMAKE_CURRENT_BINARY_DIR}/ted.map
	VERBATIM)
