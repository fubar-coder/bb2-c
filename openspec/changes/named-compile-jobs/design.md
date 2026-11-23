# Design

## Context

See proposal.md, "Why". The current state the design rests on:

- tedstub writes one log per session (`CORPUS:out/session.log`; a data
  source with its own build configuration: `CORPUS:DIR/session.log` of
  its part, `corpus.h` `build=`). The log format is the header of
  `tests/corpus/amiga/tedstub.c`: `BATCH start|end`, then per step lines
  `KEYWORD<TAB>ID<TAB>…` (`JOB`, `XTRA`, `XTRAERR`, `RESULT`, `MENU`,
  `POINTER`, `OPENIO`, `PANIC`, `CLOSEIO`, `CONSOLE`, `OUTPUT`,
  `REQUESTERS`, `PROGRESS`, `STRIPE`, `GUARD`), and the frame's requester
  lines without an ID (`openreq …`, `getinput …`, `ezfreq …`, `ERROR …`,
  `ABORT: …`, `extalloc …`). A `job=`, `run=` or `resident=` step starts
  with its `JOB` line (`begin_job`); a `menu=` step logs nothing before
  the menu item returns, so its requester lines come before its `MENU`
  line (`menu_job`). The version requester's lines stand before
  `BATCH start`.
- `compile.log` is the compiler's output followed by tedstub's log as
  `bugorig_observe` gives it (`log_of`): the size of `RESULT ID ok SIZE`
  is already removed (`tests/corpus/runner/bugorig.c`, `log_of`;
  checked by `test_bugorig.c`, "size removed"). The obstacle of
  racing-stripe-wide is therefore the one assertion per query, not the
  size (see "Risks", the contradiction in three texts).
- The runner's job IDs: `j000` the driver, `bNNN` the data sources
  (`cr_data_pass`, NNN the position among the `basic-compile` sources),
  `r001` the driver's run in an editor-run cell (`corpus.c`, written
  behind the `jobline` lines). The query `data.FILE` already maps a file
  to its `bNNN` (`observe_data`, `bugorig_compile`).
- `jobline` elements are passed through except `@DRIVER@` (`cr_cell_pass`);
  testconf does not read their IDs. A placeholder of the form
  `@DATA:FILE@` does not match `[A-Z][A-Z0-9_]*` and would today stay in
  the job line unchanged, silently.
- inject's report of the compiler (`compile.inject`) is written when the
  wrapped command ends (`tests/bug-orig/amiga/inject.c`, header: "rc=N of
  COMMAND, with track the allocations left"), i.e. once per compiler
  process, which compiles all jobs of the session; the build parts of data
  sources run without `wrap` (`corpus.c`, the part script).

## Goals / Non-Goals

**Goals:** every output of a compile job is addressed by the job's name;
no assertion and no job line depends on the position of a data source;
no change of tedstub's log, so the recorded originals (`ID.orig`) and the
old-form tests stay valid.

**Non-Goals:** a new log format; per-job queries of the compiler's own
output (`blitz2.out` is not split by job); renaming the internal `bNNN`.

## Decisions

### D1 Names

The driver's job keeps its ID `j000` (documented in `tests/README.md` as
"job `j000`"), a jobline job its ID, the editor run `r001`. A data source
is named by its file (`victim.bas`), as `data.FILE` names it; its `bNNN`
is not accepted as a name, so no assertion can depend on the position.
Names are unambiguous: a jobline ID has no dot and a `basic-compile` file
ends in `.bas`. The checks of the IDs (`[a-z][a-z0-9_]*`, at most 15
characters: tedstub's `cur_id[16]`; unique; not a name of the runner;
not `log` or `inject`, which would clash with the fixed queries) are
testconf's, at configuration time, from the `jobline` elements of every
cell. Alternatives: names chosen in a new table (more keys, no gain: the
ID is already the author's name); the data source by `bNNN` (the problem
of Q-A24). Plan choice (below).

### D2 The block of a job

