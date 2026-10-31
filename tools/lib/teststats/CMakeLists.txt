# the @stat line of the test runners (statline.h; statline_emulation.h
# fills it from an emulator session) and the statistics of test runs from
# ctest's JUnit files (junit.h, teststats.h)
add_library(lib_statline STATIC statline.c)
set_target_properties(lib_statline PROPERTIES OUTPUT_NAME statline)
target_include_directories(lib_statline PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_statline PUBLIC m)

add_library(lib_teststats STATIC junit.c teststats.c)
set_target_properties(lib_teststats PROPERTIES OUTPUT_NAME teststats)
target_include_directories(lib_teststats PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_teststats PUBLIC lib_statline m)
