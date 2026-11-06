add_library(lib_bbdetok STATIC bbdetok.c)
set_target_properties(lib_bbdetok PROPERTIES OUTPUT_NAME bbdetok)
target_include_directories(lib_bbdetok PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_bbdetok PUBLIC lib_blitztokens)
