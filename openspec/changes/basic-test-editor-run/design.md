# Design

## Context

See proposal.md, "Why". The state this design starts from (main
50e02ed34):

- A cell is a pass of `corpus.h` with `cell=on` (`cr_cell_pass`,
  `tests/corpus/runner/caserun.c`). Its session script has two parts per
  cell: the compile (`blitz2:blitz2 CORPUS:jobs` through tedstub, optionally
  through `wrap`, its output with `rc=N` in `CORPUS:out/blitz2.out`) and
  the shell run (the launcher reqtrap, optionally `runwrap`, output in
  `CORPUS:out/test.run`). The driver is `CORPUS:src/test.bb2`, the
  executable `CORPUS:out/test`, the result file `CORPUS:out/test.res`
  (`CR_PROGRAM`, `CR_RESULT` in `caserun.h`).
- `jobline` is already a run setting and is copied into the cell's job
  list; tedstub's `run=ID|SOURCE|CONSOLE[|PANIC]` runs a source with
  COMPILE/RUN (the program in a process of blitz2, the stop requester
  open, its console a file whose lines tedstub logs as `OUTPUT ID LINE`).
  A `run=` of the driver in a job list would run the driver in the compile
  part and the shell run would then write `test.res` a second time, over
  the editor run's.
- `cr_cell_pass` stages the runner's `cd-defaultdbug` only in cells with
  original material; there is no port build of the module
  (`libs/acidlibs/dhandlerlib.c`: blitz2 hands it to dhandlerlib in
  makeinits).
- The query `compile.log` is the compiler's output (`[blitz2]`) followed by
  tedstub's log (`[log]`, `observe_cell`); the query `inject` holds the
  `inject:` lines of the *program's* output (`cr_cell_inject`: `pad`, or a
  `runwrap` of inject). Nothing selects the `inject:` lines of
  `blitz2.out`.
