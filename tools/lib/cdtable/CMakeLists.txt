add_library(lib_cdtable STATIC cdtable.c)
set_target_properties(lib_cdtable PROPERTIES OUTPUT_NAME cdtable)
target_include_directories(lib_cdtable PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
