add_library(lib_biglib STATIC biglib.c)
set_target_properties(lib_biglib PROPERTIES OUTPUT_NAME biglib)
target_include_directories(lib_biglib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_biglib PUBLIC lib_hunk)
