# Tasks

## 1. Host coverage (stage 1)

- [x] 1.1 Option `HOST_COVERAGE` in `host/CMakeLists.txt` (`--coverage -O0`) passed through from the top level like `HOST_SANITIZE` (D1). Verify: a host build with the option has `.gcno` files, one without has none; a clean normal build has no warnings and the same programs
- [x] 1.2 Per-test `GCOV_PREFIX` for every host test, set in one place after all tests are defined (D1). Verify: after `ctest --test-dir <host build>` each test has its own directory under `coverage/`, and a TED core test's data names TED core functions

## 2. Coverage variants of the compilers (stage 2)

- [x] 2.1 `shared/cov/cov.c`/`cov.h`: hook `__cyg_profile_func_enter`, bitmap over the code hunk, the start and end symbols checked against `blitz2-ab3.map` (D2). Excluded from instrumentation: the module and `glue*.c`
- [x] 2.2 `cov_write`: window pointer -1, `TEST:cov` lock, read-OR-write of `<name>.cov` with header, size mismatch overwrites, errors ignored; called on every exit path of blitz2 (find out whether libnix13 runs `atexit`, else direct calls) (D3)
- [x] 2.3 Option `BLITZ2_COVERAGE` in `programs/blitz2/CMakeLists.txt` (flags, exclude list, the module linked) and the presets `amiga-cov` (configure) and `coverage` (test) (D4). Verify: the `amiga-cov` build passes all its checks (`checklink`, `checkcore`) with no warnings; the normal build's `blitz2`, `blitz2-ab3`, `ted` and library files are byte-identical to before
- [x] 2.4 Check in the emulator: the coverage `blitz2` compiles the corpus with executables byte-identical to the normal `blitz2`'s; with `TEST:cov` it writes `blitz2.cov`, two runs merge; without the directory no file and no requester; on the A500 machine (Kickstart 1.3) it runs and writes. Measure the slowdown

## 3. Collecting in the runners

- [x] 3.1 List every runner that starts a compiler in the emulator (corpus, `tests/ab3/cli.cmake`, `tests/blitz2`, `tests/bug-orig`, `tests/ted-ab`, the RWE reference build, `libs/tests` if it compiles) and how each one knows its test name
- [x] 3.2 In the coverage configuration: create `cov/` in the test's `TEST:` directory before the session, move `cov/*.cov` to `<work>/coverage/<ctest name>/` afterwards (D5); raise time limits only there if 2.4 measured a need. Verify: `ctest --preset coverage -R 'corpus$|ab3_cli$'` leaves coverage files per test; the normal configuration's runs are unchanged

## 4. Map and selection tools

- [x] 4.1 `tools/lib/covmap` + `tools/covmap`: read gcov JSON (host) and `.cov` bitmaps with a link map (Amiga), write `map.tsv` with the commit, per-function and per-object lines (D6). Host tests with inputs from memory, including broken inputs (bad JSON, bitmap size not matching the map)
- [x] 4.2 `cmake/coverage-map.cmake`: after a coverage test run, run gcov per host test and `covmap` over all collected data; a target or script entry documented in `tests/README.md`
- [x] 4.3 Function line ranges of the m68k sources: try `-g` objects of the coverage build with `m68k-amigaos-objdump`/`nm -l`; if it works, feed them to `covmap`, otherwise record object-level selection for m68k sources (D7)
- [x] 4.4 `tools/lib/covsel` + `tools/covsel`: map + `git diff -U0` + ninja deps + `tools/covsel/rules.txt` → `selected.txt`; rules of D8 (header, test sources, `libs/`, build files, unknown files → all; new tests; stale map → all with a message). Host tests for each rule with inputs from memory
- [x] 4.5 `cmake/coverage-select.cmake` (runs git, `ninja -t deps`, `ctest -N`, then `covsel`) and the usage `ctest --preset all --tests-from-file …` in `tests/README.md`

