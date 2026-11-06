add_library(lib_cmdref STATIC cmdref.c)
set_target_properties(lib_cmdref PROPERTIES OUTPUT_NAME cmdref)
target_include_directories(lib_cmdref PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_cmdref PUBLIC lib_blitzlib lib_hunk)
