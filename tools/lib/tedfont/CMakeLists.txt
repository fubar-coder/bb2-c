add_library(lib_tedfont STATIC tedfont.c)
set_target_properties(lib_tedfont PROPERTIES OUTPUT_NAME tedfont)
target_include_directories(lib_tedfont PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
