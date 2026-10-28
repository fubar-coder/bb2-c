# The host side of the TED A/B tests:
#   lib_teddump  screen dumps of teddrive: read, decode, compare (teddump.h)
#   teddump      the same by hand (info / text / cmp)
set(TED_FONTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../archive/sources/ted-1.1)
get_filename_component(TED_FONTS_DIR ${TED_FONTS_DIR} ABSOLUTE)

add_library(lib_teddump STATIC teddump.c)
target_include_directories(lib_teddump PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_teddump PUBLIC lib_runner_util lib_sha256)
target_compile_options(lib_teddump PRIVATE -Wall -Wextra)
set_target_properties(lib_teddump PROPERTIES C_STANDARD 11)

add_executable(teddump main_teddump.c)
target_link_libraries(teddump PRIVATE lib_teddump)
target_compile_definitions(teddump PRIVATE TED_FONTS_DIR="${TED_FONTS_DIR}")
target_compile_options(teddump PRIVATE -Wall -Wextra)
