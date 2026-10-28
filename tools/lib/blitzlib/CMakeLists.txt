add_library(lib_blitzlib STATIC blitzlib.c compare.c)
set_target_properties(lib_blitzlib PROPERTIES OUTPUT_NAME blitzlib)
target_include_directories(lib_blitzlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_blitzlib PUBLIC lib_hunk)
