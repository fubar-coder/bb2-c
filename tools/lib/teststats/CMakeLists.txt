# the @stat line of the test runners (statline.h; statline_emulation.h
# fills it from an emulator session)
add_library(lib_statline STATIC statline.c)
set_target_properties(lib_statline PROPERTIES OUTPUT_NAME statline)
target_include_directories(lib_statline PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_statline PUBLIC m)
