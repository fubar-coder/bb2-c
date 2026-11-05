# Design

## Context

- **Host side:** the host side is an ExternalProject (`host/CMakeLists.txt`) with an option
  `HOST_SANITIZE` passed through from the top level. Its 94 tests run as
  the single top-level ctest test `host` (`CMakeLists.txt`,
  `add_test(NAME host …)`). gcov works on it as it is: with
  `--coverage -O0` it builds in 13 s, and with `GCOV_PREFIX` per test
  the 94 tests leave 536 `.gcda` files in 22 s (study 2026-11-05,
  scratchpad `cov-gcov/`).
- **blitz2 and blitz2-ab3:** they link one object library
  `blitz2_core` with `-fbaserel` (data + bss below 64 KB) and
  `libnix13` (Kickstart 1.3, `checklink.cmake` refuses libnix20).
  `glue*.c` are the only files with assembler; their entry stubs load
  a4 (`GLUE_LOAD_A4`), and code that runs before that must not touch
  globals.
- **Instrumentation options:** `m68k-amigaos-gcc` 16.2-rc22/rc23
  compiles `-finstrument-functions`. It adds about 45 % text and no
  data. `--coverage` doubles text and adds 108 KB of data/bss. That
  breaks `-fbaserel` and pulls libnix20, and with it Kickstart 2.0.
- **The emulator runners:** they give each test its own host directory
  as `TEST:` (`tools/lib/amiga_emulation`). A file the program writes
  there is on the host already.
- **The map files:** the link maps (`blitz2.map`, `blitz2-ab3.map`) list
  every function symbol with its address. Function line ranges of the
  m68k sources are not available from the hunk files (the study could
  not decode the `.dwarf2` hunk).

## Goals / Non-Goals

**Goals:**
- Function granularity for every measured program.
- One recording mechanism that later serves TED and the Blitz libraries
  too.
- No change to the normal configuration or to any result.

**Non-Goals:**
- Line or branch coverage on the Amiga side.
- Coverage of Blitz source lines of compiled programs.
- Replacing the full runs as acceptance.
- Coverage reports for humans beyond a plain list of unreached
  functions.

## Decisions

### D1 Host coverage with gcov, per-test prefix
`HOST_COVERAGE` adds `--coverage` (and `-O0`, so that inlining does not
hide functions) to the host build, next to `HOST_SANITIZE`. Each host
test gets the environment `GCOV_PREFIX=<host build>/coverage/<test>`
through a test property set in one place, after all tests are defined
(a directory-wide loop over the tests). The coverage of host test `X` is
recorded as test `host/X`, the ctest name inside the host project.

**Alternative:** clang's source-based coverage. Not chosen, because the
host side builds with gcc.

### D2 Recording with `-finstrument-functions` and an own hook
A module `shared/cov/cov.c` (with `cov.h`) implements
`__cyg_profile_func_enter`: it sets one bit per function in a bitmap in
bss. The bit is indexed by the function's offset in the code hunk, `/2`.
The bitmap is about 5 KB for `blitz2-ab3`. The hook and the module are
built with `no_instrument_function`.

- **Excluded from instrumentation:** `glue*.c` and the hook module
  (`-finstrument-functions-exclude-file-list`), so that nothing is
  called before a4 is loaded.
- **The code range:** taken from two symbols, the start of the first
  object and the linker's end of text. Task 2.1 verifies them against
  the map.

**Alternatives:** `--coverage` (rejected for size, baserel and
Kickstart 1.3, see Context); `-fsanitize-coverage=trace-pc` (per edge
and slower, nothing gained at function granularity).

### D3 Writing the bitmap
The program's exit path calls `cov_write(name)`. This is an `atexit`
handler if libnix13 runs it, otherwise a call in `main`'s return and in
the error exit (decided in task 2.2). The steps:

1. The process window pointer is set to -1, so that a missing `TEST:`
   raises no requester.
2. `TEST:cov` is locked. If it is missing, nothing is written.
3. `TEST:cov/<name>.cov` is read if it exists, OR-ed with the bitmap and
   written back with `Open`/`Read`/`Write`/`Close`. These calls exist in
   Kickstart 1.3.

The file format:
- a header with a magic, a version, the size of the code hunk and the
  bitmap length;
- then the bitmap.

A size mismatch (another build) overwrites the file instead of merging
it. Write errors are ignored.

### D4 A coverage configuration, not extra targets
- **Configure preset:** `amiga-cov` sets `BLITZ2_COVERAGE=ON` in its own
  build directory. In that build `blitz2` and `blitz2-ab3` *are* the
  instrumented programs, so every existing test uses them without a
  change to how tests name the compiler (`$<TARGET_FILE:blitz2>`,
  `${BLITZ2_DIR}/blitz2-ab3`).
- **Test preset:** `coverage` runs `all` and `bug-orig` in that build.
- **Checks:** all checks stay on. `checklink` passes, because the hook
  needs nothing from libnix20. The CD byte comparisons concern the
  libraries and generated programs, which stay unchanged.

**Alternative:** `blitz2-cov` targets next to the normal ones in the
normal build. Every test definition would need a switch, and the normal
build would get slower.

### D5 Collecting in the runners
- **Before the session:** in the coverage configuration
  (`BLITZ2_COVERAGE`), each runner that starts a compiler creates
  `cov/` in the test's `TEST:` host directory.
- **After the session:** it moves `cov/*.cov` to
  `<work>/coverage/<ctest name>/`.

