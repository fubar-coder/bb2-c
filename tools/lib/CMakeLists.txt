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
#   bbdetok    a tokenised Blitz 2 / AmiBlitz source as plain text (TED's
#              or AmiBlitz's rules, unknown tokens as {L:N})
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
#   hdf        AmigaDOS hardfiles: RDB or raw partition, OFS/FFS files read
#              and written (the setup of the emulation data)
#   cmdref     the command table of a library file as JSON (the website's
#              command reference)
#   cdtable    the CD result table written from a fresh run, keeping the
#              note rows (tests/data/blitz2-cd.tsv)
#   oracletable  the oracle data of the TED tests written from a fresh run
#              of the original TED, keeping the note rows
#              (tests/data/ted-oracle.sha256)
#   prevcmp    the library files of two build directories byte by byte (the
#              prev build, the build of the previous commit)
#   previntended  the intended differences of the corpus and ted_ab against
#              the prev build (PREV_INTENDED entries)
#   corpusexpect  the expected data of the corpus (tests/corpus/expect) as
#              cases: the file that applies to a configuration, the record
#              from two runs of the original, the update from the port,
#              the cases for fix-scope
#   libscmp    the judge of the library tests: the cases of a run against
#              the expected data (libs/tests/expected), the expected data
#              from two recording runs or with the port's [fixed #N] values,
#              the cases a branch changes beyond its issues (fix-scope),
#              the raw block observations of a run (NAME.bin)
#   bugorigcmp  the bug-orig tests in the terms of the shared test model:
#              their observation files with the wildcard lines of a reason,
#              the recorded original against an observation, the cases of
#              a test (fix-scope), the acceptance of two recording runs
#   cli        the command-line shell of the host programs on CLI11
#              (cli.hpp, C++20, header only: cli::run, the help format)
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
add_subdirectory(bbdetok)
add_subdirectory(xtra)
add_subdirectory(ab3job)
add_subdirectory(tokenizer)
add_subdirectory(teststats)
add_subdirectory(outmatch)
add_subdirectory(covmap)
add_subdirectory(covsel)
add_subdirectory(hdf)
add_subdirectory(cmdref)
add_subdirectory(cdtable)
add_subdirectory(oracletable)
add_subdirectory(prevcmp)
add_subdirectory(previntended)
add_subdirectory(corpusexpect)
add_subdirectory(libscmp)
add_subdirectory(bugorigcmp)
add_subdirectory(cli)
