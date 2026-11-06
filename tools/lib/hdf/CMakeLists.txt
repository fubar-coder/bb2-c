add_library(lib_hdf STATIC hdf.c)
set_target_properties(lib_hdf PROPERTIES OUTPUT_NAME hdf)
target_include_directories(lib_hdf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
