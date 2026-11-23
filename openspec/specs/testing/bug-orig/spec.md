# testing/bug-orig Specification

## Purpose
Defines how tests of original bugs are judged during the bug-fix phase:
which tests run where, what a verdict means, and how the original's
behaviour is documented without a reference session on every run.

## Requirements

### Requirement: Fixed bug-orig tests run in the normal presets
A bug-orig test whose header holds `; @fixed-orig #NNN` SHALL carry the label `emulator` and not `bug-orig`, SHALL run in `emulator` and `all` without a `ref` session, and SHALL pass only if every judged configuration is FIXED. A fix SHALL add the line in the commit that fixes the bug. The marker's old name, a line `; @fixed #NNN`, SHALL be refused with an error naming the new form.

#### Scenario: A fixed test is in all
- **WHEN** `ctest --preset all -N` lists the tests after a bug was fixed and its test got `; @fixed-orig #NNN`
- **THEN** the test is listed, and `ctest --preset bug-orig -N` does not list it

#### Scenario: A fix regresses
- **WHEN** a later change makes a `; @fixed-orig` test's configuration show the bug again
- **THEN** the test fails in `ctest --preset all` and the verdict names the configuration

#### Scenario: No reference needed
- **WHEN** a `; @fixed-orig` test runs on a machine without the CD
- **THEN** it runs (or skips only for missing Amiberry), because it starts no `ref` session

#### Scenario: The old name
- **WHEN** the header of a bug-orig test or a line of a TED script holds `; @fixed #NNN`
- **THEN** configuring (a script of `tests/ted-ab`: its lint and its run) fails naming the test and the new form `@fixed-orig`

### Requirement: Recorded observation of the original
For every bug-orig test outside the model of `testing/basic-test-runner`
(the TED bug-orig tests, the tests whose correct result is a compile result only, and the tests not moved yet), open or fixed, the original's observation SHALL be committed as `ID.orig` (the sections of the `ref` configuration, raw), written by the recording target `bug-orig-record`, which needs the reference.

#### Scenario: Open bug still reproduced
- **WHEN** a bug-orig test with an `ID.orig` runs and the port shows what `ID.orig` holds
- **THEN** the verdict is REPRODUCED, as with a live reference session

#### Scenario: Different wrong behaviour
- **WHEN** the port neither meets `ID.expect` nor matches `ID.orig`
- **THEN** the verdict is DIFFERS

#### Scenario: A fixed test keeps its original
- **WHEN** a fix adds `; @fixed-orig #N` to a test
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

### Requirement: Validity of a test is checked on the recorded file
The test runner SHALL give the verdict BROKEN when `ID.orig` matches `ID.expect`, for open and for fixed tests, since the test then would not show the bug. This SHALL be checked without a reference session.

#### Scenario: Wrong expectation
- **WHEN** an `ID.expect` is edited so that `ID.orig` matches it
- **THEN** the validity check (`bug_orig_selftest_broken` form) reports BROKEN

#### Scenario: A fixed test with an equal pair
- **WHEN** a fixed test's `ID.orig` matches its `ID.expect`
- **THEN** the validity check reports BROKEN

### Requirement: Labels of known failures
Tests of open original bugs SHALL carry the label `bug-orig` and tests of known port defects `bug-port`; both SHALL stay out of `host`, `emulator` and `all` and run in the preset `bug-orig`. A `; @fixed` test SHALL carry neither.

#### Scenario: Preset contents
- **WHEN** `ctest --preset bug-orig -N` lists the tests
- **THEN** it lists only tests with open bugs (verdict REPRODUCED or DIFFERS expected) and known port defects

### Requirement: Recording and testing are separate
The ctest test `bug_orig_record_ID` (label `originals`) SHALL only compare: it observes the `ref` configuration, matches the observation against the committed `ID.orig` (wildcard lines included), SHALL fail naming the section and the first differing line, and SHALL NOT write any file of the source tree.

#### Scenario: A test never writes
- **WHEN** `ctest --preset originals` runs
- **THEN** no file under `tests/bug-orig/` changes

#### Scenario: The original changed
- **WHEN** the original's observation differs from the committed `ID.orig`
- **THEN** `bug_orig_record_ID` fails naming the differing line, and `ID.orig` is unchanged

