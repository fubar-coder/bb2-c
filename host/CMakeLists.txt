cmake_minimum_required(VERSION 3.28)

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

enable_testing()
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Snitch.cmake)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../tools tools)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../programs/ted/tests/host ted-tests-host)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../programs/blitz2/tests/host blitz2-tests-host)
# tools on the TED core: tedtok (tokenising like TED, for the corpus)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../programs/ted/tools/tedtok ted-tools/tedtok)
# teddetok (a source as TED shows it: the readable library sources)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../programs/ted/tools/teddetok ted-tools/teddetok)
# the corpus runner (tests/corpus)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../tests/corpus/runner corpus-runner)
# host tests of the TED A/B tools (tests/ted-ab)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../tests/ted-ab/runner ted-ab-runner)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../tests/ted-ab/host ted-ab-host)

# -h / --help of every callable host program: usage on stdout, exit 0
foreach(t amiga_run asmcmp biglib blitztokens blitzlibcheck blitzlibcompare checkdata checklabels
		hunkcheck mktestdata bb22 blitzres bblines xtra vasmlisting tedtok teddetok corpusrun cdrun tedab teddump
		outmatch ab3tok)
	add_test(NAME help_${t} COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:${t}>
		-P ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/check-help.cmake)
endforeach()

# the owner of the emulator snapshots (cmake/emulation-queue.cmake): the
# build directory - in the m68k build its top-level one (the same
# snapshots as its other emulator tests), else this one
set(AMIGA_EMULATION_OWNER ${CMAKE_BINARY_DIR} CACHE PATH
	"owner of the emulator snapshots of this build's tests (the build directory)")
emulation_snapshot_owner(${AMIGA_EMULATION_OWNER})
