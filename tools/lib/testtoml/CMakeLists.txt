add_library(lib_testtoml STATIC testtoml.c)
set_target_properties(lib_testtoml PROPERTIES OUTPUT_NAME testtoml)
target_include_directories(lib_testtoml PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