The log is cut into steps. A line `KEYWORD<TAB>ID…` of the keyword list
above belongs to the step `ID`. A line without an ID belongs to the step
of the next ID line, except before a `JOB` line: there it belongs to the
step of the previous ID line (a job begins with `JOB`; a menu item's
requesters precede its `MENU` line). Lines before the first ID line of a
session (behind `BATCH start`) belong to the first step; `BATCH` lines and
the lines before `BATCH start` belong to no step. The block of `NAME` is
the lines of its step, in log order, with the `RESULT` size removed as
`log_of` does and the version line normalised as there. A `resident=`
step is one block (its `JOB`, `RESULT`, `MENU` and `POINTER` lines share
the ID). With the IDs unique (D1) a step's lines are contiguous; a
second, later run of the same ID (a new session after an abort) is
appended to the block. In the block of a data source its `bNNN` in the
ID field is written as `FILE` and its staged path (`CORPUS:src/bNNN.bb2`
or `CORPUS:DIR/src/bNNN.bb2`) as `@DATA:FILE@`. The function lives
beside `bugorig_compile` in `tests/corpus/runner/bugorig.c` (one parser
of tedstub's log for both). Alternative: a start line for menu items in
tedstub (simpler cut, but a changed log: the recorded originals of the
menu tests would have to be recorded again with the CD). Plan choice.

Example (the port side of racing-stripe-wide, from its `.orig`):

```
JOB	j001	CORPUS:src/compiler-racing-stripe-wide.bb2
XTRA	j001	defaults
ezfreq "Name of Executable to create" -> CORPUS:out/stripe-j001
openreq "***** COMPILING *****"
openreq "***** COMPILING *****"
RESULT	j001	ok
PROGRESS	j001	bar 0	222…
…
GUARD	j001	ok
```

is `compile.log.j001`; the `ezfreq`/`openreq` lines belong to j001
because they stand behind its `JOB` line.

### D3 `compile.NAME`

Only for `job=` and `resident=` jobs of `jobline`: `bugorig_compile` of
the cell's log with the ID, line numbers as logged (the line of the
compiled source; for a job of `@DRIVER@` the driver's line, not mapped to
the test source as `compile` is). No default: a jobline job is the
tested behaviour and may be meant to fail. Refused for `j000` (the
output is `compile`, with its default), for a data source (`data.FILE`,
with its default and BROKEN rule) and for `run=`/`menu=` steps (no
`RESULT ok` line: a successful run logs none, a menu item logs `MENU`),
each naming the query that holds the output: one query per output, no
two defaults for one result. The user's answer (2026-11-23,
`rounds/tr/decisions.md`, "Named jobs queries; snapshots; the abnormal-end
rule; hang check and leak threshold"): "(a) one query per output
(Recommended)".

### D4 No `compile.inject.NAME`

inject reports once per compiler process (Context), after all jobs of
the session; its lines cannot be given to one job, and a data source's
own build session runs without inject. So `compile.inject` stays the one
query of it; the spec says so. The user's answer (a) above: "no
`compile.inject.NAME`"; it is not possible in a meaningful form either.

### D5 `@DATA:FILE@`

testconf recognises `@DATA:` … `@` before the environment placeholders,
keeps it in a `jobline` element when `FILE` is a `basic-compile` data
source of the test, and refuses it elsewhere or for another file (file,
line, placeholder), as `@DRIVER@`. `cr_cell_pass` replaces it with the
staged path the data pass uses (`CORPUS:src/bNNN.bb2`, or the part's
path), from the same position count as `cr_data_pass`, so the two cannot
differ. The placeholder stands for the tokenised source, not for the
executable (`@TESTDIR@/NAME` already names that). Form as in the user's
answer ("e.g. `@DATA:victim.bas@`"). Plan choice.

### D6 The users

