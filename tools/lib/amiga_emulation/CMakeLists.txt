# the directory with the Kickstart ROM and the hardfiles (amiga-emulation/
# README.md); at run time $AMIGA_EMULATION_DIR or the params override it
set(AMIGA_EMULATION_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../amiga-emulation
	CACHE PATH "directory with the Kickstart ROM and the hardfiles")
get_filename_component(emulation_dir ${AMIGA_EMULATION_DIR} ABSOLUTE)

add_library(lib_amiga_emulation STATIC amiga_emulation.c)
set_target_properties(lib_amiga_emulation PROPERTIES OUTPUT_NAME amiga_emulation)
target_include_directories(lib_amiga_emulation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(lib_amiga_emulation PRIVATE
	AMIGA_EMULATION_DIR_DEFAULT="${emulation_dir}")
# the templates are embedded (#embed)
set_source_files_properties(amiga_emulation.c PROPERTIES
	OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/default.uae;${CMAKE_CURRENT_SOURCE_DIR}/a500.uae")
