cmake_minimum_required(VERSION 3.28)

# Blitz Basic 2 in C: the compiler (blitz2), the editor (ted) and the
# libraries blitz2 reads (acidlibs, deflibs), built for AmigaOS with the
# m68k-amigaos-gcc.  The m68k toolchain is the default, also for IDEs that
# configure without a preset.
if(NOT CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/cmake/m68k-amigaos.cmake)
endif()

project(blitzbasic2 VERSION 1.95 LANGUAGES C)

if(NOT AMIGAOS)
	message(FATAL_ERROR "Wrong toolchain file ${CMAKE_TOOLCHAIN_FILE}: only AmigaOS (cmake/m68k-amigaos.cmake)")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)              # gnu11: inline asm in the glue, flexible array initialisers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # for clangd

# .clangd in the source directory for this machine's toolchain (generated,
# not in git; the first configured build directory is the one clangd uses)
option(BLITZ2_CLANGD "write .clangd into the source directory" ON)
if(BLITZ2_CLANGD AND NOT EXISTS ${PROJECT_SOURCE_DIR}/.clangd)
	string(REGEX MATCH "^[0-9]+" AMIGA_GCC_MAJOR ${CMAKE_C_COMPILER_VERSION})
	file(RELATIVE_PATH CLANGD_DATABASE ${PROJECT_SOURCE_DIR} ${CMAKE_BINARY_DIR})
	configure_file(cmake/clangd.in ${PROJECT_SOURCE_DIR}/.clangd @ONLY)
endif()

# build/<preset>/blitz2/ is the result: blitz2, ted, acidlibs, deflibs as
# they belong into the blitz2: directory
set(BLITZ2_DIR ${CMAKE_BINARY_DIR}/blitz2)
file(MAKE_DIRECTORY ${BLITZ2_DIR})

# shared sources, compiled by each user with its own options
set(FILEREQ_DIR ${PROJECT_SOURCE_DIR}/shared/filereq)

# the host side (host/): the tools (tools/) and the host tests of the
# components, built with the native compiler before the tools are used.
# The target keeps its name host_tools (the m68k steps depend on it).
include(ExternalProject)
set(HOST_DIR ${CMAKE_BINARY_DIR}/host)
set(HOST_TOOLS_DIR ${HOST_DIR}/tools)
set(BIGLIB ${HOST_TOOLS_DIR}/biglib/biglib)                     # joins library files
set(BLITZLIBCHECK ${HOST_TOOLS_DIR}/blitzlibcheck/blitzlibcheck) # checks a library file
set(BLITZLIBCOMPARE ${HOST_TOOLS_DIR}/blitzlibcompare/blitzlibcompare) # declarations vs original
set(ASMCMP ${HOST_TOOLS_DIR}/asmcmp/asmcmp)                     # asm routines vs original
set(HUNKCHECK ${HOST_TOOLS_DIR}/hunkcheck/hunkcheck)             # checks the first hunk
set(AMIGA_RUN ${HOST_TOOLS_DIR}/amiga_run/amiga_run)             # runs a script in Amiberry
set(VASMLISTING ${HOST_TOOLS_DIR}/vasmlisting/vasmlisting)       # views of a vasm listing
set(CHECKLABELS ${HOST_TOOLS_DIR}/checklabels/checklabels)     # labels of ted.s marked in C
set(CHECKDATA ${HOST_TOOLS_DIR}/checkdata/checkdata)           # C data vs reference build
set(TEDTOK ${HOST_DIR}/ted-tools/tedtok/tedtok)                 # tokenise like TED
option(HOST_SANITIZE "host side with address/undefined behaviour sanitizers" OFF)
ExternalProject_Add(host_tools
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/host
	BINARY_DIR ${HOST_DIR}
	CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
		-DAMIGA_EMULATION_DIR=${PROJECT_SOURCE_DIR}/amiga-emulation
		-DHOST_SANITIZE=${HOST_SANITIZE}
	INSTALL_COMMAND ""
	BUILD_ALWAYS ON
	BUILD_BYPRODUCTS ${BIGLIB} ${BLITZLIBCHECK} ${BLITZLIBCOMPARE} ${ASMCMP} ${HUNKCHECK} ${AMIGA_RUN} ${VASMLISTING} ${CHECKLABELS} ${CHECKDATA} ${TEDTOK})

include(cmake/BlitzLibrary.cmake)

# ctest: the host side's tests (tools with broken inputs, host tests of
# the components) and the checks that are not part of a build step.  Label
# emulator: runs Amiberry (skipped without it or without the data in
# amiga-emulation/); ctest -LE emulator leaves them out.  Single host tests:
# ctest --test-dir build/<preset>/host
enable_testing()
add_test(NAME host COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${HOST_DIR}
	--output-on-failure -LE emulator)
add_test(NAME host_emulator COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${HOST_DIR}
	--output-on-failure -L emulator)
set_tests_properties(host_emulator PROPERTIES LABELS emulator TIMEOUT 300)

# downloading the originals the tests need (not part of ALL: needs network)
add_custom_target(fetch-originals
	COMMAND ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/cmake/fetch-originals.cmake
	USES_TERMINAL
	COMMENT "Downloading the original material (archive/README.md)")

add_subdirectory(shared/edstub)
add_subdirectory(libs)
add_subdirectory(programs/blitz2)
add_subdirectory(programs/ted)
add_subdirectory(tests/corpus)

# cmake --install: a blitz2: directory (default build/<preset>/install/blitz2)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "" FORCE)
endif()
install(PROGRAMS ${BLITZ2_DIR}/blitz2 ${BLITZ2_DIR}/ted DESTINATION blitz2)
install(FILES ${BLITZ2_DIR}/acidlibs ${BLITZ2_DIR}/deflibs DESTINATION blitz2)
