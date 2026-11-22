add_library(lib_testpp STATIC testpp.c testform.c testdriver.c)
set_target_properties(lib_testpp PROPERTIES OUTPUT_NAME testpp)
target_include_directories(lib_testpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
