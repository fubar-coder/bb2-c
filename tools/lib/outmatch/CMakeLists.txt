add_library(lib_outmatch STATIC outmatch.c)
set_target_properties(lib_outmatch PROPERTIES OUTPUT_NAME outmatch)
target_include_directories(lib_outmatch PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
