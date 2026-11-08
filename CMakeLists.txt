cmake_minimum_required(VERSION 4.2.3)

# Blitz Basic 2 in C: the compiler (blitz2), the editor (ted) and the
# libraries blitz2 reads (acidlibs, deflibs), built for AmigaOS with the
# m68k-amigaos-gcc.  The m68k toolchain is the default, also for IDEs that
# configure without a preset.
#
# BLITZ2_HOST=ON (preset native): only the host side (host/: the tools and
# the host tests) with the native compiler, as a project of its own - the
# m68k build has it as ExternalProject (build/<preset>/host), which an IDE
# does not see; this profile is for working on, running and debugging the
# host code in an IDE (docs/development.md, "IDE").
option(BLITZ2_HOST "only the host side (tools, host tests) with the native compiler" OFF)
if(BLITZ2_HOST)
	cmake_minimum_required(VERSION 4.2.3)
	project(blitzbasic2_native LANGUAGES C CXX)
	enable_testing()
	add_subdirectory(host)
	return()
endif()
if(NOT CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/cmake/m68k-amigaos.cmake)
endif()

project(blitzbasic2 VERSION 1.95 LANGUAGES C CXX)
include(cmake/emulation-queue.cmake)
include(cmake/bug-orig-order.cmake)
include(cmake/test-cost.cmake)

if(NOT AMIGAOS)
	message(FATAL_ERROR "Wrong toolchain file ${CMAKE_TOOLCHAIN_FILE}: only AmigaOS (cmake/m68k-amigaos.cmake)")
endif()

# the glue relies on __saveds (m68k-amigaos-gcc 16.2-rc21 or later)
include(cmake/check-saveds.cmake)
amiga_check_saveds(${CMAKE_C_COMPILER} ${PROJECT_SOURCE_DIR}/cmake/saveds/good.c)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)              # gnu23: inline asm in the glue, flexible array initialisers
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

# the linker script of the -fbaserel programs without C startup code (ted,
# ab3cli): .bss stored as zeros in the data hunk, as Kickstart 1.x's
# LoadSeg does not clear an unstored tail (#963; see the script)
set(BASEREL_NOSTART_LD ${PROJECT_SOURCE_DIR}/cmake/baserel-nostart.ld)

