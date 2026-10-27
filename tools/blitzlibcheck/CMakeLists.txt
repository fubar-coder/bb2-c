add_library(blitzlib STATIC blitzlib.c)
target_include_directories(blitzlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(blitzlib PUBLIC hunk)
target_compile_options(blitzlib PRIVATE -Wall -Wextra)

add_executable(blitzlibcheck main.c)
target_link_libraries(blitzlibcheck PRIVATE blitzlib)
target_compile_options(blitzlibcheck PRIVATE -Wall -Wextra)
