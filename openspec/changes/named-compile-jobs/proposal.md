# Proposal

## Why

The compile jobs of a cell of the BASIC test model (the driver's job, the
compiles of the data sources of `ID.test/`, the jobs of `jobline`) write
their outputs into one compiler log, and an assertion can address only
the whole log (`compile.log`) and the driver's result (`compile`). Two
consequences (`~/.local/share/bb2-c-work/decisions/rounds/tr/open-questions.md`,
Q-A20 and Q-A24):

- `compiler/compiler-racing-stripe-wide` (#980) judges two blocks of the
  log, one per job (j000 and j001); one assertion per query holds one
  `contains`, so the test stays in the old form.
- A data source is staged as `CORPUS:src/bNNN.bb2`, NNN its position in
  `[data]`. `compiler/compiler-panic-remtask` and
  `rwe/rishapeslib-cludged-shape-pop` name `CORPUS:src/b000.bb2` in their
  `jobline`; a second data source before `victim.bas` would shift the
  name and break the job line silently.

The user's answers of 2026-11-23 (`rounds/tr/decisions.md`, "Flaky
marker; named compile jobs (Q-A17, Q-A20, Q-A24)", status active):
"Together: named jobs (Recommended)"; and, recorded as a proposed entry
for this plan ("Q-A23 and Q-A24 remarks"): "Das Ziel sollten benannte
Ausgaben sein, die dann auch in den assertions genutzt werden können."

## What Changes

- **Every compile job of a cell has a name**: the driver's job `j000`;
  in a cell with `editor-run = "on"` also its run `r001`; each data
  source of `basic-compile` by its file name (`victim.bas`), never by its
  internal job `bNNN`; each job of `jobline` (`job=`, `run=`, `menu=`,
  `resident=`) by its ID. A jobline ID is checked: `[a-z][a-z0-9_]*`, at
  most 15 characters, unique in the job list, not a name of the runner
  (`j000`, `b` and three digits, `r001` in an editor-run cell), not `log`
  or `inject`.
- **Named outputs in the assertions**:
  - `compile.log.NAME`: the job's block of tedstub's log (definition in
    the design), with the size of a `RESULT ID ok SIZE` line removed as
    in `compile.log`; in the block of a data source its internal ID is
    written as its file name and its staged path as `@DATA:FILE@`, so the
    block does not depend on the position in `[data]`.
  - `compile.NAME`: the compile result (`ok`, `error LINE MESSAGE`, a
    failure, `none`) of a `job=` or `resident=` job of `jobline`; no
    default assertion.
  - The result of a data source stays `data.FILE`, the driver's
    `compile`; `compile.j000` and `compile.FILE` are refused naming the
    query that holds the output.
- **The staged path of a data source in a job line**: the placeholder
  `@DATA:FILE@` in an element of `jobline` stands for the tokenised
  source of the data source `FILE` on the cell's volume; elsewhere, or
  for a file that is not a data source of `basic-compile`, it is an error
  naming file, line and placeholder.
- **The unnamed queries stay as they are**: `compile` is the driver's
  result, `compile.log` the whole log (the compiler's output, then
  tedstub's log), `compile.inject` inject's report of the compiler
  process.
- **Users**: `compiler/compiler-racing-stripe-wide` (#980, fixed) moves
  to the model with `compile.log.j000` and `compile.log.j001`;
  `compiler/compiler-panic-remtask` (#24, open) and
  `rwe/rishapeslib-cludged-shape-pop` (#807, fixed) name `@DATA:victim.bas@`
  instead of `CORPUS:src/b000.bb2`.
- **Not in scope**: a per-job `compile.inject` (inject reports once at
  the end of the compiler process, not per job: design D4); a compile
  result of a `run=` or `menu=` step (`compile.NAME` refuses them; their
  outcome is in their block); a change of tedstub, its log format, the
  driver, testlib or the job IDs written to tedstub (`bNNN` stays
  internal); rewriting the existing `compile.log` assertions into named
  ones; parameters of the driver (D5 stays: "Bei D5 bleiben"); the
  `flaky` key (another change of the same round).

## Capabilities

### New Capabilities

(none)

### Modified Capabilities

- `testing/basic-test-runner`: the names of a cell's compile jobs, the
  queries `compile.log.NAME` and `compile.NAME`, the placeholder
  `@DATA:FILE@` in `jobline`.

## Impact

- Code (host side only): `tools/lib/testconf` (the jobline IDs and
  their checks, the queries, `@DATA:FILE@`), `tools/lib/testjudge` (the
  queries), `tests/corpus/runner/caserun.c` (the blocks in the
  observation, the placeholder in the cell pass),
  `tests/corpus/runner/bugorig.c`/`.h` (the block of a job in a tedstub
  log, beside `bugorig_compile`); no change of `corpus.c`, tedstub,
  inject or testlib.
- Results (rule 3): none change. blitz2, TED, tedstub, inject, the
  libraries, `testlib.obj`, `macros.bas` and the driver text are
  unchanged; the job lines of panic-remtask and rishapeslib read the same
  after the replacement; the moved racing-stripe-wide compiles the same
  source for j001 as its driver. No `PREV_INTENDED`.
- No original bug is fixed here (no rule-4 workflow). racing-stripe-wide
  keeps `fixed-orig = 980` (FIXED on the side port, the original's
  blocks as declared deviations); panic-remtask stays REPRODUCED (#24
  open on purpose); rishapeslib keeps its verdicts.
- Tests: host tests of `testconf`/`testassert` (each refusal with broken
  input), `testjudge`, `test_bugorig` (blocks), `test_caserun` (the
  placeholder, the blocks of a data source in a part); the three users
  on both sides; the prev comparison of the moved test.
- Docs: `tests/README.md` ("The run configuration": `@DATA:FILE@`;
  "Assertions": the grammar and table; "Moving a bug-orig test to the
  model": the `[log]` row), `tests/corpus/runner/README.md`,
  `docs/testing-old-form.md` (section 5 loses racing-stripe-wide).
- Overlap with open changes: `basic-test-editor-run` lists
  racing-stripe-wide in its open task 3.3 and adds the `@DRIVER@`
  requirement this change builds on (both ADDED only, other names; both
  edit `testconf`, `testassert.c`, `testjudge`, `caserun.c`,
  `test_caserun.c`, `tests/README.md`, `tests/corpus/runner/README.md`).
  `bug-orig-basic-tests` (task 3.5, round U4) edits `tests/README.md` and
  `docs/testing-old-form.md`. The parallel `flaky` work edits
  `tools/lib/testconf/testconf.c`, its host test and `tests/README.md`.
  Order: design D7.
