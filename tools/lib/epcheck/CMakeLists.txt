add_library(lib_epcheck STATIC epcheck.c)
set_target_properties(lib_epcheck PROPERTIES OUTPUT_NAME epcheck)
target_include_directories(lib_epcheck PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
