add_library(lib_mktestdata STATIC mktestdata.c)
set_target_properties(lib_mktestdata PROPERTIES OUTPUT_NAME mktestdata)
target_include_directories(lib_mktestdata PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
