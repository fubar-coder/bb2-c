# Proposal

## Why

The runner's bundle mode (change `basic-test-bundle-runner`) runs only when
a ctest test starts it. This change lets the registration step (testreg)
derive the bundles from the session key and declare them with ctest form b3:
a bundle test per chunk, the members' case tests DEPENDS on it as ordering
and grouping only, each case test served from the store or run alone
("O1-O3 of the handoff answered", O2: "OK"; `rounds/tb/decisions.md`, "O2
start", Rule: "Bundles are a grouping derived by testreg from the session
key (no dependencies in the toml)", recorded there as planning input).

## Decisions of the user

`rounds/tb/decisions.md`, "Q3-Q9: the S0 questions as recommended" ("Q3-Q9:
Wie empfohlen"): Q4 chunks of 16 cells; Q5 the bundle test passes when it ran
through, verdicts only from the case tests; Q6 the first set. "Q10-Q13:
bundling details as recommended" ("Q10-Q13: Wie empfohlen"): Q10 one cell per
case; Q11 chunks of exactly 16 cells, the remainder padded. "K1, K2
follow-up, G1": "Zu G1: Ok, wie empfohlen falls möglich (b) ..." — one
volume per bundle, which the spike of `basic-test-bundle-key` showed works:
no padding, and a bundle of one case is no special case.

## What Changes

- **testreg groups** the cases that `tj_bundle_case` admits by registration
  (one `basic_test_register` call), session key, label set and fixture set,
  in the order of the sources and cases, and cuts each group into chunks of
  16 members.
- **A bundle test per chunk**: `PREFIX + bundle_N`, the command of I1
  (`basic-test-bundle-key`), the group's labels and fixtures, TIMEOUT per
  design decision 4 of `basic-test-bundle-runner`, a COST, the test
  environment (`basic_test_environment`).
- **The member case tests**: their command gains `--test TEST
  --from-bundle NAME` (I2), their properties `DEPENDS NAME`; names, labels,
  TIMEOUT and COST stay.
- **The switch** `BASIC_TEST_BUNDLE` (CMake cache, ON): OFF registers no
  bundle (every case test runs alone as today); off in a coverage build.
- No result changes (rule 3). The set of ctest tests grows by the bundle
  tests; `ctest -R CASE` still runs a case alone.

## Not in scope

- Cells with `[data]` and runs from the editor (K2: a follow-up change).
- The runner (`basic-test-bundle-runner`), COST refresh rules
  (`basic-test-bundle-costs`), the originals side, other keys, multi-cell
  sources, `corpus_exprmatrix`, bundles across registrations.

## Open questions

(none)

## Capabilities

### New Capabilities

- `testing/basic-test-bundles` (also created by `basic-test-bundle-runner`;
  whichever is archived first creates it): the registration of bundles.

### Modified Capabilities

- `testing/basic-test-runner`: "Registration at build time without the
  emulator" (the bundle tests besides the tests per case and side).

## Impact

- `tools/lib/testreg/testreg.[ch]` (groups, chunks, bundle tests,
  `write_test` with DEPENDS and the two options), `tools/testreg/main.cpp`
  (`--bundle`/`--no-bundle`), `tools/testreg/tooltest.cmake`,
  `tools/testreg/README.md`; host tests in `tools/tests/`.
- `cmake/basic-test-model.cmake` (the switch, the option to testreg).
- `tests/README.md` (the bundle tests, selection, the switch), `TODO.md`.
- Overlaps: `basic-test-bundle-key` edits `cmake/basic-test-model.cmake`
  before (order); `tests/README.md` also by `basic-test-bundle-runner`,
  `basic-test-bundle-costs`, `coverage-ted-libs`; `TODO.md` also by
  `coverage-ted-libs`.
