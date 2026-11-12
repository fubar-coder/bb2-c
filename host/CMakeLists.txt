cmake_minimum_required(VERSION 4.2.3)

# The host side of the build: everything built with the native compiler -
# the tools the m68k build uses (tools/) and the host tests of the
# components (programs/ted/tests/host, programs/blitz2/tests/host).  The
# top-level CMakeLists.txt adds this directory as ExternalProject
# (build/<preset>/host).
project(blitzbasic2_host LANGUAGES C CXX)
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/emulation-queue.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # for clangd (host side in .clangd)

option(HOST_SANITIZE "address and undefined behaviour sanitizers for all host code" OFF)
if(HOST_SANITIZE)
	add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
	add_link_options(-fsanitize=address,undefined)
endif()
# gcov coverage of the program code the host tests compile: the sources
# below programs/*/src and shared/ (TED core, shared/xtra, the tokeniser,
# ...), not the tools, the test sources or the TED tools - gcc instruments
# only the functions of files the filter matches (-fprofile-filter-files,
# a regex on the source path as CMake passes it: absolute).  -O0, so that
# inlining hides no function.  Each test writes its data below
# coverage/<test> (GCOV_PREFIX, set at the end of this file);
# cmake/coverage-map.cmake reads it.
option(HOST_COVERAGE "gcov coverage of the program code of the host tests, the data of each test apart" OFF)
if(HOST_COVERAGE)
	get_filename_component(repo ${CMAKE_CURRENT_SOURCE_DIR}/.. ABSOLUTE)
	string(REGEX REPLACE "([][.*+?^$(){}|\\])" "\\\\\\1" repo_re "${repo}")
	add_compile_options(--coverage -O0 "-fprofile-filter-files=^${repo_re}/(programs/[^/]+/src|shared)/")
	add_link_options(--coverage)
endif()

enable_testing()
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Snitch.cmake)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/CLI11.cmake)

# the tests of every host program on CLI11 (tools/lib/cli,
# docs/development.md, "Host programs"): help_PROGRAM - -h and --help of
# the program and of each subcommand SUB, an unknown option, the
# subcommands of the help against the list (cmake/check-help.cmake);
# readme_PROGRAM - every help verbatim in the README and every usage block
# of the README a help (cmake/check-readme.cmake).  The program is a
# target of this project.
#   host_help_test(PROGRAM [SUB...])
#   host_readme_test(PROGRAM README.md [SUB...])
set(HOST_CHECK_HELP ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/check-help.cmake)
set(HOST_CHECK_README ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/check-readme.cmake)
function(host_help_test program)
	string(JOIN "," subcommands ${ARGN})
	add_test(NAME help_${program} COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:${program}>
		-DSUBCOMMANDS=${subcommands} -P ${HOST_CHECK_HELP})
endfunction()
function(host_readme_test program readme)
	get_filename_component(readme ${readme} ABSOLUTE)
	string(JOIN "," subcommands ${ARGN})
	add_test(NAME readme_${program} COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:${program}>
		-DREADME=${readme} -DSUBCOMMANDS=${subcommands} -P ${HOST_CHECK_README})
endfunction()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../tools tools)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../programs/ted/tests/host ted-tests-host)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../programs/blitz2/tests/host blitz2-tests-host)
# host tests of the library test framework (libs/tests/capture.c)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../libs/tests/host libs-tests-host)
# the tool on the TED core: tedsrc (tokenising like TED, for the corpus; a
# source as TED shows it: the readable library sources)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../programs/ted/tools/tedsrc ted-tools/tedsrc)
# the corpus runner (tests/corpus)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../tests/corpus/runner corpus-runner)
# host tests of the TED A/B tools (tests/ted-ab)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../tests/ted-ab/runner ted-ab-runner)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../tests/ted-ab/host ted-ab-host)

# the owner of the emulator snapshots (cmake/emulation-queue.cmake): the
# build directory - in the m68k build its top-level one (the same
# snapshots as its other emulator tests), else this one
set(AMIGA_EMULATION_OWNER ${CMAKE_BINARY_DIR} CACHE PATH
	"owner of the emulator snapshots of this build's tests (the build directory)")
emulation_snapshot_owner(${AMIGA_EMULATION_OWNER})

# HOST_COVERAGE: every test of this project (all directories) writes its
# .gcda files below coverage/<test>/<absolute object path> - one set per
# test, read by cmake/coverage-map.cmake as test host/<test>
if(HOST_COVERAGE)
	function(host_coverage_prefix dir)
		get_property(tests DIRECTORY ${dir} PROPERTY TESTS)
		foreach(t IN LISTS tests)
			set_property(TEST ${t} DIRECTORY ${dir} APPEND PROPERTY
				ENVIRONMENT "GCOV_PREFIX=${CMAKE_BINARY_DIR}/coverage/${t}")
		endforeach()
		get_property(subdirs DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
		foreach(s IN LISTS subdirs)
			host_coverage_prefix(${s})
		endforeach()
	endfunction()
	host_coverage_prefix(${CMAKE_CURRENT_SOURCE_DIR})
endif()
