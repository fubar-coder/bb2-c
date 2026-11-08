# Host tools used by the m68k build (part of the host side, host/):
#   lib/     the logic, as libraries
#   NAME/    the tools: main.c around lib/
#   tests/   tests of lib/ with inputs built in memory
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra)

add_subdirectory(lib)
add_subdirectory(biglib)
add_subdirectory(blitztokens)
add_subdirectory(blitzlibcheck)
add_subdirectory(blitzlibcompare)
add_subdirectory(hunkcheck)
add_subdirectory(amiga_run)
add_subdirectory(refcheck)
add_subdirectory(mktestdata)
add_subdirectory(bb22)
add_subdirectory(blitzres)
add_subdirectory(bblines)
add_subdirectory(bbdetok)
add_subdirectory(xtra)
add_subdirectory(testinfo)
add_subdirectory(results)
add_subdirectory(ab3tok)
add_subdirectory(hdftool)
add_subdirectory(cmdref)

add_subdirectory(tests)
