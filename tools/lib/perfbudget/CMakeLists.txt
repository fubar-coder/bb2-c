add_library(lib_perfbudget STATIC perfbudget.c)
set_target_properties(lib_perfbudget PROPERTIES OUTPUT_NAME perfbudget)
target_include_directories(lib_perfbudget PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
