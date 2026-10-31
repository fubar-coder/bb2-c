# the directory with the Kickstart ROM and the hardfiles (amiga-emulation/
# README.md); at run time $AMIGA_EMULATION_DIR or the params override it
set(AMIGA_EMULATION_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../amiga-emulation
	CACHE PATH "directory with the Kickstart ROM and the hardfiles")
get_filename_component(emulation_dir ${AMIGA_EMULATION_DIR} ABSOLUTE)

# SNAP:waitjob of the snapshot mode, an AmigaDOS executable: assembled by
# the vasm of the m68k toolchain (the m68k build passes its VASM, the
# native profile finds it like the toolchain file does) and embedded
# into the library (#embed; its hunk header is checked by static_assert)
find_program(AMIGA_EMULATION_VASM vasmm68k_mot
	PATHS $ENV{AMIGA_TOOLCHAIN_PREFIX}/bin /opt/amiga/bin /opt/m68k-amigaos/bin
	DOC "vasmm68k_mot of the m68k toolchain (assembles SNAP:waitjob)")
if(NOT AMIGA_EMULATION_VASM)
	message(FATAL_ERROR "vasmm68k_mot not found (m68k toolchain; set AMIGA_EMULATION_VASM)")
endif()
set(waiter ${CMAKE_CURRENT_BINARY_DIR}/waitjob)
add_custom_command(OUTPUT ${waiter}
	COMMAND ${AMIGA_EMULATION_VASM} -quiet -m68000 -Fhunkexe -nosym -o ${waiter}
		${CMAKE_CURRENT_SOURCE_DIR}/waitjob.s
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/waitjob.s
	COMMENT "Assembling SNAP:waitjob")
add_custom_target(amiga_emulation_waiter DEPENDS ${waiter})

add_library(lib_amiga_emulation STATIC amiga_emulation.c)
set_target_properties(lib_amiga_emulation PROPERTIES OUTPUT_NAME amiga_emulation)
target_include_directories(lib_amiga_emulation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(lib_amiga_emulation PRIVATE
	AMIGA_EMULATION_DIR_DEFAULT="${emulation_dir}")
target_compile_options(lib_amiga_emulation PRIVATE --embed-dir=${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(lib_amiga_emulation amiga_emulation_waiter)
# the templates and the waiter are embedded (#embed)
set_source_files_properties(amiga_emulation.c PROPERTIES
	OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/default.uae;${CMAKE_CURRENT_SOURCE_DIR}/a500.uae;${waiter}")
