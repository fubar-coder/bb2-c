# Spec Delta

## ADDED Requirements

### Requirement: Bug-orig tests of Blitz programs judge themselves
A bug-orig test whose program runs SHALL be a test of the model of
`testing/basic-test-runner`: its program checks the correct behaviour with
the test library, and its configuration declares the deviations the
original's bug causes. It SHALL have no `ID.expect` and no `ID.orig`. Tests
whose correct result is a compile result only, and the TED bug-orig tests,
SHALL keep their form.

#### Scenario: A moved test
- **WHEN** a bug-orig test has been moved to the model
- **THEN** its directory holds `ID.test.bas` and, if it needs one, `ID.test.toml`, and no `ID.expect` or `ID.orig`

#### Scenario: A compile-only test
- **WHEN** a bug-orig test's correct result is a compile error
- **THEN** it keeps `ID.expect` and `ID.orig`

### Requirement: Deviations are declared for the cells with the faulty part
A deviation SHALL be a failing check or an assertion about the run (such
as a missing end entry), declared with a selector of the cells that hold
the original's faulty part: the CD's libraries for a library bug, the
original compiler for a compiler bug. Such a cell SHALL be REPRODUCED when
it shows exactly the declared deviations and their validators hold; checks
that did not run because of a declared crash SHALL NOT count.

#### Scenario: A library bug
- **WHEN** a test of a library bug declares the failing check "behind" for the cells with the CD's libraries
- **THEN** the cells with the original compiler or the port and the CD's libraries are REPRODUCED by that failure, and the cells with the C libraries must pass

#### Scenario: Another check fails
- **WHEN** such a cell also fails a check that is not declared
- **THEN** the cell fails naming that check

#### Scenario: A declared crash
- **WHEN** a cell's declared deviation is the missing end entry and the program crashes after its first check
- **THEN** the checks after the crash count as not run and the cell is REPRODUCED

### Requirement: A test shows its bug
A test SHALL declare at least one deviation; a test without one SHALL be
BROKEN, open or fixed, since it would not show the bug.

#### Scenario: Nothing declared
- **WHEN** a moved test's configuration declares no deviation
- **THEN** the runner reports the test as BROKEN

### Requirement: An open bug is reproduced by the port
For a test without a fixed issue, the cells without the original's faulty
part SHALL be judged like the cells with it: REPRODUCED when they show
exactly the declared deviations. A case with a declared deviation of an
open test SHALL carry the label `bug-orig`; its other cases SHALL carry
`emulator`.

#### Scenario: Open bug still there
- **WHEN** the C libraries' cell of an open test fails exactly the declared check
- **THEN** the verdict is REPRODUCED and the case runs in the preset `bug-orig`

#### Scenario: Open bug fixed by accident
- **WHEN** the C libraries' cell of an open test passes all its checks
- **THEN** the case fails naming the test as passing without a fixed issue

### Requirement: The fixed issue is one key of three kinds
A fix of an original bug SHALL set `fixed-orig = N` in the test's own
configuration; a fix of another bug that a test of this model shows SHALL
set `fixed = N`; the test of an extension of the port SHALL set
`extension = N`. A test SHALL have at most one of the three. `fixed-orig`
or `extension` without a declared deviation, and `fixed` with one, SHALL
be errors naming the test. The checks and the declared deviations SHALL
stay unchanged by the fix; the test's cases SHALL then carry the label
`emulator`.

#### Scenario: The fix commit
- **WHEN** the original bug of a moved test is fixed
- **THEN** the test's configuration gains `fixed-orig = N` and nothing else of the test changes

#### Scenario: Two of the keys
- **WHEN** a test's configuration sets two of `fixed-orig`, `fixed` and `extension`
- **THEN** the runner fails naming the test

#### Scenario: An extension without a deviation
- **WHEN** a test sets `extension = N` and declares no deviation
- **THEN** the runner fails naming the test

### Requirement: The test of an extension
The test of an extension of the port (issue N: behaviour the original does
not have, where the original refuses or does something else) SHALL
declare as its deviations what the original does instead, for the cells
with the original's part, and SHALL be judged like a test with
`fixed-orig`.

#### Scenario: The test of an extension
- **WHEN** a test sets `extension = N` and declares for the cells with the CD's libraries the failing checks of the original
- **THEN** those cells pass with the declared deviations, the cells with the C libraries pass without them, and its cases carry the label `emulator`

## MODIFIED Requirements

### Requirement: Recorded observation of the original
For every bug-orig test outside the model of `testing/basic-test-runner`
(the TED bug-orig tests, the tests whose correct result is a compile result only, and the tests not moved yet), open or fixed, the original's observation SHALL be committed as `ID.orig` (the sections of the `ref` configuration, raw), written by the recording target `bug-orig-record`, which needs the reference. `ID.orig` is the `[orig]` case of the shared test model (`testing/test-model`) and `ID.expect` the expected case (the `[fixed #N]` case once the bug is fixed) of the same test. The normal run SHALL judge `port` and `clib` against `ID.expect` (FIXED) and, for an open bug, `ID.orig` (REPRODUCED, else DIFFERS) and SHALL NOT start a `ref` session. A fixed test (`; @fixed #N`) SHALL keep its `ID.orig`.

#### Scenario: Open bug still reproduced
- **WHEN** a bug-orig test with an `ID.orig` runs and the port shows what `ID.orig` holds
- **THEN** the verdict is REPRODUCED, as with a live reference session

#### Scenario: Different wrong behaviour
- **WHEN** the port neither meets `ID.expect` nor matches `ID.orig`
- **THEN** the verdict is DIFFERS

#### Scenario: A fixed test keeps its original
- **WHEN** a fix adds `; @fixed #N` to a test
- **THEN** `ID.orig` stays in the commit and the test passes in `all` with FIXED without reading it

#### Scenario: Recording is separate
- **WHEN** the target `bug-orig-record` runs
- **THEN** it writes `ID.orig` next to the test and changes no other file; a test run never writes it

#### Scenario: A fixed test without a recorded original
- **WHEN** a fixed test has no `ID.orig`
- **THEN** configuring fails naming the test

#### Scenario: A test of the model
- **WHEN** `bug-orig-record` runs and a bug-orig test is in the model of `testing/basic-test-runner`
- **THEN** it writes no `ID.orig` for that test
