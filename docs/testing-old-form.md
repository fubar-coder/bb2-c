# Tests left in the old form

A bug-orig test of the **old form** is a program or script (`ID.bas`,
`ID.drv`) with the expected behaviour in `ID.expect` and the recorded
original in `ID.orig`; a test of the **BASIC test model** is `ID.test.bas`
with `ID.test.toml` and no `.expect` or `.orig` (`tests/README.md`, "Tests
that judge themselves"; moving: "Moving a bug-orig test to the model").
Every bug-orig test whose program runs and checks itself has moved. What is
listed here has not, **for now**: it does not fit the model today, or it
waits for a model feature or a decision. This is not a statement that it
stays in the old form for good; the user's decision is
`~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`, "Group 4:
split the two long bug-fix-workflow requirements; the old form \"for
now\"". This file is the list the decision asks for; `TODO.md` ("Tests left
in the old form") points here.

The counts are from the tree at the commit of this file (`.expect` files
of `tests/bug-orig/{compiler,regression}` without `*.broken.expect`, `.drv`
files, `.test.bas` files). A moved test has `ID.test.bas` in the same
directory; the old form has `ID.bas` or `ID.drv`.

| Category | Tests | Moves when |
|---|---|---|
| [1. TED tests](#1-ted-tests) | 41 | a model feature for editor scripts (not planned) |
| [2. Compile-only tests](#2-compile-only-tests) | 71 hand-written, 159 generated | a model feature for a compile result (needs a decision) |
| [3. Generated expression-matrix tests](#3-generated-expression-matrix-tests) | 205 (46 with a program run, 159 compile-only) | the generator condition, below |
| [4. Self tests of the old runner](#4-self-tests-of-the-old-runner) | 3 Blitz, 5 TED, fixtures | the old form is gone |
| [5. Single tests the model does not express](#5-single-tests-the-model-does-not-express) | 2 | a two-run mode (not planned); open question Q-A20 |
| [6. Runnable tests not moved yet](#6-runnable-tests-not-moved-yet) | 3 | round U4 (task 3.5) |
| [7. Other suites with `.expect`](#7-other-suites-with-expect) | `tests/blitz2` 13, `tests/ted-ab` 3 | outside the change |

`compiler/` has 151 old-form tests and `regression/` 130 (281 in all; the
Blitz self tests of `selftest/` are in section 4 and not counted there).

## 1. TED tests

**What.** `tests/bug-orig/ted/*.drv`, 41 scripts with their `.orig`, for
example `ted-prtfile-umlaut-codes.drv`. A script is played in TED (the
port's TED-C and, for the recording, the original TED 1.1) in an emulator
session by `tedab` in its bug-orig mode (`tests/README.md`, "bug-orig tests
of TED"). `compiler-newallox-fallback.drv` is one of the 41.

**Why it does not fit.** A model test is a Blitz source that checks itself
with the test library (`tests/README.md`, "Tests that judge themselves");
a TED test has no Blitz program to check: its steps are keystrokes and
expectations on the editor's state. The change keeps it out on purpose
(`openspec/changes/bug-orig-basic-tests/design.md`, Non-Goals: "The TED
bug-orig tests"; spec `testing/bug-orig`: "the TED bug-orig tests, SHALL
keep their form").

**What would let it move.** A model feature that drives the editor from
a test description and judges its state. None is planned; it would need a
decision and its own OpenSpec change.

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

**Why it does not fit.** A model test is compiled with its driver and must
compile; the runner reports a compile error in the test's text as a failure
of the test (spec `testing/basic-test-runner`, scenario "A compile error in
the test's text"). A source that must be refused cannot be a model test.
The three editor tests are named as compile-only tests that stay with the
others in the planning of the editor-run change
(`openspec/changes/basic-test-editor-run/proposal.md`, "Not in scope"); the
change's classification of the U4 tests is class (d), "compile-only, stay
in the old form (230)" (`~/.local/share/bb2-c-work/editor-run-u4-classification.md`;
the 230 are the 71 here and the 159 of section 3).

**What would let it move.** A model feature that expects a compile result
(a refusal with line and message, a compile that does not run) instead of
a run, with the verdict kept on both sides (recorded original and port).
It needs a decision; the spec `testing/bug-orig` says now that a bug-orig
test whose correct result is a compile error keeps `ID.expect` and
`ID.orig`.

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

**Why it does not fit.** Two reasons, one per group. The 159 with
`[compile] error` are compile-only tests (section 2). The 46 with a run
are expressible in the model; the user's decision is to keep them in the
old form until moving them costs no performance: "Die Generator-Tests
würde ich gerne auch in der neuen Form sehen, allerdings erst, wenn man
mehrere Tests auf einmal ausführen kann. Und wenn der Start der Tests
durch cmake ebenfalls gebündelt werden kann. Mit anderen Worten: Sofern
die Tests ohne Performance-Nachteile in das neue Format übertragen werden
können." (decisions.md, "Group 4: split the two long bug-fix-workflow
requirements; the old form \"for now\""). Class (c) of the planning
classification names them as a generator change, not a model gap
(`editor-run-u4-classification.md`; its count of 48 includes two
hand-written depth tests, the generated ones with a run are 46).

**What would let it move.**
- For the 46 with a run: several model tests can run in one emulator
  session, and the start of the tests through CMake and ctest can be
  bundled (one start for many tests), so that 46 tests as model tests are
  not slower than the 46 in the old form; then the generator
  (`tools/lib/exprmatrix`, `exprmatrix expect`, with its host tests
  `test_exprmatrix*.cpp`) writes `ID.test.bas` and `ID.test.toml` instead
  of `ID.bas`, `ID.expect` and `ID.orig` (a `.orig` is recorded by
  `bug-orig-record` today).
- For the 159 compile-only: the model feature of section 2.

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

**Why it does not fit.** They test the old form: the verdicts BROKEN and
REGRESSED of `.expect` and `.orig` and the checks of the old-form header
(planning classification, class (c): "they test the old form, which stays
for TED and the compile-only tests"). A model version would test
something else.

**What would let it move.** Nothing: they go when the old form goes. While
TED tests and compile-only tests are in the old form, the self tests stay.
Not bug-orig tests but of the same kind are the self tests of the helper
tools at the top of `tests/bug-orig/` (`inject-*.cmake`, `wbrun-*.cmake`
with 12 `.expect` files): they compare a helper's output with an expected
file and have no model counterpart.

## 5. Single tests the model does not express

- **`compiler/compiler-panic-remtask`** (#24, by design, open on purpose).
  Two runs in a row: an editor run (COMPILE/RUN) that PANIC hits, then a
  shell run (`runargs=check`) that tests whether an exclusive lock was
  left behind. The model's `editor-run` setting replaces the shell run; it
  does not run both, and the model has no two-run mode. Decided: it stays
  in the old form for now, and no `editor-run = "before"` mode is added
  (`~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`, "Q-A2
  follow-up: reopen #1020; Q-A4: panic-remtask stays in the old form for
  now"; the question: `open-questions.md`, Q-A4). Tasks 3.4 of
  `basic-test-editor-run` and 3.5 of `bug-orig-basic-tests` stay open for
  this test. What would move it: a two-run mode (an editor run followed by
  a judged shell run), which needs a new decision.
- **`compiler/compiler-racing-stripe-wide`** (#980). Compiles its own
  source a second time (a `jobline` job with `@DRIVER@`); the compiler log
  holds one block per job, each after a line `RESULT j000 ok 10204`, and
  the number is the executable's size, which changes with the driver and
  testlib. One `compile.log contains` cannot match both blocks without
  it, and there is one assertion per query. Open question Q-A20
  (`open-questions.md`), recommendation (a): an additional query per job
  (`compile.log.j001`); "no (old form kept)" in the table of the open
  questions. What moves it: that query (a small spec change) or option
  (b) of Q-A20.

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
