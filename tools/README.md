# Host tools

The programs the build and the tests run on the host: they check, convert
and judge what the m68k build makes. Each tool has a README with its usage
(the help text, checked against the program by the build) and the place of
its logic.

| Tool | What it covers |
|---|---|
| [`blitzlib`](blitzlib/README.md) | Blitz Basic 2 library files and BigLib files (`acidlibs`, `deflibs`): join, extract, library numbers, checks of a library built from C, comparison with the original, the command table as JSON for the website |
| [`bbsrc`](bbsrc/README.md) | Blitz Basic 2 and AmiBlitz sources without TED: the token list of a compiler, detokenising, editing lines of a tokenised source, the BB22 format, tokenising as AmiBlitz3 does |
| [`blitzres`](blitzres/README.md) | resident files (`.res`): write one with constants, print one, its constants as Blitz source |
| [`xtra`](xtra/README.md) | the companion settings of a source: `.xtra` files, the editor header of BB22 sources, AmiBlitz3's embedded header (read, check, convert) |
| [`hunkcheck`](hunkcheck/README.md) | the hunks of an AmigaDOS load file the build made: bytes at offsets, layout, CODE/DATA stored whole |
| [`refcheck`](refcheck/README.md) | the port against the originals: the assembler routines of the acidlibs, the labels and data of TED, vasm listings of the reference builds |
| [`results`](results/README.md) | a result against its expectation or the previous build: test output against expect files, the library files against the `prev` build, the CD result table, the TED oracle data |
| [`testinfo`](testinfo/README.md) | records of test runs: statistics from JUnit files, coverage per test, the tests a change can affect |
| [`mktestdata`](mktestdata/README.md) | binary test data (8SVX, ILBM, modules, `.xtra`, fixed layouts) from a text description |
| [`hdftool`](hdftool/README.md) | AmigaDOS hardfiles (RDB or raw partition, OFS/FFS): read and adapt; sets up the Workbench hardfiles from Amiga Forever |
| [`tedfont`](tedfont/README.md) | TED-C's screen fonts of 256 glyphs: TED 1.1's font files and the glyph text sources joined, and the check of that build step |
| [`amiga_run`](amiga_run/README.md) | an AmigaDOS script in Amiberry, without a window and isolated (front end of `lib/amiga_emulation`) |

Three tools live with the component they belong to:

| Tool | What it covers |
|---|---|
| [`tedsrc`](../programs/ted/tools/tedsrc/README.md) | a source through TED's C core on the host: tokenised or detokenised exactly as TED does it |
| [`tedab`](../tests/ted-ab/runner/README.md) | the TED A/B tests in Amiberry, their lint, and the screen dumps of `teddrive` |
| [`corpusrun`](../tests/corpus/runner/README.md) | the emulator passes of the compiler tests (corpus, bug-orig) and the run of the whole CD |

## Layout

* `TOOL/`: the program (`main.cpp` and the subcommands); the command line
  is CLI11 with the helpers of `lib/cli`.
* `lib/`: the logic, in C, one directory per topic; the tools and the
  tests link it.
* `tests/`: the host tests of the logic, with inputs from memory.

## Building

The tools are built with the host side: `cmake --build --preset amiga`
puts them in `build/amiga/host/tools/TOOL/`; `cmake --preset native`
builds only the host side, every tool a target of its own
(`docs/development.md`, "IDE").

## Command line

Every tool has subcommands (except `mktestdata` and `amiga_run`) and
`--help` on every level. Exit status: 0 success, 1 the check failed or
an error, 2 a wrong command line, 77 skipped (a missing input such as the
CD or Amiberry); a tool's README names the codes it adds.
