# testing/bug-orig Specification

## Purpose
Defines how tests of original bugs are judged during the bug-fix phase:
which tests run where, what a verdict means, and how the original's
behaviour is documented without a reference session on every run.

## Requirements

### Requirement: Fixed bug-orig tests run in the normal presets
A bug-orig test whose header holds `; @fixed #NNN` SHALL carry the label `emulator` and not `bug-orig`, SHALL run in `emulator` and `all` without a `ref` session, and SHALL pass only if every judged configuration is FIXED. A fix SHALL add the line in the commit that fixes the bug.

#### Scenario: A fixed test is in all
- **WHEN** `ctest --preset all -N` lists the tests after a bug was fixed and its test got `; @fixed #NNN`
- **THEN** the test is listed, and `ctest --preset bug-orig -N` does not list it

#### Scenario: A fix regresses
- **WHEN** a later change makes a `; @fixed` test's configuration show the bug again
- **THEN** the test fails in `ctest --preset all` and the verdict names the configuration

#### Scenario: No reference needed
- **WHEN** a `; @fixed` test runs on a machine without the CD
- **THEN** it runs (or skips only for missing Amiberry), because it starts no `ref` session

### Requirement: Recorded observation of the original
For every open bug-orig test the original's observation SHALL be committed once as `ID.orig` (the sections of the `ref` configuration, raw), made by a recording run that needs the reference. The normal run SHALL judge `port` and `clib` against `ID.expect` (FIXED) and `ID.orig` (REPRODUCED, else DIFFERS) and SHALL NOT start a `ref` session.

#### Scenario: Open bug still reproduced
- **WHEN** a bug-orig test with an `ID.orig` runs and the port shows what `ID.orig` holds
- **THEN** the verdict is REPRODUCED, as with a live reference session

#### Scenario: Different wrong behaviour
- **WHEN** the port neither meets `ID.expect` nor matches `ID.orig`
- **THEN** the verdict is DIFFERS

#### Scenario: Recording is separate
- **WHEN** the recording run is started under the label `originals`
- **THEN** it writes `ID.orig` next to the test and changes no other file; a normal run never writes it

### Requirement: Validity of a test is checked on the recorded file
The test runner SHALL give the verdict BROKEN when `ID.orig` matches `ID.expect`, since the test then would not show the bug. This SHALL be checked without a reference session.

#### Scenario: Wrong expectation
- **WHEN** an `ID.expect` is edited so that `ID.orig` matches it
- **THEN** the validity check (`bug_orig_selftest_broken` form) reports BROKEN

### Requirement: Labels of known failures
Tests of open original bugs SHALL carry the label `bug-orig` and tests of known port defects `bug-port`; both SHALL stay out of `host`, `emulator` and `all` and run in the preset `bug-orig`. A `; @fixed` test SHALL carry neither.

#### Scenario: Preset contents
- **WHEN** `ctest --preset bug-orig -N` lists the tests
- **THEN** it lists only tests with open bugs (verdict REPRODUCED or DIFFERS expected) and known port defects
