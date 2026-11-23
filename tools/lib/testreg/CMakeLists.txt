add_library(lib_testreg STATIC testreg.c)
set_target_properties(lib_testreg PROPERTIES OUTPUT_NAME testreg)
target_include_directories(lib_testreg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_testreg PUBLIC lib_testjudge lib_testconf lib_testpp lib_testcosts)
