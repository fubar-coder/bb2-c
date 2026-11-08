# blitzlib: Blitz Basic 2 library files (README.md); cmdref-check.cmake and
# cmdref-complete.cmake: the checks of the command reference (cmake/BlitzLibrary.cmake,
# libs/CMakeLists.txt)
set(BLITZLIB_SUBCOMMANDS join extract numbers check compare cmdref)
add_executable(blitzlib main.c cmd_biglib.c cmd_check.c cmd_compare.c cmd_cmdref.c)
target_link_libraries(blitzlib PRIVATE lib_subcmd lib_biglib lib_blitzlib lib_cmdref)
host_help_test(blitzlib ${BLITZLIB_SUBCOMMANDS})
host_readme_test(blitzlib README.md)
