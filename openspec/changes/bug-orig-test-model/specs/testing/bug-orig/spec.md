## MODIFIED Requirements

### Requirement: Recorded observation of the original
For every bug-orig test, open or fixed, the original's observation SHALL be committed as `ID.orig` (the sections of the `ref` configuration, raw), written by the recording target `bug-orig-record`, which needs the reference. `ID.orig` is the `[orig]` case of the shared test model (`testing/test-model`) and `ID.expect` the expected case (the `[fixed #N]` case once the bug is fixed) of the same test. The normal run SHALL judge `port` and `clib` against `ID.expect` (FIXED) and, for an open bug, `ID.orig` (REPRODUCED, else DIFFERS) and SHALL NOT start a `ref` session. A fixed test (`; @fixed #N`) SHALL keep its `ID.orig`.

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

### Requirement: Validity of a test is checked on the recorded file
The test runner SHALL give the verdict BROKEN when `ID.orig` matches `ID.expect`, for open and for fixed tests, since the test then would not show the bug. This SHALL be checked without a reference session.

#### Scenario: Wrong expectation
- **WHEN** an `ID.expect` is edited so that `ID.orig` matches it
- **THEN** the validity check (`bug_orig_selftest_broken` form) reports BROKEN

#### Scenario: A fixed test with an equal pair
- **WHEN** a fixed test's `ID.orig` matches its `ID.expect`
- **THEN** the validity check reports BROKEN

## ADDED Requirements

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
