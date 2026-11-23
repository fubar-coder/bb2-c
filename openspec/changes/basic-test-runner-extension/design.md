# Design

## Context

See proposal.md, "Why". What exists (main 036596658):

- **`corpusrun case`** (`tests/corpus/runner/caserun.c`, `caserun()`):
  one case per call. Without `--case` it already builds the driver of
  all cases (`caserun.c` ~1170, used by `--write-driver`, function
  `write_driver` ~1039), but `cmd_case.cpp` refuses a run or
  `--print-driver` without `--case` ("--case is required").
- **Sessions**: `cr_session_key` (`caserun.c:329`) groups cells by
  `machine`, `rtg`, `timeout`, `hang-after`; a cell with `serial = "on"`
  gets a key of its own (its cell number). All compiles of a session
  come first, then the runs; when a session ends early, the first cell
  that did not finish gets its end and the cells behind it run again in a
  new session (`run_session`, `caserun.c:731`). Each cell has its own
  `CORPUS:` (`CELLk:`); `RAM:` and the memory belong to the session.
- **The library and the interface already handle several cases in one
  program** (`testing/basic-test-interface`, "The order of the driver",
  "Preconditions and failed checks of the batch init": every called case
  gets its `block` entry, a failed init check or precondition is
  repeated per case).
- **The judge** (`tools/lib/testjudge`, `read_run`) judges one case: the
  block of another case in the result file sets `foreign`, which fails
  the cell ("holds the case X, which was not selected"). Observations of
  the whole program (`rc`, requesters, `output`, `serial`, `session`,
  `end.run`, `compile`) are one per cell; a requester has `after =
  (size_t)-1` (reqtrap does not mark the result file's position), so it
  cannot be attributed to a case. The verdict over cells
  (`tj_verdict`): error over broken over failed over reproduced over
  passed.
- **`inject`**: `observe_cell` sets `o.inject = NULL`; inject's report
  lines are part of the program's output (`inject: rc=0` in 185 lines of
  `tests/bug-orig/**/*.expect`/`.orig`, 202 files with an `inject:` line;
  the pilot asserts them on `output`).
- **Varying ends**: 31 `.orig` files carry `# varies:` wildcard lines
  (`tests/README.md`, the bug-orig comparison), uncorrelated per line
  (e.g. `blitzoslib-selectbox-range.orig`: `[session]` `*` and `[run]`
  `...`). The model has `matches` for values only.
