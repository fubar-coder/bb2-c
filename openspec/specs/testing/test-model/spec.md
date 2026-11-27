# testing/test-model Specification

## Purpose
Defines one model for the tests of this repository that compare program
behaviour with an expectation: a test is a list of cases, each case has a
key, an input and an expected output, and the verdict per case is the diff
of the actual output against the expected one. The model is written for
all such suites; it is implemented for the library tests, the bug-orig tests (change
bug-orig-test-model), the corpus (change corpus-test-model) and the site
examples (one expected file per example, in the machinery of the corpus).
TED already splits fixed tests this way (the `; @fixed #N` tests run with
combination Cc in `all` and with C5 in `originals`, task 9.9 of the
archived change `2026-11-12-expected-results-testing`).

The model has a table form: a table with one row per case, in which a note
row with `#NNN` is the deviation of a fixed original bug. The CD table
`tests/data/blitz2-cd.tsv` (`corpus_cd`, `corpus_cd_ab3`) and the TED
oracle are tests of this form; the table of `ab3_examples` (change
ab3-mode-g5) will be one. No move of them into case files is planned.

Not yet moved, each for a technical reason:
- the command line cases of `tests/ab3` (`ab3_cli`, `ab3_cli_ref`; its
  run-output cases are tests of the model in `tests/ab3/model/`, change
  ab3-test-model): AmiBlitz3's own test programs (fetched, a rewrite would
  check in a derived copy), the compiler's input files (an include or
  resident found by its name next to the main source), the source forms a
  driver cannot have (a tokenised main source with its `.xtra`, a program
  without `End` or one that ends inside a macro line), and the cases
  judged on the compiler's console output and return code, on
  byte-identical executables, on the hunk layout, with other arguments,
  trees or fixed dates: the model has no query of the compiler's console
  text and return code, no `SAME` or `HUNKS`.
- the expect tests of `tests/blitz2` (`NAME.bas` with `NAME.expect`): no
  feature is missing, only the rewrite; the move comes after the change
  expr-tree-5.
- TED A/B (`tests/ted-ab`): the program under test is the editor, driven by
  scripts and judged on dumps, files and logs, and `ted_ab_originals`
  compares runs of the original and the port with each other at test time.
  Missing: a cell that drives TED; it moves with the TED bug-orig tests.

Outside the model, as they do not compare a program's output with a case's
expectation: the host tests, the perf budgets, the comparisons of build
results and data (declarations, asmcmp, tokens, checkdata, rwe numbers and
residents, `ab3_tokens`, `ab3_tokenise`, `ab3_bb2_equivalence`), the self
tests of the test tools and the `lsclear` checker of `tests/bug-port`.

Blitz programs that judge themselves with the test library
(`testing/basic-testlib`) are tests of the model too: their checks are the cases, their expectation is in the source and the
configuration.

## Requirements

### Requirement: A test is a list of cases with expected output
A test SHALL be a list of cases. A case SHALL have a key (unique within the test), an input and an expected output, and SHALL be reported by its key. The verdict of a case SHALL be the difference of the actual output against the expected output; a failure SHALL name the key and show both outputs. The expected output SHALL be part of the test definition (committed data), not something a run derives from the original at test time. An input that is a value (an opcode word, a register set) SHALL be data of the case; an input that is a sequence (a program of a system or hardware library, a TED script, a source file) SHALL be code or an input file the case names. A test that judges itself (requirement "A test that judges itself") SHALL follow that requirement instead of the difference of output.

#### Scenario: One case differs
- **WHEN** a test has many cases and one case's actual output differs from its expected output
- **THEN** only that case fails and the report names its key and both outputs

#### Scenario: A case that is missing or added
- **WHEN** a run produces a key the test does not have, or lacks a key the test has
- **THEN** the test fails naming the key

#### Scenario: A test that judges itself
- **WHEN** a test of the model of `testing/basic-test-runner` prints an additional line and all its checks pass
- **THEN** the test passes; its printed output is judged only by an assertion that names it

### Requirement: Agreement of original and port is one case
A case on which the original and the port agree SHALL be one case with an unqualified key, run against both and judged against the same expected output.

