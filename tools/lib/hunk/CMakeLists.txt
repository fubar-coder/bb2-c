add_library(lib_hunk STATIC hunk.c)
set_target_properties(lib_hunk PROPERTIES OUTPUT_NAME hunk)
target_include_directories(lib_hunk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
