add_library(hunkchecklib STATIC hunkcheck.c)
target_include_directories(hunkchecklib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hunkchecklib PUBLIC hunk)
target_compile_options(hunkchecklib PRIVATE -Wall -Wextra)

add_executable(hunkcheck main.c)
target_link_libraries(hunkcheck PRIVATE hunkchecklib)
target_compile_options(hunkcheck PRIVATE -Wall -Wextra)
