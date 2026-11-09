# The host side of the TED A/B tests:
#   lib_teddump  screen dumps of teddrive: read, decode, compare (teddump.h)
#   lib_tedab    the TED A/B tests without emulator: directives, lint, option
#                block, requesters, session script, evaluation (tedab.h)
#   tedab        runs them in Amiberry (run, lint: cmd_run.cpp, run.c) and shows the
#                screen dumps by hand (info, text, cmp: cmd_dump.cpp); README.md
set(TED_FONTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../archive/sources/ted-1.1)
get_filename_component(TED_FONTS_DIR ${TED_FONTS_DIR} ABSOLUTE)

add_library(lib_teddump STATIC teddump.c)
target_include_directories(lib_teddump PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_teddump PUBLIC lib_runner_util lib_sha256)
target_compile_options(lib_teddump PRIVATE -Wall -Wextra)
set_target_properties(lib_teddump PROPERTIES C_STANDARD 23)

add_library(lib_tedab STATIC tedab.c)
target_include_directories(lib_tedab PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../amiga)
target_link_libraries(lib_tedab PUBLIC lib_teddump lib_runner_util lib_sha256 lib_previntended)
target_compile_options(lib_tedab PRIVATE -Wall -Wextra -Wno-unused-function)
set_target_properties(lib_tedab PROPERTIES C_STANDARD 23)

set(TEDAB_SUBCOMMANDS run lint info text cmp)
add_executable(tedab main.cpp cmd_run.cpp run.c cmd_dump.cpp)
target_link_libraries(tedab PRIVATE lib_cli lib_tedab lib_amiga_emulation lib_statline)
target_compile_definitions(tedab PRIVATE TED_FONTS_DIR="${TED_FONTS_DIR}")
target_compile_options(tedab PRIVATE -Wall -Wextra)
set_target_properties(tedab PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
host_help_test(tedab ${TEDAB_SUBCOMMANDS})
host_readme_test(tedab README.md ${TEDAB_SUBCOMMANDS})
