# Spec Delta

## Purpose
Defines one model for the tests of this repository that compare program
behaviour with an expectation: a test is a list of cases, each case has a
key, an input and an expected output, and the verdict per case is the diff
of the actual output against the expected one. The model is written for
all suites (library tests, blitz2 corpus, bug-orig tests, TED); this change
implements it for the library tests only, bug-orig and the corpus follow in
later changes. TED already splits fixed tests this way (the `; @fixed #N`
tests run with combination Cc in `all` and with C5 in `originals`, task 9.9
of the archived change `2026-11-12-expected-results-testing`).

## ADDED Requirements

### Requirement: A test is a list of cases with expected output
A test SHALL be a list of cases. A case SHALL have a key (unique within the test), an input and an expected output, and SHALL be reported by its key. The verdict of a case SHALL be the difference of the actual output against the expected output; a failure SHALL name the key and show both outputs. The expected output SHALL be part of the test definition (committed data), not something a run derives from the original at test time. An input that is a value (an opcode word, a register set) SHALL be data of the case; an input that is a sequence (a program of a system or hardware library, a TED script, a source file) SHALL be code or an input file the case names.

#### Scenario: One case differs
- **WHEN** a test has many cases and one case's actual output differs from its expected output
- **THEN** only that case fails and the report names its key and both outputs

#### Scenario: A case that is missing or added
- **WHEN** a run produces a key the test does not have, or lacks a key the test has
- **THEN** the test fails naming the key

### Requirement: Agreement of original and port is one case
A case on which the original and the port agree SHALL be one case with an unqualified key, run against both and judged against the same expected output.

#### Scenario: Both agree
- **WHEN** original and port give the same output for a case
- **THEN** the test has one case for it and both runs are judged against it

### Requirement: A fixed original bug is two cases
A fixed original bug SHALL be two cases whose keys differ in a qualifier: `KEY [orig]` holds the original's behaviour and SHALL be run only against the original (label `originals`); `KEY [fixed #N]` holds the port's behaviour after the fix and SHALL be run only against the port (every run of `all`). `#N` is the issue of the fix. The expected value of a `[fixed #N]` case SHALL be written by an update from the port and reviewed, or written by hand in the same format. No separate note or exception table SHALL exist for it, and staleness needs no special rule: a port that regresses fails `[fixed #N]`, a changed original fails `[orig]`. A case that exists only in the port or only in the original (a fix that adds or removes a case) SHALL have only `KEY [fixed #N]` or only `KEY [orig]`.

#### Scenario: A fix changes a result
- **WHEN** a fix changes the port's output of a case
- **THEN** the case becomes `KEY [orig]` (the original's value, `originals` only) and `KEY [fixed #N]` (the port's value, `all`), and the issue number is in the name

#### Scenario: The port regresses
- **WHEN** the port's output of a `[fixed #N]` case changes back
- **THEN** the test with the port fails on that case

#### Scenario: The original's file changed
- **WHEN** the output of the original for an `[orig]` case differs from the committed value
- **THEN** the test with the original fails on that case

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
