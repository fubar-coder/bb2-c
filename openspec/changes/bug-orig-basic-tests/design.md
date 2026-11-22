# Design

## Context

See proposal.md, "Why". Measured on 2026-11-21 in `tests/bug-orig`:

- 1007 `.bas`, 1015 `.expect`, 1017 `.orig`, 57 `.drv` (TED).
- 68 of 1003 `.expect` have no `[run]` section (62 in `compiler/`).
- Run section of `.orig` against `.expect` (962 pairs): 42 equal (the bug
  is in the compile or log sections), 542 differ in one line, 214 in two,
  120 in three to eight, four in more than 30.
- 29 `.orig` with `# varies`; 129 recorded originals end in a timeout or a
  halt. 132 `.expect` with wildcard lines, mostly the ends of requester
  texts and `rc=*`.
- 121 `.expect` hold a `REQUESTER:` line (a runtime error is the correct
  result), 21 an exit code other than 0.
- 506 RWE tests use the CD's blitz2 2.10 as the original, the others the
  build of `blitz2.s` 1.95.
- 9 tests start through `wbrun`, 6 run from the editor (`jobline=run=`).
- 24 sources use Blitz mode, 28 `Forbid`/`Disable`; 53 have more than one
  `End`.
- Many header comments already describe setup, act and expectation
  ("Here: ... Correct: ...").
- Spike 2 moved #265 and #263: the cells with the CD's libraries (`ref`
  and `port`) fail exactly the expected checks with the recorded
  original's values; `clib` passes.

## Goals / Non-Goals

**Goals:**
- Every Blitz bug-orig test whose program runs in the new model, without
  a recording.
- The verdict of every test after the move equals its verdict before.

**Non-Goals:**
- The TED bug-orig tests, `tests/bug-port`, the compile-only tests.
- New tests or new fixes in the same commits.

## Decisions

**D1 Rounds by area, as the fix rounds** (user: "OK so", plan in
`~/.local/share/bb2-c-work/decisions/rounds/tr/plan.md`): P pilot (5), U1 libraries
graphics (~150), U2 libraries system/IO/text (~135), U3 libraries sound and
compute (~45), U4 compiler with a program run, regression, selftest (~100),
U5-U7 RWE user libraries in three groups (~170 each). A tester agent per group moves its tests in a
worktree; the coordinator runs `all`, `bug-orig` and `originals` once per
round after the merge (CLAUDE.md rule 5).

