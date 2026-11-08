add_library(lib_prevcmp STATIC prevcmp.c)
set_target_properties(lib_prevcmp PROPERTIES OUTPUT_NAME prevcmp)
target_include_directories(lib_prevcmp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
