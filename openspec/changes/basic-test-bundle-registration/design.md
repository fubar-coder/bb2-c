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
   members. (spike) With one volume per bundle (G1 (b)) a chunk of one
   member is a bundle like any other; if the spike falls back to padding
   (G1 (a)), a group or last chunk of one member is not bundled and its case
   test runs alone; the coordinator brings the result into this design (and
   the scenario "Seventeen plain cases" of the spec delta). The source must
   be read once for the key; testreg already
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

(none: G1 is answered, "K1, K2 follow-up, G1"; its form follows the spike)

## Risks / Trade-offs

- `Point: a selection by list (testinfo select, --tests-from-file, --rerun-failed) names case tests only — they run alone, correct but without the gain — L3 Likely / I1 Low / E2 Reasoned → Monitor`.
- `Point: a false pass from leftovers in a shared session — L1 Unlikely / I3 Observable / E2 Reasoned → Plan` (decision 5 and the switch; `basic-test-bundle-runner`, "Risks").

## Order

After `basic-test-bundle-key`; parallel with `basic-test-bundle-runner`
(tasks 1-3 need only the interface; task 4 needs the runner merged) and
`basic-test-bundle-costs`.
