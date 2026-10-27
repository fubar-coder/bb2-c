cmake_minimum_required(VERSION 3.28)

# Host tools used by the m68k build (built with the native compiler: the
# top-level CMakeLists.txt adds this directory as ExternalProject).
#   lib/     the logic, as libraries
#   NAME/    the tools: main.c around lib/
#   tests/   tests of lib/ with inputs built in memory
project(blitzbasic2_tools LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # for clangd (tools/** in .clangd)
add_compile_options(-Wall -Wextra)

add_subdirectory(lib)
add_subdirectory(biglib)
add_subdirectory(blitzlibcheck)
add_subdirectory(hunkcheck)

enable_testing()
add_subdirectory(tests)
