# Tasks

## 1. Configuration

- [x] 1.1 Parser of the strict TOML subset in `tools/lib` (one key per line; string, integer, boolean, one-line list of strings; table headers; comments; file and line of every key); verify: host tests with valid files and with deliberately broken input (each refused construct names file and line), with and without `-DHOST_SANITIZE=ON`
- [x] 1.2 Resolution of the hierarchy (`tests.toml` up to `root = true`, `ID.test.toml`; nearest wins per key, tables merge, lists replace; keys of a single test refused in directory files; unknown keys; relative paths; placeholders from the build's environment file); verify: host tests per rule, each with broken input
- [x] 1.3 Matrix and cells (explicit matrix table, run settings only, product minus exclusions, cell names, the side "needs original material"); verify: host tests including a key that is no run setting
- [x] 1.4 The grammar of queries, predicates and cell selectors, written down in `tests/README.md`, and the replacement per query; verify: host tests of every query and predicate, and of a more specific assertion replacing the default

## 2. Preprocessor and driver

- [x] 2.1 The preprocessor in `tools/lib`: check calls found and their arguments split by the compiler's rules (`findit`), the number K over the whole source in the call, the table (namespace: case, procedure, `init`, `end`; number in the namespace; name, line, expression text), the fixed placeholders; verify: host tests on the cases of spike 1 (strings, braces, nested macros, a check as macro argument), with broken input for an unknown placeholder
- [x] 2.2 The stability condition of the numbering (user: "Wenn das stabil funktioniert"): the same table for repeated runs and the numbers in the result files of 1.95, 2.10 and the port match it; verify: an emulator test with a source of several cases, procedures, a loop and `CNIF`; if it fails, switch to the constant counter (`basic-testlib` D4) and record it in design.md
- [x] 2.3 The driver generator (the macro text of `shared/basictest/macros.bas`, copied text, jump over it, start with the result file's name, batch init only if a case is selected, `Gosub` per selected case, batch end) and the map of driver lines to `ID.test.bas` lines; verify: host tests of the generated text, and an emulator test where a compile error in the test is reported as `ID.test.bas:LINE`

## 3. Judging in `corpusrun`

- [x] 3.1 A subcommand of `corpusrun` that runs one case of a test in the cells of one side from the environment file, the resolved configuration and the driver; verify: a test of the model with a passing case runs with `port` and `clib` in one session
- [x] 3.2 Judging, reading result files with the codec of `shared/basictest/`: verdicts of checks (several entries), preconditions, end entries of case and run, announced runtime errors against the requesters, the default assertions, declared deviations with validators and "not run" behind a declared crash; verify: host tests of the judge on recorded result files and observations, each rule with broken input
- [x] 3.3 Diagnostics: the effective configuration with origins, and the generated driver; verify: a host test of the printed configuration of a three-level hierarchy

- [x] 3.4 Stage the test library for every configuration (`basic-testlib` task 4.1, done here after 3.1); verify: as `basic-testlib` 4.1, and that task ticked there

## 4. Registration and selection

- [x] 4.1 The build-time registration tool: sources and configurations read, the source form checked (blocks paired in source order with the case at fault named, no `CNIF`/`CEND` cutting a block, only a comment after the case macros, order, no code outside the blocks, case names against the token lists, no check in a test's macro), names `PREFIX_ID__CASE` / `_originals`, labels, time limit, cost and fixtures from the configuration, empty sides not registered, the ctest include file; the build step depends on every file it read; verify: host tests per rule with broken input, and `ctest -N` lists the tests of a sample directory after the build
- [x] 4.2 The glob of `*.test.bas` in `tests/bug-orig/CMakeLists.txt` and `tests/blitz2/CMakeLists.txt` next to the existing tests; verify: a configure and build with one test of the model and the old tests unchanged in `ctest -N`
- [x] 4.3 The test selection: configuration files and test sources select their tests (`tools/testinfo/rules.txt`); verify: the selection tests of `testing/test-selection` with a changed `tests.toml`

## 5. Documentation and integration

- [x] 5.1 `tests/README.md` and `docs/testing.md`: the model, the files, the configuration keys, the assertions, the diagnostics; verify: the examples of the text are tests of the build
- [ ] 5.2 Move the `tests/blitz2/*-runerrs.bas` twins and their partners (eight pairs) into one `ID.test.bas` each with runtime error checking as an axis, as the first real use, and `array-index-div0-runerrs` (no twin) to one with runtime error checking on; their prev comparison is that of section 7 (after 7.4); verify: each keeps its verdict, the old pairs are gone, `ctest --preset all` passes (the coordinator's run)

## 6. String values by length and CRC32 (decided 2026-11-21, ~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md "String values: length and CRC32")

- [x] 6.1 The requirement "Queries of a string value" in the delta of `testing/basic-test-runner`: `==`/`!=` by length and CRC32, `contains`/`matches` only on values that are not cut, the report of a cut value; verify: `openspec validate basic-test-runner --strict`
- [x] 6.2 The judge (`tools/lib/testjudge`) and the documented query rules: `==`/`!=` on a string value by length and CRC32 (also on cut values), `contains`/`matches` on a cut value a configuration error, the report with `(cut)` and the length; verify: host tests with broken input (a wrong CRC32 in a result file does not pass `==`), with and without `-DHOST_SANITIZE=ON`
- [x] 6.3 design.md records the decision; verify: the text

## 7. Prev comparison of the tests of the model (decided 2026-11-23, ~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md "Test rework step 1: prev comparison of model tests; extension key")

- [ ] 7.1 The requirement "The tests of the BASIC test model are compared with the prev build" in a delta of `testing/previous-commit`, and D10 in design.md; verify: `openspec validate basic-test-runner --strict`
- [ ] 7.2 The driver of all cases of a source, written by the build step of its directory (`corpusrun case --write-driver`: the preprocessing, driver and tokenising of a run, every case selected); verify: a host test of the driver text with every case called, and a source whose driver cannot be written fails the build step
- [ ] 7.3 The comparison (`corpusrun case --prev`): one corpus pass with `run=off`, `clib` and `prev|exe:clib` per value of `runtime-errors` among the cells without original material, named `ID` and `ID-runerrs`, the PREV_INTENDED entries `PREFIX_ID_prev:NAME`; an entry of the test that names no compile of it is stale; a test with RWE user libraries refused; verify: host tests of the passes and of the entries with broken input (a stale and an unknown name, `rwe`)
- [ ] 7.4 The registration (`cmake/prev.cmake`, `basic_test_register(... PREV REGEX)`): the sources of `tests/blitz2` and the compiler's bug-orig and regression sources, the names in `prev_check_entries`; verify: emulator self tests without a prev build (the same compiler passes; a prev compiler that differs fails naming the compile; an intended name passes; a stale and an unknown name fail), and with `-DBLITZ2_PREV` every `blitz2_ID_prev` of the model passes
- [ ] 7.5 `tests/README.md` ("The prev build") and the header of `cmake/prev.cmake` name the new comparison and its entries; verify: the text
