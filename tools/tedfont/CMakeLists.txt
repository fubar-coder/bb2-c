# tedfont: TED-C's 256-glyph screen fonts (README.md); the build step of
# programs/ted/fonts/fonts.cmake runs it
set(TEDFONT_SUBCOMMANDS build check)
add_executable(tedfont main.cpp)
target_link_libraries(tedfont PRIVATE lib_cli lib_tedfont)
set_target_properties(tedfont PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
host_help_test(tedfont ${TEDFONT_SUBCOMMANDS})
host_readme_test(tedfont README.md ${TEDFONT_SUBCOMMANDS})