**D2 Per test**: the header's "Here/Correct" text becomes the parts
(batch init for shared data, a case per behaviour, preconditions for the
setup); every printed observation that `.expect` judged becomes a check
with the expected value of `.expect`; the lines in which `.orig` differs
become declared deviations for the cells with the faulty part (a library
bug: the CD's libraries; a compiler bug: the original compiler); the
header directives become the test's configuration, and what is the same
for a directory goes to its `tests.toml`. Top-level `Dim`, allocations
and `Gosub` helpers move into the batch init, a case or a procedure (cases
are subroutines, `basic-testlib` D3); the 15 tests started from the
Workbench or the editor move like the others, since the control data are
compiled into the driver. The test runs on both sides with
the verdict it had; then `.bas`, `.expect` and `.orig` give way to
`ID.test.bas` and `ID.test.toml` in the same commit.

**D3 Runtime errors**: the case announces the expected error before the
statement that raises it; the requester is matched by the runner (user:
"Ich würde auch (i) empfehlen").

**D4 Crash tests**: the deviation is an assertion (missing end entry of the
case or the run, the way the session ends); checks behind the crash count
as not run. What varies today with `# varies` is left out of the
assertion or matched by a predicate.

**D5 Fixed marker** (user: "fixed-orig / fixed"): `fixed-orig = N` for an
original bug, `fixed = N` for any other issue, mutually exclusive, checked
against the declared deviations. The wording (user: "OK so", 2026-11-21),
written when this change is implemented:

- CLAUDE.md, rule 4: "one fix commit with the code change, the test of the
  expected behaviour (FIXED; in a test of the BASIC test model the key
  `fixed-orig = N` in its `ID.test.toml`, in a test of the old form the
  header `; @fixed #N`)"; at the end of rule 4: "A fix of a bug that is
  not in the original sets `fixed = N` instead."
- `.claude/agents/tester.md`: "once the bug is fixed it must come out
  FIXED in `ctest --preset all`: a test of the BASIC test model
  (`ID.test.bas`) gets `fixed-orig = N` in its `ID.test.toml` and nothing
  else of it changes (its checks and declared deviations stay); a test of
  the old form gets the header `; @fixed #N` and keeps its `ID.orig` (...).
  A test of a bug that is not in the original uses `fixed = N`. In the old
  form `ID.orig` is written by the target `bug-orig-record` ..."
- `docs/development.md`, "Fixing an original bug", step 2: "the bug-orig
  test gets `fixed-orig = NNN` in its `ID.test.toml` (a test of the old
  form: the header `; @fixed #NNN`; another suite: its equivalent)".

At the end of the change the tests left in the old form (TED tests,
compile-only tests) get `; @fixed-orig #N` (user: "Ja, zum Abschluss"),
and the texts above lose the old-form clause accordingly; that wording is
shown to the user again before it is written.

**D6 A pilot round**: the five tests of round R12's I4 (`shapeslib-*`),
whose form is already setup / act / verify; spike 2 moved two of them.

## Risks / Trade-offs

- [A moved test no longer shows the bug, because a check was written
  wrongly] → the test must keep its verdict on both sides (D2); a test
  without a declared deviation is BROKEN.
- [A moved test passes the original by accident: a check that expects the
  correct value also holds for the original's wrong one] → the cells with
  the faulty part must show exactly the declared deviations; the
  original's side runs in every round (`originals`).
- [Information of the `.orig` log (the original's batch session) is
  lost] → it is the environment, not the tested behaviour; the runner's
  report keeps the log of every run.
- [Long move] → old and new tests run side by side; nothing forces a date.

## Findings of the implementation

- **Task 1.2, where the rules are**: the judging of the declared
  deviations in the cells with the faulty part and the open-bug rule for
  the other cells are `tj_verdict` (`tools/lib/testjudge`, host test
  `testjudge`: "a fixed original bug", "an open original bug", "a declared
  crash"); the labels per case are the registration's (`tools/testreg`,
  `testreg_tool`: `bug-orig;emulator` for a case with a declared deviation
  of an open test, `emulator` for its other cases and for a fixed test,
  `originals` for the side with original material); the errors of
  `fixed-orig`/`fixed` are the configuration's (`tools/lib/testconf`,
  `testconf` and `testassert`: both keys, `fixed-orig` without a
  deviation, `fixed` with one, either in a `tests.toml`). A bug-orig test
  without a declared deviation is BROKEN in `tj_verdict` when the runner
  says it is a bug-orig test (`tj_test.require_deviation`, `corpusrun case
  --require-deviation`, which the registration of `tests/bug-orig` passes
  for every area but `regression/`); host test `testjudge` ("a bug-orig
  test without a declared deviation") and the emulator self test
  `bug_orig_selftest_model_no_deviation`.
- **Task 1.3, fix-scope** (`tools/lib/bugorigcmp`, `bugorigcmp_model.c`):
  a test of the model is projected onto one case per check of its source
  (`ID|check.NS.NAME||1`, `NAME` or `#N` for a check without a name, its
  value the macro and its arguments, not its line or number, so a line
  added above a check is no change), one per key of its `ID.test.toml`
  (`ID|toml:TABLE.KEY||1`) and the case `ID|fixed-orig||1 [fixed #N]` (or
  `fixed`), which is the only case the fix of issue N adds. The
  `tests.toml` of `tests/bug-orig` and of its areas are cases too
  (`AREA/tests.toml|toml:...`); the root `tests/tests.toml` is outside the
  tree that fix-scope reads (open: it holds only `root = true` today).
  Host test `bugorigcmp` with the scenarios of the spec (only the fixed
  issue; an expected value changed in another test and in the own one; a
  deviation changed, an assertion added; `fixed = N`) and broken input.
- **Task 1.4, bug-orig-record**: the list of the target holds only the
  tests of the old form (`bug_orig_test()` writes it; the glob of the old
  form leaves out `*.test.bas`), and `record.cmake` also skips an entry
  whose directory has `ID.test.bas`, with the message "a test of the
  BASIC test model: nothing to record" (`bug_orig_record_target`: one test
  of each form; the model test's runs are not started, its `ID.orig` not
  written).