#### Scenario: Both agree
- **WHEN** original and port give the same output for a case
- **THEN** the test has one case for it and both runs are judged against it

### Requirement: A fixed original bug is two cases
A fixed original bug SHALL be two cases whose keys differ in a qualifier: `KEY [orig]` holds the original's behaviour and SHALL be run only against the original (label `originals`); `KEY [fixed #N]` holds the port's behaviour after the fix and SHALL be run only against the port (every run of `all`). `#N` is the issue of the fix. The expected value of a `[fixed #N]` case SHALL be written by an update from the port and reviewed, or written by hand in the same format. No note or exception table separate from the cases SHALL exist for it (in a test of the table form described in the Purpose, the note row is the deviation), and staleness needs no special rule: a port that regresses fails `[fixed #N]`, a changed original fails `[orig]`. A case that exists only in the port or only in the original (a fix that adds or removes a case) SHALL have only `KEY [fixed #N]` or only `KEY [orig]` (in the library expected data `KEY [orig #N]`, naming the fix that removes it, `testing/library-capture`). In a test that judges itself, a fixed original bug SHALL instead follow the requirement "A fixed original bug in a test that judges itself".

#### Scenario: A fix changes a result
- **WHEN** a fix changes the port's output of a case
- **THEN** the case becomes `KEY [orig]` (the original's value, `originals` only) and `KEY [fixed #N]` (the port's value, `all`), and the issue number is in the name

#### Scenario: The port regresses
- **WHEN** the port's output of a `[fixed #N]` case changes back
- **THEN** the test with the port fails on that case

#### Scenario: The original's file changed
- **WHEN** the output of the original for an `[orig]` case differs from the committed value
- **THEN** the test with the original fails on that case

#### Scenario: A test that judges itself
- **WHEN** the bug of a test of the model of `testing/basic-test-runner` is fixed
- **THEN** no `[orig]` / `[fixed #N]` pair is written; the fix sets the fixed issue in the test's configuration

### Requirement: Recording and testing are separate
Writing expected data and testing SHALL be separate operations. To record SHALL mean to write expected data from the original; to update SHALL mean to write expected data from the port. Both SHALL be build targets with hyphenated names (as `corpus-expect-update`), run on purpose, and their results reviewed by `git diff`; a test SHALL only compare and SHALL NOT write committed files. The expectation SHALL be called expected data or recorded original, never "the reference" (`ref` means the original program). A record SHALL accept a result only if two runs with a different memory layout give the same output, and otherwise SHALL report "test not deterministic" naming the cases and write nothing.

#### Scenario: A test never writes
- **WHEN** a test runs
- **THEN** no committed file changes

#### Scenario: Not deterministic
- **WHEN** two recording runs differ in a case
- **THEN** the record reports "test not deterministic" with the differing cases and writes nothing

### Requirement: The judge is a tool with tests
Comparing and accepting SHALL be logic in a C tool under `tools/lib` with host tests from memory and tests with deliberately broken input; CMake SHALL only orchestrate.

#### Scenario: Broken input
- **WHEN** the judge gets a run with one changed case
- **THEN** it fails naming that key

### Requirement: A test that judges itself
A test whose program uses the test library (`testing/basic-testlib`) SHALL
have its checks and assertions as the cases of this model (a case of the
library is a named group of checks). Its expected output SHALL be the
expected values in its source and the assertions of its configuration
(`testing/basic-test-runner`), committed with it; it SHALL have no file of
expected output and no recorded original.

#### Scenario: A check fails
- **WHEN** one check of such a test fails
- **THEN** the report names the library case, the check, the expected and the actual value

### Requirement: A fixed original bug in a test that judges itself
In a test that judges itself, a fixed original bug SHALL be a deviation
(a failing check or an assertion) declared for the cells that hold the
original's faulty part, with the issue of the fix in the configuration; the
other cells SHALL pass. Its expectation SHALL be written in the source and
the configuration, never recorded from a run.

#### Scenario: The port regresses
- **WHEN** a cell without the original's faulty part fails the check of a fixed bug
- **THEN** the test fails naming the check

#### Scenario: The original no longer deviates
- **WHEN** a cell with the original's faulty part passes a check whose failure is declared
- **THEN** the test fails naming the check