- The predicate string of an assertion takes every character but `"` and
  `\` (`testassert.c`), and the TOML subset has `\n` and `\t`, so a
  `contains` can span several lines of the log.

What the U4 tests need: `~/.local/share/bb2-c-work/editor-run-u4-classification.md`
(class (b): eight tests; class (a): 97; class (c): the three self tests of
the old runner and the 48 generated exprmatrix tests; class (d): 230
compile-only tests).

Recorded answers this design rests on
(`~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`):
"basic-test-local-data: dhandlerlib, timeout, order, spec split" (the
change itself, its two named parts and its place before U4); "Driver
control data: D5 stays" ("Bei D5 bleiben": the driver takes no
parameters); "Runner extension: three answers" (the inject query, the
cases that run on their own); "Model tests get the emulator environment:
fix right away" (the environment of the registered tests, unchanged here).

## Goals / Non-Goals

**Goals:** a cell runs its driver from the editor with a debugger module;
the inject lines of the compiler's run are a query; the eight users of U4
and `dhandlerlib-no-cache-flush` move.

**Non-Goals:** two runs in one cell (editor then shell, two editor runs),
PANIC (Q1); the requesters and exit code of an editor run; a query of the
compiler's other output or of the log alone; changes of tedstub, inject,
blitz2, the libraries or the driver text.

## Decisions

### D1 `editor-run` replaces the shell run

Plan choice (the planning agent's): a run setting `editor-run = off|on`;
with `on` the cell pass gets the key `editrun=on` (new in `corpus.h`, only
with `cell=on`), and `corpus.c` adds behind the cell's jobs the line
`run=r001|CORPUS:src/test.bb2|CORPUS:out/test.con` and writes no shell run
for the cell. The compile stays job j000 (the query `compile`, the
executable for nothing else). So the result file has one writer and the
judging is unchanged.

Alternatives: (B) the author writes the `run=` job line, both runs happen,
the shell run's result file is judged and the editor run is seen only
through `compile.log` — the checks of the editor run would be lost without
notice, and `debuglib-setint-runtime-error` would need `runargs` and
`NumPars` to keep the shell run from repeating the error; (C) both result
files judged — two runs per case to merge, for one user
(`compiler-panic-remtask`, Q1). A setting also keeps the driver's name out
of the configuration for the editor run.

Point: a later test needs an editor run and a shell run in one cell (the
pattern of `compiler-panic-remtask` and the compile-only
`compiler-data-pad-byte`) — L2 Possible / I2 Medium / E2 Reasoned → Plan
(a further value of `editor-run` is additive; Q1 asks the user about the
one known case).

### D2 What an editor run shows

Plan choices:
- `output` is the console file `CORPUS:out/test.con`, read by the runner
  (it survives a crash; tedstub's `OUTPUT` lines come only after the run
  and stay in `compile.log`).
- `rc` and `requester` are not captured: COMPILE/RUN gives no exit code,
  and reqtrap, which records the requesters, starts only the shell run
  (the compile part of the script has no launcher: `compile_part`,
  `corpus.c`). Their defaults do not apply; an assertion on them, and
  `!ExpectError` (judged by its requester), are refused when the
  configuration is read. `end.case`, `end.run`, the checks, `session`,
  `compile`, `compile.log`, `compile.inject` are as in any cell.
- `runwrap`, `runargs`, `launcherargs` and `pad` configure the shell start
  and are refused with `editor-run = "on"`; `serial`, `wrap`, `jobline`,
  `files` and the others stay.

Point: a requester of the program in an editor run blocks the session
until its time limit, and the test sees only `session == timeout` — L2
Possible / I1 Low / E2 Reasoned → Monitor (the old form has the same
limit; no U4 user expects a requester).

### D3 The hang check and the end of an editor-run cell

Plan choice: the hang check of the session (`hang_from=run`) arms on the
console file `CORPUS:out/test.con`, which tedstub opens when the program
starts (openio); a cell with `editrun=on` is complete when `blitz2.out`
ends with `rc=` (`cell_complete`). Task 2.2 measures both in the emulator.
Point: the arm file is not created before the program starts, so a hang
of the editor run takes the whole time limit — L2 Possible / I1 Low / E1
Assumption → Monitor (the self test of task 2.4 shows it).

### D4 `defaultdbug` is a path, staged in every cell it applies to

The user named the setting ("a setting `defaultdbug`"). Plan choices: its
value is a path setting like `wrap` (relative to its file, placeholders:
`@CD_BLITZ2@/DEFAULTDBUG` for the CD's module), as the old form's
`defaultdbug=PATH`; it replaces the runner's `cd-defaultdbug` in the cells
with original material and adds the module to the others; without it the
staging is as today. A cell that sets it without `editor-run = "on"` is
refused (the module has no effect on an executable; the runner's own
staging in the cells with original material stays and is no setting).

Side (Q2, the user's): the spec's definition of the side names the
compiler and the libraries; a cell with the C libraries and the CD's module
stays on the side port and is skipped (77) without the CD, as the old
fixed test (`; @pass defaultdbug=@CD_BLITZ2@/DEFAULTDBUG`, "skipped
without the CD"). The plan works with that answer.

### D5 `compile.inject`

The user named the query ("a query on the inject lines of the compiler's
run"); the name is the planning agent's Plan choice. It is filled when the
path field of the cell's `wrap` is `@INJECT@` or the runner's `inject` (as
`cr_cell_inject` does for `runwrap`) with the `inject:` lines of
`blitz2.out` (`cr_inject_lines`), and is not captured otherwise. The
U4 tests need nothing more: their `[blitz2]` sections hold only `inject:`
lines and `rc=0`, their `[log]` sections are one `compile.log contains`
each (classification, class (a)). So no `compile.output` and no log query
(Plan choice). Point: a mover needs two separate parts of the log for one
cell, which one assertion per query cannot give — L2 Possible / I2 Medium
/ E2 Reasoned → Plan (racing-stripe-wide's two blocks are joined by fixed
lines; a further query is additive).

### D6 `@DRIVER@` in `jobline`

Plan choice: the runner replaces `@DRIVER@` in `jobline` elements by
`CORPUS:src/test.bb2` when it writes the cell pass; the placeholder is the
runner's, not the environment file's, and is refused in every other key
(the error of an unknown placeholder, which names it). Alternative: the
movers write `CORPUS:src/test.bb2` (an internal name of the runner in the
configurations). Point: a later rename of the driver — L1 Unlikely / I1
Low / E2 Reasoned → Monitor.

### D7 The prev comparison

Plan choice: `cr_prev_pass` drops `editrun` (as it drops `defaultdbug`,
`runwrap`, `runargs`), so the prev pass compiles job j000 with both
compilers and runs nothing; the `mask_traps` rule for a `run=` job line
stays for tests that write one themselves. No executable a compiler writes
changes (rule 3), so no `PREV_INTENDED`.

### D8 The users

| Test | Settings | Judged |
|---|---|---|
| `libs/dhandlerlib-no-cache-flush` (#409, `fixed-orig`) | `editor-run`, `runtime-errors = "on"`, `defaultdbug = "@CD_BLITZ2@/DEFAULTDBUG"`, `wrap = "@INJECT@\|cacheflush=Debug"`; cells `libs=clib` (port) and `libs=cd` (originals) | `compile.inject contains "caches flushed before"`; deviation `libs=cd`: "caches not flushed before" |
| `regression/debuglib-setint-runtime-error` | `editor-run`, `runtime-errors = "on"` | `output == "start\n"`, the end entries as the run shows them; `compile.log contains "REQUESTERS\tr001\t0"` |
| `compiler/compiler-insasm-size-byte`, `-libsnode-name`, `-macro-textlen-max`, `-resconsts-last-next` | `wrap` of inject (`track`, `pad`) | `compile.inject ==` the expected lines; deviation `compiler=orig195` with the original's |
| `regression/compiler-macrobuff-nomem` | `wrap` (`allocfail`), `jobline` | `compile.inject` and `compile.log contains` the XTRAERR lines |
| `compiler/compiler-racing-stripe-wide` | `jobline` with `@DRIVER@` | `compile.log contains` the PROGRESS/STRIPE/GUARD lines |

The moves follow `tests/README.md`, "Moving a bug-orig test to the
model" (each keeps its verdict on both sides; old files removed in the
same commit). Round U4 (task 3.5 of `bug-orig-basic-tests`) then leaves
them out.

## Risks / Trade-offs

- [The driver's own code and text change a test that sits on a compiler
  limit] → not this change's: listed for the U4 movers in the
  classification (class (a), risks).
- [Parallel edits with `basic-test-local-data` in `testconf`, `testjudge`,
  `caserun.c`, `corpus.c`/`.h`, the READMEs] → textual conflicts only
  (different keys, queries and requirements); the later merge rebases —
  L3 Likely / I1 Low / E2 Reasoned → Monitor.
- [The console of COMPILE/RUN is a file on the cell's volume; a program
  that reads its console gets end of file] → no user reads input; the old
  form used `RAM:con` the same way.

## Migration Plan

Additive: no key, query or default of an existing test changes; the moved
tests replace their `.bas`/`.expect`/`.orig` in the commit that adds the
model files.

## Findings of the implementation

- **D3 measured** (task 2.4, `blitz2_selftest_model_editor_run_hang`,
  2026-11-23): the program of an editor-run cell that prints a line and
  loops ends its session after 11.3 s and 11.5 s (two runs; `hang-after`
  the runner's 8 s, the session's limit 90 s): the hang check armed on the
  console file `CORPUS:out/test.con` works, the hang does not take the
  time limit. The case that ends normally (`blitz2_selftest_model_editor_run`)
  takes a session of 3 s.
- **The arm of a shared session.** `corpus_session_start` arms the hang
  check by the first cell of the session (its pass): if that cell runs from
  the editor, its console arms the check, and the compiles of the cells
  behind it in the same session count against `hang-after` (a compile
  silent for longer would end the session as a hang); if it runs from the
  shell, an editor run of a later cell is not hang-checked until the first
  cell's shell run starts. No user mixes the two in one session (the moved
  tests have one value of `editor-run`) — L1 Unlikely / I1 Low / E2
  Reasoned → Monitor.
- **The observation is a function of its own.** For the host test of task
  2.3 ("a recorded cell directory of each kind") the observation of a cell
  is `cr_observe` (`caserun.h`, with `struct cr_cellrun` and `struct
  cr_obs`), which reads the files of a cell's volume; the run calls it as
  before.
- **The defaults** of `rc` and `requester` are left out by
  `tc_assertions_for` for an editor-run cell (`struct tc_default.shell`),
  so the judge, `cr_case_alone` and `--print-config` see the same set;
  `cr_cell_clean` takes such a cell as clean without an exit code
  (`tj_obs.editor_run`).
- **racing-stripe-wide does not fit one `compile.log contains`** (task
  3.3; D5's point came true): the two blocks the test judges (the
  PROGRESS/STRIPE/GUARD lines of j000 and of j001) are not joined by fixed
  lines only: each block follows its job's `RESULT ID ok SIZE` line, and
  the size of the executable is the driver's (it changes with the driver
  and `testlib`). One assertion per query can hold one block, not both.
  The test stays in the old form until the user chooses (report of the
  implementation): a further query (D5's "a further query is additive",
  e.g. the log without the sizes of the RESULT lines, or one query per
  job), or the move with only j001's block judged (j000's empty bitmap
  then unchecked: a narrowing), or the old form for good.
- **macro-textlen-max on the original**: with `macrobuf=100000` 1.95's
  compile of a driver does not end ok (its end depends on the memory, #965)
  and it hangs at its exit (#336, as compiler-macrobuff-size), so the old
  "Macro too Big" is no stable observation; the deviation declares what
  holds every time (`compile != ok`, `session == timeout`, no inject
  report), with `isolate` for 1.95 (the hang ended the shared session
  before the cell port,cd had run). The two runs here gave `compile ==
  none`.