- [x] 4.6 lcov as the one format (D6): `covmap` writes one tracefile per test, the merged `all.info` and the commit file instead of `map.tsv`; `covsel` builds its map from the tracefiles; `cmake/coverage-map.cmake`/`coverage-select.cmake`, README and the host tests of both tools adjusted. Verify: `genhtml` (if installed) reads `all.info`; the selection gives the same tests as with `map.tsv` for the sample diffs of 4.4

## 5. Check of stages 1 and 2

- [x] 5.1 Full coverage run (`ctest --preset coverage`) and map; check the scenarios of the specs: a TED core function named with its one host test, a change of one `eval.c` function selects only tests containing it, a header change, a CMake change (all), a new test, a stale map
- [x] 5.2 The list of blitz2 functions no test reaches (from the map), written to the scratch area and summarised in `docs/` (where the coverage is described) as the state at that commit
- [x] 5.3 Docs: `tests/README.md` (coverage configuration, map, selected runs; full runs stay the acceptance), `TODO.md` entry; a clean normal build, `ctest --preset all` and `ctest --preset bug-orig` with the known verdicts

## 6. Stage 3: TED and the Blitz libraries with gcov

- [ ] 6.1 Spike: a Blitz library compiled with `--coverage` (`gcov_info` table behind a marker): are the pointers inside the gcov data relocated when blitz2 copies the library into a program and when `libs_tests` loads it? Write the result into design.md; if not, the libraries use the function bitmap of stage 2 instead (D9)
- [ ] 6.2 TED with gcov in the coverage configuration: `--coverage`, flat `.gcda` names, written from its quit path through `__gcov_info_to_gcda()` and dos.library; baserel kept if it fits, else not in the coverage configuration; `ted_ab` and the TED bug-orig tests collect the `.gcda` (D9)
- [ ] 6.3 The Blitz libraries with gcov (or the bitmap, after 6.1); `debuglib.c` excluded; `blitzlibcheck` still passes; `libs_tests` writes the data of the loaded libraries
- [ ] 6.4 A coverage library whose finit routine writes the libraries' data in a compiled program; a compiler option of the coverage configuration that links it into every program
- [ ] 6.5 Collection: several `.gcda` sets of one test merged with `m68k-amigaos-gcov-tool merge`; `covmap` reads them with the coverage build's `.gcno`; `libs/` and `programs/ted/` changes then select by coverage (rules of D8 adjusted); check and docs as in 5.1 and 5.3

## 7. Coverage of the programs only

- [x] 7.1 `HOST_COVERAGE` instruments only the program code the host tests compile (`programs/*/src/`, `shared/`), not `tools/`, the test sources and the TED tools (`-fprofile-filter-files`, D1). Verify: the `.gcno` files with functions are those of `programs/*/src/` and `shared/`; a full `ctest --preset coverage` and `coverage-map` give tracefiles that name no file outside them
- [x] 7.2 `tools/covsel/rules.txt` for the tools and the runners: what the emulator tests run on the host selects every emulator test and the host tests, a tool only the host tests use the host tests, a tool of the build all tests; `test_covsel` checks each kind with the real rules. Verify: the scenarios of 5.1 on the new map, and a change of `tools/lib/amiga_emulation`, a runner, `tools/lib/covmap` and `tools/lib/biglib`
- [x] 7.3 `genhtml` and `genhtml --branch-coverage` (lcov 2.6) read `all.info` without errors; if branch errors remained, `covmap` would drop the branch records of lines run with no branch evaluated (not needed: none remain, design.md)
- [x] 7.4 `covmap` writes in a test's section from a coverage file one `DA:<first line>,<count>` per function whose first line is known (the function's entry, not a line count) and `LF`/`LH` for them, so that `genhtml` reports the compilers' files; `covsel` still selects by the functions. Verify: `test_covmap` (the records, made once per write), `test_covsel` (an entry line selects by its function); `covmap` on the map inputs of a full run: host tracefiles unchanged, `genhtml` and `genhtml --branch-coverage` (lcov 2.6) read `all.info` without errors and list `programs/blitz2/src` files, `covsel` selects the same tests as before (design.md)
