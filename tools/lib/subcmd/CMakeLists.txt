add_library(lib_subcmd STATIC subcmd.c)
set_target_properties(lib_subcmd PROPERTIES OUTPUT_NAME subcmd)
target_include_directories(lib_subcmd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
