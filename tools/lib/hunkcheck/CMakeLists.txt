add_library(lib_hunkcheck STATIC hunkcheck.c)
set_target_properties(lib_hunkcheck PROPERTIES OUTPUT_NAME hunkcheck)
target_include_directories(lib_hunkcheck PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_hunkcheck PUBLIC lib_hunk)