# the host side (host/): the tools (tools/) and the host tests of the
# components, built with the native compiler before the tools are used.
# The target keeps its name host_tools (the m68k steps depend on it).
include(ExternalProject)
set(HOST_DIR ${CMAKE_BINARY_DIR}/host)
set(HOST_TOOLS_DIR ${HOST_DIR}/tools)
set(BIGLIB ${HOST_TOOLS_DIR}/biglib/biglib)                     # joins library files
set(BLITZLIBCHECK ${HOST_TOOLS_DIR}/blitzlibcheck/blitzlibcheck) # checks a library file
set(BLITZLIBCOMPARE ${HOST_TOOLS_DIR}/blitzlibcompare/blitzlibcompare) # declarations vs original
set(BLITZTOKENS ${HOST_TOOLS_DIR}/blitztokens/blitztokens)       # token list of compiler + libs
set(CMDREF ${HOST_TOOLS_DIR}/cmdref/cmdref)                     # command table of a library as JSON
set(ASMCMP ${HOST_TOOLS_DIR}/asmcmp/asmcmp)                     # asm routines vs original
set(HUNKCHECK ${HOST_TOOLS_DIR}/hunkcheck/hunkcheck)             # checks the first hunk
set(AMIGA_RUN ${HOST_TOOLS_DIR}/amiga_run/amiga_run)             # runs a script in Amiberry
set(VASMLISTING ${HOST_TOOLS_DIR}/vasmlisting/vasmlisting)       # views of a vasm listing
set(CHECKLABELS ${HOST_TOOLS_DIR}/checklabels/checklabels)     # labels of ted.s marked in C
set(CHECKDATA ${HOST_TOOLS_DIR}/checkdata/checkdata)           # C data vs reference build
set(MKTESTDATA ${HOST_TOOLS_DIR}/mktestdata/mktestdata)         # test data from a description
set(BLITZRES ${HOST_TOOLS_DIR}/blitzres/blitzres)               # resident files with constants
set(BBLINES ${HOST_TOOLS_DIR}/bblines/bblines)                  # line edits of tokenised sources
set(BBDETOK ${HOST_TOOLS_DIR}/bbdetok/bbdetok)                  # tokenised sources as plain text
set(BB22 ${HOST_TOOLS_DIR}/bb22/bb22)                           # BB22 sources in TED's format
set(OUTMATCH ${HOST_TOOLS_DIR}/outmatch/outmatch)               # an output against an expect file
set(CDTABLE ${HOST_TOOLS_DIR}/cdtable/cdtable)                   # the CD result table from a fresh run
set(AB3TOK ${HOST_TOOLS_DIR}/ab3tok/ab3tok)                     # tokenise as AmiBlitz3 loads a source
set(TEDTOK ${HOST_DIR}/ted-tools/tedtok/tedtok)                 # tokenise like TED
set(TEDDETOK ${HOST_DIR}/ted-tools/teddetok/teddetok)           # a source as TED shows it
set(CORPUSRUN ${HOST_DIR}/corpus-runner/corpusrun)              # one corpus pass in Amiberry
set(CDRUN ${HOST_DIR}/corpus-runner/cdrun)                      # the CD through one compiler
set(TEDAB ${HOST_DIR}/ted-ab-runner/tedab)                      # the TED A/B tests in Amiberry
set(PREVCMP ${HOST_TOOLS_DIR}/prevcmp/prevcmp)                   # library files against the prev build
option(HOST_SANITIZE "host side with address/undefined behaviour sanitizers" OFF)
option(HOST_COVERAGE "host side with gcov coverage, the data of each host test apart" OFF)
ExternalProject_Add(host_tools
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/host
	BINARY_DIR ${HOST_DIR}
	CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
		-DAMIGA_EMULATION_DIR=${PROJECT_SOURCE_DIR}/amiga-emulation
		-DAMIGA_EMULATION_VASM=${VASM}
		-DHOST_SANITIZE=${HOST_SANITIZE}
		-DHOST_COVERAGE=${HOST_COVERAGE}
		-DAMIGA_EMULATION_OWNER=${CMAKE_BINARY_DIR}
	INSTALL_COMMAND ""
	BUILD_ALWAYS ON
	BUILD_BYPRODUCTS ${BIGLIB} ${BLITZLIBCHECK} ${BLITZLIBCOMPARE} ${BLITZTOKENS} ${CMDREF} ${ASMCMP} ${HUNKCHECK} ${AMIGA_RUN} ${VASMLISTING} ${CHECKLABELS} ${CHECKDATA} ${MKTESTDATA} ${BLITZRES} ${BBLINES} ${BBDETOK} ${BB22} ${OUTMATCH} ${CDTABLE} ${AB3TOK} ${TEDTOK} ${TEDDETOK} ${CORPUSRUN} ${CDRUN} ${TEDAB} ${PREVCMP})

include(cmake/BlitzLibrary.cmake)
include(cmake/prev.cmake)                # the build of the previous commit, on request

# ctest: the host side's tests (tools with broken inputs, host tests of
# the components) and the checks that are not part of a build step.  Label
# emulator: runs Amiberry (skipped without it or without the data in
# amiga-emulation/); ctest -LE emulator leaves them out.  Single host tests:
# ctest --test-dir build/<preset>/host
enable_testing()
add_test(NAME host COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${HOST_DIR}
	--output-on-failure -LE emulator)
# the emulator tests of the host side three at a time (each holds its own
# emulator slots and waits for free ones)
add_test(NAME host_emulator COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${HOST_DIR}
	--output-on-failure -L emulator -j 3)
math(EXPR limit "300 + ${EMULATION_QUEUE_WAIT}")
set_tests_properties(host_emulator PROPERTIES LABELS emulator TIMEOUT ${limit})
# the __saveds check of the configure step with a function without it
add_test(NAME toolchain_saveds_broken COMMAND ${CMAKE_COMMAND} -DCC=${CMAKE_C_COMPILER}
	-DSRC=${PROJECT_SOURCE_DIR}/cmake/saveds/broken.c -P ${PROJECT_SOURCE_DIR}/cmake/check-saveds.cmake)
set_tests_properties(toolchain_saveds_broken PROPERTIES
	PASS_REGULAR_EXPRESSION "__saveds does not[ \n]+load a4")  # CMake wraps long messages

