# Host tools used by the m68k build (part of the host side, host/):
#   lib/     the logic, as libraries
#   NAME/    the tools: main.c around lib/
#   tests/   tests of lib/ with inputs built in memory
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra)

add_subdirectory(lib)
add_subdirectory(biglib)
add_subdirectory(blitzlibcheck)
add_subdirectory(hunkcheck)
add_subdirectory(amiga_run)

add_subdirectory(tests)
