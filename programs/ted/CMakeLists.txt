# ted - the C translation of TED 1.1, the Blitz Basic 2 editor (ted.s).
# blitz2 loads it as blitz2:ted and calls hunk offset 4 (`outside`).

# Core modules: no AmigaOS calls (they also build natively on a host, for
# unit tests); everything else is the AmigaOS part.
set(TED_CORE_MODULES state loadsave lines services fileio tokit lineedit keymap)
set(TED_AMIGA_MODULES main edit mouse reqs screen input data filereq rawkey)

# resources: the binary includes of ted.s (incbin) as C arrays;
# src/core/res.c embeds the original files of resources/ at build time
set(RES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)
set(RES_FILES sleep.bin pointer.bin src.info chars.b chars8.b)
list(TRANSFORM RES_FILES PREPEND ${RES_DIR}/)
set_source_files_properties(src/core/res.c PROPERTIES
	COMPILE_OPTIONS --embed-dir=${RES_DIR}
	OBJECT_DEPENDS "${RES_FILES}")

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
target_link_options(ted PRIVATE $<$<CONFIG:Release,MinSizeRel>:-s>)
set_target_properties(ted PROPERTIES OUTPUT_NAME ted SUFFIX ""
	RUNTIME_OUTPUT_DIRECTORY ${BLITZ2_DIR})
