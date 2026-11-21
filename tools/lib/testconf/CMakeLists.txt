add_library(lib_testconf STATIC testconf.c testmatrix.c testassert.c)
set_target_properties(lib_testconf PROPERTIES OUTPUT_NAME testconf)
target_include_directories(lib_testconf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_testconf PUBLIC lib_testtoml)
