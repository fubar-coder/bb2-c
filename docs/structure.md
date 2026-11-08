# Structure of the repository

```
CMakeLists.txt, CMakePresets.json
archive/                     unchanged originals (sources, reference data),
                             see archive/README.md
cmake/m68k-amigaos.cmake     toolchain (the only one)
cmake/BlitzLibrary.cmake     blitz_library(), biglib_join()
programs/blitz2/             compiler (src/: sections of blitz2.s + glue)
programs/ted/                editor (src/: Amiga part, src/core/: OS-free
                             core)
programs/ab3cli/             headless editor of blitz2-ab3's command line
                             (edstub frame + TED's core + shared/xtra)
libs/include/                blitzlib.h & co. (conventions of the libraries)
libs/variants/               the variants of the libraries (NAME/libnums.h:
                             the library numbers; bb2-2.10 the default)
libs/userlibs/               userlibs of a variant (rwe/: Red When Excited,
                             variant rwe-1999; rievallib: rwe-1999-eval,
                             risoundlib2: rwe-1999-ahi)
libs/blitzlib.ld             linker script: one code hunk, header at offset 0
libs/acidlibs/               one C file per library
libs/deflibs/                one C file per AmigaOS library
libs/tests/                  libs_tests: the libraries against their
                             originals in the emulator (m68k, snitch);
                             variants/: the test variants, check.cmake
shared/filereq/              file requester of freqlib and TED (one source,
                             compiled by both with their own options)
shared/edstub/               frame for programs that blitz2 loads as
                             blitz2:ted (entry points, comdata, no screen)
shared/ab3job/               the command line of blitz2-ab3 and the job it
                             hands to ab3cli (C99, no libc)
shared/xtra/                 the companion settings of a source (.xtra)
                             as one structure: Blitz 2 .xtra, BB22
                             header, AmiBlitz3 embedded header (C99, no
                             libc; compiled by each user: blitz2-ab3,
                             ab3cli, the host tools)
Doxyfile                     the API reference (Doxygen, Pages workflow;
                             docs/development.md)
site/                        the website (Hugo, site/README.md); data/
                             libraries/: the command tables of the
                             libraries (tools/cmdref, checked by the build)
shared/tokenizer/            source text to tokenised lines with a rule
                             set (AmiBlitz3's), the ends of AmiBlitz3's
                             embedded headers, its 250-character lines
                             (C99, no libc; compiled by each user: ab3cli,
                             the host tools)
host/                        the host side: everything built with the
                             native compiler (tools, host tests), as
                             ExternalProject (build/<preset>/host)
programs/ted/tests/host/     host tests of the TED core (snitch)
programs/ted/tools/          host tools on the TED core: tedtok (tokenize
                             like TED), teddetok (source as TED shows it;
                             target bblibsrc)
programs/ted/tests/oracle/   TED oracle: original TED as a library (m68k)
tests/data/                  generated test data (oracle hashes, token list,
                             CD reference of blitz2 2.10)
tests/ted-ab/                TED A/B: original TED against TED-C in the
                             emulator; amiga/: teddrive, teddrivetest,
                             tedprobe
                             scripts/: the tests (.drv), src/: sources,
                             runner/: tedab (the runs), teddump
                             (screen dumps), both host; host/: tests
tests/corpus/                corpus: blitz2-c against blitz2 2.10 in the
                             emulator; amiga/: tedstub (blitz2:ted for
                             batch runs), reqtrap, reqclose, reqclosetest
                             tests*/: the test programs (plain text, .bas),
                             runner/: corpusrun (one pass), cdrun (the
                             CD through a compiler), both host
tools/                       host tools:
  lib/                       the logic as libraries: hunk (read the hunk
                             format), blitzlib (header, token table,
                             check rules), biglib (write/read BigLib),
                             blitztokens (the token list of compiler
                             and libraries),
                             hunkcheck (bytes in the first hunk, the
                             layout, hunks stored whole),
                             amiga_emulation (scripts in Amiberry),
                             vasmlisting (read vasm listings), sha256,
                             checklabels (labels of ted.s in the C code),
                             checkdata (data against the reference build),
                             asmcmp (assembler routines against originals),
                             mktestdata (binary test data from a text
                             description: 8SVX, ILBM, modules),
                             bb22 (the BB22 source format of RWE, in
                             TED's format),
                             blitzres (residents: constants written,
                             contents read; NDK FD files and includes),
                             bblines (line edits of tokenised sources),
                             bbdetok (tokenised sources as text, TED's or
                             AmiBlitz's rules),
                             xtra (shared/xtra compiled for the host),
                             ab3job (shared/ab3job for the host),
                             tokenizer (shared/tokenizer for the host),
                             teststats (the @stat line of the emulator
                             sessions, statistics of JUnit files),
                             hdf (AmigaDOS hardfiles: RDB or raw
                             partition, OFS/FFS files read and written),
                             cmdref (the command table of a library file
                             as JSON, the website's command reference);
                             blitzlib also compares declarations
  biglib/, blitzlibcheck/,   the tools: one main.c each around lib/
  blitzlibcompare/,
  blitztokens/
  hunkcheck/, amiga_run/,
  refcheck/,
  mktestdata/, bb22/, blitzres/,
  bblines/, bbdetok/, xtra/,
  testinfo/, results/,
  ab3tok/, hdftool/, cmdref/
  bbdetok/archive_text.cmake the target archive_text: archive/sources/
                             as text (build/<preset>/archive-text/)
  tests/                     tests of lib/ with inputs from memory
openspec/                    OpenSpec: specs/ (what is specified),
                             changes/ (planned and archived changes)
.claude/agents/              agent definitions for Claude Code; the
                             OpenSpec skills and commands
                             (.claude/skills/, .claude/commands/) are
                             created locally by `openspec init` or
                             `openspec update` and are not in git
```

The libraries are compiled with the Blitz conventions
(`-ffixed-a3/a4/a5`: strings, locals and globals of the Blitz program;
freestanding; one hunk), blitz2 and TED as normal programs with
`-fbaserel` (a4 = base of their global data). That is why
`libs/acidlibs` (freqlib) and `programs/ted` each compile the shared file
requester themselves; each side provides its own `filereq_env.h` for it.
