# Proposal

## Why

A test of the BASIC test model is one program, the driver the runner
generates; a test cannot bring a second program. Six bug-orig tests stay
in the old form because their program starts itself as a child (the
child's own start is the observation: a patched exec version, input from
`NIL:` or a file, a second program at once, a moved VBR):
`palettelib-make4bitrgbs-lowbyte`, `editlib-edit-eof`,
`rundebuglib-ctrlc-edit`, `rundebuglib-supertrap-d0-lost`,
`rundebuglib-two-programs`, `multi-68010-vbr`
(`~/.local/share/bb2-c-work/tr-u1b-report.md`, `tr-u2c-report.md`,
`tr-u2d-report.md`). A model program cannot run off its end either, so the
main scenario of #6 (`rundebuglib-printerr-numeric-code`) is tested
nowhere in BB2 mode (`analysis-u2-observations.md`, 1b). The user's idea
of 2026-11-23 (`~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`,
"Test-local data `ID.test/` with build actions (the user's idea)"): a
directory `ID.test/` beside `ID.test.bas` whose files the `.test.toml`
declares in `[data]` with a build action each, provided to the test at
`CORPUS:ID.test/...`; and the answer of the same day ("After the U2
analysis: PutReg, menuslib, run-off-end, texts", run-off-end: "With the
child feature (Recommended)"). The shape of the change follows the
user's answers to the plan's questions (same file, "basic-test-local-data:
form, compiler values, !RunProgram" and "basic-test-local-data:
dhandlerlib, timeout, order, spec split").

## What Changes

- **Test-local data.** `ID.test/` beside `ID.test.bas` holds the test's
  own files; `ID.test.toml` declares each in the table `data` with a
  build action and its options. Only the test's own file may declare
  data ("Die Daten gelten pro .test.bas"); a file of `ID.test/` that is
  not declared, or a declared one that is missing, is an error of the
  registration naming file and line.
- **Build actions.** `basic-compile` (a Blitz source compiled in the
  emulator: by the cell's compiler, libraries and runtime error setting,
  or by a fixed one; the executable without `.bas`), `copy` (the file as
  it is) and `mktestdata` (a `NAME.spec` made into `NAME` by
  `tools/mktestdata` on the host). The set is open for later actions
  ("Auch andere Werte könnten möglich sein").
- **The place.** Each cell's `CORPUS:` has `ID.test/` with the built
  files (the idea's Q3: "OK"); the placeholder `@TESTDIR@` stands for
  `CORPUS:ID.test` in the source and in the data sources.
- **The compile of a data source is judged**: the query `data.FILE` is
  its compile result (default `== ok`); a failed build that nothing
  declares makes the cases of the cell BROKEN.
- **Starting a program from a case** (the user's second idea; "!RunProgram
  (Recommended)"; "NewType pointer + file (Recommended)"):
  `*r.TLRun = !RunProgram{COMMAND$,"NAME"}` starts a command and returns
  the address of the library's NewType `TLRun` with its return code and
  its output as a string (the first 1024 bytes, a flag when cut), which
  the case checks with the check macros; the output also goes to a file
  the runner reads (also after a hang or crash), the return code is an
  entry of the result file, and the queries `run.NS.NAME`,
  `run.NS.NAME.rc`, `run.NS.NAME.output` judge what the case cannot. The requesters, a crash or a
  hang of the started program count for the case that started it (the
  idea's Q4: "OK, dem Case zuordnen"). No timeout ("None (Recommended)"):
  the session's hang check ends a child that hangs.
- **The users move to the model**: the six tests above, and the
  run-off-end scenario of #6 as a child program of
  `rundebuglib-printerr-numeric-code` (the originals side pins the empty
  requester text).
- **Not in scope**: `dhandlerlib-no-cache-flush` (#409): its
  observation is inject's report of the compiler's run; a small change of
  its own before round U4 adds a setting `defaultdbug` and a query on the
  inject lines of the compiler's run, and the test moves then ("Own step
  before U4 (Recommended)"); the split of the requirement "Entries of the
  result file" of `testing/basic-test-interface` (a small change of its
  own, "Split, own change (Recommended)"; until then this change adds its
  entries as a requirement of their own); checks inside a started
  program (it has no result file of its own); a timeout of the library
  for a started program; moving the tests that build data files with `Poke`
  (for example `shapeslib-loadshape-depth-zero`,
  `-height-zero`, `-width-zero`, `rwe/rifnslib-codes-160-end-text`,
  `rwe/ristonecrackerlib-stcload-no-header-check`,
  `rwe/rifnslib-fnsload-bank-55`): a later move may replace their bytes by
  a `mktestdata` or `copy` file; parameters of the driver (D5 stays).

## Capabilities

### New Capabilities

(none)

### Modified Capabilities

- `testing/basic-test-runner`: test-local data, their build actions and
  place, the query of a data compile, the queries of a started program
  and their assignment to the case, the prev comparison of compiled data.
- `testing/basic-test-interface`: the macro `!RunProgram`, its
  preprocessed form, the routine, the NewType `TLRun`, the entries `run`
  and `runend`.
- `testing/basic-testlib`: the library starts a program and keeps its
  result in one static block; the rule that the library allocates
  nothing gets the started program as its exception.

## Impact

- `tools/lib/testconf` (the table `data`, its checks), `tools/testreg`
  (the files of `ID.test/` as inputs of the registration), `tools/lib/testpp`
  (`@TESTDIR@`, `!RunProgram`), `tests/corpus/runner` (`caserun.c`: the
  builds in the cell's session; `corpus.c`/`corpus.h`: build parts of a
  cell pass), `tools/lib/testjudge` (the queries), `shared/basictest`
  (codec, `macros.bas`), `libs/userlibs/testlib` (the routine).
- Results (rule 3): `testlib.obj` changes (a new routine; intended,
  `PREV_INTENDED` and named in the commit). The driver text of every test
  holds the new macro of `macros.bas`; the prev comparison compiles the
  same driver with both compilers, so no executable a compiler writes
  changes. No other library and no executable of `blitz2` or TED changes.
- Tests: host tests of `testconf`, `testpp`, `testjudge`, `caserun`,
  `testlib_core`, the codec; emulator selftests in `tests/blitz2/selftest`;
  the moved tests on both sides.
- Docs: `tests/README.md` ("The run configuration", "Tests that judge
  themselves", "Moving a bug-orig test to the model"),
  `tests/corpus/runner/README.md`, `libs/userlibs/testlib` help texts.
- Overlap: `tests/README.md` and the six old-form test files are also
  touched by `bug-orig-basic-tests` (task 4.1 renames `; @fixed #N` of the
  tests left in the old form, 4.2 rewrites the docs). Order (user, "In
  parallel with U3-U7 (Recommended)"): this change is implemented
  alongside the rounds U3-U7 and its moves land before task 4.1.