### Requirement: The recording target accepts only agreeing runs
The build target `bug-orig-record` (not part of ALL, not a test, filterable by test id) SHALL write `ID.orig` from the original. It SHALL run the `ref` configuration twice with a different memory layout (the second run with a Stack larger by a fixed amount, so that what the original allocates lies elsewhere; nothing else of the run changes) and SHALL write a file only if both observations agree; otherwise it SHALL report "test not deterministic" naming the test and the differing sections and lines, and SHALL write nothing for that test. A run limited to some tests SHALL leave the files of the others untouched. The proof of determinism is limited to the memory layout.

#### Scenario: Deterministic recording
- **WHEN** the two runs of `bug-orig-record` give the same observation
- **THEN** `ID.orig` is written and nothing else changes

#### Scenario: Not deterministic
- **WHEN** the two runs differ in a section
- **THEN** `bug-orig-record` reports "test not deterministic" with the test and the section, and the committed file stays as it was

#### Scenario: A filtered recording
- **WHEN** `bug-orig-record` runs for one test id
- **THEN** the `ID.orig` of every other test is neither read as missing nor changed

### Requirement: Legitimately varying lines are wildcards with a reason
A line of `ID.orig` that varies from run to run for a reason outside the tested behaviour SHALL be a wildcard line of the expectation's kind (`*` at the end of a line, `...` for any number of lines), directly preceded by a comment line `# varies: REASON`. A wildcard line without that comment SHALL be an error of the runner, reported with file and line. A rewrite by `bug-orig-record` SHALL keep the wildcard lines of the file where both runs match them and SHALL report a wildcard that does not.

#### Scenario: The four known files
- **WHEN** the `.orig` files of `blitzcoplib-maxslice-zero`, `bb_blitlib-createshape-count0`, `rigfxlib-cpucls-no-plane-crash` and `rishapeslib-read-palette-colour-count` are read
- **THEN** each has its varying line as a wildcard with a `# varies:` reason, and runs of the original match it

#### Scenario: Wildcard without reason
- **WHEN** an `ID.orig` holds a wildcard line with no `# varies:` line before it
- **THEN** the runner fails naming the file and line

### Requirement: The checks of this model have tests with broken input
Every check of the recording model SHALL have a test with deliberately broken input: the compare of the record test on a changed `ID.orig`, a non-deterministic recording refused with nothing written, a wildcard line without reason, a fixed test without `ID.orig`, a fixed test whose `ID.orig` equals `ID.expect`, a wildcard that does not match both runs in a rewrite.

#### Scenario: Broken input
- **WHEN** the acceptance gets two runs that differ in one section
- **THEN** it fails naming the test and the section and writes no committed file

### Requirement: A hanging program ends its session early in the normal run
In the normal run of a bug-orig test (port and clib sessions) a session whose first program run has started and whose job has changed no file for `BUG_ORIG_RUN_HANG_AFTER` seconds SHALL end like a session that reached its time limit (state `timeout`, same observation). The silence SHALL count from the first program run, never from the start of the job, so a compile that is silent for long on a loaded host SHALL NOT end a session. A test that only compiles (`run=off`) SHALL have no such check. A test whose program is silent for longer SHALL set its own limit (`; @pass hang_after=N`). The verdict of every bug-orig test SHALL be the same as with the time limit alone.

#### Scenario: The program crashes into the alert
- **WHEN** the program of an open bug crashes and the machine stays in the alert
- **THEN** the session ends after the silence, not at the limit, and the verdict is the one a run to the limit gives

#### Scenario: A slow compile
- **WHEN** the compile is silent for longer than the silence limit and the program then runs normally
- **THEN** the session is not ended early

#### Scenario: A pass that cannot arm
- **WHEN** a pass file has `hang_from=run` without `hang_after=`, or with `run=off`
- **THEN** the runner refuses it

### Requirement: The TED gates are checked once per run
The gates of the TED bug-orig sessions (gate 0 and gate D) SHALL run once per ctest run, in a fixture, and not in every session. The TED bug-orig tests SHALL stay single ctest entries that require the fixture; with the fixture's mark their sessions SHALL leave out gate D and the evaluation of gate 0 (the probe program of gate 0 still runs, the memory layout of the tests depends on it), without it (the runner started by hand) they SHALL run the gates as before. A gate that fails SHALL fail the fixture's setup test, publish no mark, and ctest SHALL NOT run the tests that require it.

#### Scenario: Gates fine
- **WHEN** ctest runs several TED bug-orig tests
- **THEN** the gates run in one session of the setup test and the sessions of the tests have none

