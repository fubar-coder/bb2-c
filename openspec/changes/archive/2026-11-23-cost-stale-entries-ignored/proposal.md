## Why

The main spec `testing/basic-test-runner`, requirement "A measured COST",
says an entry of `tests/data/test-costs.txt` that names no test fails the
build. The code does not: `cmake/test-cost.cmake` ignores such an entry
(commits 109f2b15e, ce7043f87). The user decided so in
`rounds/tr/decisions.md`, entry "Q-A1: COST is an optimisation; stale
entries are ignored": "Silently ignore (c1) (Recommended)" and "Keep the
estimate (Recommended)". The COST only orders the tests, so a stale entry
must not stop a build.

## What Changes

- "A measured COST": no longer says that an entry naming no test fails the
  build; its scenario "An entry of a removed test" now says the
  configuration succeeds and every test keeps its COST.
- New requirement "Stale and malformed entries of the measured COSTs": a
  stale entry is ignored without an error or warning; a test without an
  entry keeps its estimate; a malformed line still stops the configuration.

## Impact

Spec text only; `cmake/test-cost.cmake` already behaves so. "A configured
cost wins over a measurement" names no stale check and stays unchanged.
