add_library(lib_sha256 STATIC sha256.c)
set_target_properties(lib_sha256 PROPERTIES OUTPUT_NAME sha256)
target_include_directories(lib_sha256 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
