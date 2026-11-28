# Design: test-model-warnings

## Context

How the warnings reach the runners today:

- **Editor path** (blitz2 through tedstub; the model's cells of `port`,
  `orig195`, `orig210`): since commit 6fc22afac tedstub logs a message
  `Warning: ...` that comes during a compile as a line
  `WARNING<TAB>ID<TAB>MESSAGE` and the compile goes on; the step blocks of
  `bugorig.c` know the keyword, so the line is in the block of its job
  (`compile.log.JOB`). blitz2's type warnings come after the first pass
  (`parse.c` `typewarnend`): first the shown places in their order,
  `Warning: line N: TEXT [NAME]` (N the driver's line, 1-based), then per
  name `Warning: NAME: K more`; `resclash` is `Warning: Clash in
  Residents: #NAME` without line and name (`parse.c` `warn`). The original
  compilers give no warning.
- **Command line** (`compiler = "ab3"`; `caserun.c` `compile_part`): the
  console goes to `CORPUS:out/blitz2.out`, followed by `rc=$RC`.
  blitz2-ab3 prints `SOURCE: warning: MESSAGE [NAME]`, with `:LINE` where
  the warning has a line, and inside an include the include's name and line
  (`ab3main.c` `ext_warning`); return code 5 after warnings, 10 for a
  warning in the error set (printed as an error). `cr_cli_compile` already
  skips `warning:` lines when it looks for the error; `cr_compile_mapped`
  maps a driver line to the test source (`tp_driver_where`).
- The judge (`tools/lib/testjudge`) gets the observations in `struct
  tj_obs`; the queries and their predicates are in
  `tools/lib/testconf/testassert.c`; refusals per cell in `testmatrix.c`;
  the jobline IDs in `testconf.c` `check_jobline`; `whole_program()` in
  `testjudge.c` decides which queries make a case run on its own.
- The warning names: `shared/ab3job/warnlist.h` (one list for core, job
  and command line).
- `tests/ab3/cli`: `datafrac`, `datafrac_bb2`, `datafrac_many`,
  `datafrac_inc` (+ `datafrac_inc_data.ab3`), `datafrac_r` (`-r`),
  `datafrac_off` (`-r -w -datafrac SAME datafrac_r`), `datafrac_werr`
  (`-werr datafrac`). `tests/blitz2/warn-datafrac-limit` asserts the nine
  WARNING lines of the editor with driver lines (85..92 = source line +
  71) in one `compile.log` `contains`.

## Goals / Non-Goals

**Goals:** one query form for both paths; a missing, an added and a
reordered warning fail; source lines, not driver lines; the return code of
a command-line compile; every new check with a test of broken input.

**Non-Goals:** see proposal, "Not in scope". In particular no run setting
for the warning sets (D6) and no warnings of other jobs (D8).

## Decisions

The choices below are the planning agent's (D-011 (b)), not the user's;
each with its rating. The recorded answer this change follows:
tasks/decisions.md, "2026-11-28 Compiler warnings in the test model"
(active).

### D1. Where the requirements go

`testing/test-model` gets the general requirement (warnings are a list of
entries, judged apart from other output), as the recorded rule names a
"testing/test-model delta"; the queries, their refusals and the two parsers
go into `testing/basic-test-runner`, where all other queries are;
`testing/ab3-tests` says which warning cases leave `ab3_cli`. The Purpose
of `testing/test-model` (its "Not yet moved" note) cannot be a delta: task
6.2 edits it, as ab3-test-model's task 8.2 did.
Point: the split of the requirements over three specs — L1 Unlikely / I2
Medium / E2 Reasoned → Monitor.

### D2. The form of the query

`compile.warnings` is a text, one entry per line, `PLACE NAME TEXT`:
blanks between the fields (as `error LINE MESSAGE` of `compile`), PLACE a
source line, `driver:N` (as `compile`'s `error driver:N`), the printed
place of another file, or `-`; NAME `-` where the compiler names none (the
editor's resclash). The editor's `NAME: K more` is `- NAME K more`. The
text form reuses the string predicates; `count` gives "no more than these"
(and `count == 0` "none"); `compile.warnings.K` asserts one entry per
TOML line, so that a list of nine needs no 600-character string (the
current `warn-datafrac-limit`). Order: `==` on the whole list, or the
entries by number plus `count`. A missing warning: `count == 0`,
`== ""`, or `not contains` on its entry.
Alternatives: a table per warning (`[warnings]` with keys) — new
configuration grammar for one query; only the whole-list text — unreadable
for the limit test.
Point: the query names and the entry form — L2 Possible / I2 Medium / E2
Reasoned → Plan.

### D3. No default assertion

The query has no default (as `run.NS.NAME.rc`). A default "no warnings"
would fail every existing test whose source meets a type exception once
type-system-rework task 3.5 adds the warning sites (the expression tests
of `tests/blitz2`, the bug-orig compiler tests), unrelated to what they
test; the warnings are checked by the tests written for them.
Point: no default assertion of the warnings — L2 Possible / I2 Medium / E2
Reasoned → Plan.

### D4. Refusals by compiler

`compile.rc` exists only for a command-line compile: the editor path's
return code is that of the whole job list, not of the driver's job;
refused at registration (as `rc` in an editor-run cell). The
`compile.warnings` queries are refused for `amiblitz3` cells: AmiBlitz3's
warnings have another form and other names; a warning test excludes the
oracle cell or selects `compiler=ab3`. The jobline IDs `warnings` and `rc`
are refused, as `log` and `inject` are, since `compile.JOB` would collide.
Point: refusing instead of "not captured" — L2 Possible / I2 Medium / E2
Reasoned → Plan.

### D5. A whole-program observation

The warnings and the return code belong to the compile, which the spec
lists among the observations of the whole program ("compile result or
log"); they are added to `whole_program()`, so a case that asserts them
runs on its own and is never bundled. The driver holds the whole source in
every run (only the case calls differ), so the list is the same in each
run of a source.
Point: the cost of one more session per asserting case — L1 Unlikely / I2
Medium / E2 Reasoned → Monitor.

### D6. No setting for the warning sets

The recorded rule asks for a query. `-w`/`-werr` (command line) and
tedstub's `warnings=off|on|errors` (editor path, all warnings at once) are
different inputs; a run setting for them would be a new axis of the model
with its own parser and refusals. So `datafrac_werr` (`-werr datafrac`),
`datafrac_off` (`-w -datafrac`, `SAME datafrac_r`) and with it
`datafrac_r` stay in `ab3_cli` (reasons "other arguments", "SAME"); the
"Not yet moved" note names them. A later change can add the setting.
Point: the three cases stay in ab3_cli — L2 Possible / I2 Medium / E2
Reasoned → Plan.

### D7. The moved cases

Each moved case becomes a test of its own in `tests/ab3/model/` (the
warnings are those of one compile, so one source per expected list), its
case named as the old `ab3_cli` case: `warn-datafrac.test.bas`
(`datafrac`), `warn-datafrac-bb2` (`datafrac_bb2`, `source-mode =
"bb2"`), `warn-datafrac-many` (`datafrac_many`), `warn-datafrac-inc`
(`datafrac_inc`). Each `ID.test.toml` excludes `compiler=amiblitz3` (the
`ab3_cli` cases had no `REF`, so no oracle is added). The `[run]` values
become checks (`!CheckEq`), the exit code keeps its default; the
`[compile]` lines become `compile.warnings.K` entries with the line of the
new test source and `count`, `rc=5` becomes `compile.rc == 5`.
`datafrac_inc` moves if its include can be staged next to the driver
(`files = ["src/datafrac_inc_data.ab3|..."]`, the `INCLUDE` line unchanged)
or by `@TESTDIR@` in the `INCLUDE` line with the file a `copy` data file;
the implementer takes the first that registers and runs, the PLACE then
being the include's printed path; if neither works it stays with the
reason "an include found by its name".
Point: excluding the oracle cell — L1 Unlikely / I1 Low / E2 Reasoned →
Monitor. Point: whether `datafrac_inc` can move — L2 Possible / I1 Low /
E1 Assumption → Monitor (raised by the registration and one run in task
5.1).

### D8. Only the driver's compile

The warnings of a data source (`basic-compile`) or a jobline job stay in
their block (`compile.log.JOB`); no `data.FILE.warnings` or
`compile.warnings.JOB` now. No test needs them yet.
Point: no per-job query — L1 Unlikely / I1 Low / E2 Reasoned → Monitor.

### D9. Order with the other open changes; shared files

- type-system-rework: its warning tests (task 3.5) are written after this
  change, by the recorded rule ("then ... the type-warning tests of
  type-system-rework are written there"). Task 7.1 of this change adds
  the note to `openspec/changes/type-system-rework/tasks.md`, task 3.5
  (the coordinator, through `openspec-update-change`).
- ab3-mode-g5 task 4.3 adds the `procend` warning case to `ab3_cli`
  "(console line, return code 5) and a `-werr`-style case". After this
  change the requirement "Warning cases are tests of the model" puts the
  first into the model. Two recorded answers bear on it (tasks/decisions.md
  2026-11-25 "new AB3-mode cases keep the ab3_cli form until tests/ab3 is
  moved"; 2026-11-28, scope "the tests of compiler warnings (blitz2 and AB3
  mode)"): answered by the user (Q1 below): in the model.
- Files that open changes also edit: `tests/README.md` (ab3-mode-g5 1.5,
  blitz-coverage 3.3, coverage-ted-libs 4.3, type-system-rework 7.4; other
  sections), `tests/ab3/CMakeLists.txt` (console-break-option 2.2 and
  ab3-mode-g5 4.3 add `ab3_cli_case` lines; this change removes the
  moved datafrac lines). No other open change has a delta for
  `testing/basic-test-runner`, `testing/test-model` or `testing/ab3-tests`.
Point: textual conflicts in the shared files — L2 Possible / I1 Low / E3
Proven (the task lists of the named changes) → Monitor.

### D10. Unreadable warning lines

A WARNING line or console warning line the parser cannot read, or a NAME
not in `warnlist.h`, makes the cell cannot-be-judged (ERROR) naming the
line, so that a change of the compiler's message form shows instead of
silently giving an empty list. The runner includes `warnlist.h` (host C,
macros only) for the names.
Point: ERROR on an unknown form — L2 Possible / I2 Medium / E2 Reasoned →
Plan.

## Risks / Trade-offs

- [The editor's line of a warning inside an include is not known] → no
  test of it here; the include case is a console case (D7). Monitor.
- [A moved case's lines differ from its `.expect` (the checks replace the
  `NPrint` lines)] → the entries are asserted with the new source lines;
  the text, name, order, count and rc are those of the `.expect`
  (requirement "A moved warning case keeps its compile lines").
- [The prev comparison of the console output of the moved cases ends with
  their `ab3_cli` cases] → their expected warnings are now committed
  assertions, which catch a changed text in every run.

## Migration Plan

Groups 1-4 add the queries without moving anything; group 5 moves the
cases, each move committed with the removal of its `ab3_cli` case after
both passed on the same build (spec `testing/ab3-tests`). Rollback: revert
the move commits; the queries stay unused.

## Answered Questions

- Q1, answered 2026-11-28 (tasks/decisions.md "2026-11-28
  test-model-warnings: procend, -w/-werr, the proposal"): "Ins Modell
  (Recommended)", option (A). The question was: the `procend` warning case of ab3-mode-g5 task
  4.3 — (A) in the model with the queries, ab3-mode-g5's task 4.3 adjusted
  and this change before its group 4 (recommended); (B) in `ab3_cli` as
  task 4.3 says, and the requirement "Warning cases are tests of the
  model" names it as an exception until it moves. Point: the order and
  form of ab3-mode-g5's warning test — L2 Possible / I4 High / E2 Reasoned
  → Decide.
