add_library(lib_libscmp STATIC libscmp.c libscmp_accept.c libscmp_scope.c)
set_target_properties(lib_libscmp PROPERTIES OUTPUT_NAME libscmp)
target_include_directories(lib_libscmp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
