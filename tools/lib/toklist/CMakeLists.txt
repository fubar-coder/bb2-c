add_library(lib_toklist STATIC toklist.c)
set_target_properties(lib_toklist PROPERTIES OUTPUT_NAME toklist)
target_include_directories(lib_toklist PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
