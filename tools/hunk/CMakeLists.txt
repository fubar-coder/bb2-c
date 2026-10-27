add_library(hunk STATIC hunk.c)
target_include_directories(hunk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(hunk PRIVATE -Wall -Wextra)
