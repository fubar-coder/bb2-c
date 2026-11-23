# Design

## Context

- `teststats_costs` (`tools/lib/teststats/teststats.h:88`): per JUnit
  testcase the runs with sessions; the COST is the median over the runs that
  have sessions; a test without a warm session gets no line. A served run
  has no `@stat` line, so it already drops out of the median; but a test
  whose runs were all served gets no line, and a full refresh (without
  `--update`, `tests/README.md` "The measured COST") then drops its entry:
  the case would go back to its estimate (the sum of its cells' session
  limits, e.g. 90 instead of 2).
- `testinfo stats` gives a suggested COST from the median duration of a
  test's runs, served runs included.
- The `@served` line and the bundle's `@stat` naming: `basic-test-bundle-key`,
  design.md, I7 and I8.

## Goals / Non-Goals

**Goals:** Q7 as recorded; the bundle's entry needs nothing new.

**Non-Goals:** a change of `@stat` (`statline.h`) or of the estimates.

## Decisions

1. **`@served`** is parsed (`teststats.c`, a line `@served v=1 test=T
   bundle=B`; a malformed one is reported on stderr as a bad `@stat` line
   is, exit 1, the rest counts). A testcase run with it is a served run.
2. **`--previous FILE`** for `testinfo costs`: a test whose every run in
   the JUnit files was served keeps the line of FILE; with `--update FILE`
   FILE is the previous file. Without it such a test gets no line (as today
   for a test without warm sessions) and stderr names how many were left out.
3. **`testinfo stats`**: served runs are counted per test and left out of
   its durations and its suggested COST.

### Planning choices (mine)

- `Point: --previous as an option, not read by default in a full refresh — a refresh without it drops the served cases' entries (their estimate orders them again) — L2 Possible / I1 Low / E2 Reasoned → Monitor` (the README's refresh command names it).

## Order

After `basic-test-bundle-key` (only I7/I8 are needed); parallel with the
runner and the registration.

## Findings (implementation)

- The `@served` line is parsed in `teststats.c` (`served_parse`), with the
  rules of the `@stat` reader: fields in any order, an unknown key ignored;
  refused (reported, the run counts as not served): a key twice, one of `v`,
  `test`, `bundle` missing, `v` other than 1, an empty value, a character
  outside `[A-Za-z0-9._,:/+-]`, an empty field (two blanks). A line that
  only starts with `@served` (`@servedx`) is no `@served` line.
- `test=` of the line is not compared with the testcase's name: the run
  that holds the line is the served one.
- A served run's `@stat` lines (none by I7) would not count either: the
  measured COST leaves out every session of a served run.
- `teststats_costs_previous` (lib_teststats now links lib_testcosts) takes
  the previous entries; `teststats_costs` is it without them, so its output
  on JUnit files without `@served` is unchanged (checked against the
  `testinfo` of main on two acceptance JUnit files: stdout, stderr and an
  `--update` of `tests/data/test-costs.txt` byte-identical; also with
  `--previous`).
- `--previous` and `--update` exclude each other (CLI11 `excludes`, usage
  error, exit 2): with `--update` its FILE is the previous file.
- `testinfo stats`: a column `served` in the section `tests` (also in the
  TSV, after `skipped`); the durations and the suggested COST leave served
  runs out; the section titles say so. A test with only served runs shows
  `-` and gets no COST row.
