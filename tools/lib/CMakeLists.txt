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
#   bblines    edits of the lines of a tokenised Blitz 2 source (bytes)
#   xtra       the .xtra model of shared/xtra (Blitz 2 .xtra, BB22 header,
#              AmiBlitz3 embedded header), compiled for the host
#   ab3job     the command line of blitz2-ab3 (shared/ab3job), compiled for
#              the host
#   tokenizer  source text to tokenised lines (shared/tokenizer: AmiBlitz3's
#              rules), compiled for the host
#   teststats  the @stat line of the test runners' emulator sessions
#              (lib_statline) and statistics of test runs from ctest's
#              JUnit files: durations, flaky results, time limits, COST
#   outmatch   the output of a test run against an expect file (the
#              emulator tests of blitz2-ab3)
#   covmap     the map from functions to the tests that executed them (gcov
#              JSON of the host tests, coverage files of the Amiga programs
#              with their link maps)
#   covsel     the tests a change can affect, from that map, a diff, ninja's
#              dependencies and rules
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
add_subdirectory(bblines)
add_subdirectory(xtra)
add_subdirectory(ab3job)
add_subdirectory(tokenizer)
add_subdirectory(teststats)
add_subdirectory(outmatch)
add_subdirectory(covmap)
add_subdirectory(covsel)
