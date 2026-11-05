# the map from functions to the tests that executed them (covmap.h), with
# its helpers: a JSON reader (json.h), interned strings, paths, ninja's
# dependency lists (covutil.h); the coverage file format is
# shared/cov/covfmt.h
get_filename_component(COV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared/cov ABSOLUTE)
add_library(lib_covmap STATIC covmap.c covutil.c json.c)
set_target_properties(lib_covmap PROPERTIES OUTPUT_NAME covmap)
target_include_directories(lib_covmap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${COV_DIR})
