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
