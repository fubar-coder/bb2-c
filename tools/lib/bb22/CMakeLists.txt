add_library(lib_bb22 STATIC bb22.c)
set_target_properties(lib_bb22 PROPERTIES OUTPUT_NAME bb22)
target_include_directories(lib_bb22 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