# the library files byte by byte against the prev build (cmake/prev.cmake,
# tests/README.md "The prev build"); skipped (77) without one.  PREV_INTENDED
# names the files that may differ (a fix names them in its commit): ON = all,
# or a list of paths relative to the build directory, each of which must
# differ (-DPREV_INTENDED="libs/acidlibs/2dlib.obj;blitz2/acidlibs")
set(PREV_INTENDED OFF CACHE STRING
	"prev_results: ON = every library file may differ from the prev build, or a ;-list of paths (libs/acidlibs/2dlib.obj;blitz2/acidlibs)")
set(prev_results_args)
if(PREV_INTENDED STREQUAL "ON")
	list(APPEND prev_results_args --all)
elseif(PREV_INTENDED AND NOT PREV_INTENDED STREQUAL "OFF")
	foreach(f IN LISTS PREV_INTENDED)
		list(APPEND prev_results_args --intended ${f})
	endforeach()
endif()
add_test(NAME prev_results COMMAND ${PREVCMP} ${prev_results_args} ${CMAKE_BINARY_DIR} ${PREV_DIR})
set_tests_properties(prev_results PROPERTIES SKIP_RETURN_CODE 77)

# downloading the originals the tests need (not part of ALL: needs network)
add_custom_target(fetch-originals
	COMMAND ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/cmake/fetch-originals.cmake
	USES_TERMINAL
	COMMENT "Downloading the original material (archive/README.md)")
# the coverage of each test as lcov tracefile after a coverage test run (tests/README.md
# "Coverage"; not part of ALL)
add_custom_target(coverage-map
	COMMAND ${CMAKE_COMMAND} -DBUILD=${CMAKE_BINARY_DIR} -P ${PROJECT_SOURCE_DIR}/cmake/coverage-map.cmake
	USES_TERMINAL
	COMMENT "Coverage of each test as lcov tracefile: ${CMAKE_BINARY_DIR}/coverage/lcov/")
add_dependencies(coverage-map host_tools)

add_subdirectory(shared/edstub)
add_subdirectory(shared/xtra)
add_subdirectory(shared/tokenizer)
add_subdirectory(libs)
add_subdirectory(programs/blitz2)
add_subdirectory(programs/ted)
add_subdirectory(programs/ab3cli)
add_subdirectory(tests/corpus)
add_subdirectory(tests/bug-orig)
add_subdirectory(tests/bug-port)
add_subdirectory(tests/blitz2)
add_subdirectory(tests/ab3)
add_subdirectory(tests/ted-ab)

# the Blitz sources of the libraries, readable: build/<preset>/bblibsrc/
include(programs/ted/tools/teddetok/bblibsrc.cmake)
# the source snapshots of archive/sources/ as text: build/<preset>/archive-text/
include(tools/bbdetok/archive_text.cmake)

# cmake --install: a blitz2: directory (default build/<preset>/install/blitz2);
# ab3cli next to blitz2-ab3, which loads it as PROGDIR:ab3cli
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "" FORCE)
endif()
install(PROGRAMS ${BLITZ2_DIR}/blitz2 ${BLITZ2_DIR}/ted
	${BLITZ2_DIR}/blitz2-ab3 ${BLITZ2_DIR}/ab3cli DESTINATION blitz2)
install(FILES ${BLITZ2_DIR}/acidlibs ${BLITZ2_DIR}/deflibs DESTINATION blitz2)

# -h / --help of the Amiga test tools, in Amiberry
add_test(NAME help_amiga COMMAND ${CMAKE_COMMAND} -DAMIGA_RUN=${AMIGA_RUN}
	-DWORK=${CMAKE_BINARY_DIR}/help-amiga
	"-DPROGRAMS=$<TARGET_FILE:reqtrap>;$<TARGET_FILE:reqclose>;$<TARGET_FILE:reqclosetest>;$<TARGET_FILE:inject>;$<TARGET_FILE:wbrun>;$<TARGET_FILE:teddrive>;$<TARGET_FILE:teddrivetest>;$<TARGET_FILE:tedprobe>"
	-P ${PROJECT_SOURCE_DIR}/cmake/check-help-amiga.cmake)
math(EXPR limit "120 + ${EMULATION_QUEUE_WAIT}")
set_tests_properties(help_amiga PROPERTIES LABELS emulator SKIP_RETURN_CODE 77 TIMEOUT ${limit})

# after all tests: every emulator test gets the owner of its snapshots,
# this build directory (cmake/emulation-queue.cmake)
long_test_costs()

emulation_snapshot_owner(${CMAKE_BINARY_DIR})
# and in the coverage configuration the directory of its coverage files
if(BLITZ2_COVERAGE)
	emulation_coverage()
endif()
