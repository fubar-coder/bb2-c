# toklist: the compiler's own token list (programs/blitz2/src/toklist.h) as
# the JSON of its BASIC keywords for the website (README.md). The check of
# the committed site/data/compiler/blitz2.json runs in blitz2's build step
# (programs/blitz2/CMakeLists.txt, with the update target
# blitz2_keywords_update and the broken-input test blitz2_keywords_broken).
set(TOKLIST_SUBCOMMANDS json)
add_executable(toklist main.cpp)
target_link_libraries(toklist PRIVATE lib_cli lib_toklist)
set_target_properties(toklist PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
host_help_test(toklist ${TOKLIST_SUBCOMMANDS})
host_readme_test(toklist README.md ${TOKLIST_SUBCOMMANDS})

# the committed export against the token list (the check of the build step,
# here also in a build of the host side alone)
get_filename_component(repo ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)
add_test(NAME toklist_json COMMAND toklist json --check ${repo}/site/data/compiler/blitz2.json
	${repo}/programs/blitz2/src/toklist.h)
