# Spec Delta

## MODIFIED Requirements

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

### Requirement: A fixed original bug is two cases
A fixed original bug SHALL be two cases whose keys differ in a qualifier: `KEY [orig]` holds the original's behaviour and SHALL be run only against the original (label `originals`); `KEY [fixed #N]` holds the port's behaviour after the fix and SHALL be run only against the port (every run of `all`). `#N` is the issue of the fix. The expected value of a `[fixed #N]` case SHALL be written by an update from the port and reviewed, or written by hand in the same format. No separate note or exception table SHALL exist for it, and staleness needs no special rule: a port that regresses fails `[fixed #N]`, a changed original fails `[orig]`. A case that exists only in the port or only in the original (a fix that adds or removes a case) SHALL have only `KEY [fixed #N]` or only `KEY [orig]` (in the library expected data `KEY [orig #N]`, naming the fix that removes it, `testing/library-capture`). In a test that judges itself, a fixed original bug SHALL instead follow the requirement "A fixed original bug in a test that judges itself".

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

## ADDED Requirements

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