#### Scenario: A gate that fails
- **WHEN** gate D fails (a script that fails on purpose in the self test)
- **THEN** the gates pass ends with exit status 1 and publishes nothing

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
`emulator`. On the side `originals` a REPRODUCED case SHALL pass (exit
status 0, reported as REPRODUCED as declared); a cell there without its
declared deviations SHALL fail it.

#### Scenario: Open bug still there
- **WHEN** the C libraries' cell of an open test fails exactly the declared check
- **THEN** the verdict is REPRODUCED and the case runs in the preset `bug-orig`

#### Scenario: Open bug fixed by accident
- **WHEN** the C libraries' cell of an open test passes all its checks
- **THEN** the case fails naming the test as passing without a fixed issue

#### Scenario: The original shows the open bug
- **WHEN** the cells of the side `originals` of an open test show exactly the declared deviations
- **THEN** the case passes on that side with exit status 0 and the report says "REPRODUCED (as declared, side originals)", while the side `port` of the same case still fails as REPRODUCED

#### Scenario: The original does not show the declared deviation
- **WHEN** a cell of the side `originals` of an open test passes the check its declared deviation says fails
- **THEN** the case fails on that side naming the deviation that does not hold

### Requirement: The fixed issue is one key of three kinds
A fix of an original bug SHALL set `fixed-orig = N` in the test's own
configuration; a fix of another bug that a test of this model shows SHALL
set `fixed = N`; the test of an extension of the port SHALL set
`extension = N`. A test SHALL have at most one of the three.

#### Scenario: The fix commit
- **WHEN** the original bug of a moved test is fixed
- **THEN** the test's configuration gains `fixed-orig = N` and nothing else of the test changes

#### Scenario: Two of the keys
- **WHEN** a test's configuration sets two of `fixed-orig`, `fixed` and `extension`
- **THEN** the runner fails naming the test

### Requirement: A fixed issue key needs matching deviations
`fixed-orig` or `extension` without a declared deviation, and `fixed` with
one, SHALL be errors naming the test.

#### Scenario: An extension without a deviation
- **WHEN** a test sets `extension = N` and declares no deviation
- **THEN** the runner fails naming the test

#### Scenario: A fixed issue with a deviation
- **WHEN** a test sets `fixed = N` and declares a deviation
- **THEN** the runner fails naming the test

### Requirement: A fix leaves the checks and deviations unchanged
The checks and the declared deviations SHALL stay unchanged by the fix;
the test's cases SHALL then carry the label `emulator`.

#### Scenario: Cases after the fix
- **WHEN** a test's configuration gains `fixed-orig = N`
- **THEN** its checks and declared deviations are as before and its cases carry the label `emulator`

### Requirement: The test of an extension
The test of an extension of the port (issue N: behaviour the original does
not have, where the original refuses or does something else) SHALL
declare as its deviations what the original does instead, for the cells
with the original's part, and SHALL be judged like a test with
`fixed-orig`.

#### Scenario: The test of an extension
- **WHEN** a test sets `extension = N` and declares for the cells with the CD's libraries the failing checks of the original
- **THEN** those cells pass with the declared deviations, the cells with the C libraries pass without them, and its cases carry the label `emulator`

### Requirement: The recorded files are cases of one test
`ID.orig` SHALL be the `[orig]` case of the shared test model (`testing/test-model`) and `ID.expect` the expected case (the `[fixed #N]` case once the bug is fixed) of the same test.

#### Scenario: The files of a fixed test
- **WHEN** a recorded test's bug is fixed
- **THEN** `ID.expect` is its `[fixed #N]` case and `ID.orig` stays its `[orig]` case

### Requirement: The normal run judges against the recorded files
The normal run SHALL judge `port` and `clib` against `ID.expect` (FIXED) and, for an open bug, `ID.orig` (REPRODUCED, else DIFFERS) and SHALL NOT start a `ref` session.

#### Scenario: No reference session
- **WHEN** a bug-orig test with an `ID.orig` runs in the normal run
- **THEN** no `ref` session is started

### Requirement: A fixed test keeps its recorded original
A fixed test (`; @fixed-orig #N`) SHALL keep its `ID.orig`.

#### Scenario: The original of a fixed test
- **WHEN** a test has `; @fixed-orig #N`
- **THEN** its `ID.orig` is still committed next to it
