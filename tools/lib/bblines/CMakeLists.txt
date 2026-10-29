add_library(lib_bblines STATIC bblines.c)
set_target_properties(lib_bblines PROPERTIES OUTPUT_NAME bblines)
target_include_directories(lib_bblines PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
