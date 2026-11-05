# the tests a change can affect (covsel.h), from the map of covmap
add_library(lib_covsel STATIC covsel.c)
set_target_properties(lib_covsel PROPERTIES OUTPUT_NAME covsel)
target_include_directories(lib_covsel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_covsel PUBLIC lib_covmap)
