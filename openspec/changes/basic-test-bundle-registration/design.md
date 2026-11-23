# Design

## Context

- testreg (`tools/lib/testreg/testreg.c`): `treg_add_source` per source,
  one `treg_test` per case and side; `write_test` (`:670`) writes
  `add_test` + `set_tests_properties` (LABELS, TIMEOUT, SKIP_RETURN_CODE 77,
  COST, FIXTURES_REQUIRED) + `basic_test_environment`; `treg_write_include`
  (`:712`) writes the tests without the label bug-orig in order, then those
  with it sorted by COST with the old-form entries.
- One testreg run per `basic_test_register` call (`cmake/basic-test-model.cmake:117`):
  `bug_orig_`, `regression_`, `blitz2_`. Bundles stay inside one run.
- The presets select by label (`CMakePresets.json`: `all` excludes
  `oracle|originals|bug-orig|bug-port|perf`, `bug-orig` includes
  `bug-orig|bug-port`); `testinfo select` writes a list of case tests.
- Coverage: `AMIGA_EMULATION_COVERAGE=BIN/coverage/NAME` per test
  (`cmake/emulation-queue.cmake`); the selection reads one tracefile per test
  (`testing/test-selection`).
- ctest scheduling with DEPENDS, measured (cmake 4.5.1, a toy project, `-j2`):
  a test of COST 1 that three tests DEPEND on starts before four tests of
  COST 50; `ctest -R case` runs a dependent test without its dependency.
- The interface: `basic-test-bundle-key`, design.md, I1-I3; the bundle's
  budget and TIMEOUT: `basic-test-bundle-runner`, design decision 4.

## Goals / Non-Goals

**Goals:** bundle tests derived at build time, nothing in the tomls; presets
and `-R`/`-L` selections keep selecting what they select today.

**Non-Goals:** the proposal, "Not in scope".

## Decisions

1. **Groups.** After all sources: the case tests of the side port for
   which `tj_bundle_case` is 1, grouped by (session key, sorted labels,
   sorted fixtures), each group in the order of `r->t`; chunks of 16
   members. With one volume per bundle (G1 (b), shown to work by the spike
   of `basic-test-bundle-key`) a chunk of one member is a bundle like any
   other: it runs as an ordinary session with the same emulator
   configuration. The source must be read once for the key; testreg already
   has `conf`, `m`, `src` in `treg_add_source` and keeps the key and the
   flag in `treg_test`.
2. **The bundle test** (`struct treg_test` with `bundle` set and its
   members): name I3; command I1 with the registration's `--command` words
   (it holds `--require-deviation` for `bug_orig_`); LABELS and
   FIXTURES_REQUIRED of the group; TIMEOUT = chunks x (3 x the key's limit) +
   the registration's extra (60 + `EMULATION_QUEUE_WAIT`); COST: its entry of
   the measured COSTs (by name, `treg_apply_costs`), else the sum of its
   members' COSTs; `basic_test_environment`. A bundle with the label
   bug-orig is ordered with the bug-orig tests by that COST.
3. **The members**: `--test TEST --from-bundle NAME` behind the command,
   `DEPENDS NAME` in the properties; nothing else changes.
4. **Switch.** `BASIC_TEST_BUNDLE` (BOOL cache, ON); OFF or
   `BLITZ2_COVERAGE` passes `--no-bundle` to testreg. In a coverage build a
   served case would have no tracefile, and the bundle's would hold the
   functions of 16 cases.
5. **Comparison run** (once, before the merge; task 4.2): `ctest --preset
   all` and `bug-orig` with bundles and with `BASIC_TEST_BUNDLE=OFF`, the
   verdict of every member compared from the two JUnit files. A difference
   stops the merge and is reported.

### Planning choices (mine)

- `Point: groups split by labels and fixtures — fewer cases per bundle, but a preset that selects a case also selects its bundle — L1 Unlikely / I2 Medium / E2 Reasoned → Monitor`.
- `Point: no bundles in a coverage build — coverage runs stay as slow as today — L1 Unlikely / I2 Medium / E2 Reasoned → Monitor` (else `testing/test-selection` loses the served cases' tracefiles).
- `Point: the bundle's TIMEOUT 3 x limit per chunk + extra — a bundle killed by ctest stores what it stored before; its case tests run alone — L1 Unlikely / I1 Low / E2 Reasoned → Monitor`.
- `Point: the estimate COST of a bundle = the sum of its members' COSTs — ctest starts a dependee first anyway (measured) — L1 Unlikely / I1 Low / E3 Proven (toy project, cmake 4.5.1) → Monitor`.
- `Point: bundles only within one registration (bug_orig_, regression_, blitz2_ apart) — L1 Unlikely / I1 Low / E2 Reasoned → Monitor`.

## Questions

(none: G1 is answered, "K1, K2 follow-up, G1", and the spike chose (b))

## Risks / Trade-offs

- `Point: a selection by list (testinfo select, --tests-from-file, --rerun-failed) names case tests only — they run alone, correct but without the gain — L3 Likely / I1 Low / E2 Reasoned → Monitor`.
- `Point: a false pass from leftovers in a shared session — L1 Unlikely / I3 Observable / E2 Reasoned → Plan` (decision 5 and the switch; `basic-test-bundle-runner`, "Risks").

## Order

After `basic-test-bundle-key`; parallel with `basic-test-bundle-runner`
(tasks 1-3 need only the interface; task 4 needs the runner merged) and
`basic-test-bundle-costs`.

## Findings (implementation, tasks 1-3)

- The tree's registration with bundles (cmake 4.5.1, 2026-11-23): 41
  bundle tests (`bug_orig_` 37, of them one with the label bug-orig,
  `regression_` 3, `blitz2_` 1) with 580 members (553, 25, 2): the 580
  cases `tj_bundle_case` admits in the spike of `basic-test-bundle-key`.
  `ctest -N`: 3241 tests against 3200. With `-DBASIC_TEST_BUNDLE=OFF` the
  three include files and `ctest --show-only=json-v1` are byte-identical
  to those of main (no comment line differs).
- testreg has `--no-bundle` only (bundles are its default); the proposal's
  `--bundle` would be a second spelling of the default.
- The samples of `testreg_tool` before the bundle section have a port cell
  of a plain key each, so they would be bundled; their runs pass
  `--no-bundle` and keep their expectations, the bundles have a sample of
  their own at the end.
- `cmake/emulation-owner-test.cmake` is a coverage configuration
  (`emulation_coverage()`), whose sample cases would be bundled; it sets
  `BLITZ2_COVERAGE` and checks that no bundle test is registered (the test
  of decision 4's coverage rule).
- A bundle's COST when no member has one (none configured, none measured,
  no open bug): none, as for its members.
