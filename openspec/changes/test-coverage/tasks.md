# Tasks

## 1. Host coverage (stage 1)

- [x] 1.1 Option `HOST_COVERAGE` in `host/CMakeLists.txt` (`--coverage -O0`) passed through from the top level like `HOST_SANITIZE` (D1). Verify: a host build with the option has `.gcno` files, one without has none; a clean normal build has no warnings and the same programs
- [x] 1.2 Per-test `GCOV_PREFIX` for every host test, set in one place after all tests are defined (D1). Verify: after `ctest --test-dir <host build>` each test has its own directory under `coverage/`, and a TED core test's data names TED core functions

## 2. Coverage variants of the compilers (stage 2)

- [ ] 2.1 `shared/cov/cov.c`/`cov.h`: hook `__cyg_profile_func_enter`, bitmap over the code hunk, the start and end symbols checked against `blitz2-ab3.map` (D2). Excluded from instrumentation: the module and `glue*.c`
- [ ] 2.2 `cov_write`: window pointer -1, `TEST:cov` lock, read-OR-write of `<name>.cov` with header, size mismatch overwrites, errors ignored; called on every exit path of blitz2 (find out whether libnix13 runs `atexit`, else direct calls) (D3)
- [ ] 2.3 Option `BLITZ2_COVERAGE` in `programs/blitz2/CMakeLists.txt` (flags, exclude list, the module linked) and the presets `amiga-cov` (configure) and `coverage` (test) (D4). Verify: the `amiga-cov` build passes all its checks (`checklink`, `checkcore`) with no warnings; the normal build's `blitz2`, `blitz2-ab3`, `ted` and library files are byte-identical to before
- [ ] 2.4 Check in the emulator: the coverage `blitz2` compiles the corpus with executables byte-identical to the normal `blitz2`'s; with `TEST:cov` it writes `blitz2.cov`, two runs merge; without the directory no file and no requester; on the A500 machine (Kickstart 1.3) it runs and writes. Measure the slowdown

## 3. Collecting in the runners

- [ ] 3.1 List every runner that starts a compiler in the emulator (corpus, `tests/ab3/cli.cmake`, `tests/blitz2`, `tests/bug-orig`, `tests/ted-ab`, the RWE reference build, `libs/tests` if it compiles) and how each one knows its test name
- [ ] 3.2 In the coverage configuration: create `cov/` in the test's `TEST:` directory before the session, move `cov/*.cov` to `<work>/coverage/<ctest name>/` afterwards (D5); raise time limits only there if 2.4 measured a need. Verify: `ctest --preset coverage -R 'corpus$|ab3_cli$'` leaves coverage files per test; the normal configuration's runs are unchanged

## 4. Map and selection tools

- [ ] 4.1 `tools/lib/covmap` + `tools/covmap`: read gcov JSON (host) and `.cov` bitmaps with a link map (Amiga), write `map.tsv` with the commit, per-function and per-object lines (D6). Host tests with inputs from memory, including broken inputs (bad JSON, bitmap size not matching the map)
- [ ] 4.2 `cmake/coverage-map.cmake`: after a coverage test run, run gcov per host test and `covmap` over all collected data; a target or script entry documented in README
- [ ] 4.3 Function line ranges of the m68k sources: try `-g` objects of the coverage build with `m68k-amigaos-objdump`/`nm -l`; if it works, feed them to `covmap`, otherwise record object-level selection for m68k sources (D7)
- [ ] 4.4 `tools/lib/covsel` + `tools/covsel`: map + `git diff -U0` + ninja deps + `tools/covsel/rules.txt` → `selected.txt`; rules of D8 (header, test sources, `libs/`, build files, unknown files → all; new tests; stale map → all with a message). Host tests for each rule with inputs from memory
- [ ] 4.5 `cmake/coverage-select.cmake` (runs git, `ninja -t deps`, `ctest -N`, then `covsel`) and the usage `ctest --preset all --tests-from-file …` in README

## 5. Check of stages 1 and 2

- [ ] 5.1 Full coverage run (`ctest --preset coverage`) and map; check the scenarios of the specs: a TED core function named with its one host test, a change of one `eval.c` function selects only tests containing it, a header change, a CMake change (all), a new test, a stale map
- [ ] 5.2 The list of blitz2 functions no test reaches (from the map), written to the scratch area and summarised in `docs/` (where the coverage is described) as the state at that commit
- [ ] 5.3 Docs: README (coverage configuration, map, selected runs; full runs stay the acceptance), `TODO.md` entry; a clean normal build, `ctest --preset all` and `ctest --preset bug-orig` with the known verdicts

## 6. Stage 3: TED and the Blitz libraries

- [ ] 6.1 TED with the same module and flags in the coverage configuration, writing from its quit path; `ted_ab` and the TED bug-orig tests collect `ted.cov` (D9). Checks that cannot hold for an instrumented TED (`checkdata` against the reference) are skipped only in the coverage configuration
- [ ] 6.2 The Blitz libraries instrumented in the coverage configuration (hook and bitmap inside the hunk behind a marker; `debuglib.c` excluded); `blitzlibcheck` still passes. `libs_tests` writes the bitmaps of the loaded libraries
- [ ] 6.3 A coverage library whose finit routine finds the libraries' bitmaps in a compiled program and writes them; if needed a compiler option that always links it (coverage configuration only). Mapping of library offsets through `uselib3`'s placement and `NAME.linked`
- [ ] 6.4 `covmap` and `covsel` for TED and the libraries (`libs/` changes then select by coverage instead of all emulator tests); check and docs as in 5.1 and 5.3
