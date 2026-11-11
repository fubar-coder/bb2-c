# Design

## Context

- The coverage configuration (preset `amiga-cov`, test preset
  `coverage`) and the tools `covmap`/`covsel` exist: the compilers record
  called functions in a bitmap (`shared/cov/`), the runners collect the
  files of `TEST:cov` per test, `covmap` writes one lcov tracefile per
  test and `covsel` selects tests for a `git diff`. Their design is in
  the archived change `test-coverage`.
- `tools/covsel/rules.txt` selects every emulator test for a change of
  `libs/` and of TED's Amiga side, because neither is measured.
- **TED** has no libnix startup and is base-relative (data + bss below
  64 KB). `--coverage` adds counters and file names to data/bss.
- **Blitz libraries** are one hunk each (`libs/blitzlib.ld`,
  `-ffixed-a3/a4/a5`, freestanding). blitz2 copies a used library as a
  whole into the program (`asmlibs.c` `uselib3`); `libs_tests` loads
  library files itself.
- A study of 2026-11-05 found that `debuglib.c` has an inline asm that
  conflicts with the instrumentation (`debuglib.c:502`).

## Goals / Non-Goals

**Goals:**
- Line and branch coverage of TED and the Blitz libraries in the
  coverage configuration, as lcov tracefiles per test.
- Changes of `programs/ted/` and `libs/` select tests by coverage.
- No change to the normal configuration or to any result.

**Non-Goals:**
- Coverage of the Blitz source of compiled programs (change
  `blitz-coverage`).
- Replacing the full runs as acceptance.

## Decisions

### D1 gcov for TED
TED is compiled with `--coverage` and `-fprofile-dir`/
`-fprofile-prefix-path`, so that the `.gcda` names are flat names in
`TEST:cov`. TED's quit path calls the freestanding
`__gcov_info_to_gcda()` with own write callbacks (dos.library) for the
`gcov_info` objects of its sources, found through a table kept by the
linker script. If the data and bss growth breaks TED's base-relative
64 KB, the coverage configuration builds TED without baserel; its byte
checks against the reference (`checkdata`) are skipped only there.

### D2 gcov for the Blitz libraries, after a spike
Each library is compiled with `--coverage` and keeps a table of its
`gcov_info` objects behind a magic marker in its single hunk. First a
spike: whether the pointers inside the gcov data (counters, file names)
are relocated when blitz2 copies a library into a program and when
`libs_tests` loads it. If not, the libraries fall back to the function
bitmap of the compilers' coverage variant.
- `libs_tests` finds the markers in the loaded libraries and writes
  their `.gcda`.
- In compiled programs a coverage library's finit routine finds the
  markers and writes the `.gcda`; a compiler option of the coverage
  configuration links it into every program.
- `debuglib.c` stays out of the instrumentation.

### D3 Several runs in one session
Each run writes its own `.gcda` set (the freestanding writer does not
merge); the host merges them with `m68k-amigaos-gcov-tool merge` before
`covmap` reads them with the coverage build's `.gcno`.

### D4 Selection
Once TED and the libraries have tracefiles, the rules of
`tools/covsel/rules.txt` that select every emulator test for
`programs/ted/` and `libs/` are replaced by selection through the map.
Files of those directories that no tracefile names (data, headers) keep
their rules.

## Risks / Trade-offs

- [The gcov data are not relocated in a copied library] → The spike
  comes first; the fallback is the function bitmap, which needs no
  pointers.
- [The instrumentation changes register use of library routines that
  rely on gcc's register choice (see #1015)] → The coverage
  configuration's library tests run the register checks of
  `libs_emulator` as well.
- [TED without baserel behaves differently] → Only in the coverage
  configuration; `ted_ab` runs there as well.