- A crash answered by reqtrap leaves a suspended process and the
  requester in the session (`tests/README.md`: "Every test is its own
  emulator session, the suspended process and the requester are gone with
  it").

## Goals / Non-Goals

**Goals:**
- A direct run of all cases of a source with verdicts that match those
  of the registered tests (one case each), at the cost of one shared run
  when everything passes.
- The three gaps of the pilot closed before round U1.

**Non-Goals:**
- ctest using the run of all cases: the ctest unit is decided ("ctest
  unit: Erst pro Case, Bündeln später (Recommended)", 2026-11-21, round
  tr); not asked, not planned.
- Several sources in one session, `CONTAMINATED`, restoring `RAM:`
  between programs (the batching non-goal of `basic-test-runner`).
- Parameters of the driver ("Bei D5 bleiben").
- Attributing a requester to a position of the result file (reqtrap).

## Decisions

**D1 The run of all cases reuses the run of one case.** `caserun()` is
split into "run a set of cases in the cells of the side" (the driver of
the set, the cells, the sessions, the observations; today's code with a
case list instead of one name) and "judge a case from the observations".
The run of one case is the set of one; the run of all cases calls the
set repeatedly (D3). The work directory is `ID` / `ID_originals` (a case
name holds no `__`, so no clash with `ID__CASE`), each run of a set a
subdirectory (`shared-N`, `alone-CASE`); it stays when a case did not
pass, as today.

**D2 Which cases share.** Before the first session the runner sets
apart the cases that run on their own: those whose assertions or
deviations, in any cell of the side and any judging mode, name a query of
the whole program (`rc`, `requester`, `compile`, `compile.log`,
`output`, `serial`, `inject`, `session`, `end.run`) other than by the
defaults, and those with an `!ExpectError` in their own namespace (the
table of the preprocessor). In the cells `isolate` names (D5) every
case runs on its own. Alternatives: judge the whole-program queries on the shared run
(wrong: a case's `output contains` would pass on another case's line; an
announced runtime error ends the program for the cases behind it).

**D3 A clean shared run, else rerun** (user, 2026-11-23: "Allein
wiederholen (Recommended)", `~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`,
"Runner extension: three answers"). A cell's run is clean if its session ended (`done`), the driver
compiled, the exit code is 0, the run's end entry is there, and no
requester was shown. If every cell is clean, each case of the set is
judged by the judge in a mode that accepts the blocks of the other
selected cases (`tj_test` gets the list of the driver's cases; a block of
an unselected case stays `foreign`). If not, the case whose block was
open at the end of an unclean cell runs on its own, the cases of the set
not judged run again as a new set without it; with no open block (a
compile error, a crash in the batch init or end, a requester after a
case closed) every case of the set runs on its own. Each round removes at
least one case, so the loop ends; worst case one shared run per case plus
one run per case. Then every case judged failed, broken or error in a
shared run runs again on its own, and that verdict counts; when it
differs, the report says `case NAME: VERDICT alone (VERDICT2 in the run
shared with ...)`. The shared run is a fast path for the cases that pass;
a failure is always the run of the case alone, which is what its ctest
test shows. Not taken: the shared verdicts standing with only the rest
rerun, stopping with the cases behind reported as not run, a verdict
`CONTAMINATED`.

**D4 Report and exit.** Per case the block of today (`== ID.test.bas,
case NAME, side SIDE: cells`, the messages, `ID.test.bas: case NAME:
VERDICT`), with ` (alone)` when judged on its own; at the end
`ID.test.bas: N cases, side SIDE: VERDICT`. Exit status: the worst
verdict in the order of `tj_verdict` over cells (error, broken, failed,
reproduced, passed), with skipped (a run that could not start after
others were judged, the case then SKIPPED) between failed and
reproduced, so a partial skip never hides a failure and never shows as
a pass. 77 when the side is skipped before any session (as today). The
`corpusrun pass` bug-orig mode orders differently (broken 3 below
differs 4) but aborts with 2 on an error, so error is worst there too.

**D5 A cell in a session of its own** (user, 2026-11-23: "Können wir bei
"isolate" nicht 1+2 gleichzeitig machen? Also "on" für dein "einfachen"
Fall und Zell-Selektoren falls man mehr braucht?", same entry). The run
setting `isolate` takes one of two TOML forms:

```toml
isolate = "on"                                  # every cell its own session
isolate = ["libs=cd", "compiler=orig195"]       # the cells a selector matches
```

A string must be `on` or `off` (`off`, the default, as if the key were
absent); a list holds cell selectors (the grammar of `exclude`, at least
one element, each checked against the axes and settings like an
`exclude` selector). `testconf` gets one value kind more,
`TC_V_ONOFF_OR_SELECTORS` (the key table types each key; the TOML parser
already gives a string or a list and is not changed); the check of the
key is the enum check for a string and the selector check of `exclude`
for a list. Not an axis (refused in `[matrix]`, naming the key). A
nearer file replaces the key whole, in either form (the rule of every
key; a list is never merged). Alternatives: a list only, with `["on"]`
or a selector that matches every cell for the simple case (one value
kind, but "on" as a list element is a special word in a selector list,
and no selector matches every cell of an arbitrary matrix); a string
only, `"on"` or one selector (no union of selectors, so `libs=cd` and
`compiler=orig195` could not both be named). Rating: `Point: the form
of isolate (string on/off or a list of selectors) — L1 Unlikely / I2
Medium / E2 Reasoned → Monitor`: the user named both forms, the open
part is the TOML spelling; if it turns out wrong, this change's
testconf task and docs change.

In `cr_session_key` an isolated cell gets the cell's number in its key,
as a cell with `serial = "on"` does. In the run of all cases (D2, D3)
the cells are split: in an isolated cell every case runs on its own (a
session per case and cell); the other cells of the side run the shared
run with its reruns. A case is judged over all cells of the side from
the observations of its cells (the isolated cells' from its own runs,
the others' from the shared run that judged it); its confirmation alone
(D3) runs it in every cell of the side, as its ctest test does. With
`isolate = "on"` that is every case on its own. Why: the wrecked memory
of an original hits the next case of the same program as it hits the
next cell; the cells that need no isolation keep the fast path.
`Point: isolated cells run their cases alone, the other cells share —
L2 Possible / I2 Medium / E2 Reasoned → Plan`.

The ctest time limit is the sum of the cells' time limits plus the
build's margin; task 2.2 checks that the margin covers one session start
per isolated cell (about 2 s each), else `testreg` adds it.

**D6 Alternative ends** (user, 2026-11-23: "Benannte Alternativen
(Recommended)", same entry). In the test's own file:

```toml
[ends."libs=cd"]
reason = "the original draws a box from the memory behind the table (#82); whether it then returns or hangs depends on that memory"
returns = ["session == done", "rc == 0", "check.range.after == ok"]
hangs = ["session == timeout", "end.run not exists", "check.range.after == not-run"]
```

Each key but `reason` is an alternative, a list of `QUERY PREDICATE`
(the grammar of the assertions, a blank between query and predicate).
The table counts as a deviation: it applies in the cells where the
deviations apply (`TC_DEVIATED`, and for an open bug `TC_DEVIATED_ANY`
in the other cells), and counts as a declared deviation for
`require_deviation` and for `fixed-orig`/`extension`; `fixed` refuses
it, a `tests.toml` refuses it. In such a cell the queries any alternative
names leave the other assertions and the defaults; the cell holds that
point if one alternative holds entirely. Correlated alternatives are
stricter than the old wildcard lines (which accepted any session and any
output). Not taken: alternatives per query in the predicate, a key that
leaves the end unjudged, no support. Queries allowed in an
alternative: `session`, `rc`, `requester`, `end.case`, `end.run`,
`output`, `check.*`; `compile`, `serial`, `inject` are refused (no
recorded case varies there).

**D7 The query `inject`** (step 1: the spec `testing/basic-test-runner`,
"Assertions judge what the program cannot state", requires the queries to
cover "the output of `inject`", so the query is filled, not dropped).
When the cell's program ran through inject (`pad = "on"`, or `runwrap`
with `@INJECT@`), `inject` is the lines of the output starting with
`inject:`; otherwise not captured, as today. The output keeps the lines
(copy, not move), so the pilot's `output` assertions stay valid.

## Risks / Trade-offs

- [A case that passes in the shared run but would fail alone (it depends
  on the memory, `CORPUS:` files or globals another case left)] → its
  verdict differs from ctest's; the rule that tests must not depend on
  the memory state (`basic-test-runner`, Risks) and the pitfall "cases
  share the globals" of the pilot's instructions; the run of all cases is
  a convenience, ctest decides.
- [Every failing case costs a second run] → only on failure; a failed
  batch init makes every case run alone (as ctest).
- [`isolate` adds a session per isolated cell, and in the run of all
  cases a session per case and isolated cell] → only where set; the
  selector form keeps the other cells together.
- [Alternative ends weaken a test if an alternative is too wide] → a
  reason is required, alternatives are reviewed with the move; the
  pilot's rule "report the test, do not weaken it" stays in the
  instructions for an end that no alternative set describes.
- [Overlap with `bug-orig-basic-tests`] → both edit `tests/README.md`;
  the pilot's section ("Moving a bug-orig test to the model", task 2.2,
  on branch `worktree-agent-ac56a24532c6cee36`) says that `inject` is
  not filled, that an own session per cell is a request to the runner and
  that a varying end cannot be declared; task 5.2 rewrites those three
  points after the pilot is merged.

## Migration Plan

None: no test changes form or name. The pilot tests keep working
(`output` keeps inject's lines); `plane-64k` may add its cell `port,cd`
back with `isolate = ["libs=cd"]` in a round of `bug-orig-basic-tests`, not
here.

## Findings

- **`End` in a case leaves the run's end entry (task 1.5).** The test
  library's finit writes `incomplete` and then the run's `end` entry when
  the program ends inside a block (`basic-testlib`, "The end of a case and
  of the run are marked"), so a shared run whose case `b` executes `End`
  has its end entry, and D3's list (session done, compiled, exit code 0,
  end entry, no requester) would call it clean: `b` would be judged from
  it (and pass with `"end.case" = "not exists"`), and every case behind it
  would fail as not started. The scenario "A case ends the program" of the
  delta spec needs it unclean, so `cr_cell_clean` also requires that the
  entry before the run's end entry is no `incomplete` (the program did not
  end inside a block). The requirement "A case open at an unclean end runs
  again" lists conditions that make a run unclean; this one is added, no
  listed one is dropped, so the spec text stands. The open case is the one
  whose block the `incomplete` closes (`cr_open_case`); a block of the
  batch init or cleanup open at the end is no case, and every case of the
  set runs on its own.
- **A case that asserts nothing of the whole program but is set apart in
  every judging mode (task 1.4).** D2 counts the assertions and deviations
  "in any cell of the side and any judging mode", so the deviation of a
  fixed bug for the other side's cells (`libs=cd`) also sets the case apart
  on the side `port`, where it is never judged with the deviations
  (`TC_DEVIATED_ANY` sees it with its axis terms ignored). Taken as
  written: the case only runs on its own more often than needed.
- **The tags of the report.** In the run of all cases the cell list of a
  case names the run each cell was judged from (`port,cd (shared-0/c0)`,
  `(alone-NAME/c0)`), the directory below the run's work directory that
  holds that cell's `CORPUS:`; the run of one case keeps `port,cd (c0)`.
- **The time limit with isolated cells (task 2.2).** Measured on this
  machine (2026-11-23, the build's snapshots present): a session of
  `selftest/model-isolate.test.bas` (compile and run of one small case)
  takes 0.9-1.0 s from a restored snapshot and 3.1 s for the first one
  that saves it (`@stat session=`); the two isolated sessions of its ctest
  test took 1.9-2.0 s of wall time in all, so the runner adds about 0.05 s
  besides the sessions. The registered test's limit is the sum of the
  cells' time limits (`timeout`, default 90 s) plus the margin
  `--timeout-extra` (60 s plus `EMULATION_QUEUE_WAIT`): an isolated cell
  has its own session, but its time limit is already one term of the sum,
  and a session start of 1-3 s per isolated cell stays far below the 60 s
  of the margin (twenty cells would use it up at the cold start). `testreg`
  is not changed.
- **The run of all cases with isolated cells (task 2.3).** On the side
  `originals` of `selftest/model-all-cases.test.bas` with
  `isolate = ["libs=cd"]` (cells `port,cd`, `orig195,cd` isolated,
  `orig195,clib` shared) the run of the six cases took 22 sessions: two
  shared runs in `orig195,clib`, a run of each of the four cases the second
  one judged clean in the two isolated cells (8), and four runs on their
  own in the three cells (12).
- **Alternative ends (tasks 3.1-3.3).** The table's own checks need its
  header: a missing `reason` and a table without an alternative are
  reported at the line of `[ends."SELECTOR"]`, the other refusals at the
  line of the key. Two tables whose selectors both match a cell are judged
  each on its own (each must have an alternative that holds), and a query
  any of them names leaves the assertions; the more specific selector does
  not replace the other, as no recorded case needs two. An alternative
  replaces the defaults only for the queries it names, so an alternative
  for a hang names `rc not exists` besides `session == timeout` and the
  missing end entries (the default `rc == 0` would fail otherwise); the
  report names the alternative that held also for a cell that passed (a
  line under the cell's heading). The emulator check
  (`selftest/model-varying-end.test.bas`) has the form of an open bug, as
  a selftest has no issue for `fixed-orig`: both cells are judged with the
  alternatives and the case comes out REPRODUCED, the ctest test checks
  the report; `model-varying-end-one` (the same source, only `returns`)
  is its check with broken input and fails in the cell that hangs.
- **The query `inject` (task 4.1).** A cell runs through inject when
  `pad = "on"` or the path field of its `runwrap` is the placeholder
  `INJECT` of the environment file (or the runner's value `inject`, the
  same program); `cr_cell_inject` decides it from the configuration, not
  from the text `@INJECT@`, which the placeholder resolution has replaced.
  Checked on the pilot `shapeslib-loadshape-width-zero` with
  `inject = "contains \"inject: rc=0\""` in `[asserts]` and the guard line
  as an `inject` deviation of `libs=cd` added for the run (not committed):
  both sides PASSED; without the `runwrap` the port cell FAILED with
  `assertion inject contains "inject: rc=0" (...) does not hold: not
  captured` (log `~/.local/share/bb2-c-work/accept-runner-ext/s4-inject.log`).

## Addendum: the announced runtime error as a check (section 6)

**D8 `!ExpectError` stays for the port; a deviation declares the
original's error** (user, 2026-11-23: "Feature into the extension
(Recommended)", `~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`,
"!ExpectError stays for the port; a deviation declares the original's
error"; the user before it: "dass der Testfall mit dem Original dort
fehlschlägt ist ja eigentlich gewollt ... Wenn aber die Original-Fehler auch
validiert und als Erfolgreich geprüft werden sollen, dann scheint der
Workaround die sinnvollere Alternative zu sein - aber nur solange wie
`!ExpectError` für den Port weiter im Testfall stehen kann/darf."). The
movers of round U1 found that an announcement is judged in every cell: a
deviation could not change it, and a test whose original shows another
error (or none) had to drop `!ExpectError` and spell the error out
(`requester contains`, `end.case not exists`, the check behind it
`== not-run`).

The announcement becomes a check of the judge. `!ExpectError{"MESSAGE",
"NAME"}` already takes an optional name (`basic-test-interface`, "Check
macros of a test source"; `testpp` keeps it in the table, the driver has
neither message nor name), so no form, no driver and no library change:
the query is `check.NS.NAME` as for every other check, with the verdict
words of the checks: `ok` (the matching requester came after the
announcement), `fail` (it did not come), `not-run` (no entry). An
announcement that no assertion or deviation names is judged as today (the
default "every check with entries passed", reported as "the announced
runtime error ... did not come"); one that a deviation names, e.g.
`"check.CASE.NAME" = "== fail"` with `requester = "contains \"Can't Read
File\""`, is judged by it, and the requester that matched no announcement
counts for `requester` as before. `.expected`/`.actual` of an announcement
are refused when judged (it has no values). Not taken: a query by number
(`check.NS.#N`; no other check is named that way), a verdict word `failed`
(the checks say `fail`).
- **The announcement as a check (section 6).** The judge counts an
  `expect-error` entry as an entry of its check: matched by a requester
  after it, it passes; not matched, it is the check's failure, reported
  by the default of the checks as before ("the announced runtime error ...
  did not come"), now after the assertions of the cell instead of before
  them. The emulator check is the bug of #1052 (LoadShape of an ILBM with
  0 planes): `!ExpectError{"Illegal Shape Depth","refused"}`, the CD's
  libraries stop with "Can't Read File", declared with
  `"check.depth_zero.refused" = "== fail"`, `requester`, `"end.case" = "not
  exists"` and `rc = "exists"`; both sides PASSED, without the deviation
  the side originals FAILED naming the announcement
  (`selftest/model-expect-error*.test.bas`).

## Addendum: tests of open bugs in the order of the bug-orig tests (section 7)

**D9 testreg declares the bug-orig tests** (user, 2026-11-23: "testreg
into the order (Recommended)",
`~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`, "Model tests
of open bugs join the bug-orig order"). The movers of round U1 found that a
model test of an open bug (label `bug-orig`, REPRODUCED on purpose) had no
`COST` and was declared before the old-form tests (the ctest include file
of the registration comes first in `CTestTestfile.cmake`), so
`bug_orig_order` failed. Only the build step knows the model tests, so it
declares the old-form ones too: `bug_orig_declare_tests(FILE)` writes the
recorded tests, sorted, as a ctest include of its own with a line `# @test
NAME COST` before each declaration (at the end of the top directory by
`cmake_language(DEFER)`, after `emulation_snapshot_owner` and
`emulation_coverage`, whose environment `emulation_test_environment` gives
them as before), and `basic_test_register(... ORDER FILE)` hands it to
`testreg --order FILE`, which writes the tests of the label `bug-orig`
last in its include, its own and the entries in one list, the largest
`COST` first, equal ones by name. A model test of the label `bug-orig`
without `cost` gets the sum of its cells' session limits (the time it can
take; the old form's "session limit times sessions" with a session per
cell, as the test's `TIMEOUT` counts it). Without model sources the order
file is the declaration itself. `bug_orig_order` stays as strict;
`bug_orig_order_model_late` and `_nocost` are its checks with a model
test out of order and one without a `COST`. Not taken: leaving the model
tests out of the check (the user's other option).
- **Verified** without moving the deferred tests of slice A
  (`shapeslib-anim-delta-bounds`, `polylib-word-range`; their move is the
  follow-up agent's): `testreg_tool` (an open bug of COST 60 between
  entries of 240, 60 and 10; broken order files), and a probe
  `tests/bug-orig/libs/order-probe.test.bas` of an open bug, not committed:
  registered with COST 90 among the 22 old-form tests, `bug_orig_order`
  passed with 23 tests (`~/.local/share/bb2-c-work/accept-runner-ext/order-probe.log`).
- **A gap found on the way:** the tests the registration declares (all
  model tests) get no `AMIGA_EMULATION_OWNER` and no coverage directory:
  `emulation_snapshot_owner` sets them on the tests CMake declared, and
  the include files are not CMake's. The old-form bug-orig tests keep them
  (written into the order file); the model tests are as before. Reported,
  not changed here.

## Addendum: the measured COST (section 8)

**D10 A COST measured from a test run** (user, 2026-11-23: "Am besten
gleich mit berücksichtigen, dann haben wir das für neue Testfälle",
`~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`, "Measured
COST in the runner extension"). `testinfo costs [--update FILE]
JUNIT.xml...` (`tools/lib/teststats` `teststats_costs`, the file format
`testcosts.h`) reads the `@stat` lines of ctest's JUnit files and writes
`tests/data/test-costs.txt`, a line `NAME COST` per test, sorted by name.
The COST of a run of a test is the sum of its sessions' `session` times
(the slot wait is not in them); the COST is the median over the runs,
rounded up, at least 1. The design choices:
- **Cold sessions.** A session that did not start warm (`cold`,
  `first-save`, `fallback`: once per emulator slot and ctest run) counts
  as the median of the test's warm (`restored`) sessions over all runs, so
  a test that happened to take a slot first does not look dear; a test
  that had no warm session in the runs gets no entry (it keeps its
  estimate) rather than a COST from a boot.
- **Measurement or `cost` key.** A model test's explicit `cost` key wins
  over its measurement (user, 2026-11-23: "Key wins (Recommended)",
  `rounds/tr/decisions.md`, "COST: an explicit key wins over a
  measurement"); a measurement replaces every estimate: the sum of a
  model test's cells' session limits, an old-form bug-orig test's session
  limit times sessions, and the list of long tests in
  `cmake/test-cost.cmake`; the estimates stay for the tests without an
  entry. Why: the estimates were guesses for lack of a measurement (the
  list itself is "the rounded durations of one cost file"); a key is
  written on purpose. The list stays as the fallback until the file covers those
  tests; then it can go (not here: the order of the long tests must stay
  sensible, and the file of this change holds only what this agent ran).
- **Where it is read.** CMake reads the file at the start of the top
  directory (`test_costs_load`, a configure dependency):
  `long_test_costs()` gives every CMake test with an entry its COST, the
  writer of the bug-orig order file the old-form bug-orig tests; testreg
  (`--costs`, a dependency of its step) the tests of the model, also in
  the sorted declaration of the label `bug-orig`.
- **The check.** An entry whose test does not exist fails the build:
  `long_test_costs()` stops the configuration for an entry that is no
  CMake test, no recorded bug-orig test and no `PREFIX...__CASE` of a
  registered prefix; testreg fails its step for a `PREFIX...__CASE` entry
  of its prefix that it did not register (each test with broken input:
  `test_cost_broken`, `testreg_tool`). The two parts are needed because
  CMake does not know the tests of the model.
- **Mixed scales.** Until the file is refreshed from a full run, measured
  tests (seconds) and estimated ones (limits) are ordered together, so
  the estimated ones come first; the coordinator refreshes the file after
  the acceptance run (`tests/README.md`, "The measured COST"). The file of
  this change holds the 250 tests of the run of `tests/blitz2` and the
  model tests of `tests/bug-orig`
  (`~/.local/share/bb2-c-work/accept-runner-ext/junit-s8.xml`).
