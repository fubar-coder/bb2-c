# The logic of the host tools as libraries (the tools are thin main.c
# around them; the tests use them directly):
#   hunk       reading AmigaDOS load files
#   blitzlib   Blitz Basic 2 library files: header, token table, checks
#   biglib     BigLib files (acidlibs, deflibs): writing and reading
#   blitztokens  the token list of the compiler and its libraries (as
#              the editor gets it)
#   hunkcheck  checking bytes at offsets of the first hunk
#   amiga_emulation  running AmigaDOS scripts in Amiberry for tests
#   vasmlisting  reading vasm listings (rows, symbols)
#   sha256     SHA-256 (hashes of generated test data)
#   checklabels  every code label of an assembler source marked once in C
#   checkdata  data of a C build against the reference build (labels)
#   asmcmp     assembler routines against the original (objdump)
#   mktestdata  binary test data (8SVX, ILBM, MED and tracker modules) from
#              a text description (the bug-orig tests)
#   bb22       the "BB22" source format of some RWE sources, in TED's format
#   blitzres   Blitz Basic 2 resident files: constants written, contents
#              read; the constants of NDK FD files and includes
add_subdirectory(hunk)
add_subdirectory(blitzlib)
add_subdirectory(biglib)
add_subdirectory(blitztokens)
add_subdirectory(hunkcheck)
add_subdirectory(amiga_emulation)
add_subdirectory(vasmlisting)
add_subdirectory(sha256)
add_subdirectory(checklabels)
add_subdirectory(checkdata)
add_subdirectory(asmcmp)
add_subdirectory(mktestdata)
add_subdirectory(bb22)
add_subdirectory(blitzres)
