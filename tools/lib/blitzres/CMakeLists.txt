add_library(lib_blitzres STATIC blitzres.c)
set_target_properties(lib_blitzres PROPERTIES OUTPUT_NAME blitzres)
target_include_directories(lib_blitzres PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_blitzres PUBLIC lib_hunk)
