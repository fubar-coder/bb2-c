# Spec Delta

## Purpose

Keeps the measurement of how much of AmiBlitz3's own code `blitz2-ab3`
compiles, so that progress and regressions in AB3 mode show in a test
and not only in a throw-away survey.

## ADDED Requirements

### Requirement: AmiBlitz3's sources are compiled
The test `ab3_examples` SHALL compile every `.ab3` source of AmiBlitz3 v3.14.0's `Sourcecodes/Examples`, `Templates` and `Amiblitz3/Tests` with `blitz2-ab3` (compile only, nothing is run), and every top-level include of `Sourcecodes/Includes` as the `XINCLUDE` of a wrapper source.

#### Scenario: An include
- **WHEN** the test meets `Sourcecodes/Includes/dos.include.ab3`
- **THEN** it compiles a wrapper with `OPTIMIZE 4`, the include and `End`, and records the result under the include's name

### Requirement: The results are compared with a committed table
A table under `tests/data/` SHALL hold, per source, its name, its group, whether it compiles and, if not, the first message of the compiler. A source that is not in the table, a row without a source, and a result that differs from its row SHALL fail the test.

#### Scenario: Result equals the table
- **WHEN** every source compiles or fails with the message its row names
- **THEN** the test passes

#### Scenario: A source changes its result
- **WHEN** a change makes a source compile that its row says fails, or fail that compiles, or with another first message
- **THEN** the test fails and names the source, the row and the new result

#### Scenario: A source is new or gone
- **WHEN** the fetched tree has a source without a row, or a row has no source
- **THEN** the test fails and names it

### Requirement: The table is updated by a target, not by hand
A build target SHALL write the table from a fresh run of `blitz2-ab3`; the test SHALL never write it. A row whose result is an improvement SHALL be committed together with the change that caused it.

#### Scenario: A task closes a gap
- **WHEN** a change lets a source compile that failed
- **THEN** the target rewrites its row, and the diff of the table shows exactly the sources the change opened

### Requirement: The table records AmiBlitz3's result
Every row SHALL also hold AmiBlitz3 v3.14.0's result for the same source (compiles, or its first error), recorded by a target of the label `originals` that runs AmiBlitz3's compiler in the emulator. The test SHALL NOT compare against it; the column tells which gaps are still open and lets a reader see a source that compiles with `blitz2-ab3` and not with AmiBlitz3.

#### Scenario: Recording
- **WHEN** the `originals` target runs
- **THEN** it rewrites the AmiBlitz3 column and leaves the `blitz2-ab3` column as it is

### Requirement: Missing data skips the test
Without the fetched AmiBlitz3 sources, `Data` or residents, or without the emulator, `ab3_examples` SHALL be skipped with return code 77 and SHALL NOT fail.

#### Scenario: Sources not fetched
- **WHEN** `archive/amiblitz3` lacks the files the test needs
- **THEN** ctest reports the test as skipped

### Requirement: The test has a label and a time limit
`ab3_examples` SHALL carry the label `emulator`, so that the presets `emulator` and `all` select it, and SHALL have a ctest time limit of 3600 seconds plus the emulator queue wait. The check of the table against broken input (a row with another result, an extra row, a missing row) SHALL be a test without the emulator.

#### Scenario: Broken table
- **WHEN** the comparison runs on a table with a changed row
- **THEN** it fails with the row named
