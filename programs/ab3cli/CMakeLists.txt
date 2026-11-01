# ab3cli - the headless editor of a command line run of blitz2-ab3
# (src/main.c): blitz2-ab3 -s SOURCE -e EXE loads it as PROGDIR:ab3cli
# instead of blitz2:ted.  An editor-world program: the edstub frame
# (shared/edstub), TED's core (tedcore: loading and tokenising a source
# as TED does), shared/xtra and shared/tokenizer (AB3 mode's text), all
# compiled with TED's options - not linked into blitz2.  Next to blitz2-ab3 in build/<preset>/blitz2/.

# the frame and shared/xtra compiled here with TED's options (like
# shared/filereq for TED): with TED's -fbaserel the frame's entries load
# a4 (start.c), EDSTUB_INIT: ab3cli sets its hooks before main
set(xtra_dir ${PROJECT_SOURCE_DIR}/shared/xtra)
add_library(ab3cli_frame OBJECT ${EDSTUB_DIR}/start.c ${EDSTUB_DIR}/edstub.c)
target_compile_options(ab3cli_frame PRIVATE ${EDSTUB_COMPILE_OPTIONS})
target_compile_definitions(ab3cli_frame PRIVATE EDSTUB_INIT=1)

set(tokenizer_dir ${PROJECT_SOURCE_DIR}/shared/tokenizer)
add_executable(ab3cli $<TARGET_OBJECTS:ab3cli_frame> src/main.c src/hooks.c
	${xtra_dir}/xtra.c ${xtra_dir}/xtra_ab3.c ${tokenizer_dir}/tokenizer.c)
target_include_directories(ab3cli PRIVATE src ${EDSTUB_DIR} ${xtra_dir}
	${PROJECT_SOURCE_DIR}/shared/ab3job ${tokenizer_dir})
target_compile_definitions(ab3cli PRIVATE EDSTUB_INIT=1)
target_compile_options(ab3cli PRIVATE -fomit-frame-pointer -fno-strict-aliasing -ffreestanding
	-fno-builtin -Wall -Wno-pointer-sign -Wno-parentheses)
target_link_options(ab3cli PRIVATE ${EDSTUB_LINK_OPTIONS}
	$<$<CONFIG:Release,MinSizeRel>:-s> -Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/ab3cli.map)
# only the core modules that are used (not --whole-archive as TED)
target_link_libraries(ab3cli PRIVATE tedcore)
if(TED_BASEREL)
	target_compile_options(ab3cli_frame PRIVATE -fbaserel)
	target_compile_options(ab3cli PRIVATE -fbaserel)
	target_link_options(ab3cli PRIVATE -fbaserel)
endif()
set_target_properties(ab3cli PROPERTIES OUTPUT_NAME ab3cli SUFFIX ""
	RUNTIME_OUTPUT_DIRECTORY ${BLITZ2_DIR})

# checks after every link; if one fails, ab3cli does not count as built:
#   - hunk layout of the frame: +0 moveq #20,d0 (shell start fails), +4
#     movem.l (outside, blitz2 calls it), as the other edstub programs
# (no C library, -nostdlib: a libgcc or libnix helper - they need
# UtilityBase, which nothing opens - does not even link)
add_dependencies(ab3cli host_tools)
add_custom_command(TARGET ab3cli POST_BUILD
	COMMAND ${HUNKCHECK} $<TARGET_FILE:ab3cli> 0:7014 4:48e7
	VERBATIM)
