# CLI11 (command-line parser of the host programs,
# https://github.com/CLIUtils/CLI11, BSD-3-Clause), fixed version: the
# target CLI11::CLI11.  CLI11_PRECOMPILED: its non-template code is compiled
# once into a static library instead of in every translation unit that
# includes CLI/CLI.hpp (with -O3 that took about ten times as long per
# file).  A source given as FETCHCONTENT_SOURCE_DIR_CLI11 is
# used (the top-level build gives its own to the host side and to a prev
# build, cmake/prev-build.cmake); otherwise FetchContent downloads it.
# CLI11_SOURCE_ONLY (the top-level m68k build): only the source, no target
# - cli11_SOURCE_DIR is what the top-level build passes on.
include(FetchContent)
set(CLI11_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CLI11_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CLI11_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(CLI11_INSTALL OFF CACHE BOOL "" FORCE)
set(CLI11_PRECOMPILED ON CACHE BOOL "" FORCE)
if(CLI11_SOURCE_ONLY)
	set(cli11_subdir SOURCE_SUBDIR no-such-dir)   # FetchContent adds no directory
else()
	set(cli11_subdir)
endif()
FetchContent_Declare(CLI11
	URL https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.7.2.tar.gz
	URL_HASH SHA256=46eef3101da70852ec7af026e09d485ccee81813331c8c6052d39344443b83da
	${cli11_subdir}
	EXCLUDE_FROM_ALL
	SYSTEM)
FetchContent_MakeAvailable(CLI11)
