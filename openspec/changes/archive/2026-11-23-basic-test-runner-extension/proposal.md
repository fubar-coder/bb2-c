# Proposal

## Why

`corpusrun case` runs one case of a test of the BASIC test model per
call, and the cells of a side that share the session settings always
share one emulator session. Running a source by hand takes one call per
case, and the pilot of `bug-orig-basic-tests`
(`~/.local/share/bb2-c-work/tr-pilot-report.md`) showed three gaps of
the model before the rounds U1-U7: an original that wrecks the memory of
the session crashes the next cell (`shapeslib-loadshape-plane-64k`
writes 66560 bytes behind a block of 1032), an original whose end varies
with the memory layout (the 31 `.orig` files with `# varies:`, among
them #82 and #689) cannot be declared, and the query `inject` is never
filled. The user's requests of 2026-11-23
(`~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`, "corpusrun
case: all cases of a file; the driver of all cases on stdout": "ja,
beides einplanen"; "Pilot: six tests; runner extension before the
rounds": "Erst Runner-Erweiterung planen (Recommended)"; "Driver control
data: D5 stays": "Bei D5 bleiben").

## What Changes

- **All cases of a source in one run**: `corpusrun case` without
  `--case` runs every case of `ID.test.bas` on one side, from the driver
  of all cases (the one the build step writes with `--write-driver`), in
  as few sessions as the cells allow, and judges each case on its own. A
  case whose configuration asserts something of the whole program, or
  that announces a runtime error, runs on its own; what a failing case
  does to the cases behind it is handled by running cases again on their
  own, and a case that does not pass in a shared run is confirmed alone. A line per case, a summary line, and the exit
  status of the worst verdict.
- **`--print-driver` without `--case`** prints the driver of all cases
  (the same text `--write-driver` writes as `.bas`) to stdout.
- **A cell in a session of its own**: the setting `isolate` gives every
  cell (`isolate = "on"`) or the cells its selectors match (`isolate =
  ["libs=cd"]`) an emulator session of its own, so its memory and `RAM:`
  are its own; in the run of all cases each case runs on its own in those
  cells.
- **A cell whose end varies**: the test's own file declares alternative
  ends of the cells with the original's faulty part in
  `[ends."SELECTOR"]`, each a named set of assertions, with a required
  `reason`; such a cell passes that point if one alternative holds,
  so an open bug stays REPRODUCED and a fixed one PASSED.
- **The query `inject` is filled** from inject's report (the lines
  `inject: ...` of the program's output when it ran through inject); the
  printed output keeps them.
- **The announced runtime error as a check**: a named `!ExpectError` is
  the query `check.NS.NAME` (`ok`, `fail`, `not-run`), so a deviation
  declares the original's error (`== fail` with its `requester`) while
  `!ExpectError` stays in the test for the port (design.md D8; added
  2026-11-23 after round U1).
- Control data stay compiled into the driver (D5 of `basic-testlib`,
  "Bei D5 bleiben"): no parameter interface of the driver.

Not in scope: several `.test.bas` files in one session (the user: later;
the batching non-goal of the archived change `basic-test-runner`, with
its proposed `CONTAMINATED` verdict and restoring `RAM:` between
programs); the ctest registration (it stays one test per case and side);
moving tests (`bug-orig-basic-tests`); the requester's position against
the result file (reqtrap would have to mark it; open in
`basic-test-runner`, "Findings of phase 2").

## The user's answers

Recorded in `~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`,
"Runner extension: three answers" (2026-11-23):

- **A failing case in the run of all cases**: "Allein wiederholen
  (Recommended)" (design.md D3).
- **The own session**: "Können wir bei "isolate" nicht 1+2 gleichzeitig
  machen? Also "on" für dein "einfachen" Fall und Zell-Selektoren falls
  man mehr braucht?" — `isolate = "on"` or a list of cell selectors
  (design.md D5).
- **A varying end**: "Benannte Alternativen (Recommended)" (design.md D6).

Recorded in the same file, "!ExpectError stays for the port; a deviation
declares the original's error" (2026-11-23): "Feature into the extension
(Recommended)" (design.md D8, section 6 of tasks.md).

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `testing/basic-test-runner`: the run of all cases of a source and its
  report and exit status; the driver of all cases printed; a cell in a
  session of its own (not an axis); alternative ends of a cell; what the
  query `inject` holds.

## Impact

- `tests/corpus/runner` (`caserun.c`, `caserun.h`, `cmd_case.cpp`,
  `test_caserun.c`, `README.md`), `tools/lib/testconf` (the key
  `isolate`, the table `ends`, their checks), `tools/lib/testjudge` (a
  result file with several selected cases, the alternative ends; host
  test `testjudge`), `tools/lib/testreg` / `tools/testreg` if the time
  limit needs a margin per isolated session.
- `tests/blitz2/selftest/` (sources of the emulator checks) and
  `tests/blitz2/CMakeLists.txt` (their ctest tests).
- `tests/README.md` (also the section "Moving a bug-orig test to the
  model" of `bug-orig-basic-tests` task 2.2, once merged), the site page
  `site/content/docs/project-structure/running-tests.md`.
- No result of the compilers or libraries changes; no registered test
  changes its name.