- `compiler/compiler-racing-stripe-wide` (#980): `ID.test.bas` (the
  program, its `a=a+1` lines and `Include "inc1.bb2"`, a case that prints
  what `[run]` held), `ID.test.toml` with `fixed-orig = 980`, `files` for
  the three tokenised includes (`src/incN.bb2|@TESTDATA@/…incN`, as
  `compiler-data-pad-byte`), `jobline = ["bitmap=4000|49152",
  "progress=on", "job=j001|@DRIVER@|CORPUS:out/stripe-j001"]`; asserts:
  `compile.log.j000` contains `PROGRESS\tj000\tnone\nSTRIPE\tj000\tnone\nGUARD\tj000\tok\n`,
  `compile.log.j001` contains its PROGRESS/STRIPE/GUARD lines; the
  deviations of the original cells declare the original's blocks (from
  the `.orig`: STRIPE rows in j000, `GUARD … before 95 after 0` in both).
  The old files (`.bas`, `.expect`, `.orig`, `.incN.src` move to the
  test data the `files` name) go in the same commit; the line in
  `docs/testing-old-form.md` section 5 goes.
- `compiler/compiler-panic-remtask` and
  `rwe/rishapeslib-cludged-shape-pop`: `CORPUS:src/b000.bb2` becomes
  `@DATA:victim.bas@`; their comments naming `b000` are updated. The job
  line after the replacement is the same text, so the cells run as before.
  rishapeslib is not named in the user's answer; it has the same job line
  and the same risk (Plan choice, below).

### D7 Order with the open changes

This change lands after the parallel `flaky` work (it edits
`tools/lib/testconf/testconf.c` and `tests/README.md` too; this change
starts from main after that merge). It takes over racing-stripe-wide
from task 3.3 of `basic-test-editor-run` (that task's other four tests
are untouched); the coordinator ticks the racing part of 3.3 when this
change merges. `bug-orig-basic-tests` task 3.5 touches no file of this
change except `tests/README.md` and `docs/testing-old-form.md` (other
sections). Plan choice.

## Plan choices (the planning agent's, for the coordinator-decisions file)

- D1 names. Point: a name form the user would choose otherwise (e.g. a
  namespaced `compile.log.data.FILE`) — L2 Possible / I2 Medium / E2
  Reasoned → Plan.
- D2 the block's cut without a tedstub change. Point: a line without ID
  attributed to the wrong step (a requester after a job's last ID line
  and before a menu item) — L1 Unlikely / I1 Low / E2 Reasoned → Monitor;
  as a choice of the query's definition — L2 Possible / I2 Medium / E2
  Reasoned → Plan.
- D3 `compile.NAME` only for `job=`/`resident=`: decided by the user
  ("(a) one query per output (Recommended)"), no longer a Plan choice; a
  later test that needs the compile result of a `run=` step is additive.
- D4 no per-job inject query: in the user's answer (a) ("no
  `compile.inject.NAME`"), no longer a Plan choice.
- D5 the placeholder's form and meaning. Point: the user meant the
  executable — L1 Unlikely / I2 Medium / E2 Reasoned → Monitor.
- D6 rishapeslib switches too. Point: the user wanted only panic-remtask
  — L1 Unlikely / I1 Low / E2 Reasoned → Monitor.
- D7 order. Point: the order sets which change moves racing-stripe-wide
  — L1 Unlikely / I4 High / E3 Proven (the user's answer: "compiler-
  racing-stripe-wide moves" in this change) → Plan.

## Risks / Trade-offs

- [Contradiction, rule 2] `docs/testing-old-form.md` section 5, the
  design of `basic-test-editor-run` ("racing-stripe-wide does not fit
  one `compile.log contains`") and `tests/README.md` ("lines that hold a
  size … (`RESULT ID ok SIZE`) cannot be inside it") say the size is in
  `compile.log`; `log_of` removes it (`test_bugorig.c`). Named here, not
  resolved silently: task 3.1 corrects the README row and the old-form
  section; the other change's design is the coordinator's to note.
- [A block that shows the job's ID] Moved `[log]` lines keep their form
  (`OUTPUT\tr001\t…`), so the existing `compile.log` assertions can be
  narrowed to a block later by renaming the key → only for data sources
  is the ID rewritten (D2).
- [`not exists` on a missing job] A job that never ran gives no block:
  `not exists` holds. An author who wants "ran and logged nothing" uses
  `compile.NAME` or a line of the block → documented in the README.
- [Two parsers of the log] `bugorig_compile` and the block share
  `bugorig.c` → one module, host-tested together.

## Migration Plan

No migration: new queries and a new placeholder; existing tests are
unchanged except the three users. Rollback: revert the commits.
