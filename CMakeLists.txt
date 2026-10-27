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

# build/<preset>/blitz2/ is the result: blitz2, ted, acidlibs, deflibs as
# they belong into the blitz2: directory
set(BLITZ2_DIR ${CMAKE_BINARY_DIR}/blitz2)
file(MAKE_DIRECTORY ${BLITZ2_DIR})

# shared sources, compiled by each user with its own options
set(FILEREQ_DIR ${PROJECT_SOURCE_DIR}/shared/filereq)

# host tools (tools/), built with the native compiler before they are used
include(ExternalProject)
set(HOST_TOOLS_DIR ${CMAKE_BINARY_DIR}/host-tools)
set(BIGLIB ${HOST_TOOLS_DIR}/biglib/biglib)
ExternalProject_Add(host_tools
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/tools
	BINARY_DIR ${HOST_TOOLS_DIR}
	CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
	INSTALL_COMMAND ""
	BUILD_ALWAYS ON
	BUILD_BYPRODUCTS ${BIGLIB})

include(cmake/BlitzLibrary.cmake)

add_subdirectory(libs)
add_subdirectory(programs/blitz2)
add_subdirectory(programs/ted)

# cmake --install: a blitz2: directory (default build/<preset>/install/blitz2)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "" FORCE)
endif()
install(PROGRAMS ${BLITZ2_DIR}/blitz2 ${BLITZ2_DIR}/ted DESTINATION blitz2)
install(FILES ${BLITZ2_DIR}/acidlibs ${BLITZ2_DIR}/deflibs DESTINATION blitz2)
