# Tests left in the old form

A bug-orig test of the **old form** is a program or script (`ID.bas`,
`ID.drv`) with the expected behaviour in `ID.expect` and the recorded
original in `ID.orig`; a test of the **BASIC test model** is `ID.test.bas`
with `ID.test.toml` and no `.expect` or `.orig` (`tests/README.md`, "Tests
that judge themselves"; moving: "Moving a bug-orig test to the model").
Every bug-orig test whose program runs and checks itself has moved. What is
listed here has not, **for now**. For each category this file states the
technical reason (what the model cannot do today) and what is missing to
move it (the feature and its size). The list was asked for in
`~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`, "Group 4:
split the two long bug-fix-workflow requirements; the old form \"for
now\""; `TODO.md` ("Tests left in the old form") points here. Two paths
the user suggested are marked **proposal** (not decided, not planned) in
sections 1 and 2; sizes marked "estimate" are this file's own.

The counts are from the tree at the commit of this file (`.expect` files
of `tests/bug-orig/{compiler,regression}` without `*.broken.expect`, `.drv`
files, `.test.bas` files). A moved test has `ID.test.bas` in the same
directory; the old form has `ID.bas` or `ID.drv`.

| Category | Tests | Technical reason | Missing to move |
|---|---|---|---|
| [1. TED tests](#1-ted-tests) | 41 | the test is a keystroke script played against the editor TED and judged on TED's screen, files and log; the model's program is a compiled Blitz driver | an editor-driving step with its judging (large) |
| [2. Compile-only tests](#2-compile-only-tests) | 71 hand-written, 159 generated | the result is a refused or unrun compile; the model can judge that, but each source must be rewritten for the driver | the moves; a few single gaps (medium in all) |
| [3. Generated expression-matrix tests](#3-generated-expression-matrix-tests) | 205 (46 with a program run, 159 compile-only) | the generator writes the old form; the cost of the model form is not measured | a generator change and a measurement (medium) |
| [4. Self tests of the old runner](#4-self-tests-of-the-old-runner) | 3 Blitz, 5 TED, fixtures | they test code of the old form | nothing; they go with the old form |
| [5. Single tests the model does not express](#5-single-tests-the-model-does-not-express) | 2 | a cell has one run step; a compile log of two jobs has one query | two small features |
| [6. Runnable tests not moved yet](#6-runnable-tests-not-moved-yet) | 3 | none | the move (round U4, task 3.5) |
| [7. Other suites with `.expect`](#7-other-suites-with-expect) | `tests/blitz2` 13, `tests/ted-ab` 3 | not bug-orig tests | outside the change |

`compiler/` has 151 old-form tests and `regression/` 130 (281 in all; the
Blitz self tests of `selftest/` are in section 4 and not counted there).

## 1. TED tests

**What.** `tests/bug-orig/ted/*.drv`, 41 scripts with their `.orig`, for
example `ted-prtfile-umlaut-codes.drv`. A script is played in TED (the
port's TED-C and, for the recording, the original TED 1.1) in an emulator
session by `tedab` in its bug-orig mode (`tests/README.md`, "bug-orig tests
of TED"). `compiler-newallox-fallback.drv` is one of the 41.

**Technical reason.**
- The program under test is the editor, started as `blitz2:blitz2 FILE`
  (or `blitz2:ted FILE`), not a Blitz program of the test. A script step is
  a synthetic input event: keys, menu picks, gadget and requester answers
  put as IntuiMessages into the UserPort of TED's current window
  (`tests/ted-ab/amiga/teddrive.c`, 1371 lines, with its patches of
  intuition's CloseWindow and dos's Open and its wait for TED to be idle).
  A model test is a Blitz driver whose checks run inside the tested
  program (`tests/README.md`, "Tests that judge themselves").
- The judgement is on TED's observable state: screen dumps
  (`PREFIX NAME.scr`), files written to `work/`, teddrive's log, TED's
  stdout and the end of the run (`OK`, `FAIL step N`, `TIMEOUT`, `HALT`),
  matched by the script's `; @expect ...` lines. The model's queries judge
  the compile of the driver, its result file, requesters, return code and
  output (`tests/README.md`, the table of queries); none reads a screen
  dump or a file TED wrote.
- The two sides are the original TED 1.1 and TED-C, both under blitz2 2.10
  (R5, C5). The model's keys are `compiler` (the blitz2 compiler: `port`,
  `orig195`, `orig210`), `libs`, `machine` and the others of
  `tools/lib/testconf/testconf.c`; none selects the TED.
- The sessions depend on the environment the TED tools leave: gate 0
  (`tedprobe`) and gate D (`teddrive`) run first, and seven tests that
  fail allocations depend on the memory layout `tedprobe` leaves (#340;
  `tests/README.md`, "bug-orig tests of TED", "The gates"). A Blitz driver
  running beside TED changes that layout.

**Missing to move.** A step in the model's cell that starts TED and plays
a script into it, a key that selects TED-C or the original TED, and
queries or helper checks for the screen dump, the files and the log; the
gates as a fixture of the model's runs. Large (estimate): a new kind of
cell in the runner (`tests/corpus/runner/caserun.c` and its session
script), three new observations to judge, the gate fixture.

**Proposal (the user's idea, not decided).** "Die Test-Library könnte ja
Funktionen haben, die die ".drv"-Funktionalität nachbildet und in einer
`!RunProcess`-Variante mit übergibt." What it would take:
- *Library functions that replay a script.* Re-implementing teddrive in
  Blitz inside testlib means the 1371 lines above in another language,
  including the library patches and the idle detection, in a library that
  every test links. A smaller variant needs no re-implementation: teddrive
  and `reqtrap` are Amiga programs, so `!RunProgram{"reqtrap teddrive
  SCRIPT PREFIX blitz2:blitz2 FILE"}` would run a script. `!RunProgram`
  starts any shell command line, writes its output to
  `CORPUS:out/run-K.out`, gives the return code and the output and has the
  queries `run.NS.NAME`, `.rc` and `.output` (`tests/README.md`,
  "Test-local data"). Whether a cell's volume holds the two programs and
  TED's `blitz2:` directory is not checked here; the TED session script
  mounts `TEDAB:tools` itself.
- *A `!RunProgram` variant.* It is needed for what a plain `!RunProgram`
  lacks: the start under `reqtrap`, the files teddrive writes handed back
  as fields of `TLRun`, a longer limit. Small to medium (estimate).
- *Judging TED's output files.* The script's `@expect screen`, `file` and
  `log` lines become checks in the Blitz test (reading `PREFIX result`,
  the `.scr` dumps and the log with dos calls and comparing with
  `!CheckStrEq`), or new host-side queries. This is the larger part.
- *What stays missing in any case.* The key for original TED against
  TED-C, the gates, the layout dependence above.

## 2. Compile-only tests

**What.** Tests whose correct result is a compile result: an error with a
line and a message (the compiler refuses the input), or a compile that ends
without a program to run, judged by the compiler's own output (`[compile]`,
`[blitz2]`, `[log]`, `[session]`). 71 are hand-written:

| Area | Judged by | Tests | Example |
|---|---|---|---|
| `compiler/` | `[compile]` | 50 | `compiler-newtype-self-field` |
| `compiler/` | `[compile]` and `[blitz2]` (inject lines) | 9 | `compiler-incbin-leaks` |
| `compiler/` | `[blitz2]` | 4 | `compiler-flushlibs-leak` |
| `compiler/` | `[log]` (tedstub log) | 3 | `compiler-createproc-fail` |
| `compiler/` | `[session]` and `[blitz2]` | 1 | `compiler-wbexit-returncode` |
| `regression/` | `[compile]` | 3 | `compiler-data-quick-over` |
| `regression/` | `[compile]`, `[blitz2]`, `[log]` | 1 | `compiler-macrobuff-nomem-limit` |

(The grouping is by the sections of the `.expect` files; the example is one
file of the group, to be checked there.) Three of the `[log]` and `[blitz2]`
tests drive the compiler through the editor (`run=` or `menu=` of tedstub):
`compiler-createproc-fail`, `compiler-data-pad-byte`,
`compiler-loaddmod-pointer`. The generated compile-only tests (159) are in
section 3.

**Technical reason.** A model cell compiles the driver (the test source
plus generated call code; the lines of the source stay, `tools/lib/testpp/testpp.h`)
and runs it. What a refused compile can do in that frame, checked in the
code:
- The query `compile` (`ok`, `error LINE MESSAGE`, `none`) has no default;
  the defaults are `rc == 0`, `requester count == 0`, `end.case` and
  `end.run exists` (`tools/lib/testconf/testassert.c:527`). So a driver that
  does not compile fails the test only through those defaults.
  `[asserts]` accepts `compile contains "..."` on the port side; with
  `end.case`, `end.run` and `rc` set to `not exists` the test passes: the
  judge's "no result file" is a message
  (`tools/lib/testjudge/testjudge.c:672`), not a verdict. This is in use:
  `compiler-loadres-clash-werror` asserts a port-side compile error
  (`compile = "contains \"Constant Not Found\""`) that way, and
  `compiler-macro-end-line` and `compiler-quick-literal-data` declare the
  original compiler's refusal as a deviation of `compiler=orig195`.
- The error is reported at its line of `ID.test.bas`
  (`cr_compile_mapped`, `tests/corpus/runner/caserun.c:645`), so
  `error 9 Illegal Operator for Type` of an old `.expect` keeps its line.
  The runner reports the same error as "compile error" in its output; the
  verdict is the judge's.
- A refused source can therefore be a model test in form. What differs from
  the old form, per test:
  - the source has to fit the driver: outside the blocks only
    declarations are allowed, so every executable statement, `Include`
    and call of the source's own macros moves into a `Case` block
    (`tests/README.md`, "Outside the blocks only declarations"), and an
    error that depends on the position in the whole program (end-of-source
    errors, buffer sizes that the driver's own code consumes) changes with
    the driver;
  - the recorded `ID.orig` has no model counterpart: the original's result
    is declared by hand as a deviation of `compiler=orig195`, per test;
  - a source that compiles and is judged by the compiler's output
    (`[blitz2]`, `[log]`) is a cell whose driver runs with no case
    checks; `compile.inject` and `compile.log` exist for it
    (`tests/blitz2/selftest/model-compile-inject`);
  - the three editor tests need an editor action (`menu=`, item 9 of the
    compiler menu): the model's `editor-run = "on"` runs the driver from
    COMPILE/RUN, it does not play menu items.
- For the prev comparison a model test compares the driver's executable or
  compile result with the previous commit's (`cmake/prev.cmake`,
  `prev_compile_model_test`), so a refused source is compared as a compile
  result.

**Missing to move.** For the 50 plain `[compile]` tests and, as far as
checked, the `[blitz2]` ones, no model feature: the move of each (the
source into `Case` blocks, a toml with the assertions and the original's
deviations, a check that the verdict stays on both sides). Missing for the
rest: a menu action in the cell (the three editor tests) and one query per
compiler job (section 5). Size: the number of tests (71 and the 159 of
section 3) times a small rewrite each, plus the two features (estimate:
medium).

**Proposal (the user's idea, not decided).** "Die Test-Funktion könnte ja
den Compiler aufrufen und die Compiler-Ausgabe prüfen (hoffentlich,
zumindest)?" What it would take:
- *The port.* `blitz2-ab3 -s SOURCE -e EXE [-r] [-mode bb2]` compiles one
  source without the editor, prints a compile error as `FILE:LINE:
  MESSAGE` and returns 10 (5 with warnings, 20 for an argument error;
  `docs/ab3/usage.md`). A test can start it:
  `*r.TLRun = !RunProgram{"blitz2:blitz2-ab3 -s CORPUS:ID.test/bad.bas -e
  RAM:bad"}`, then `!CheckEq` on `*r\rc` and `!CheckStrEq` on the output,
  or the assertions `run.NS.NAME.rc` and `.output`. That needs no new
  feature; that `blitz2-ab3` is in the cell's `blitz2:` volume is not
  checked here.
- *The originals.* The original blitz2 has no compile-from-shell mode: a
  CLI start takes one argument, the file the editor loads, and always
  returns 0 (`programs/blitz2/src/main.c:617`, `docs/blitz2/deviations.md`,
  "blitz2's exit"); its headless compile is the editor stub `tedstub` with
  a job list, which is what a cell already does. So for the original
  compiler the call from the test does not exist; the route that does is a
  data source: a file of `ID.test/` with the action `basic-compile` is
  compiled by the cell's compiler in its session, and
  `[asserts] "data.FILE" = "contains \"...\""` judges the compile
  (`tests/README.md`, "Test-local data"; `data.FILE` gives `ok`, `error
  LINE MESSAGE` or `none`, with the keys `compiler`, `libs` and
  `runtime-errors` of the action). `compiler-fold-buffer-full` uses it on
  the original side (`"data.child.bas" = "== none"`); no test asserts an
  error of a data source on the port side yet (the grammar allows it).
  The test then holds a trivial case so that the driver exists. Small
  (estimate), no feature.

## 3. Generated expression-matrix tests

**What.** The tests written by `exprmatrix expect` (generator
`tools/lib/exprmatrix`, command `tools/exprmatrix`, description
`tests/corpus/tests-exprmatrix/matrix.txt`, target `expr-matrix-update`,
check `exprmatrix_committed`; `tools/exprmatrix/README.md`, "Outputs"):

| Files | Tests | Program run | Example |
|---|---|---|---|
| `compiler/compiler-expr-ISSUES-CTX-SET[-u\|-c]-NN` | 46 | yes (`[compile] ok`, `[run]`) | `compiler-expr-1178-1189-dl-B-01` |
| `compiler/compiler-expr-ISSUES-refused-OP-L-R[-c]` and 3 more of the 82 files that expect an error | 36 | no (`[compile] error`) | `compiler-expr-1179-refused-and-Q-C` |
| `regression/compiler-expr-refused-OP-L-R[-c]` | 123 | no (`[compile] error`) | `compiler-expr-refused-and-C-C` |

The 82 files of `compiler/compiler-expr-*` are 46 + 36. The three
`regression/compiler-expr-depth*` tests are hand-written (the generator
has no depth cells); `compiler-expr-depth` and `compiler-expr-depth-d7`
are in section 6, `compiler-expr-depth-prec` (`[compile]` only) in
section 2.

**Technical reason.** Two, one per group.
- The 159 with `[compile] error` are compile-only tests: each is one small
  source with one expected compile error (the example
  `compiler-expr-refused-and-C-C` expects one `[compile] error` line);
  their reason and gaps are those of section 2.
- The 46 with a run are expressible in the model, but the generator
  (`tools/lib/exprmatrix`) writes `ID.bas`, `ID.expect` and `ID.orig`.
  A test is one source of about 28 lines with about 8 cells
  (`compiler-expr-1178-1189-dl-B-01`: 28 lines, 8 cells, one `[run]`). As model tests, the cells of a source would
  be cases, and the model registers one ctest test per case and side with
  an emulator session each (`tests/README.md`, "ctest still runs one case
  per test"; the run of all cases shares one run of the driver only
  inside one invocation), so 8 cells would be 8 ctest tests instead of one,
  and every driver carries testlib. The cost is not measured here; the
  tool for it is `testinfo costs` (`tools/testinfo/cmd_costs.cpp`) on both
  forms.

**Missing to move.**
- For the 46 with a run: a decision on the form (one case per cell, or one
  case per source with a `!Check` per cell, which keeps one ctest test per
  source), a measurement that the model form is not slower, then the
  generator writes `ID.test.bas` and `ID.test.toml` (the original's
  per-cell values as deviations) instead of `ID.bas`, `ID.expect` and
  `ID.orig` (a `.orig` is recorded by `bug-orig-record` today), with its
  host tests `test_exprmatrix*.cpp`. Medium (estimate).
- For the 159 compile-only: the moves of section 2.

EOF
## 4. Self tests of the old runner

**What.** Tests of the old form's own machinery, with deliberately broken
input (`tests/bug-orig/selftest/`; the CMake side is
`tests/bug-orig/CMakeLists.txt`):

- 3 Blitz tests: `bug-orig-selftest-broken` (an expectation equal to what
  the original does: the runner must say BROKEN, exit status 3),
  `regression-selftest-wrong` (a regression expectation the port does
  not meet: REGRESSED, exit status 1), `regression-selftest-machines`
  (`@machine` with two names: one ctest per machine).
- 5 TED scripts: `ted-bug-orig-selftest-broken`,
  `ted-fixed-selftest-fixed`, `ted-fixed-selftest-regressed`,
  `ted-fixed-port-selftest-fixed`, `ted-fixed-port-selftest-regressed`;
  with the directories `gates-broken` (2 scripts), `ted-fixed-wrong` (8),
  and the fixtures of the configure-time checks of the marker
  (`fixed-old-name`, `fixed-orig`, `fixed-orig-ok`) and of the recorded
  files (`garbage.orig`, `wildcard-noreason.orig`, `record-changed.orig`,
  `ted-record-changed.orig`, `ted-wildcard-noreason.orig`).
- The model's own self tests are not here: `selftest/model-no-deviation.test.bas`
  is a model test.

**Technical reason.** They test code of the old form: the verdicts BROKEN
and REGRESSED that `corpusrun` and `tedab` derive from `.expect` and
`.orig`, and the checks of the old-form header. A model version would test
the model's runner instead (which has its own self tests in
`tests/blitz2/selftest`).

**Missing to move.** Nothing: they go when the old form goes. While the
TED tests and compile-only tests are in the old form, the self tests stay.
Not bug-orig tests but of the same kind are the self tests of the helper
tools at the top of `tests/bug-orig/` (`inject-*.cmake`, `wbrun-*.cmake`
with 12 `.expect` files): they compare a helper's output with an expected
file and have no model counterpart.

## 5. Single tests the model does not express

- **`compiler/compiler-panic-remtask`** (#24, by design, open on purpose).
  Technical reason: the test needs two runs in order in one session: an
  editor run (COMPILE/RUN) that PANIC hits, then a shell run
  (`runargs=check`) that tests whether an exclusive lock was left behind.
  A model cell has one run step after the driver's compile: the shell run
  of the driver, or, with `editor-run = "on"`, the editor run in its
  place (`tests/README.md`, "A run from the editor"); the result file
  and the assertions belong to that one run. The checking program of the
  second run cannot be the driver's `!RunProgram`, because the driver is
  the process PANIC hits. Missing: a second run step in the cell script
  (`tests/corpus/runner/caserun.c`, `corpus_script_run`), a second
  observation (return code, output) and a way for assertions to name the
  run. Medium (estimate). Tasks 3.4 of `basic-test-editor-run` and 3.5 of
  `bug-orig-basic-tests` stay open for this test.
- **`compiler/compiler-racing-stripe-wide`** (#980). Compiles its own
  source a second time (a `jobline` job with `@DRIVER@`); the compiler log
  holds one block per job, each after a line `RESULT j000 ok 10204`, and
  the number is the executable's size, which changes with the driver and
  testlib. One `compile.log contains` cannot match both blocks without
  it, and there is one assertion per query. Missing: a query per job
  (`compile.log.j001`; a small change of the query grammar in
  `tools/lib/testconf/testassert.c`, the judge and `tests/README.md`), or
  an assertion that ignores the size. Small (estimate). Open question
  Q-A20 (`open-questions.md`) holds the options.

## 6. Runnable tests not moved yet

`regression/ldivlib-div0-check-fold`, `regression/compiler-expr-depth` and
`regression/compiler-expr-depth-d7` have a program run, are hand-written
and fit the model (class (a) of the planning classification names the
other `regression/` tests with a run as expressible). They are old-form
tests only because round U4 (`openspec/changes/bug-orig-basic-tests`, task
3.5) is not finished. Nothing in the model blocks them. Risk for the
mover: the driver's own text and code are compiled with the test, so a
test that sits on a compiler limit may need re-tuning (the planning
classification, "Risks for the movers"; `compiler-expr-depth` tests the
depth of expressions).

## 7. Other suites with `.expect`

- `tests/blitz2/*.expect`: 13 port tests of the compiler (7 with
  `[run]`, 6 compile-only), next to 10 model tests in the same directory
  (`*.test.bas`). They are not bug-orig tests and not part of the change
  `bug-orig-basic-tests`; they are listed so that the count of old-form
  tests is complete. No decision on moving them is recorded.
- `tests/ted-ab/scripts`: 51 `.drv` scripts and 3 `.expect` files of the
  TED A/B suite (original TED against the port; `tests/README.md`, "TED
  A/B"). Own suite with its own runner; the fixed scripts carry
  `; @fixed-orig #NNN PAIR...`. It belongs with section 1 in what it needs
  to move.
- `tests/bug-port` and `libs/tests/bugorig.c` (snitch tests) are neither
  old-form nor model tests of this kind; see "bug-port tests" in
  `tests/README.md`.
