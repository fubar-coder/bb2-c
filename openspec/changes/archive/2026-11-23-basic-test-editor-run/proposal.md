# Proposal

## Why

A test of the BASIC test model compiles its driver as an executable and
runs it from the shell. Some tests observe what only a run from the editor
shows (COMPILE/RUN: the program in blitz2's own process, with the debugger
module and dhandlerlib) or what inject reports about the compiler's own
run. They stay in the old form: `libs/dhandlerlib-no-cache-flush` (#409)
and, of round U4 of `bug-orig-basic-tests`, eight tests
(`~/.local/share/bb2-c-work/editor-run-u4-classification.md`). The user's
answer of 2026-11-23
(`~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`,
"basic-test-local-data: dhandlerlib, timeout, order, spec split",
dhandlerlib: "Own step before U4 (Recommended)"): the runner addition for
a program run from the editor with the debugger (a setting `defaultdbug`,
a query on the inject lines of the compiler's run) is a small change of
its own before round U4, designed with the needs of the U4 compiler tests.

## What Changes

- **The run from the editor.** A run setting `editor-run` (`off`, `on`;
  default `off`): with `on` the driver is compiled as before (job `j000`,
  the query `compile`) and then run from the editor with COMPILE/RUN
  (tedstub's `run=`) **instead of** from the shell. One run, one result
  file: the result file is written once. The printed output is the
  editor's console; the exit code and the requesters of such a run are not
  captured, so their defaults do not apply and an assertion on them, an
  announced runtime error (`!ExpectError`) and the settings of the shell
  start (`runwrap`, `runargs`, `launcherargs`, `pad`) are refused for such
  a cell.
- **The debugger module.** A run setting `defaultdbug` (a path, as `wrap`):
  the file is `blitz2:defaultdbug` of the cell, in the cells of both sides;
  without it the cells with original material keep the CD's module as
  today and the others have none. A cell of the side port whose module is
  missing (no CD) is skipped like one with another missing file.
- **The query `compile.inject`**: the lines starting with `inject:` of the
  compiler's output, when the cell's compiler runs through inject
  (`wrap` of `@INJECT@` or the runner's inject); otherwise not captured.
  The U4 tests need no query of the rest of the compiler's output or of
  the editor's log: `compile.log` holds both and takes a multi-line
  `contains`.
- **The driver's source in a job line**: the placeholder `@DRIVER@` in
  `jobline` stands for the driver's tokenised source on the cell's volume,
  for a job that compiles the test's own source again
  (`compiler-racing-stripe-wide`).
- **The users move to the model**: `libs/dhandlerlib-no-cache-flush`
  (#409), `regression/debuglib-setint-runtime-error`,
  `compiler/compiler-insasm-size-byte`, `-libsnode-name`,
  `-macro-textlen-max`, `-resconsts-last-next`,
  `regression/compiler-macrobuff-nomem`, `compiler/compiler-racing-stripe-wide`;
  round U4 then holds only tests the model expresses.
- **Not in scope**: an editor run followed by a shell run, two editor runs
  and the PANIC gadget (`compiler-panic-remtask`, #24: open question Q1);
  the compile-only tests that use the editor (`compiler-createproc-fail`,
  `-data-pad-byte`, `-loaddmod-pointer`: they stay in the old form with the
  other compile-only tests); a port build of the debugger module; a query
  of the compiler's other output (`compile.output`) or of the log alone;
  the requesters of an editor run; parameters of the driver (D5 stays:
  "Bei D5 bleiben"); the generated exprmatrix tests and the self tests of
  the old runner (they stay in the old form).

## Open questions (for the user)

- **Q1 `compiler-panic-remtask`** (#24, by design): it needs an editor run
  hit by PANIC and then a shell run that checks the lock. Options: it stays
  in the old form for now (recommended); or this change adds a value
  `editor-run = "before"` (editor run, then the shell run, whose result
  file is judged) and a setting for the PANIC ticks.
- **Q2 the side of a port cell with the CD's module**: the model's
  definition of the side names only the compiler and the libraries.
  Options: the cell stays on the side port and is skipped without the CD,
  as the old fixed test (recommended); or a cell with a module of the CD
  counts as a cell with original material (side originals; the fix of #409
  is then checked only in `originals`).

The plan works with the recommended answers.

## Capabilities

### New Capabilities

(none)

### Modified Capabilities

- `testing/basic-test-runner`: the run settings `editor-run` and
  `defaultdbug`, the observations of a run from the editor, the query
  `compile.inject`, the placeholder `@DRIVER@` in `jobline`, the prev
  comparison without the editor run.

## Impact

- Code (host side only): `tools/lib/testconf` (the two settings, their
  checks, the query, the placeholder), `tools/lib/testjudge` (the query,
  the defaults of an editor-run cell), `tests/corpus/runner/caserun.c`
  (the cell pass: the module, the editor run, the observation),
  `tests/corpus/runner/corpus.c`/`corpus.h` (a key of a `cell=on` pass for
  the editor run: the job line, no shell run, the cell's end, the hang
  check), `tools/testreg` only if the refusals need the registration.
- Results (rule 3): none change. blitz2, TED, tedstub, inject, the
  libraries, `testlib.obj`, `macros.bas` and the driver text are unchanged;
  the module is never part of an executable; the prev comparison compiles
  the same driver as before and leaves the editor run out.
- Tests: host tests of `testconf`, `testassert`, `testjudge`, `caserun`
  (`test_caserun`), `corpus` (`test_corpus`), each check with broken input;
  an emulator self test of an editor run; the moved tests on both sides.
- Docs: `tests/README.md` ("The run configuration", "Assertions", "Moving a
  bug-orig test to the model"), `tests/corpus/runner/README.md`,
  `tests/corpus/runner/corpus.h`.
- Overlap with open changes: `basic-test-local-data` (implemented in
  parallel) edits `testconf`, `testjudge`, `caserun.c`, `corpus.c`,
  `corpus.h`, `tests/README.md` and `tests/corpus/runner/README.md` too;
  both add requirements to `testing/basic-test-runner` (ADDED only, other
  names). `bug-orig-basic-tests` (round U4, tasks 4.1 and 4.2) touches the
  moved tests and `tests/README.md`; this change lands before U4 (the
  user's answer), so U4 no longer lists the moved tests.
  `split-long-requirements` touches no file of this change.
