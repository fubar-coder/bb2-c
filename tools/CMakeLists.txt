cmake_minimum_required(VERSION 3.28)

# Host tools used by the m68k build (built with the native compiler: the
# top-level CMakeLists.txt adds this directory as ExternalProject).
project(blitzbasic2_tools LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # for clangd (tools/** in .clangd)

add_subdirectory(hunk)
add_subdirectory(biglib)
add_subdirectory(blitzlibcheck)
add_subdirectory(hunkcheck)

enable_testing()
add_subdirectory(tests)
