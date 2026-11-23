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
