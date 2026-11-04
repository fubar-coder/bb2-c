# Host tests of blitz2-ab3's pure code (programs/blitz2/src, no AmigaOS
# call), built with the native compiler by host/CMakeLists.txt: one snitch
# program per test file.
#   ab3date  dos.library's day number as a date (src/ab3date.c, the
#            escapes \\__DATE__ and \\__DATE_GER__)
set(BLITZ2_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../src)

add_library(blitz2_ab3date_host STATIC ${BLITZ2_SRC}/ab3date.c)
set_target_properties(blitz2_ab3date_host PROPERTIES C_STANDARD 99 C_EXTENSIONS OFF)
target_compile_options(blitz2_ab3date_host PRIVATE -Wall -Wextra -pedantic)
target_include_directories(blitz2_ab3date_host PUBLIC ${BLITZ2_SRC})

add_executable(test_ab3date test_ab3date.cpp)
target_link_libraries(test_ab3date PRIVATE blitz2_ab3date_host snitch::snitch)
target_compile_options(test_ab3date PRIVATE -Wall -Wextra)
set_target_properties(test_ab3date PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
add_test(NAME blitz2_ab3date COMMAND test_ab3date)
