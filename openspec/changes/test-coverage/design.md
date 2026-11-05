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

### D6 Tools: `covmap` and `covsel`, lcov as the one format
These are C tools under `tools/`, with their logic in `tools/lib/cov*`
and host tests with inputs from memory. The raw data stays in the format
of its source (gcov's `.gcda`, the `.cov` bitmaps); everything after the
collection on the host is lcov (tracefile format of lcov 2.x).

- **`covmap`:**
  - It reads the per-test inputs:
    - host and stage 3: the gcov JSON (`gcov --json-format --stdout`,
      called from a CMake script, which passes the text on);
    - stage 2: the `.cov` bitmaps plus the program's link map and the
      line ranges of the `-g` objects.
  - It writes one tracefile per test, `<build>/coverage/lcov/<test>.info`
    (`TN:<test>`, per file `SF:`, `FN:first,last,name`, `FNDA:count,name`,
    and `DA`/`BRDA` where gcov gives them; the bitmaps give only `FN`/`FNDA`,
    count 1 for a hit), a merged `<build>/coverage/lcov/all.info`, and
    `<build>/coverage/commit` with the commit (`git rev-parse HEAD`, given
    by the script).
- **`covsel`:**
  - It reads the tracefiles and the commit file, the output of `git diff
    -U0`, the ninja dependency list (`ninja -t deps`) and a rules file
    `tools/covsel/rules.txt`. The map from functions to tests is built
    in memory from the tracefiles.
  - The rules file maps path patterns to tests: `all`, ctest labels or
    name patterns.
  - It writes `<build>/coverage/selected.txt` for `ctest
    --tests-from-file`.
  - A CMake script `cmake/coverage-select.cmake` runs git and ninja and
    passes their output to `covsel`. The tool itself starts no
    processes.
- **Reports for people:** the tracefiles are read by `genhtml`, editors
  and coverage services; `genhtml` is optional, like Amiberry, and not
  part of the build.

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

### D9 Stage 3: TED and the libraries with gcov (own task group)
Stage 3 uses gcov (lines and branches) instead of the function bitmap.

- **TED:** compiled with `--coverage` and `-fprofile-dir`/
  `-fprofile-prefix-path`, so that the `.gcda` names are flat names in
  `TEST:cov`. TED has no libnix startup: its quit path calls the
  freestanding `__gcov_info_to_gcda()` with own write callbacks
  (dos.library) for the `gcov_info` objects of its sources (found through
  a table kept by the linker script). If the data and bss growth breaks
  TED's base-relative 64 KB, the coverage configuration builds TED
  without baserel; its byte checks against the reference (`checkdata`)
  are skipped only there.
- **Blitz libraries:** compiled with `--coverage`; each library keeps a
  table of its `gcov_info` objects behind a magic marker in its single
  hunk. First a spike: whether the pointers inside the gcov data
  (counters, file names) are relocated when blitz2 copies a library into
  a program (`asmlibs.c` `uselib3`) and when `libs_tests` loads it. If
  not, the libraries fall back to the function bitmap of stage 2.
  - `libs_tests` finds the markers in the loaded libraries and writes
    their `.gcda`.
  - In compiled programs a coverage library's finit routine finds the
    markers and writes the `.gcda`; a compiler option of the coverage
    configuration links it into every program.
- **Several runs in one session:** each run writes its own `.gcda` set
  (the freestanding writer does not merge); the host merges them with
  `m68k-amigaos-gcov-tool merge` before `covmap`.
- **Excluded:** `debuglib.c` has an inline asm that conflicts with the
  instrumentation (study, `debuglib.c:501`) and stays out.
- **Blitz source coverage** of compiled programs is a change of its own
  (`blitz-coverage`), with lcov output as well.

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

### Selection (tasks 4.4, 4.5, D6, D8)
- **The changes are taken against the map's commit**, not against the
  base: the map's line numbers are those of its commit, and the changes
  since the map include those since any later base. The base only
  decides whether the map is stale (`git merge-base --is-ancestor`).
  Untracked files count as new files.
- **Header:** each source whose object depends on the header counts as
  changed as a whole, by its own rule. A measured source gives the tests
  of all its functions. `glue*.c` is in the map, but it is not
  instrumented and has no tests there, so its rule is `all`. Every
  compiler header that `glue.c` includes (`blitz2.h`) therefore selects
  all tests.
- **Host tests:** the map names them `host/X`; ctest runs them through
  the top-level tests `host` and `host_emulator` (label `emulator`).
  `covsel` reads the host project's `ctest --show-only=json-v1` and
  selects the runner.
- **Tests absent from the map** are always selected (spec): new tests,
  and every test that runs no measured program (the library checks, the
  host-only checks). With only the compilers measured, that is a fixed
  part of every selection.
- **Rules beyond D8:** `none` for documentation (`docs/`, `openspec/`,
  top-level `*.md`). `libs/` also selects the library checks that do not
  run in the emulator (`declarations_*`, `tokens_*`, `asmcmp_*`, ...).
  Every `*.cmake` and `CMakeLists.txt` selects all tests, including the
  runner scripts of a single suite.

### lcov tracefiles (task 4.6, D6)
- **Files:** `covmap -o <build>/coverage` writes
  `lcov/<test>.info` (host test `X`: `lcov/host/X.info`),
  `lcov/all.info` and `commit`; `coverage-map.cmake` removes the
  tracefiles of an earlier run first. A test name that cannot be such a
  path (empty, absolute, `.`/`..` components, `all`) or a `TN:` (comma,
  tab, newline) is an input error.
- **`TN:`** is the test's name as it is (spec). lcov 2.x keeps only
  `[A-Za-z0-9_]` in test names and reads every other character as `_`,
  with a warning per tracefile ("invalid characters removed from
  testname"); so `genhtml` warns once for `all.info` and for each
  `host/...` or hyphenated test, without errors. `covsel` accepts the
  name in either form. If the warning should go, the spec's "the test's
  name as `TN:`" would have to allow lcov's form.
- **`all.info`** is the sections of all tests one after the other, in the
  order of their names: lcov's own form of tracefiles of different tests
  merged (`lcov -a` keeps a section per test name); `genhtml` adds them
  up per file.
- **Functions without known lines** (objects without `-g`, data symbols
  in `.text`) are written `FN:1,<name>` without a last line: lcov rejects
  line 0, and an `FN:` without a last line is lcov's older form. `covsel`
  reads an `FN:` without a last line as "range not known"; a change in
  such a file selects the tests of all its functions.
- **What a tracefile lists:** every function gcov names for the test, and
  every function of each program whose coverage file the test left, with
  `FNDA:0` if it did not run (lcov's convention: found and hit
  functions). gcov counts of one function, line or branch in several
  objects of a test add up; a function entered in two programs of one
  test (`blitz2`, `blitz2-ab3`) counts 1. `BRDA:`'s block is the number of
  the line's entry in gcov's list of the file (a line of several blocks
  or inlined copies has several entries), the taken count `-` if the line
  did not run.
- **Selection:** `covsel --coverage <dir>` reads `commit` and every
  tracefile below `lcov/` but `all.info`, the test's name from the path.
  A test counts as named by the map if its tracefile has a function with
  a count above 0. The map does not tell the programs apart: a file's
  functions are those of all programs (one source, one set of line
  ranges). `test_covsel`'s sample diffs select the same tests from the
  tracefiles as they did from `map.tsv`.
- **Format check:** `genhtml` is not installed here. `test_covmap`
  checks the written tracefiles against the reader rules of lcov 2.x
  (`lib/lcovutil.pm` of lcov at 1ad5aed: sections, line numbers above 0,
  `FN:` before `FNDA:`, each function once, the `FNF`/`FNH`, `BRF`/`BRH`,
  `LF`/`LH` counts), with a broken tracefile for each rule.
- **Branches need `gcov --branch-probabilities`:** without it gcov 16's
  JSON has empty `branches` lists; `coverage-map.cmake` passes it.
- **End to end** (`amiga-cov` with `-DHOST_COVERAGE=ON`, `ctest --preset
  coverage -R '^(host|corpus|ab3_cli)$'`, the target `coverage-map`, 34 s):
  tracefiles of 98 host tests and of `corpus` (`blitz2`, 14 files, 560
  functions entered) and `ab3_cli` (`blitz2-ab3`, 31 files, 682);
  `all.info` 3.2 MB, 636 sections, 97048 `BRDA:` records, consistent by
  the rules above. `coverage-select.cmake` on that build, and `covsel`
  with a change in `eval.c`'s `pokewd` (entered by both tests) or
  `pokewda5d` (entered by none), selects `corpus` and `ab3_cli`, or
  neither, besides the tests without a tracefile.

### Coverage variants of the compilers (tasks 2.1-2.4, D2-D4)
- (Task 2.1) `-finstrument-functions-once` (GCC 14 and later) instead of
  `-finstrument-functions`: the hook is called at the first entry of a
  function only; a guard byte per function in `.bss` (a4-relative) is
  tested on every later call, and `__cyg_profile_func_exit` is called at
  most once. A `__saveds` function (`runtheprog`) loads a4 before it
  tests its guard. Sizes: `blitz2-ab3` text 0x18464 → 0x271b0 (+61 %),
  bss +0x448 (the guards), data unchanged; `blitz2` text 0x1da0c.
- (Task 2.1) The bitmap is not in `.bss`: `cov_start` allocates it
  (`AllocMem`, `(__etext - __stext) / 16` bytes, about 10 KB for
  `blitz2-ab3`) from libnix's constructor list, before `main`;
  `cov_write` frees it.
- (Task 2.1) The code range: `__stext` is 0 (the start of `nbcrt0.o`,
  the first object), `__etext` the end of `.text`; both equal the code
  hunk of the executable (`blitz2-ab3`: hunk size 0x9c6c longs =
  0x271b0 = `__etext` in `blitz2-ab3.map`; `blitz2`: 0x7683 longs =
  0x1da0c). A bit's offset is the function's address in the map.
- (Task 2.1) The exclusion is a per-source `-fno-instrument-functions`
  (`cov_exclude` in `shared/cov/cov.cmake`) instead of
  `-finstrument-functions-exclude-file-list`, whose substring match on
  paths would also hit directories that happen to contain the pattern.
- (Task 2.3) `cov_instrument` compiles the instrumented programs with
  `-g` as well, for the line tables of task 4.3; the release link strips
  the programs, and their `.text` is the same with and without it.
- (Task 2.2) libnix13 has no exit code of its own: `exit()` and the
  return from `main` both go to `nbcrt0.o`'s `__exit`, which runs the
  `__EXIT_LIST__` from the highest priority down (`atexit` handlers via
  `__exitatexit`, priority 0, then `__exitlibraries`, priority -79, which
  closes `dos.library`). blitz2 ends only this way (`main`'s return,
  `exit(RETURN_FAIL)` in `parse.c`'s error path); the process of a
  running program (`runtheprog`) never exits through libnix. `cov.c`
  puts `cov_write` into that list with `ADD2EXIT` (priority 0) and
  `cov_start` into the constructor list with `ADD2INIT`: no call in
  blitz2's sources, and no `malloc`, which `atexit` would need.
- (Task 2.4) Same output: the `corpus` sources (runtime error checking
  off) give byte-identical executables with both compilers, on the
  A1200 (configurations port and clib) and on the A500 (a compile-only
  pass of the 70 sources with both compilers on Kickstart 1.3: 70
  executables identical), and `corpus_cd` gives its 724 results. The
  `ab3_cli` executables are identical apart from bytes that depend on
  the memory layout, not on the instrumentation: 84 of 110 differ only
  in the 8-byte blocks behind `trap #1`/`trap #4` of runtime error
  checking (the address of the editor's line item, which the corpus
  runner masks for this reason), and `tilde_string` also in the pad
  byte of `Even` after `Dc.b "J~",0`: `doeven`/`alignpc` advance `pc`
  without writing, so the byte is whatever the code buffer held there
  (the original does the same). A compiler of another size moves its
  allocations, so these bytes differ: the requirement's "byte-identical"
  holds for executables without such bytes (open point for the spec).
- (Task 2.4) Merging and the missing directory, in one session with
  `blitz2-ab3` (compiles of `atoi`, `int_tests_nostop`, `atoi`): 310,
  363, 363 bits set, each file a superset of the one before; a file of
  another build in `TEST:cov` (code size 64) was replaced at the first
  compile; after `Delete TEST:cov` a fourth compile wrote nothing, gave
  rc 0 and the session ended without a requester. The tests without
  `TEST:cov` (`corpus`, `corpus_cd`, `ab3_cli`) pass with the coverage
  compilers. On the A500 (Kickstart 1.3) `blitz2` writes its file
  (`bug_orig_deflibs_init_endless_retry`, the compile-only pass).
- (Task 2.4) Slowdown: on the A1200 (JIT) none beyond the noise of the
  shared emulator: `corpus_cd`'s three compile-only sessions 77.0 s
  with the coverage `blitz2`, 79.6 s with the normal one; `corpus`
  port 30.3 s against 32.0 s. On the A500 (68000, no JIT) the 70
  compiles took 16.3 to 17.1 s against 13.5 to 14.7 s (+10 to 25 %).
  No time limit needs raising.

### Collecting in the runners (tasks 3.1, 3.2, D5)
- (Task 3.1) The runners that start `blitz2` or `blitz2-ab3` in the
  emulator: the corpus runner `corpusrun` (`corpus*`,
  `ab3_bb2_equivalence*`, `blitz2_*` of `tests/blitz2`, the bug-orig
  tests of compiler, libs and rwe, `rwe_residents_cd`), `cdrun`
  (`corpus_cd`, `corpus_cd_ab3`; `rwe_refbuild*` runs the CD's
  compiler), `tests/ab3/cli.cmake` through `amiga_run` (`ab3_cli`,
  `ab3_cli_broken*`; `ab3_cli_ref*` runs AmiBlitz3) and `tedab`
  (`ted_ab` and the TED bug-orig tests: TED-C runs under `blitz2`).
  The others (`tests/ab3/tokenise.cmake`, the TED oracle with the
  original `blitz2`, `tests/bug-port`, `libs_emulator`, `help_amiga`)
  start none. All of them run their sessions through
  `tools/lib/amiga_emulation`, and none knows its ctest name. So D5 is
  implemented once, in the session library: CMake gives every emulator
  test of the coverage configuration (but `host_emulator`, the library's
  own tests) `AMIGA_EMULATION_COVERAGE=<test's binary
  directory>/coverage/<ctest name>` (`emulation_coverage` in
  `cmake/emulation-queue.cmake`); with it a session creates `TEST:cov`
  at its start, and its join moves the `.cov` files there, merged with a
  file of the same name from an earlier session of the test (OR, under
  `flock`; another build's file is replaced), and removes an empty
  `TEST:cov`. A runner that starts no compiler leaves nothing.
- (Task 3.2) `ctest --preset coverage -R 'corpus$|ab3_cli$'` leaves
  `tests/corpus/coverage/corpus/blitz2.cov` and
  `tests/ab3/coverage/ab3_cli/blitz2-ab3.cov` in the build directory and
  no `TEST:cov` behind; the tests of the normal configuration have no
  `AMIGA_EMULATION_COVERAGE`. No time limit is raised (task 2.4).
