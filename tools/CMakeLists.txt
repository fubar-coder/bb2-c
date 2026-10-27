cmake_minimum_required(VERSION 3.28)

# Host tools used by the m68k build (built with the native compiler: the
# top-level CMakeLists.txt adds this directory as ExternalProject).
project(blitzbasic2_tools LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_subdirectory(biglib)
