add_library(lib_exprmatrix STATIC exprmatrix.c)
set_target_properties(lib_exprmatrix PROPERTIES OUTPUT_NAME exprmatrix)
target_include_directories(lib_exprmatrix PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
