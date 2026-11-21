# Proposal

## Why

The Blitz test programs (the bug-orig tests, `tests/blitz2`) print raw
observations, and the verdict is the diff of that output against
`ID.expect` and the recorded original `ID.orig`. Setup, act and verify
exist only in the header comments ("Here: ... Correct: ..."), the expected
values live outside the program, and outputs that hold addresses need
wildcards and recordings in two memory layouts. User decisions of
2026-11-21 (chat, `task-decisions.md`): the test programs judge themselves
through a test library, in setup / act / verify form, and the library
works with the original compilers as well as with the port.

## What Changes

- **A Blitz test library** (a user library in C with the Blitz
  conventions, library number 81 reserved) and **its macros**: cases,
  checks of the expected behaviour, preconditions, the announcement of an
  expected runtime error, a batch init and a batch end.
- **A test source holds declarations and named cases only**; it is not
  the main program. The runner generates a driver program that defines the
  macros, copies in the text of the test source and calls the selected
  cases by name
  (change `basic-test-runner`).
- **A result file of its own**: the library writes its results (case,
  check, verdict, expected and actual value, the end of each case and of
  the run) to a file the runner names, never to the program's output, so
  `Print`/`NPrint` cannot collide with it; the program does not see the
  library's control data in its arguments.
- **Preconditions and checks are different**: a failed precondition
  (`Require`) means the test is broken, a failed check (`Check`) means the
  tested behaviour is wrong.
- Works with the original blitz2 1.95 (the build of `blitz2.s`), the CD's
  blitz2 2.10 and their libraries, and with the port and the C libraries.

## Capabilities

### New Capabilities

- `testing/basic-testlib`: the test library of Blitz test programs - the
  form of a test source, its statements, the result file, and what the
  library keeps unchanged for the tested program.

### Modified Capabilities

None. The driver, the configuration and the judging are
`testing/basic-test-runner` (change `basic-test-runner`); the bug-orig
tests move to the model in change `bug-orig-basic-tests`.

## Impact

- `libs/userlibs/` (the new library; number 81 reserved in
  `libs/variants/*/libnums.h` and in the documentation of the library
  numbers), the macro text, host tests of the C logic and of the register
  contract.
- `tests/corpus/runner`: staging of the library for every configuration
  (for 1.95 in `blitz2:DIR/` and `blitz2:userlibs/DIR/`).
- `tests/README.md`, `docs/testing.md`: how a test source is written.
- No result of the compilers or of the shipped libraries changes: the
  library is test infrastructure and is not part of `acidlibs`/`deflibs`.
- Prototype: branch `spike/testlib` (commit 595081a3a, not for merge),
  reports `tr-spike1-macros-report.md`, `tr-spike2-testlib-report.md`.
