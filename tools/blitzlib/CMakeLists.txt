# blitzlib: Blitz Basic 2 library files (README.md); cmdref-check.cmake and
# cmdref-complete.cmake: the checks of the command reference (cmake/BlitzLibrary.cmake,
# libs/CMakeLists.txt)
set(BLITZLIB_SUBCOMMANDS join extract numbers check compare cmdref)
add_executable(blitzlib main.cpp cmd_biglib.cpp cmd_check.cpp cmd_compare.cpp cmd_cmdref.cpp)
target_link_libraries(blitzlib PRIVATE lib_cli lib_biglib lib_blitzlib lib_cmdref)
set_target_properties(blitzlib PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
host_help_test(blitzlib ${BLITZLIB_SUBCOMMANDS})
host_readme_test(blitzlib README.md ${BLITZLIB_SUBCOMMANDS})