`TEST:` is a host directory, so no copying through the emulator is
needed. Task 3.1 lists the runners: corpus, `ab3/cli.cmake`,
`tests/blitz2`, `bug-orig`, `ted-ab` (TED starts the compiler) and the
RWE reference build.

### D6 Tools: `covmap` and `covsel`
These are C tools under `tools/`, with their logic in `tools/lib/cov*`
and host tests with inputs from memory.

- **`covmap`:**
  - It reads the per-test inputs:
    - host: the gcov JSON (`gcov --json-format --stdout`, called from a
      CMake script, which passes the text on);
    - Amiga: the `.cov` bitmaps plus the program's link map.
  - It writes `<build>/coverage/map.tsv`:
    - a first line with the commit (`git rev-parse HEAD`, given by the
      script);
    - then one line per function: `program`, `file`, `function`, `first
      line`, `last line` (empty where unknown), `tests`;
    - one more line per object file, listing the tests that executed
      any of its functions.
- **`covsel`:**
  - It reads the map, the output of `git diff -U0 <base>`, the ninja
    dependency list (`ninja -t deps`) and a rules file
    `tools/covsel/rules.txt`.
  - The rules file maps path patterns to tests: `all`, ctest labels or
    name patterns.
  - It writes `<build>/coverage/selected.txt` for `ctest
    --tests-from-file`.
  - A CMake script `cmake/coverage-select.cmake` runs git and ninja and
    passes their output to `covsel`. The tool itself starts no
    processes.

### D7 From changed lines to functions
- **Host sources:** gcov's JSON gives start and end lines per function,
  so a changed hunk inside a range selects that function.
- **m68k sources:** this needs function line ranges. Task 4.3 tries to
  read them from the objects of the coverage build compiled with `-g`
  (`m68k-amigaos-objdump`/`nm -l` on the `.o` files). If that does not
  work, a changed line selects every function of its object file. That
  is coarser, but never misses a test.

### D8 Rules for what coverage cannot see
- **Header:** its objects (ninja deps), then the tests of every function
  of those objects.
- **Test sources and expect files:** the ctest tests that use them (a
  rule per suite directory).
- **Library sources** (`libs/`): all emulator tests until stage 3 gives
  library coverage.
- **Everything else**, including CMake files, linker scripts, presets
  and the toolchain: all tests.
- **New tests**, i.e. names of `ctest -N` absent from the map: always
  selected.
- **A stale map** (its commit not an ancestor of the base, `git
  merge-base --is-ancestor`): all tests, with a message.

### D9 Stage 3 outline (own task group, after stages 1 and 2)
- **TED:** compiled with the same flags and module. It writes from its
  quit path, because TED has no libnix exit.
- **Blitz libraries:** each library gets the hook and a bitmap inside
  its single hunk, behind a magic marker.
  - `libs_tests` finds the markers in the loaded library and writes the
    bitmaps.
  - For compiled programs, a coverage library's finit routine searches
    the program's hunk for the markers and writes them. A compiler
    option may force that library into every program.
- **Mapping:** library offsets are mapped with `NAME.linked`'s symbols
  and the per-library position the compiler copies the code to
  (`asmlibs.c` `uselib3`).
- **Excluded:** `debuglib.c` has an inline asm that conflicts with the
  instrumentation prologue (study, `debuglib.c:501`) and stays out.

## Risks / Trade-offs

- [The hook clobbers a register the generated code or a callback relies
  on] → The hook is plain C under the normal ABI and is called only from
  instrumented C functions. glue is excluded. The coverage test run
  compares every generated executable with the normal build's.
- [The instrumented compiler runs slower and hits test time limits] →
  Measure in task 2.4 and raise the limits only for the coverage
  configuration.
- [Function granularity selects many tests for big functions (`eval5`,
  `doline`)] → Accepted: more tests, never fewer.
- [The map goes stale] → The commit is recorded, a stale map selects
  everything, and a coverage run is one command.
- [`-O0` on the host changes inlining but not the functions measured] →
  Host coverage is a separate configuration; normal host builds stay
  optimised.

## Migration Plan

No migration. Everything is new and opt-in. Removing it means deleting
the preset, the option, the module and the tools.

## Open Questions

- Whether libnix13 runs `atexit` handlers on every exit path of blitz2,
  or a direct call is needed (task 2.2; either way the behaviour in the
  spec holds).
- How the m68k function line ranges are obtained (D7). The fallback
  keeps the spec's guarantee.

## Findings

### Function line ranges of the m68k sources (task 4.3, D7)
- Objects compiled with `-g` carry DWARF 5 line tables in the hunk
  object format, and `m68k-amigaos-objdump --dwarf=decodedline` reads
  them (`nm -l` names no lines for them). `-g` leaves the code as it is:
  the `.text` of `blitz2-ab3` built by the preset `amiga-debug` (`-Os
  -g`) is byte-identical to the normal one.
- objdump 2.46.1 (AmigaDev 16.2-rc22) crashes on the line tables of
  several Amiga objects in one call; one call per object works.
- `cmake/coverage-map.cmake` passes the tables to `covmap`. A function's
  range is the smallest and largest line of the rows of its own source
  file within its bytes; rows of inlined code from the same file widen
  it, which can only select more. Static functions come from `nm -A` of
  the objects (the link map lists only global symbols).
- So the coverage configuration should compile with `-g`. Without it
  the ranges stay empty, and a change in a file selects the tests of all
  its functions (the object-level fallback of D7).
- Symbols of constant data in `.text` (`nm` type `t`/`T`, e.g. tables)
  appear in the map like functions, with no line range and no tests.
