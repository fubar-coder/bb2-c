add_library(lib_asmcmp STATIC asmcmp.c)
set_target_properties(lib_asmcmp PROPERTIES OUTPUT_NAME asmcmp)
target_include_directories(lib_asmcmp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_asmcmp PUBLIC lib_hunk)
