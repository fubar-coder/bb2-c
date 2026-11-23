# Proposal

## Why

A case test served from its bundle (change `basic-test-bundle-runner`)
starts no session and takes milliseconds; its bundle runs the sessions of up
to 16 cases. The measured COST (`tests/data/test-costs.txt`, `testinfo
costs`) must neither take a served run as the case's cost nor lose the
case's stand-alone COST, which orders it when it runs alone.

## Decisions of the user

`rounds/tb/decisions.md`, "Q3-Q9: the S0 questions as recommended" ("Q3-Q9:
Wie empfohlen"), Rule: "Q7 served cases keep their stand-alone COST, served
runs excluded from the refresh, the bundle has its own entry".

## What Changes

- **Served runs are no runs** for `testinfo costs` and the durations of
  `testinfo stats`: a JUnit testcase whose output holds an `@served` line
  (interface I7 of `basic-test-bundle-key`) is left out.
- **A test whose runs were all served keeps its entry** in a full refresh:
  `testinfo costs --previous FILE` takes its line from FILE (with
  `--update` the old file is FILE).
- **The bundle has its own entry** from its `@stat` lines (I8), as any
  test; no new rule, a test of it.
- `testinfo stats` lists served runs as such (count per test).
- No result changes (rule 3).

## Not in scope

- The runner, the registration (the bundle's estimate COST is
  `basic-test-bundle-registration`, design decision 2), the `@stat` format.

## Open questions

(none: Q7 settles the rule; the mechanism is design decision 2)

## Capabilities

### New Capabilities

(none)

### Modified Capabilities

- `testing/basic-test-runner`: a requirement added beside "A measured COST"
  (served runs; tests with only served runs); that requirement stays.

## Impact

- `tools/lib/teststats/teststats.[ch]`, `junit.c` (the `@served` line),
  `tools/testinfo/cmd_costs.cpp`, `cmd_stats.cpp`, `tools/testinfo/README.md`;
  host tests in `tools/tests/`.
- `tests/README.md` ("The measured COST": the refresh command with
  `--previous`).
- Overlaps: `tests/README.md` also by `basic-test-bundle-runner`,
  `basic-test-bundle-registration`, `coverage-ted-libs`.
